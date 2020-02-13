#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>

#define true 1
#define false 0
#define bool int

#define MIN_STR 128
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t')
#define IS_SPECIAL(c) ((c) == '&' || (c) == '|')
#define INSIDE_WORD(c) (!(IS_SPECIAL(c) || IS_SPACE(c)))

enum { NORMAL, AND, OR };

struct command {
    char **argv;
    char type;
    char rnfn;
};

struct cmdline {
    struct command *commands;
    bool is_background;
};

char *readLine (void);
int count_tokens (char *);
char **tokenize (char *);
struct cmdline parse (char **);
int execute (struct cmdline);
void free_commands (struct command *);

int syntax_error(){
    fprintf(stderr, "syntax error!\n");
    return -1;
}

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

/* extract all arguments */
char **tokenize(char *str){
    char *string[256];
    char **retstr;
    char *token;
    char *s = " \t\n()";
    int i = 0, j = 0;

    /* get the first token */
    token = strtok(str, s);

    /* walk through other tokens */
    while( token != NULL ) {
        printf( " %s\n", token );
        string[i++]=token;
        token = strtok(NULL, s);
    }
    string[i]=NULL;

    /* malloc the string for the return string value */
    retstr = malloc(sizeof(char *)*(i+1));
    i=0;
    while(string[i]!=NULL){
        for(j=0; string[i][j]; j++);
        retstr[i] = malloc(sizeof(char)*(j+1));
        strcpy(retstr[i], string[i]);
        i++;
    }
    retstr[i]=NULL;
    return retstr;
}

char *spacer(char *str){
    char string[1000];
    char *result;
    int i = 0;
    int j = 0;

    while(str[i]){
        if(IS_SPECIAL(str[i])){
            if(str[i+1] && IS_SPECIAL(str[i+1])){
                string[j++]=' ';
                string[j++]=str[i++];
                string[j++]=str[i];
                string[j++]=' ';
            } else if (!str[i+1]) {
                string[j++]=' ';
                string[j++]=str[i];
                string[j++]=' ';
            }
            if(str[i+2] && IS_SPECIAL(str[i+2])){
                return syntax_error(); // TODO not sure how to fix this
            }
        } else {
            string[j++]=str[i];
        }
        i++;
    }
    string[j]='\0';
    for(i=0; string[i]; i++);
    result = malloc(sizeof(char)*(i+1));
    strncpy(result, string, i);
    printf("%s\n", result);
    return result;
}

int verify_rnfn(char *str, char verifier){
    char *ret;
    int i;
    /* verify for rN */
    while((ret = strchr(str, verifier))){
        i = ret-str+1;
        if(isdigit(str[++i])){
            while(isdigit(str[i++]));
            if(str[i]!='('){
                fprintf(stderr, "syntax error");
                return false;
            } else {
                while(str[i] && str[i++]!=')');
                if(str[i] && str[i] != ')'){
                    fprintf(stderr, "syntax error");
                    return false;
                }
            }
        }
    }
    return true;
}

/* verify syntax */
int syntax_verifier(char *str){
    /* verify for rN */
    if(!verify_rnfn(str, 'r') || !verify_rnfn(str, 'f')) return false;
    return true;
}


const char* syntax_error_fmt = "bash: syntax error near unexpected token `%s'\n";

struct cmdline parse (char **tokens){
    int i, j, n = 1;
    struct cmdline cmd_line;
    cmd_line.is_background = false;

    /* count commands */
    for (i = 0; tokens[i]; i++) {
        if (tokens[i][0] == '&') {
            if (!tokens[i][1]) {
                if (tokens[i+1]) {
                    fprintf(stderr, syntax_error_fmt, tokens[i+1]);
                    cmd_line.commands = NULL;
                    return cmd_line;
                }
            } else if (tokens[i][1] == '&' && !tokens[i][2]) {
                n++;
            } else {
                /*
                 *  && is the only accepted elem starting with &
                 *  if we aren't at the end
                 */
                fprintf(stderr, syntax_error_fmt, tokens[i]);
                cmd_line.commands = NULL;
                return cmd_line;
            }
        } else if (tokens[i][0]=='|') {
            if (tokens[i][1] == '|' && !tokens[i][2]) {
                n++;
            } else {
                /* || is the only accepted elem starting with | */
                fprintf(stderr, syntax_error_fmt, tokens[i]);
                cmd_line.commands = NULL;
                return cmd_line;
            }
        }
    }

    /* now allocate our array of commands */
    cmd_line.commands = malloc(sizeof(struct command) * (n + 1));
    if (!cmd_line.commands) {
        fprintf(stderr, "lack of memory");
        cmd_line.commands = NULL;
        return cmd_line;
    }

    for (i = j = n = 0; tokens[i]; i++) {
        if (tokens[i][0] == '&') {
            if (tokens[i][1])
                cmd_line.commands[n].type = AND;
            else
                cmd_line.is_background = true;
            cmd_line.commands[n++].argv = tokens + j;
            free(tokens[j = i]);
            tokens[j++] = NULL;
        } else if (tokens[i][0] == '|' && tokens[i][1]) {
            cmd_line.commands[n].type = OR;
            cmd_line.commands[n++].argv = tokens + j;
            free(tokens[j = i]);
            tokens[j++] = NULL;
        }
        // TODO if rN or fN , do something , add new rn fn boolean to struct and
        // also a integer and put the right execute logic for each cases
        else if (tokens[i][0]=='r') {
            j=1;
            while(tokens[i][j]){
                if(!isdigit(tokens[i][j++])){
                    fprintf(stderr, syntax_error_fmt, tokens[i]);
                    cmd_line.commands = NULL;
                    return cmd_line;
                }
            }
        }
    }

    if (!cmd_line.is_background) {
        cmd_line.commands[n].type = NORMAL;
        cmd_line.commands[n++].argv = tokens + j;
    }

    cmd_line.commands[n].argv = NULL; /* to know where the array ends */

    return cmd_line;
}

int execute_cmd (struct command cmd) {
    int child_code = 0;
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "could not fork process\n");
        return -1;
    } else if (pid == 0) {
        child_code = execvp(cmd.argv[0], cmd.argv);
        /* execvp only returns on error */
        fprintf(stderr, "bash: %s: command not found\n", cmd.argv[0]);
    } else {
        waitpid(pid, &child_code, 0);
        /* printf("pid : %d, argv[0] : %s\n", pid, cmd.argv[0]); */
        child_code = WEXITSTATUS(child_code);
    }

    /* child_code should only be negative if fork or execvp failed */
    return child_code;
}

/*
  il faut quon fork le process
  pi dans le child process quon fasse le exec
  pi dans lautre process faut quon check squi spasse */
int execute (struct cmdline cmd_line) {
    int i, ret;
    pid_t pid;
    struct command *cmds = cmd_line.commands;

    if(cmd_line.is_background){
        pid = fork();
        if (pid < 0) {
            /* si le fork a fail */
            fprintf(stderr, "could not fork process\n");
            free_commands(cmds);
            return -1;
        } else if (pid != 0) {
            /* t'es dans le parent */
            free_commands(cmds);
            return 0;
        }
    }

    for (i = 0; cmds[i].argv; i++) {
        ret = execute_cmd(cmds[i]);

        if (ret < 0) {
            /* here if error in execvp or fork */
            free_commands(cmds);
            /* exit after, we are inside child process or fork failed */
            return -1;
        } else if (ret == 0) {
            if (cmds[i].type == OR) {
                while (cmds[i].argv && cmds[i].type != AND)
                    i++;
                if (!(cmds[i].argv && cmds[i].type == AND)) {
                    free_commands(cmds);
                    return 1;
                }
            }
        } else {
            if (cmds[i].type == AND) {
                while (cmds[i].argv && cmds[i].type != OR)
                    i++;
                if (!(cmds[i].argv && cmds[i].type == OR)) {
                    free_commands(cmds);
                    return 1;
                }
            }
        }
    }

    free_commands(cmds);
    return 0;
}

void free_commands (struct command *cmds) {
    int i, j;
    for (i = 0; cmds[i].argv; i++) {
        for (j = 0; cmds[i].argv[j]; j++)
            free(cmds[i].argv[j]);
}
    free(cmds[0].argv);
    free(cmds);
}

void shell (void) {
    char *line;
    char **tokens;
    struct cmdline cmd_ln;
    //int i = 0, j = 0;

    //TODO add fN and lN functions as demanded
    while (1) {
        line = readLine();
        if (!syntax_verifier(line)){
            fprintf(stderr, "syntax error\n");
        }
        line = spacer(line);
        tokens = tokenize(line);
        // test
        int i=0;
        while(tokens[i]) printf("%s\n", tokens[i++]);
        //----------
        cmd_ln = parse(tokens);
        if (execute(cmd_ln) < 0)
            exit(1);
    }
}
/* temporary testing
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
*/

/*
 * Dont change main!
 *
 * You may add unit tests here, but we will rip out main entirely when we grade your work.
 */
int main (void) {
    shell();
}
