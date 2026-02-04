#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Command { SHELL_UNKNOWN, SHELL_ECHO, SHELL_EXIT };
enum Command parse_command(const char *command);
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
    // Checks the if the command "exit" was entered and terminates the program
    if (!strcmp(command, "exit")) {
      exit(0);
    }

    switch (parse_command(command)) {
    case SHELL_EXIT:
      exit(0);
      break;
    case SHELL_ECHO:
      printf("%s\n", command + 5);
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
  return SHELL_UNKNOWN;
}
