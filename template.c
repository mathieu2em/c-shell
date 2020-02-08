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
    int i = 0;
    int j = 0;
    int k = 0;
    int n = 0;

    /*
       we have to count how many groups of spaces of 1+ space
       we have so we can allocate the right array size
    */
    while(str[i] && (str[i] == ' ' || str[i] == '\t')) i++;
    for (; str[i]; i++) {
        if ( (str[i] == ' ' || str[i] == '\t') && str[i+1] != ' ' && str[i+1] != '\t'){
            n++;
        }
    }

    /*
       we use the number counted to allocate an array of strings containing the command
       written to the terminal
     */
    argv = malloc(sizeof(char*) * n+1);

    if (!argv) {
        fprintf(stderr, "argv could not be allocated\n");
        return NULL;
    }

    n = 0;

    /*
      now we have to full this array with the elements
    */
    i = 0;
    /*
      exception case of line starting with spaces ex: '_____echo_"hi!"'
    */
    while(str[i] && (str[i] == ' ' || str[i] == '\t')) i++;
    n = i;
    for (; str[n]; i++) {
        if ( (str[i] == ' ') || !str[i] ) {
            argv[j] = malloc(sizeof(char)*(i - n));

            if (!argv[j]) {
                fprintf(stderr, "argv[%d] could not be allocated\n", j);
                return NULL;
            }

            for (k=0; n < i; n++, k++)
                argv[j][k] = str[n];
            j++;

            while(str[i] && (str[i] == ' ' || str[i] == '\t')){
                i++;
            }
            n = i;
        }
    }

    return argv;
}

void shell (void) {
    char **argv;
    int j = 0;
    char *line = readLine();




    puts(line);
    argv = split_args(line);
    while(argv[j]) {
        printf("%s", argv[j]);
    }
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
