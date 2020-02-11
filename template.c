#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true 1
#define false 0
#define bool int

#define MIN_STR 128
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t')
#define IS_SPECIAL(c) ((c) == '&' || (c) == '|')
#define INSIDE_WORD(c) (!(IS_SPECIAL(c) || IS_SPACE(c)))

enum { NORMAL, BACKGROUND, AND, OR };

struct command {
    char **argv;
    char type;
};

char *readLine (void);
int count_tokens (char *);
char **tokenize (char *);
struct command *parse (char **);
int execute (struct command *);

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

int count_tokens (char *str) {
    int n = 0;
    int i = 0;

    for (; str[i]; i++) {
        if ((IS_SPECIAL(str[i]) && IS_SPACE(str[i+1])) ||
            (IS_SPECIAL(str[i]) && INSIDE_WORD(str[i+1])) ||
            (INSIDE_WORD(str[i]) && IS_SPECIAL(str[i+1])) ||
            (INSIDE_WORD(str[i]) && IS_SPACE(str[i+1])) ||
            (!IS_SPACE(str[i]) && !str[i+1]))
            n++;
    }
    return n;
}

/*
 * build_argv builds the arguments array to be passed to execvp
 */
char **tokenize (char *str) {
    char **argv;
    int i = 0, j = 0, n = 1;

    /* skip spaces in front */
    while(str[i] && IS_SPACE(str[i]))
        i++;

    n = count_tokens(str+i); /* le + i optimise */

    argv = malloc(sizeof(char *) * (n + 1));

    if (!argv) {
        fprintf(stderr, "argument array (argv) could not be allocated\n");
        /* no need to free argv */
        free(str);
        return NULL;
    }

    n = 0;
    j = i;

    for (; str[i]; i++) {
        if((IS_SPACE(str[i]) && INSIDE_WORD(str[i+1])) ||
           (IS_SPACE(str[i]) && IS_SPECIAL(str[i+1])))
            j = i+1;
        if((IS_SPECIAL(str[i]) && IS_SPACE(str[i+1])) ||
           (IS_SPECIAL(str[i]) && INSIDE_WORD(str[i+1])) ||
           (INSIDE_WORD(str[i]) && IS_SPECIAL(str[i+1])) ||
           (INSIDE_WORD(str[i]) && IS_SPACE(str[i+1])) ||
           (!IS_SPACE(str[i]) && !str[i+1])) {
            argv[n] = malloc(sizeof(char) * (i+1 - j + 1));

            if (!argv[n]) {
                fprintf(stderr, "argv[%d] could not be allocated\n", n--);
                goto free_argv;
            }
            strncpy(argv[n], str + j, i+1 - j);
            argv[n++][i+1 - j] = '\0';
            j=i+1;

        }
    }

    argv[n] = NULL;

    free(str);

    return argv;

free_argv:
    for (; n >= 0; n--)
        free(argv[n]);
    free(argv);
    free(str);
    return NULL;
}

const char* syntax_error_fmt = "bash: syntax error near unexcepted token `%s'\n";

struct command *parse (char **tokens){
    int i, j, n = 1;
    struct command *c;
    /* count commands */
    for (i = 0; tokens[i]; i++) {
        if (tokens[i][0] == '&') {
            if (!tokens[i][1]) {
                if (tokens[i+1]) {
                    fprintf(stderr, syntax_error_fmt, tokens[i+1]);
                    return NULL;
                }
            } else if (tokens[i][1] == '&' && !tokens[i][2]) {
                n++;
            } else {
                /*
                 *  && is the only accepted elem starting with &
                 *  if we aren't at the end
                 */
                fprintf(stderr, syntax_error_fmt, tokens[i]);
                return NULL;
            }
        } else if (tokens[i][0]=='|') {
            if (tokens[i][1] == '|' && !tokens[2]) {
                n++;
            } else {
                /* || is the only accepted elem starting with | */
                fprintf(stderr, syntax_error_fmt, tokens[i]);
                return NULL;
            }
        }
    }

    /* now allocate our array of commands */
    c = malloc(sizeof(struct command) * (n + 1));
    if (!c) {
        fprintf(stderr, "lack of memory");
        return NULL;
    }

    for (i = j = n = 0; tokens[i]; i++) {
        if (tokens[i][0] == '&') {
            if (tokens[i][1])
                c[n].type = AND;
            else
                c[n].type = BACKGROUND;
            c[n++].argv = tokens + j;
            free(tokens[j = i]);
            tokens[j++] = NULL;
        } else if (tokens[i][0] == '|' && tokens[i][1]) {
            c[n].type = OR;
            c[n++].argv = tokens + j;
            free(tokens[j = i]);
            tokens[j++] = NULL;
        }
    }

    if (c[n-1].type != BACKGROUND) {
        c[n].type = NORMAL;
        c[n++].argv = tokens + j;
    }
    
    c[n].argv = NULL; /* to know where the array ends */

    return c;
}


/*
  il faut quon fork le process
  pi dans le child process quon fasse le exec
  pi dans lautre process faut quon check squi spasse */
int execute (struct command *cmd) {

    return -1;
}

void shell (void) {
    char **tokens;
    struct command *cmds;
    int i = 0, j = 0;

    /* temporary testing */
    char *line = readLine();
    tokens = tokenize(line);
    cmds = parse(tokens);
    for (i = 0; cmds[i].argv; i++) {
        printf("argv: \n  ");
        for (j = 0; cmds[i].argv[j]; j++)
            printf("%s ", cmds[i].argv[j]);
        puts("");
        switch (cmds[i].type) {
        case NORMAL: puts("NORMAL"); break;
        case BACKGROUND: puts("BACKGROUND"); break;
        case AND: puts("AND"); break;
        case OR: puts("OR"); break;
        }
    }

    for (i = 0; cmds[i].argv; i++) {
        for (j = 0; cmds[i].argv[j]; j++)
            free(cmds[i].argv[j]);
    }
    free(cmds[0].argv);
    free(cmds);
}

/*
 * Dont change main!
 *
 * You may add unit tests here, but we will rip out main entirely when we grade your work.
 */
int main (void) {
    shell();
}
