# Code-Instrumentation-Tool

A code instrumentation tool based on Clang compiler adds an additional indicator in the assignment for an automatic differentiation (AD) engine to parse. It is `dco :: assignment_info(id)`. This is added on the right-hand side of the assignment with the operator “<<”. For example, a code snippet is

```c++
#include <math.h>
  
template<class T> void f (const T∗ x , T∗ y) {
  y[0] = x[1] + x[0];
  y[1] = y[1] ∗ y[0];
}
```

After the instrumentation, it is

```c++
#include <math.h>
  
template<class T> void f (const T∗ x , T∗ y) {
  y[0] = dco::assignment_info(1) << x[1] + x[0];
  y[1] = dco::assignment_info(2) << y[1] ∗ y[0];
}
```

Besides the instrumented code file, the tool creates a mapping file as well. It contains the information of the assignment like the index number, the path of the code file, the line number and the starting column number of the left-hand side of the assignment. They are separated by the semicolon. An example is

```
1;/the path of the file/;5;5
2;/the path of the file/;6;5
```
