# Code-Instrumentation-Tool

This code instrumentation tool adds an additional indicator in the assignment for an automatic differentiation (AD) engine to parse. It is `dco :: assignment_info(id)`. This is added on the right-hand side of the assignment with the operator “<<”. For example, a code snippet is

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

The number in the parentheses is the id of index of the assignment. It increases according to the sequence of the insertion of the code. When an AD engine runs the as- signment with this instrumented part, it will add the index number as the additional
information to the node corresponding to this assignment, which can be used to locate the assignment.

The tool creates a mapping file. It contains the information of the assignment like the index number, the path of the instrumented file, the line number and the starting column number of the left-hand side of the assignment. They are separated by the semicolon. An example is

```
1;/the path of the file/;5;5
2;/the path of the file/;6;5
```
