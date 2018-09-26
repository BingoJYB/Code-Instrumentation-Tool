set(LLVM_LINK_COMPONENTS support)

add_clang_executable(instrumentation
  instrumentation.cpp
  )

target_link_libraries(instrumentation
  clangAST
  clangBasic
  clangTooling
  )
