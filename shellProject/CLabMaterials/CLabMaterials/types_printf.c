/*
 * To compile: navigate in terminal to the directory containing this file, then
 * type the command "gcc -o types_printf types_printf.c".  This will create an
 * executable named "types_printf".  Note you can name the executable anything, the -o 
 * option specifies the name, so whatever follows it will be your executable.  If you 
 * do not use the -o option the executable will be named "a.out" by default.
 * To run: type "./types_printf" (do not type the quotes) or substitue whatever you named
 * your executable for "types_printf".  You must include the "./" either way.
 */

#include <stdio.h>    // library that includes printf and scanf for input/output
#include <stdlib.h>   // library that includes all sorts of other useful things

/* 
 * The gcc compiler looks for the "int main()" function and begins execution there.
 * A program will not compile without this function.
 * 0 is the standard return for a program that terminates normally.
 */
int main()
{
  // basic use of printf, simply displays the text as written, \n represents a newline
  printf("An intro to basic types in C and examples of using printf\n");

  // variables must be declared with a type, values can be assigned on the same line
  // as shown below for 'y', or separate from the declaration as shown below for 'x'.
  int x;
  x=5;
  int y=3;

  // see "printf-doc.html" on Moodle for more details on using printf
  printf("x=%d, y=%d\n", x, y);

  // common types include 'int' for integer (whole number), 'float' or 'double' for decimal
  // numbers (a float is 16 bits long, a double 32 bits for more precision), 'char' for
  // a single character, and char* for a string (really an array of characters) where the char*
  // variable is simply the memory address of the first character (i.e. a pointer to the first char)
  float a = 2.246;
  double b = 3.3;
  char c = 'r';      // single quotes indicate a single char
  char *s = "abc";   // double quotes indicate a string of chars

  // see "printf-doc.html" for more details on using printf
  printf("a=%g, b=%g, c=%c, s=%s\n", a, b, c, s);

  // return 0 to indicate that program is terminating without errors
  return 0;
}
