#include <stdio.h>

int main() {
    size_t MAX_WORD_LENGTH = 100;
    int bytes_read;
    char *buf;
    buf = (char*) malloc(MAX_WORD_LENGTH + 1);
    bytes_read = getline(&buf, &MAX_WORD_LENGTH, stdin);
    return 0;
}
