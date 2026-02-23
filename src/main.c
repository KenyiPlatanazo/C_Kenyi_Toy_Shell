#include <asm-generic/errno-base.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
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
#define MAX_TOKENS 1024
// #define TOKEN_DELIM "\t\r\n\a"

enum Command {
  SHELL_UNKNOWN,
  SHELL_ECHO,
  SHELL_EXIT,
  SHELL_TYPE,
  SHELL_PWD,
  SHELL_CD,
};

enum Lexical_State { NORMAL, IN_SINGLE_QUOTE, IN_DOUBLE_QUOTE };

void process_command(char *command);
void dispatch(int argc, char *argv[]);
enum Command parse_command(const char *command);
bool does_exec_exists(const char *command, const char *dir);

int tokenize(char *line, char *argv[]);

void handle_type(int argc, char *argv[]);
void check_type(const char *command);
void handle_echo(int argc, char *argv[]);
void handle_exec(int argc, char *argv[]);
void handle_pwd(int argc, char *argv[]);
void handle_cd(int argc, char *argv[]);

unsigned scan_and_match(char *string, const char *pattern, unsigned flags);

int main(int argc, char *argv[]) {
  while (1) {
    // Flush after every printf
    setbuf(stdout, NULL);
    printf("$ ");

    char command[1024];
    fgets(command, sizeof(command), stdin);
    // Deletes the \n char present in fgets (since the input is recieved when
    // the user presses \n, it needs to be deleted)
    command[strcspn(command, "\n")] = '\0';
    process_command(command);
  }
  return 0;
}

void process_command(char *command) {
  char *argv[1024];
  int argc = tokenize(command, argv);
  if (argc == 0)
    return;
  dispatch(argc, argv);
}

int tokenize(char *line, char *argv[]) {
  int argc = 0;
  int i = 0;
  while (line[i] != '\0') {
    // Deleting dead space
    while (line[i] == ' ')
      i++;
    if (line[i] == '\0')
      break;

    char token_buffer[TOKEN_BUFFSIZE];
    int t = 0;
    enum Lexical_State state = NORMAL;
    bool done = false;
    while (line[i] != '\0' && !done) {
      char current_char = line[i];
      switch (state) {
      case NORMAL:
        switch (current_char) {
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
          i++;
          if (line[i] != '\0')
            token_buffer[t++] = line[i];
          break;
        default:
          token_buffer[t++] = current_char;
        };
        // This break is for the while loop. Don't misread it bietchi
        break;
      case IN_SINGLE_QUOTE:
        if (current_char == '\'') {
          state = NORMAL;
        } else {
          token_buffer[t++] = current_char;
        }
        break;
      case IN_DOUBLE_QUOTE:
        if (current_char == '"') {
          state = NORMAL;
        } else if (current_char == '\\') {
          char next = line[i + 1];
          if (next == '"' || next == '\\' || next == '$' || next == '`') {
            i++;
            token_buffer[t++] = next;
          } else {
            token_buffer[t++] = current_char;
          }
        } else {
          token_buffer[t++] = current_char;
        }
        break;
      }
      if (!done)
        i++;
    }
    token_buffer[t] = '\0';
    argv[argc] = strdup(token_buffer);
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

void dispatch(int argc, char *argv[]) {
  enum Command command = parse_command(argv[0]);
  switch (command) {
  case SHELL_EXIT:
    exit(0);
    break;
  case SHELL_ECHO:
    handle_echo(argc, argv);
    break;
  case SHELL_TYPE:
    handle_type(argc, argv);
    break;
  case SHELL_PWD:
    handle_pwd(argc, argv);
    break;
  case SHELL_CD:
    handle_cd(argc, argv);
    break;
  case SHELL_UNKNOWN:
    handle_exec(argc, argv);
    break;
  }
}
enum Command parse_command(const char *command) {
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

void handle_type(int argc, char *argv[]) {
  if (argc < 2)
    return;
  for (int i = 1; i < argc; i++) {
    check_type(argv[i]);
  }
}
void check_type(const char *command) {
  char *inner_saveptr = NULL;
  if (parse_command(command) != SHELL_UNKNOWN) {
    printf("%s is a shell builtin\n", command);
    return;
  }
  char *path = getenv("PATH");
  if (path == NULL) {
    return;
  }
  char *path_copy = strdup(path);
  char *dir = strtok_r(path_copy, ":", &inner_saveptr);
  while (dir != NULL) {
    if (does_exec_exists(command, dir)) {
      printf("%s is %s/%s\n", command, dir, command);
      free(path_copy);
      return;
    }
    dir = strtok_r(NULL, ":", &inner_saveptr);
  }
  free(path_copy);
  printf("%s: not found\n", command);
}

bool does_exec_exists(const char *file, const char *dir) {
  char fullpath[1024];
  snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, file);
  return access(fullpath, X_OK) == 0;
}

void handle_echo(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    printf("%s", argv[i]);
    if (i < argc - 1) {
      printf(" ");
    }
  }
  printf("\n");
}

void handle_exec(int argc, char *argv[]) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return;
  }
  if (pid == 0) {
    execvp(argv[0], argv);
    if (errno == ENOENT) {
      fprintf(stderr, "%s: command not found\n", argv[0]);
      exit(127);
    } else {
      perror("execvp");
      exit(126);
    }
  } else {
    int status;
    waitpid(pid, &status, 0);
  }
}

void handle_pwd(int argc, char *argv[]) {
  // We keep the argc and argv to add flag detection later
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
    return;
  } else {
    perror("getcwd() error");
  }
}
void handle_cd(int argc, char *argv[]) {
  char *usr_home = getenv("HOME");
  if (usr_home == NULL) {
    return;
  }
  if (argc < 2) {
    chdir(usr_home);
    return;
  }
  // We haven't implemented flags yet, that's why we'll keep this error here for
  // the mean time.
  if (argc > 2) {
    printf("bash: cd: too many arguments\n");
    return;
  }
  if (strcmp(argv[1], "~") == 0) {
    chdir(usr_home);
    return;
  }
  if (chdir(argv[1]) != 0)
    fprintf(stderr, "cd: %s: No such file or directory\n", argv[1]);
  // perror("chdir() failed: ");
}
