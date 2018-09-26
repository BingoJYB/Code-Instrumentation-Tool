#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"


using namespace clang;
using namespace clang::tooling;
using namespace llvm;

bool opt_insert_include=false;
bool opt_verbose=false;
bool opt_fullverbose=false;
std::string opt_filefilter;

int num = 1;
std::ofstream mappingFile;
Replacements* _Replacements;
static cl::OptionCategory MyCategory("Instrumentation");


class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
    ASTContext *Context;
    SourceManager* _SourceManager;
    std::hash<std::string> str_hash;

    explicit MyASTVisitor(ASTContext *Context, SourceManager *SrcManager)
      : Context(Context), _SourceManager(SrcManager) {
    }

    bool VisitDecl(Decl *D) {
          if(VarDecl* vardecl = dyn_cast<VarDecl>(D)) {
              SourceLocation sLoc = D->getLocation();
              FileID fileID = _SourceManager->getFileID(sLoc);
              const FileEntry *fileEntry = _SourceManager->getFileEntryForID(fileID);
              if(fileEntry==0) return false;
            }

          return true;
        }

    bool VisitStmt(Stmt *stmt) {
        if(BinaryOperator* binaryOperator = dyn_cast<BinaryOperator>(stmt)) {
            if (binaryOperator->isAssignmentOp()) {
                std::stringstream buffer;
                SourceLocation rhsLoc = binaryOperator->getRHS()->getLocStart();
                SourceLocation lhsLoc = binaryOperator->getLHS()->getLocStart();
                unsigned int lineNum = _SourceManager->getExpansionLineNumber(lhsLoc);
                unsigned int colNum = _SourceManager->getExpansionColumnNumber(lhsLoc);

                SourceLocation sLoc = stmt->getLocStart();
                FileID fileID = _SourceManager->getFileID(sLoc);
                const FileEntry *fileEntry = _SourceManager->getFileEntryForID(fileID);
                std::string fileName = fileEntry->getName();

                std::stringstream file_ss(fileName);
                std::string file_item;
                std::vector<std::string> file_tokens;
                while (getline(file_ss, file_item, '/'))
                    file_tokens.push_back(file_item);

                std::ostringstream path;
                if (!file_tokens.empty()) {
                    std::copy(file_tokens.begin(), file_tokens.end()-2,
                        std::ostream_iterator<std::string>(path, "/"));

                    path << file_tokens.back();
                }

                std::string str = ";" + path.str() + ";" + std::to_string(lineNum) + ";" + std::to_string(colNum);

                buffer << "dco::assignment_info(" << num << ") << ";
                mappingFile << num++ << str << std::endl;

                auto RB = Replacement(*_SourceManager, rhsLoc, 0, buffer.str());
                _Replacements->insert(_Replacements->begin(), RB);
            }
        }

        return true;
    }
};


class MyASTConsumer : public ASTConsumer {
public:
    SourceManager *_srcManager;

    explicit MyASTConsumer(ASTContext *Context, SourceManager *SrcManager)
      :  _srcManager(SrcManager), Visitor(Context, SrcManager) {
    }

    bool HandleTopLevelDecl(DeclGroupRef DR) override {
      for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
          Decl *d=*b;
          SourceLocation loc = d->getLocStart();

          std::string locstr = loc.printToString(*_srcManager);
          if(opt_fullverbose) std::cout << " loc=" << locstr << std::endl;

          size_t nameend = locstr.find(':');
          std::string filename = locstr.substr(0,nameend);


          if (std::regex_match (locstr, std::regex("(/usr)(.*)") )) {
              if(opt_verbose) std::cout << "std include found! not processing file:\n";
              if(opt_verbose) std::cout << locstr << std::endl;
              return true;
            }

          if (std::regex_search (filename, std::regex(opt_filefilter) )) {
              std::cout << "##filename=" << filename << std::endl;
              if(opt_verbose) std::cout << "Now processing file:\n";
              if(opt_verbose) std::cout << locstr << std::endl;

              if(opt_insert_include) {
                auto RB1 = Replacement(*_srcManager, loc, 0, "\n#include \"ico.hpp\" \n");
                _Replacements->insert(RB1);
              }

              Visitor.TraverseDecl(d);
              if(opt_verbose) std::cout << "Finished processing file:\n";
              if(opt_verbose) std::cout << locstr << std::endl;
          }
          else {
              if(opt_verbose) std::cout << "filtered out by user: " << filename << std::endl;
            }
        }
      return true;
    }

private:
    MyASTVisitor Visitor;
};


class MyFrontendAction : public ASTFrontendAction {
public:
    MyFrontendAction() {}
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                   StringRef file) override {
        ASTContext *context = &CI.getASTContext();
        SourceManager &srcManager2 = CI.getSourceManager();
        SourceManager *srcManager = &srcManager2;
        return make_unique<MyASTConsumer>(context, srcManager);
    }
};


int main(int argc, const char **argv) {
    bool stop = false;
    std::ostringstream mf;

    for (int i = 1; i < argc-1; i++) {
        std::stringstream file_ss(argv[i]);
        std::string file_item;
        std::vector<std::string> file_tokens;
        while (getline(file_ss, file_item, '/'))
            file_tokens.push_back(file_item);

        if (i == 1) {
            std::copy(file_tokens.begin(), file_tokens.end()-1,
                std::ostream_iterator<std::string>(mf, "/"));
        }

        std::ostringstream ff;
        std::copy(file_tokens.begin(), file_tokens.end()-1,
            std::ostream_iterator<std::string>(ff, "/"));
        ff << file_tokens.back();

        if (i == 1)
            opt_filefilter += "(?:";

        if (ff.str() == "--") {
            opt_filefilter += ")";
            stop = true;
        }

        if (!stop) {
            if (i > 1)
                opt_filefilter += "|";

            opt_filefilter += ff.str();
        }
    }

    system(("> " + mf.str() + "mapping.txt").c_str());
    mappingFile.open(mf.str() + "mapping.txt");

    CommonOptionsParser OptionsParser(argc, argv, MyCategory);
    RefactoringTool Tool(OptionsParser.getCompilations(),
                         OptionsParser.getSourcePathList());

    _Replacements = &Tool.getReplacements();

    int val = Tool.runAndSave(newFrontendActionFactory<MyFrontendAction>().get());
    //int val = Tool.run(newFrontendActionFactory<MyFrontendAction>().get());

    mappingFile.close();

    return val;
}
