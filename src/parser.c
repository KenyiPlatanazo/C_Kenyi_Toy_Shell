#include "parser.h"
#include <unistd.h>

void handle_type(struct t_command *command);
void check_type(const char *command);
void handle_echo(struct t_command *command);
void handle_exec(struct t_command *command);
void handle_pwd(struct t_command *command);
void handle_cd(struct t_command *command);

void process_command(char *command) {
  struct raw_token *tokens[MAX_TOKENS];
  int argc = tokenize(command, tokens);
  if (argc == 0)
    return;
  if (!validate_syntax(argc, tokens)) {
    fprintf(stderr, "bash: syntax error\n");
    free_tokens(tokens, argc);
    return;
  }
  struct t_pipeline pipeline;
  init_pipeline(&pipeline, argc, tokens);
  parse_tokens(&pipeline, argc, tokens);
  dispatch(&pipeline);
  destroy_pipeline(&pipeline);
  free_tokens(tokens, argc);
}

int count_command(int argc, struct raw_token *tokens[]) {
  int count = 1;
  for (int i = 0; i < argc; i++) {
    if (tokens[i]->type == TOKEN_PIPE)
      count++;
  }
  return count;
}

void process_chars(struct raw_token *new_token, char *s) {
  enum token_type type = TOKEN_WORD;
  if (strcmp(s, ">") == 0) {
    type = TOKEN_REDIR_OUT;
  } else if (strcmp(s, ">>") == 0) {
    type = TOKEN_REDIR_APPEND;
  } else if (strcmp(s, "<") == 0) {
    type = TOKEN_REDIR_IN;
  } else if (strcmp(s, "|") == 0) {
    type = TOKEN_PIPE;
  } else if (strcmp(s, "||") == 0) {
    type = TOKEN_OR;
  }
  new_token->type = type;
  new_token->value = strdup(s);
}

int tokenize(char *line, struct raw_token *argv[]) {
  int argc = 0;
  int i = 0;
  while (line[i] != '\0') {
    struct raw_token *new_token = malloc(sizeof(struct raw_token));
    if (!new_token) {
      fprintf(stderr, "Error while trying to malloc memory for new token");
      exit(EXIT_FAILURE);
    }
    skip_spaces(line, &i);
    if (line[i] == '\0') {
      free(new_token);
      break;
    }
    char token_buffer[TOKEN_BUFFSIZE];
    read_token(line, &i, token_buffer);
    // Add a new function
    process_chars(new_token, token_buffer);
    argv[argc] = new_token;
    // argv[argc] = strdup(token_buffer);
    if (!argv[argc]) {
      perror("strdup");
      exit(EXIT_FAILURE);
    }
    argc++;
    if (argc >= MAX_TOKENS - 1)
      break;
  }
  argv[argc] = NULL;
  return argc;
}

void append_char(char *buffer, int *idx, char c) {
  if (*idx < TOKEN_BUFFSIZE - 1) {
    buffer[(*idx)++] = c;
  } else {
    fprintf(stderr, "Token too long\n");
    exit(EXIT_FAILURE);
  }
}
void skip_spaces(const char *line, int *i) {
  while (line[*i] == ' ') {
    (*i)++;
  }
}

void read_token(const char *line, int *i, char *buffer) {
  int buffer_index = 0;
  bool done = false;
  enum lexical_state state = NORMAL;
  while (line[*i] != '\0' && !done) {
    char current_char = line[*i];
    switch (state) {
    case NORMAL: {
      switch (current_char) {
      case '>':
        if (buffer_index == 0) {
          append_char(buffer, &buffer_index, '>');
          if (line[*i + 1] == '>') {
            (*i)++;
            append_char(buffer, &buffer_index, '>');
          }
          (*i)++;
        }
        done = true;
        break;
      case '<':
        if (buffer_index == 0) {
          append_char(buffer, &buffer_index, '<');
          (*i)++;
        }
        done = true;
        break;
      case '|':
        if (buffer_index == 0) {
          append_char(buffer, &buffer_index, '|');
          if (line[*i + 1] == '|') {
            (*i)++;
            append_char(buffer, &buffer_index, '|');
          }
          (*i)++;
        }
        done = true;
        break;
      case ' ':
        done = true;
        break;
      case '\'':
        state = IN_SINGLE_QUOTE;
        break;
      case '"':
        state = IN_DOUBLE_QUOTE;
        break;
      case '\\':
        (*i)++;
        if (line[*i] != '\0')
          append_char(buffer, &buffer_index, line[*i]);
        break;
      default:
        append_char(buffer, &buffer_index, current_char);
      };
      // This break is for the while loop. Don't misread it bietchi
      break;
    }
    case IN_SINGLE_QUOTE: {
      if (current_char == '\'') {
        state = NORMAL;
      } else {
        append_char(buffer, &buffer_index, current_char);
      }
      break;
    }
    case IN_DOUBLE_QUOTE: {
      switch (current_char) {
      case '"':
        state = NORMAL;
        break;
      case '\\':
        if (line[(*i) + 1] == '\0')
          break;
        char next = line[(*i) + 1];
        if (next == '"' || next == '\\' || next == '$' || next == '`') {
          (*i)++;
          append_char(buffer, &buffer_index, next);
        } else {
          append_char(buffer, &buffer_index, current_char);
        }
        break;
      default:
        append_char(buffer, &buffer_index, current_char);
        break;
      }
      break;
    }
    }
    if (!done)
      (*i)++;
  }
  buffer[buffer_index] = '\0';
  // I won't support unclosed quotes
  if (state != NORMAL) {
    fprintf(stderr, "Unmatched quote\n");
    exit(EXIT_FAILURE);
  }
}

void dispatch(struct t_pipeline *pipeline) {
  for (int i = 0; i < pipeline->count; i++) {
    enum Command cmd_type = parse_command(pipeline->commands[i].argv[0]);
    struct t_command *cmd = &pipeline->commands[i];
    if (cmd_type != SHELL_UNKNOWN) {
      exec_builtin(cmd_type, cmd);
    } else {
      handle_exec(cmd);
    }
  }
}

void exec_builtin(enum Command type, struct t_command *command) {
  int saved_stdout = dup(STDOUT_FILENO);
  int saved_stdin = dup(STDIN_FILENO);

  if (saved_stdout < 0 || saved_stdin < 0) {
    perror("Erro while duping stdout");
    return;
  }
  handle_redirections(command);

  switch (type) {
  case SHELL_EXIT:
    exit(EXIT_SUCCESS);
    break;
  case SHELL_CD:
    handle_cd(command);
    break;
  case SHELL_TYPE:
    handle_type(command);
    break;
  case SHELL_PWD:
    handle_pwd(command);
    break;
  case SHELL_ECHO:
    handle_echo(command);
    break;
  default:
    break;
  }

  dup2(saved_stdout, STDOUT_FILENO);
  dup2(saved_stdin, STDIN_FILENO);
  close(saved_stdout);
  close(saved_stdin);
}

enum Command parse_command(const char *command) {
  if (!command)
    return SHELL_UNKNOWN;

  if (strcmp(command, "exit") == 0)
    return SHELL_EXIT;
  if (strcmp(command, "echo") == 0)
    return SHELL_ECHO;
  if (strcmp(command, "type") == 0)
    return SHELL_TYPE;
  if (strcmp(command, "pwd") == 0)
    return SHELL_PWD;
  if (strcmp(command, "cd") == 0)
    return SHELL_CD;
  return SHELL_UNKNOWN;
}

void parse_tokens(struct t_pipeline *pipeline, int argc,
                  struct raw_token *argv[]) {
  int current_cmd = 0;
  int arg_index = 0;
  int redir_index = 0;

  for (int i = 0; i < argc; i++) {
    switch (argv[i]->type) {
    case TOKEN_WORD: {
      pipeline->commands[current_cmd].argv[arg_index++] = argv[i]->value;
      break;
    }
    case TOKEN_PIPE: {
      pipeline->commands[current_cmd].argv[arg_index] = NULL;
      current_cmd++;
      arg_index = 0;
      redir_index = 0;
      break;
    }
    case TOKEN_REDIR_APPEND:
    case TOKEN_REDIR_IN:
    case TOKEN_REDIR_OUT: {
      struct raw_token *next = argv[i + 1];
      process_redir(&pipeline->commands[current_cmd].redirs[redir_index++],
                    argv[i], next->value);
      // We skip over the filename as to not take as an argv for the command
      i++;
      break;
    }
    case TOKEN_OR:
      // Will be implemented later
      break;
    }
  }
  pipeline->commands[current_cmd].argv[arg_index] = NULL;
}

bool validate_syntax(int argc, struct raw_token *tokens[]) {
  if (argc == 0)
    return false;
  if (tokens[0]->type != TOKEN_WORD)
    return false;
  if (tokens[argc - 1]->type != TOKEN_WORD)
    return false;
  for (int i = 0; i < argc; i++) {
    switch (tokens[i]->type) {
    case TOKEN_PIPE:
      if (i + 1 >= argc)
        return false;
      if (tokens[i + 1]->type == TOKEN_PIPE)
        return false;
      break;
    case TOKEN_REDIR_IN:
    case TOKEN_REDIR_OUT:
    case TOKEN_REDIR_APPEND:
      if (i + 1 >= argc || tokens[i + 1]->type != TOKEN_WORD)
        return false;
      break;
    default:
      break;
    }
  }
  return true;
}

void analyze_commands(struct t_pipeline *pipeline, int argc,
                      struct raw_token *tokens[]) {
  int current = 0;

  for (int i = 0; i < argc; i++) {
    switch (tokens[i]->type) {
    case TOKEN_WORD:
      pipeline->commands[current].argc++;
      break;
    case TOKEN_REDIR_IN:
    case TOKEN_REDIR_OUT:
    case TOKEN_REDIR_APPEND:
      pipeline->commands[current].redir_count++;
      i++;
      break;
    case TOKEN_PIPE:
      current++;
      break;
    default:
      break;
    }
  }
}

void process_redir(struct t_redir *redir_buffer, struct raw_token *token,
                   char *filename) {
  enum redir_type type = REDIR_OUT;
  if (strcmp(token->value, ">>") == 0) {
    type = REDIR_APPEND;
  } else if (strcmp(token->value, "<") == 0) {
    type = REDIR_IN;
  }
  assert(redir_buffer != NULL);
  redir_buffer->type = type;
  redir_buffer->filename = strdup(filename);
}

void init_pipeline(struct t_pipeline *pipeline, int argc,
                   struct raw_token *tokens[]) {
  pipeline->count = count_command(argc, tokens);
  pipeline->commands = calloc(pipeline->count, sizeof(struct t_command));
  if (!pipeline->commands) {
    perror("Calloc pipeline command");
    exit(EXIT_FAILURE);
  }
  analyze_commands(pipeline, argc, tokens);

  for (int i = 0; i < pipeline->count; i++) {
    pipeline->commands[i].argv =
        calloc(pipeline->commands[i].argc + 1, sizeof(char *));
    if (!pipeline->commands[i].argv) {
      perror("Calloc argv");
      exit(EXIT_FAILURE);
    }

    if (pipeline->commands[i].redir_count > 0) {
      pipeline->commands[i].redirs =
          calloc(pipeline->commands[i].redir_count, sizeof(struct t_redir));
      if (!pipeline->commands[i].redirs) {
        perror("Calloc redirs");
        exit(EXIT_FAILURE);
      }
    }
  }
}

void destroy_pipeline(struct t_pipeline *pipeline) {
  for (int i = 0; i < pipeline->count; i++) {
    for (int r = 0; r < pipeline->commands[i].redir_count; r++) {
      free(pipeline->commands[i].redirs[r].filename);
    }
    free(pipeline->commands[i].redirs);
    free(pipeline->commands[i].argv);
  }
  free(pipeline->commands);
}

void free_tokens(struct raw_token *tokens[], int argc) {
  for (int i = 0; i < argc; i++) {
    free(tokens[i]->value);
    free(tokens[i]);
  }
}
