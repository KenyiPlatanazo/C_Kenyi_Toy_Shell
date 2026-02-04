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
      check_type(command);
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
  const char *cmd_name = command + 5;
  if (parse_command(cmd_name) != SHELL_UNKNOWN) {
    printf("%s is a shell builtin\n", command + 5);
    return;
  }
  char *path = getenv("PATH");
  assert(path != NULL);
  char *path_copy = strdup(path);
  char *dir = strtok(path_copy, ":");
  while (dir != NULL) {
    if (exists_in_dir(cmd_name, dir)) {
      char fullpath[1024];
      snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, cmd_name);
      printf("%s is %s\n", cmd_name, fullpath);
      free(path_copy);
      return;
    }
    dir = strtok(NULL, ":");
  }
  free(path_copy);
  printf("%s: not found\n", cmd_name);
}
bool exists_in_dir(const char *command, const char *dir) {
  char fullpath[1024];
  snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, command);
  return access(fullpath, X_OK) == 0;
}
