#include <assert.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum Command { SHELL_UNKNOWN, SHELL_ECHO, SHELL_EXIT, SHELL_TYPE };
enum Command parse_command(const char *command);
bool exists_in_dir(const char *command, const char *dir);
void check_type(const char *command);
void process_type(const char *command);

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
    switch (parse_command(command)) {
    case SHELL_EXIT:
      exit(0);
      break;
    case SHELL_ECHO:
      printf("%s\n", command + 5);
      break;
    case SHELL_TYPE:
      process_type(command);
      break;
    case SHELL_UNKNOWN:
      printf("%s: command not found\n", command);
      break;
    }
  }
  return 0;
}

enum Command parse_command(const char *command) {
  if (strcmp(command, "exit") == 0)
    return SHELL_EXIT;
  if (strncmp(command, "echo", 4) == 0)
    return SHELL_ECHO;
  if (strncmp(command, "type", 4) == 0)
    return SHELL_TYPE;
  return SHELL_UNKNOWN;
}

void check_type(const char *command) {
  if (parse_command(command) != SHELL_UNKNOWN) {
    printf("%s is a shell builtin\n", command);
    return;
  }
  char *path = getenv("PATH");
  assert(path != NULL);
  char *path_copy = strdup(path);
  char *dir = strtok(path_copy, ":");
  while (dir != NULL) {
    if (exists_in_dir(command, dir)) {
      char fullpath[1024];
      snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, command);
      printf("%s is %s\n", command, fullpath);
      free(path_copy);
      return;
    }
    dir = strtok(NULL, ":");
  }
  free(path_copy);
  printf("%s: not found\n", command);
}
bool exists_in_dir(const char *command, const char *dir) {
  char fullpath[1024];
  snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, command);
  return access(fullpath, X_OK) == 0;
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
