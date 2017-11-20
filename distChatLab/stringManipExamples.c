/*
 * Examples of some useful string manipulations in C
 * Author: Sherri Goings
 * Last Modified: 3/10/2014
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main() {

    // creates a string 'message' with exactly 12 characters (11 of 'hello world' plus
    // '\0' null terminator).  If try to access anything past 12th char will get segfault
    // or bus error
    char* message = "hello world";
    printf("message: %s\n", message);


    // easy way to print a string starting at any arbitrary character
    printf("message starting at 3rd character: %s\n", &message[2]);


    // creates a string with exactly 1024 characters, initially filled with whatever
    // happened to randomly be in that memory location before
    char msgToSend[1024];
	char msg2[1024];
    printf("msgToSend allocated but no value set: %s\n", msgToSend);


    // copies string message into 1st part of msgToSend, message is null terminated so
    // printf knows to only print up to the '\0', but the rest of the original 1024 chars
    // in msgToSend are still allocated and available to use
    strcpy(msgToSend, message);
    printf("msgToSend: %s\n", msgToSend);


    // note that if I said: msgToSend = "some string", msgToSend would become a pointer to
    // "some string" and no longer have the extra space allocated, so once you allocate extra
    // space it's important you only use things like strcpy in order to keep it!


    // casting an int to a char gives you the ascii value of the last byte of the int.
    // 33 (0010 0001 binary) is the ascii value for '!', and note that 256+33 (1 0010 0001 binary)
    // has the same last byte (8 bits) as 33 and so when cast to a char is also '!'
    int count = 33;
    int count2 = 256+33;
    char c = (char)count;
    char c2 = (char)count2;
    printf("c: %c, c2: %c\n", c, c2);


    // casts 33 and 66 to chars then prepends to the beginning of msgToSend
    // Note that msgToSend MUST have room allocated for 2 extra chars, because strcpy will access
    // 2 chars past the end of the original message
    strcpy(msg2, msgToSend);
	strcpy(&msgToSend[2], msg2);
	printf("test\n");
	msgToSend[0] = (char)count;
    msgToSend[1] = (char)(count*2);
    printf("msgToSend: %s\n", msgToSend);



    // removes first 2 chars of msgToSend and converts back to ints,  moving rest of message
    // so 1st character is now what was previously 3rd character.
    int i1 = (int) msgToSend[0];
    int i2 = (int) msgToSend[1];
    strcpy(msg2, &msgToSend[2]);
	strcpy(msgToSend, msg2);
    printf("i1: %d, i2: %d, message: %s\n", i1, i2, msgToSend);

    return 0;
}
