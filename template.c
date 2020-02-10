#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true 1
#define false 0
#define bool int

#define MIN_STR 128
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t')
#define IS_SHELL_SPECIAL(c) ((c) == '&' || (c) == '|')

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

#define inSpecial(c) ((c) == '&' || (c) == '|')
#define inSpace(c) ((c) == ' ' || (c) == '\t')
#define inWord(c) (!(inSpecial(c) || inSpace(c)))
int count_args (char *str) {
    int n = 1;
    int i = 0;

    for(;str[i];i++){
        if(inSpecial(str[i]) && inSpace(str[i+1])) n++;
        else if(inWord(str[i]) && inSpecial(str[i+1])) n++;
        else if(inWord(str[i]) && inSpace(str[i+1])) n++;
        else if(!inSpace(str[i]) && !str[i+1]) n++;
    }
    printf("%d", n);
    return n;
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

    n = count_args(str+i); /* le + i optimise */

    argv = malloc(sizeof(char *) * (n + 1));

    if (!argv) {
        fprintf(stderr, "argument array (argv) could not be allocated\n");
        /* no need to free argv */
        return NULL;
    }

    n = 0;
    for (; str[i]; i++) {
        if (IS_SHELL_SPECIAL(str[i])) {
            while(IS_SHELL_SPECIAL(++i));
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

enum { NORMAL, SHBG, SHPIPE, SHAND, SHOR }; /* execution type */

/*
  il faut quon fork le process
  pi dans le child process quon fasse le exec
  pi dans lautre process faut quon check squi spasse
*/
int execute (char **argv) {
    int i, type;

    for (i = 0; argv[i] && argv[i][0] != '&' || argv[i][0] != '|'; i++)
        ;

    if (argv[i][0] == '&') {
        if (argv[i][1] && argv[i][1] == '&' && !argv[i][2]) {
            type = SHAND;
        } else if (!argv[i][1]) {
            type = SHBG;
        } else {
            fprintf(stderr, "bash: syntax error near unexcepted token '%c'\n",
                    argv[i][2] ? argv[i][2] : argv[i][1]);
            return -1;
        }
        argv[i] = NULL;
    } else if (argv[i][0] == '|') {
        if (argv[i][1] && argv[i][1] == '|' && !argv[i][2]) {
            type = SHOR;
        } else if (!argv[i][1]) {
            type = SHPIPE;
        } else {
            fprintf(stderr, "bash: syntax error near unexcepted token '%c'\n",
                    argv[i][2] ? argv[i][2] : argv[i][1]);
            return -1;
        }
    } else {
        type = NORMAL;
    }

    return -1;
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
