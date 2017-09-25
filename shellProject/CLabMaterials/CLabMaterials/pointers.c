/*
 * some basic examples of pointers in C, as well as calling functions with both types 
 * of arguments, pass-by-reference and pass-by-value.
 */

#include <stdio.h>   
#include <stdlib.h>  

// in C functions must be declared earlier in the file than where they are called, 
// but may be defined anywhere, often even in a different file.  The following 2 
// functions are declared here, and defined at the end of this file.  Note that there
// is no need to include variable names in the declarations, just the types.
void passByValue(int, double);
void passByReference(int*, double*);

// main function as always required
int main()
{
  // define x as an integer with the value 5, print both x and the address of x
  int x = 5;
  printf("\nx = %d, &x = %p\n", x, &x);    // %p is the code to print a pointer

  // define y as a pointer to the address of x, print y and the value obtained 
  // by dereferencing y
  int* y = &x;
  printf("y = %p, *y = %d\n\n", y, *y);

  int val1 = 1;
  double val2 = 2.5;

  printf("initial values: val1=%d, val2=%g\n\n", val1, val2);

  // calling function with arguments that are actual values of val1 and val2
  passByValue(val1, val2);
  printf("after calling passByValue: val1=%d, val2=%g\n\n", val1, val2);

  // calling function with arguments that are references to val1 or val2, i.e.
  // the addresses at which val1 and val2 are stored
  passByReference(&val1, &val2);
  printf("after calling passByReference: val1=%d, val2=%g\n\n", val1, val2);
  
  return 0;
}

/* copies of the values of the arguments in this function call are passed to
 * variables a and b.  Changing a and b in the function only affects the values
 * in the local scope of the function itself, the original arguments used in the
 * function call are unchanged after the function returns
 */
void passByValue(int a, double b) {
  // increment a by 1, set b to 1.23
  a++;
  b = 1.23;
  printf("end of function passByValue: a=%d, b=%g\n", a, b);
}

/* a and b are pointers in this function, so a reference (pointer) must be passed as 
 * arguments when this function is called.  Changing the values referenced by the pointers
 * a and b changes those values everywhere, as there is only 1 copy, the original.
 */
void passByReference(int* a, double* b) {
  // increment the value of a by 1, set the value of b to 1.23
  (*a)++;
  *b = 1.23;
  printf("end of function passByReference: a=%d, b=%g\n", *a, *b);
}
