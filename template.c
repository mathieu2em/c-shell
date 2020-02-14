/* Mathieu Perron 20076170, Amine Sami */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>

#define true 1
#define false 0
#define bool int

#define MIN_SIZE 128
#define IS_SPECIAL(c) ((c) == '&' || (c) == '|')

enum { NORMAL, AND, OR };

struct command {
    char **argv;
    char type;
    char rnfn;
    int n;
};

struct cmdline {
    struct command *commands;
    bool is_background;
};

int syntax_error (char *);
int insert_char (char *, int, char, int);
char *readLine (void);
int verify_rnfn (char *, char);
int syntax_verifier (char *);
char **tokenize (char *);
struct cmdline parse (char **);
int execute_cmd (struct command);
int execute (struct cmdline);

const char* syntax_error_fmt = "bash: syntax error near unexpected token `%s'\n";

int syntax_error(char *str) {
    fprintf(stderr, syntax_error_fmt, str);
    return -1;
}

int insert_char (char *str, int pos, char c, int len)
{
    char *saved;
    if (pos == len - 1) {
        len *= 2;
        str = (char*) realloc(saved = str, sizeof(char) * len);
        if (!str) {
            fprintf(stderr, "string could not be extended\n");
            free(saved);
            return -1;
        }
    }
    str[pos] = c;
    return len;
}

/* readline allocate a new char array */
char* readLine (void) {
    char *line;
    int c, p, n = 0, len = MIN_SIZE;

    line = (char*) malloc(sizeof(char) * len); /* initialize array */
    if (!line) {
        fprintf(stderr, "could not initialize string\n");
        return NULL;
    }

    while ((c = getchar()) != EOF && c != '\n') {
        if (IS_SPECIAL(c)) {
            len = insert_char(line, n++, ' ', len);
            if (len < 0)
                return NULL;
            
            len = insert_char(line, n++, (char) (p = c), len);
            if (len < 0)
                return NULL;
            
            while ((c = getchar()) == p) {
                len = insert_char(line, n++, (char) c, len);
                if (len < 0)
                    return NULL;
            }
            
            if (c == EOF || c == '\n')
                break;

            len = insert_char(line, n++, ' ', len);
        }
        len = insert_char(line, n++, (char) c, len);
        if (len < 0)
            return NULL;
    }

    if (n == 0) {
        free(line);
        exit(0);
    }
    
    line[n] = '\0'; /* null terminate string */

    return line;
}

/* tokenize arguments */
char **tokenize(char *str){
    const char *delim = " \t\n()";
    char **tokens, **saved, *next_tok;
    int i = 0, len = MIN_SIZE;

    /* get the first token */
    next_tok = strtok(str, delim);

    tokens = (char**) malloc(sizeof(char*) * len);
    if (!tokens) {
        fprintf(stderr, "could not initialize tokens array\n");
        return NULL;
    }
    
    /* walk through other tokens */
    while (next_tok) {
        if (i == len - 1) {
            len *= 2;
            tokens = (char**) realloc(saved = tokens, sizeof(char*) * len);
            if (!tokens) {
                fprintf(stderr, "could not resize tokens array\n");
                free(saved);
                return NULL;
            }
        }
        tokens[i++] = next_tok;
        /* subsequent calls to strtok with the same string must pass NULL */
        next_tok = strtok(NULL, delim);
    }
    
    tokens[i] = NULL;

    return tokens;
}

/*
char **old_tokenize (void) {
    char *string[256];
    char **retstr;

    // malloc the string for the return string value
    retstr = malloc(sizeof(char *) * (i + 1));
    // if memory error
    if(retstr==NULL){
        fprintf(stderr, "error lack of memory\n");
        return NULL;
    }
    i = 0;
    // copy the string to return string
    while(string[i]!=NULL){
        for(j=0; string[i][j]; j++);
        retstr[i] = malloc(sizeof(char)*(j+1));
        // if memory error
        if(retstr[i]==NULL){
            for(j=0; j<i; j++){
                free(retstr[j]);
            }
            free(retstr);
            fprintf(stderr, "error lack of memory\n");
            return NULL;
        }
        strcpy(retstr[i], string[i]);
        i++;
    }
    retstr[i]=NULL;
    return retstr;
}
*/

/* this method adds spaces so that echo a&&echo b is accepted as echo a && echo b */
/*
char *spacer(char *str){
    char string[256];
    char *result;
    int i = 0;
    int j = 0;

    while(str[i]){
        if(IS_SPECIAL(str[i])){
            if(str[i+1]){
                if(IS_SPECIAL(str[i+1])) {
                    string[j++] = ' ';
                    string[j++] = str[i++];
                    string[j++] = str[i];
                    string[j++] = ' ';
                } else {
                    char str2[2] = {str[i],str[i+1]};
                    syntax_error(str2);
                    free(str);
                    return NULL;
                }
            } else if (!str[i+1]) {
                string[j++]=' ';
                string[j++]=str[i];
                string[j++]=' ';
            }
            if(str[i+2] && IS_SPECIAL(str[i+2])){
                char str6[6] = {str[i],str[i+1],str[i+2],'.','.','.'};
                syntax_error(str6);
                free(str);
                return NULL;
            }
        } else {
            string[j++]=str[i];
        }
        i++;
    }
    string[j]='\0';

    result = malloc(sizeof(char)*(j+1));
    if(!result){
        fprintf(stderr, "lack of memory\n");
        free(str);
        return NULL;
    }

    strcpy(result, string);
    free(str);
    return result;
}
*/

/* verify if rn and fn respect their given synthax */
int verify_rnfn(char *str, char verifier){
    char *ret;
    int i = 0;

    while((ret = strchr(str+i, verifier))){
        i = (int)(ret-str+1);
        if(isdigit(str[i++])){
            while(isdigit(str[i])) i++;
            if(str[i]!='('){
                return false;
            } else {
                while(str[i] && str[i]!=')') i++;
                if(str[i] && str[i] != ')'){
                    return false;
                }
            }
        }
    }
    return true;
}

/* verify very lightly the syntax for rN and fN */
int syntax_verifier(char *str){
    /* verify for rN */
    if (!verify_rnfn(str, 'r') || !verify_rnfn(str, 'f'))
        return false;
    return true;
}

/* creates the cmdline structure that containes every commands with their
   types and other particularities */
struct cmdline parse (char **tokens) {
    int i, j, k, n = 1;
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
        } else if (tokens[i][0] == '|') {
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

    for (i=0; i<n; i++) {
        cmd_line.commands[i].rnfn = '0';
        cmd_line.commands[i].n = 0;
    }

    /* now create the right structures for commands */
    for (i = j = n = 0; tokens[i]; i++) {
        /* if the argument is a and get arguments before the and and set type to AND */
        if (tokens[i][0] == '&') {
            if (tokens[i][1])
                cmd_line.commands[n].type = AND;
            else
                cmd_line.is_background = true;
            cmd_line.commands[n++].argv = tokens + j;
            tokens[i][0] = '\0';
            j = i;
            tokens[j++] = NULL;
        } else if (tokens[i][0] == '|' && tokens[i][1]) {
        /* same for or */
            cmd_line.commands[n].type = OR;
            cmd_line.commands[n++].argv = tokens + j;
            tokens[i][0] = '\0';
            j = i;
            tokens[j++] = NULL;
        }
        
        // also a integer and put the right execute logic for each cases
        else if (tokens[i][0]=='r' || tokens[i][0]=='f') {
            k=1;
            while(tokens[i][k] && isdigit(tokens[i][k++]));
            if(!tokens[i][k] && isdigit(tokens[i][k-1])) {
                cmd_line.commands[n].rnfn = (tokens[i][0] == 'f') ? 'f' : 'r';
                cmd_line.commands[n].n = atoi(tokens[i] + 1);
                tokens[i][0] = '\0';
                j = i;
                tokens[j++] = NULL;
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
        child_code = WEXITSTATUS(child_code);
    }

    /* child_code should only be negative if fork or execvp failed */
    return child_code;
}

/*
  execute & fork process
*/
int execute (struct cmdline cmd_line) {
    int i, ret;
    pid_t pid;
    struct command *cmds = cmd_line.commands;

    if (cmd_line.is_background) {
        pid = fork();
        if (pid < 0) {
            /* si le fork a fail */
            fprintf(stderr, "could not fork process\n");
            return -1;
        } else if (pid != 0) {
            return 0;
        }
    }

    for (i = 0; cmds[i].argv != NULL; i++) {
        if (cmds[i].rnfn == 'r' || cmds[i].rnfn == 'f') {
            for (int a = 0; a < cmds[i].n; a++) {
                ret = (cmds[i].rnfn == 'f') ? 0 : execute_cmd(cmds[i]);
                if (ret < 0) {
                    /* here if error in execvp or fork */
                    //free_commands(cmds);
                    /* exit after, we are inside child process or fork failed */
                    return -1;
                } else if (ret == 0) {
                    if (cmds[i].type == OR) {
                        while (cmds[i].argv && cmds[i].type != AND)
                            i++;
                        if (!(cmds[i].argv && cmds[i].type == AND))
                            return 1;
                    }
                } else {
                    if (cmds[i].type == AND) {
                        while (cmds[i].argv && cmds[i].type != OR)
                            i++;
                        if (!(cmds[i].argv && cmds[i].type == OR))
                            return 1;
                    }
                }
            }
        } else {
            ret = execute_cmd(cmds[i]);
            if (ret < 0) {
                /* here if error in execvp or fork */
                /* exit after, we are inside child process or fork failed */
                return -1;
            } else if (ret == 0) {
                if (cmds[i].type == OR) {
                    while (cmds[i].argv && cmds[i].type != AND)
                        i++;
                    if (!(cmds[i].argv && cmds[i].type == AND)) {
                        //free_commands(cmds);
                        return 1;
                    }
                }
            } else {
                if (cmds[i].type == AND) {
                    while (cmds[i].argv && cmds[i].type != OR)
                        i++;
                    if (!(cmds[i].argv && cmds[i].type == OR))
                        return 1;
                }
            }
        }
    }
    return 0;
}

void shell (void) {
    char *line;
    char **tokens;
    struct cmdline cmd_ln;

    while (1) {
        line = readLine();
        if (!syntax_verifier(line)) {
            free(line);
            fprintf(stderr, syntax_error_fmt, "'rN' or 'fN'");
        } else if (line) {
            tokens = tokenize(line);
            cmd_ln = parse(tokens);
            execute(cmd_ln);

            free(cmd_ln.commands);
            free(tokens);
            free(line);
            
            line = NULL;
        }
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
