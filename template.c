#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true 1
#define false 0
#define bool int

#define MIN_STR 128
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t')

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
             /* extend array size, shouldn't change content
                according to `man 3 realloc` */
            line = (char*) realloc(line, sizeof(char) * len);
        }
        line[n++] = c;
    }

    line[n] = '\0'; /* null terminate string */

    return line;
}

/*
 * build_argv builds the arguments array to be passed to execvp
 * it doesn't free str.
 */
char **split_args (char *str) {
    char **argv;
    int i = 0, j = 0, n = 1;

    /* skip spaces in front */
    while(str[i] && IS_SPACE(str[i]))
        i++;

    /* save this position, so don't have to skip spaces twice */
    j = i;

    /* counting no of args */
    for (; str[i]; i++) {
        if (IS_SPACE(str[i]) && str[i+1] && !IS_SPACE(str[i+1]))
            n++;
    }

    argv = malloc(sizeof(char *) * (n + 1));

    if (!argv) {
        fprintf(stderr, "argument array (argv) could not be allocated\n");
        /* no need to free argv */
        return NULL;
    }

    n = 0;
    i = j; /* restore position of first non-whitespace char */
    for (; str[i]; i++) {
        if (IS_SPACE(str[i])) {
            argv[n] = malloc(sizeof(char) * (i - j + 1));

            if (!argv[n]) {
                fprintf(stderr, "argv[%d] could not be allocated\n", n--);
                goto free_argv;
            }

            strncpy(argv[n], str + j, i - j);
            argv[n++][i - j] = '\0';

            /* skip remaining spaces */
            while(str[i] && IS_SPACE(str[i]))
                i++;

            j = i; /* prepare to read next arg */
        }
    }

    /* if there is no whitespace at end of line, there is still an arg */
    if (i-1 != j) {
        argv[n] = malloc(sizeof(char) * (i - j + 1));

        if (!argv[n]) {
            fprintf(stderr, "argv[%d] could not be allocated\n", n--);
            goto free_argv;
        }

        strncpy(argv[n], str + j, i - j);
        argv[n++][i - j] = '\0';
    }

    argv[n] = NULL;

    return argv;

free_argv:
    for (; n >= 0; n--)
        free(argv[n]);
    free(argv);
    return NULL;
}

/*
  il faut quon fork le process
  pi dans le child process quon fasse le exec
  pi dans lautre process faut quon check squi spasse
*/
int execute (char** argv) {
    
}

void shell (void) {
    char **argv;
    int j = 0;

    /* temporary testing */
    char *line = readLine();
    argv = split_args(line);
    for (j = 0; argv[j]; j++)
        printf("%s\n", argv[j]);

    /* clean up */
    for (j = 0; argv[j]; j++)
        free(argv[j]);
    free(argv);
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
