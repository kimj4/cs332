#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// operators = < > & |
// words A-Z a-z 0-9 - . / _


/**
 * Tokenizer by hmjd on stack oveflow
 * https://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c
 * Used instead of provided file because I wanted to do getline in main()
 */
char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    // following couple lines from
    // https://stackoverflow.com/questions/16677800/strtok-not-discarding-the-newline-character
    // This is to get rid of the newline character that follows every single line
    //   This was causing a problem with file names for something like
    //   'nano somename.txt' -> 'nano somename.txt^J'
    char *newline = strchr( a_str, '\n' );
    if ( newline ) {
        printf("here\n");
        *newline = 0;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result) {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        // *(result + idx) = 0;
        *(result + idx) = NULL;

    }
    return result;
}






int main() {
    size_t MAX_WORD_LENGTH = 100;
    int bytes_read;
    char *buf;
    buf = (char*) malloc(MAX_WORD_LENGTH + 1);
    while(true) {
        bytes_read = getline(&buf, &MAX_WORD_LENGTH, stdin);

        // handling too-long lines
        if (bytes_read > 100) {
            printf("Error: a line was too long\n");
            return 1;
        }
        char** words = str_split(buf, ' ');
        // use this variable as both an index and a count of tokens
        // TODO: if you put a space at the end of the last token, it overcounts by 1
        int count = 0;
        // char** wordOrSymbolArray = [];
        // str_split function adds a null string at the end of the array.
        while(words[count]) {
            if (words[count] )
            printf("%s\n", words[count]);
            count++;
        }

        // int i

        // handling EOF for when being piped input.
        if (bytes_read == EOF) {
            break;
        }
        // execute file (binary executable or script, shell commands are just scripts) given in words[0]
        // with arguments in array "words" - array of strings (think argv list in Python),
        // first string is again file to be executed (just like Python this is convention),
        // then can have any number of args, then last string MUST BE NULL pointer.
        int err = execvp(words[0], words);
        printf( "Will this get printed? Yes if exec returned with an error! err = %d\n", err );
    }

    return 0;
}
