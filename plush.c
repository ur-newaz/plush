// simpple shell project for cse321 
// target features includes basic built in commands, history support, signal handling, pipping, inline commands and in sequencing
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_HISTORY 100
#define MAX_TOKENS 100

char *history[MAX_HISTORY];
int history_count = 0;

// Signal handling part
void handle_sigint(int sig) {
    printf("\nInterrupted. type exit or ctrl+d.\nplush> ");
    fflush(stdout);
}

// History support in array, Arrow keys won't work. Simply saves each given command in an array.
void add_to_history(char *cmd) {
    if (history_count < MAX_HISTORY)
        history[history_count++] = strdup(cmd);
}
// called when history command is used
void show_history() {
    for (int i = 0; i < history_count; i++)
        printf("%d: %s\n", i + 1, history[i]);
}

// a binary tree struct which will handle | ; and &&
typedef enum { CMD, SEQ, AND, PIPE } NodeType;

typedef struct BST {
    NodeType type;
    char *cmd;
    struct BST *left;
    struct BST *right;
} BST;

BST *new_node(NodeType type, char *cmd, BST *left, BST *right) {
    BST *node = malloc(sizeof(BST));
    node->type = type;
    node->cmd = cmd ? strdup(cmd) : NULL;
    node->left = left;
    node->right = right;
    return node;
}

// turning command and arguements to token
char **tokenize(char *line, int *count) {
    char **tokens = malloc(MAX_TOKENS * sizeof(char *));
    char *token = strtok(line, " "); // separation based on spaces
    *count = 0;
    while (token) {
        tokens[(*count)++] = token;
        token = strtok(NULL, " ");
    }
    tokens[*count] = NULL;
    return tokens;
}

// parsing BNST based on ; && |
BST *parse_tokens(char **tokens, int start, int end) {
    int i, level = 0;
    for (i = end; i >= start; --i) {
        if (strcmp(tokens[i], ";") == 0 && level == 0)
            return new_node(SEQ, NULL, parse_tokens(tokens, start, i - 1), parse_tokens(tokens, i + 1, end));
        if (strcmp(tokens[i], "&&") == 0 && level == 0)
            return new_node(AND, NULL, parse_tokens(tokens, start, i - 1), parse_tokens(tokens, i + 1, end));
        if (strcmp(tokens[i], "|") == 0 && level == 0)
            return new_node(PIPE, NULL, parse_tokens(tokens, start, i - 1), parse_tokens(tokens, i + 1, end));
    }

    // command
    int len = 0;
    for (i = start; i <= end; ++i)
        len += strlen(tokens[i]) + 1;

    char *cmd = malloc(len);
    cmd[0] = '\0';
    for (i = start; i <= end; ++i) {
        strcat(cmd, tokens[i]);
        if (i < end)
            strcat(cmd, " ");
    }

    return new_node(CMD, cmd, NULL, NULL);
}

// input output redicrections using file descriptor number
void handle_redirection(char **args) {
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); //overwrites, creates file if not available
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], ">>") == 0) { //appends to exisiting file.
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) { //reads the file
            int fd = open(args[i + 1], O_RDONLY);
            dup2(fd, 0); // type 0 for stdin
            close(fd);
            args[i] = NULL;
        }
    }
}

// custom commands exit and history, similarly more can be added.
void exec_cmd(char *cmd) {
    while (*cmd == ' ') cmd++;
    if (strlen(cmd) == 0) return;

    char cmd_copy[1024];
    strcpy(cmd_copy, cmd);
    for (int i = 0; cmd_copy[i]; i++) cmd_copy[i] = tolower(cmd_copy[i]); //case insensitive

    if (strcmp(cmd_copy, "exit") == 0) exit(0);
    if (strcmp(cmd_copy, "history") == 0) {
        show_history();
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        char *args[100];
        int i = 0;
        args[i++] = strtok(cmd, " ");
        while ((args[i++] = strtok(NULL, " ")) != NULL);
        handle_redirection(args);
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }
}

void execute(BST *node) {
    if (!node) return;

    if (node->type == CMD) {
        exec_cmd(node->cmd);
    } else if (node->type == SEQ) {
        execute(node->left);
        execute(node->right);
    } else if (node->type == AND) {
        pid_t pid = fork();
        if (pid == 0) {
            exec_cmd(node->left->cmd);
            exit(0);
        } else {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                execute(node->right);
        }
    } else if (node->type == PIPE) {
        int pipefd[2];
        pipe(pipefd);
        pid_t p1 = fork();
        if (p1 == 0) {
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            execute(node->left);
            exit(0);
        }
        pid_t p2 = fork();
        if (p2 == 0) {
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[1]);
            close(pipefd[0]);
            execute(node->right);
            exit(0);
        }
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(p1, NULL, 0);
        waitpid(p2, NULL, 0);
    }
}

int main() {
    struct sigaction sa; // no idea why this error shows but code works.
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    char *line = NULL;
    size_t len = 0;

    while (1) {
        printf("plush> ");
        fflush(stdout);
        ssize_t nread = getline(&line, &len, stdin);
        if (nread == -1) {
            printf("\n");
            break;
        }
        if (line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        if (strlen(line) == 0) continue;

        add_to_history(line);

        int token_count = 0;
        char *line_copy = strdup(line);
        char **tokens = tokenize(line_copy, &token_count);
        BST *root = parse_tokens(tokens, 0, token_count - 1);
        execute(root);
        free(line_copy);
        free(tokens);
    }

    free(line);
    return 0;
}
