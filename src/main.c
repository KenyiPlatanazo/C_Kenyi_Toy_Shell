#include <assert.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define WHITESPACE ' '
enum Command {
  SHELL_UNKNOWN,
  SHELL_ECHO,
  SHELL_EXIT,
  SHELL_TYPE,
  SHELL_EXEC_FILE
};
void process_command(char *command);
void dispatch(int argc, char *argv[]);
enum Command parse_command(const char *command);
bool does_exec_exists(const char *command, const char *dir);
void check_type(const char *command);
void process_type(const char *command);
void exec_file(const char *dir);
int tokenize(char *line, char *argv[]);

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
enum Command parse_command(const char *command) {
  if (strncmp(command, "./", 2) == 0)
    return SHELL_EXEC_FILE;
  if (strcmp(command, "exit") == 0)
    return SHELL_EXIT;
  if (strcmp(command, "echo") == 0)
    return SHELL_ECHO;
  if (strcmp(command, "type") == 0)
    return SHELL_TYPE;
  return SHELL_UNKNOWN;
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
      free(path_copy);
      return;
    }
    dir = strtok_r(NULL, ":", &inner_saveptr);
  }
  free(path_copy);
  printf("%s: not found\n", command);
}

bool does_exec_exists(const char *command, const char *dir) {
  bool result = false;
  char fullpath[1024];
  snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, command);
  if (access(fullpath, X_OK) == 0) {
    printf("%s is %s\n", command, fullpath);
    result = true;
  }
  return result;
}

void handle_type(int argc, char *argv[]) {
  if (argc < 2)
    return;
  for (int i = 1; i < argc; i++) {
    check_type(argv[i]);
  }
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

void process_type(const char *command) {
  char *cmd_name = strdup(command + 5);
  char *inner_saveptr = NULL;
  char *token;
  if (!cmd_name)
    return;
  if (*cmd_name == '\0') {
    free(cmd_name);
    return;
  }
  token = strtok_r(cmd_name, " ", &inner_saveptr);
  while (token != NULL) {
    check_type(token);
    token = strtok_r(NULL, " ", &inner_saveptr);
  }
  free(cmd_name);
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
  char *save_ptr = NULL;
  char *token = strtok_r(line, " ", &save_ptr);
  while (token != NULL) {
    argv[argc++] = token;
    token = strtok_r(NULL, " ", &save_ptr);
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
  case SHELL_EXEC_FILE:
    break;
  case SHELL_UNKNOWN:
    printf("%s: command not found\n", argv[0]);
    break;
  }
}
