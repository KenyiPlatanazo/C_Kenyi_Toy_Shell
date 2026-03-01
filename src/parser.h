#ifndef PARSER
#define PARSER
#include <asm-generic/errno-base.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define TOKEN_BUFFSIZE 64
#define VALUE 1024
#define MAX_TOKENS VALUE
#define MAX_COMMANDS VALUE
#define MAX_ARGS VALUE
#define DEFAULT_FD 0

enum Command {
  SHELL_UNKNOWN,
  SHELL_ECHO,
  SHELL_EXIT,
  SHELL_TYPE,
  SHELL_PWD,
  SHELL_CD,
};

// Raw tokens
enum token_type {
  TOKEN_WORD,
  TOKEN_REDIR_OUT,
  TOKEN_REDIR_APPEND,
  TOKEN_REDIR_IN,
  TOKEN_PIPE,
  TOKEN_OR
};

struct raw_token {
  enum token_type type;
  char *value;
};

// The tokens are processed as arguments or redirects
enum redir_type { REDIR_OUT, REDIR_APPEND, REDIR_IN };

struct t_redir {
  int fd;
  enum redir_type type;
  char *filename;
};

struct t_command {
  char **argv;
  int argc;
  struct t_redir *redirs;
  int redir_count;
  int redir_capacity;
};

struct t_pipeline {
  struct t_command *commands;
  int count;
};

enum lexical_state { NORMAL, IN_SINGLE_QUOTE, IN_DOUBLE_QUOTE };

void process_command(char *command);
int count_command(int argc, struct raw_token *tokens[]);
void dispatch(struct t_pipeline *pipeline);
enum Command parse_command(const char *command);

int tokenize(char *line, struct raw_token *argv[]);
void append_char(char *buffer, int *idx, char c);
void skip_spaces(const char *line, int *i);
void read_token(const char *line, int *i, char *buffer);

void process_chars(struct raw_token *new_token, char *s);
void process_redir(struct t_redir *redir_buffer, struct raw_token *token,
                   char *filename, int fd);
void parse_tokens(struct t_pipeline *pipe, int argc, struct raw_token *argv[]);
bool is_token_pure_digits(struct raw_token *token);
bool is_token_pure_digits(struct raw_token *token);
void init_pipeline(struct t_pipeline *pipeline, int argc,
                   struct raw_token *tokens[]);
void analyze_commands(struct t_pipeline *pipeline, int argc,
                      struct raw_token *tokens[]);
void destroy_pipeline(struct t_pipeline *pipeline);
void free_tokens(struct raw_token *tokens[], int argc);
bool validate_syntax(int argc, struct raw_token *tokens[]);

void handle_redirections(struct t_command *command);

void exec_builtin(enum Command type, struct t_command *command);

#endif // PARSER
