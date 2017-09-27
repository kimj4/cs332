/**
 * simpleShell.c
 * Ju Yun Kim
 * Carleton College
 * CS 332 Operating Systems
 * A simple shell that implements <, >, |, &
 * There are some shakey features like output sometimes printing after the
 *  but nothing too major I don't think.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>

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
char** str_split(char* a_str, const char a_delim) {
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

/*
 * isValidWord
 * takes a word and checks every character against allowed characters.
 * does not allow allowed symbols. Make that a separate command.
 * implemented only because using the regex package seems like a pain.
 */
bool isValidWord(char* word) {
    bool isValid = true;
    int i = 0;
    while (word[i]) {
        if (!strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-./_", word[i])) {
            isValid = false;
        }
        i++;
    }
    return isValid;
}

bool isSymbol(char* word) {
    bool isSymbol = false;
    if (!strcmp(word, "<") | !strcmp(word, ">") | !strcmp(word, "|") | !strcmp(word, "&")) {
        isSymbol = true;
    }
    return isSymbol;
}


int main() {
    printf("simpleShell>$ ");

    size_t MAX_WORD_LENGTH = 100;
    int bytes_read;
    char *buf;
    buf = (char*) malloc(MAX_WORD_LENGTH + 1);

    bytes_read = getline(&buf, &MAX_WORD_LENGTH, stdin);
    if (!strcmp(buf, "\n")) {
        main();
    }

    // handling too-long lines
    if (bytes_read > 100) {
        printf("Error: a line was too long\n");
        return 1;
    }
    char** words = str_split(buf, ' ');
    // use this variable as both an index and a count of tokens
    int count = 0;
    int symbolsCount = 0;
    // char** wordOrSymbolArray = [];
    // str_split function adds a null string at the end of the array.

    while(words[count]) {
        if (!isSymbol(words[count])) {
            if (!isValidWord(words[count])) {
                printf("You entered an invalid word\n");
                main();
            }
        } else {
            symbolsCount++;
        }
        count++;
    }

    // This array keeps track of the idxs in the 'words' array whose element
    //  is a symbol. Just initialize it with size count since it's not that
    //  big of a number anyway.
    // This keeps track of where the '>', '<', '|' characters are in the line
    int* symbolsLocation = malloc(symbolsCount * sizeof(int*));


    // store the index the symbols
    // The spec says that a symbol cannot be the first token in a line so
    //  we don't have to worry about the array being initialized with 0s
    //  There is a bit of redundant code here but I ran out of time so...
    int i;
    int symbolsLength = 0;
    for ( i = 0; i < count; i ++) {
        if (!strcmp(words[i], "<") | !strcmp(words[i], ">") | !strcmp(words[i], "|")) {
            symbolsLocation[symbolsLength] = i;
            symbolsLength++;
        }
    }

    // support for & as the last token only
    bool is_amp = false;
    for ( i = 0; i < count - 1; i ++) {
        if (!strcmp(words[i], "&")) {
            printf("& is only supported at the end of the line\n");
            main();
        }
    }
    if (!strcmp(words[count - 1], "&")) {
        is_amp = true;
    }

    // handling EOF for when being piped input.
    if (bytes_read == EOF) {
        return 0;
    }

    // -------------- code adapted from forktest.c -------------------------
    // fork splits process into 2 identical processes that both continue
    // running from point where fork() returns. Only difference is return
    // value - 0 to the child process, pid of child to the parent process
    int pid = fork();

    // if 0 is returned, execute code for child process
    // which is going to be the command
    if( pid == 0 ) {
        if (symbolsLength) {
            for (i = 0; i < symbolsLength; i++) {
                if (!strcmp(words[symbolsLocation[i]], "<")) {
                    // run program on left, use as input file on right
                    char** inFileString = malloc(sizeof(char**));
                    memcpy(inFileString, &words[symbolsLocation[i] + 1], sizeof(*words));

                    int newfd = open(inFileString[0], O_RDONLY, 0644);
                    if (newfd == -1) {
                        printf("%s: %s\n", inFileString[0], strerror(errno));
                        return 1;
                    }
                    dup2(newfd, 0);
                    free(inFileString);
                    close(newfd);

                    char** command = malloc(count * sizeof(char**));
                    memcpy(command, words, symbolsLocation[0] * sizeof(*words));
                    int err = execvp(command[0], command);
                    free(command);
                } else if (!strcmp(words[symbolsLocation[i]], ">")) {
                    // run program on left, write output to file on right
                    char** outFileString = malloc(sizeof(char**));
                    memcpy(outFileString, &words[symbolsLocation[i] + 1], sizeof(*words));

                    int newfd = open(outFileString[0], O_CREAT|O_WRONLY, 0644);

                    dup2(newfd, 1);
                    free(outFileString);
                    close(newfd);

                    char** command = malloc(count * sizeof(char**));
                    memcpy(command, words, symbolsLocation[0] * sizeof(*words));
                    int err = execvp(command[0], command);
                    free(command);
                } else if (!strcmp(words[symbolsLocation[i]], "|")) {
                    // makes a child for the left and right sides of the pipe
                    //  in hindsight, this could probably be done with just
                    //  one child with the parent handling one of the commands

                    int desp[2];
                    pipe(desp);

                    // leftProcess | rightProcess
                    int leftProcess = fork();
                    if (leftProcess == 0) {
                        // this one should write to desp[1]
                        int leftStartIdx;
                        int leftLength;
                        if (i == 0) {
                            leftStartIdx = 0;
                            leftLength = symbolsLocation[i] - leftStartIdx;
                        } else {
                            leftStartIdx = symbolsLocation[i - 1] + 1;
                            leftLength = symbolsLocation[i] - leftStartIdx;
                        }

                        dup2(desp[1], 1);
                        close(desp[0]);


                        char** leftCommand = malloc(leftLength * sizeof(char**));
                        memcpy(leftCommand, &words[leftStartIdx], leftLength * sizeof(*words));

                        int err = execvp(leftCommand[0], leftCommand);
                        printf( "Will this get printed? Yes if exec returned with an error! err = %d\n", err );
                        free(leftCommand);

                    } else {
                        // if after forking the left, you are still the parent,
                        //  fork the right. This should mean non-nested forks
                        int rightProcess = fork();
                        if (rightProcess == 0) {
                            // this one should read from desp[0]
                            int rightStartIdx;
                            int rightLength;
                            if (i == symbolsLength - 1) {
                                rightStartIdx = symbolsLocation[i] + 1;
                                rightLength = count - rightStartIdx;
                            } else {
                                rightStartIdx = symbolsLocation[i] + 1;
                                rightLength = symbolsLocation[i + 1] - rightStartIdx;
                            }

                            dup2(desp[0], 0);
                            close(desp[1]);

                            char** rightCommand = malloc(rightLength * sizeof(char**));
                            memcpy(rightCommand, &words[rightStartIdx], rightLength * sizeof(*words));

                            int err = execvp(rightCommand[0], rightCommand);
                            printf( "Will this get printed? Yes if exec returned with an error! err = %d\n", err );
                            free(rightCommand);
                        } else {
                            close(desp[0]);
                            close(desp[1]);
                        }

                    }
                }
            }
        } else {
            // This is for when there are no symbols (or if it's just an & at the end)
            int err = execvp(words[0], words);
            printf( "Will this get printed? Yes if exec returned with an error! err = %d\n", err );
        }
    }
    // otherwise execute code for parent process
    // Wait for the child to finish the command, and then try again.
    else {
        // if the last word was &, then don't wait for child process to finish
        if (!is_amp) {
            waitpid(pid, NULL, 0);
        }
        main();
    }
    free(symbolsLocation);
    return 0;
}
