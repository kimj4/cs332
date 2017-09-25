#include <stdlib.h>
#include <stdio.h>

int main() {

    // creates a string 'message' with exactly 12 characters (11 of 'hello world' plus 
    // '\0' null terminator).  If try to access anything past 12th char will get segfault
    // or bus error
    char* message = "hello world";
    printf("message: %s  sizeof(msg): %d\n", message, (int)(sizeof(message)));
    //message[5] = 'x';  // this line will cause an error!
    printf("char at index 6: %c\n", message[6]);


    // easy way to print a string starting at any arbitrary character
    printf("message starting at 3rd character: %s\n", &message[2]);
    
	// read line of input from terminal
	printf("enter a line to read: ");
    size_t MAX_WORD_LENGTH = 128;
    int bytes_read;
    char *buf;
	buf = (char*) malloc( MAX_WORD_LENGTH+1 );
    bytes_read = getline(&buf, &MAX_WORD_LENGTH, stdin);
    printf("Your line: %s\n",buf);
      
    return 0;
}
