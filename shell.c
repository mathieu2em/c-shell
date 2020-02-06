#include <stdio.h>
#include <stdlib.h>

#define true 1
#define false 0
#define bool int

#define MIN_STR 128;

char *readLine (void);
char **split_args (char *);

/* readline allocate a new char array */
char* readLine (void) {
    char *line;
    int c, n = 0, len = MIN_STR;
    
    line = malloc(sizeof(char) * len); /* initialize array */

    if (!line) {
        fprintf(stderr, "could not initialize line\n");
        return NULL;
    }
                   
    while ((c = getchar()) != EOF && c != '\n') {
        if (n == len - 1) {
            len *= 2;
            line = (char*) realloc(line, sizeof(char) * len); /* extend array size */
        }
        line[n++] = c;
    }
    
    line[n] = '\0'; /* null terminate string */
    
    return line;
}

/* split_args doesn't free str */
char **split_args (char *str) {
    char **argv;
    int i, j = 0, n = 0;

    for (i = 0; str[i]; i++) {
        if (str[i] == ' ')
            n++;
    }

    argv = malloc(sizeof(char*) * n);

    if (!argv) {
        fprintf(stderr, "argv could not be allocated\n");
        return NULL;
    }

    n = 0;
    
    for (i = 0; str[i]; i++) {
        if (str[i] == ' ') {
            argv[j++] = malloc(sizeof(char) * (i - n));

            if (!argv[j]) {
                fprintf(stderr, "argv[%d] could not be allocated\n", j);
                return NULL;
            }
            
            argv[j][i - (++n)] = '\0';
            for (; n < i; n++)
                argv[j][i - n] = str[i - n];
        }
    }

    return argv;
}

void shell (void) {
    char *line = readLine();

    puts(line);
    
    free(line);
}

/*
 * Dont change main!
 *
 * You may add unit tests here, but we will rip out main entirely when we grade your work.
 */
int main (void) {
    shell();
}
