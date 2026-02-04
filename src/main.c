#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  // TODO: Uncomment the code below to pass the first stage
  printf("$ ");

  char command[1024];
  fgets(command, sizeof(command), stdin);
  // Deletes the \n char present in fgets (since the input is recieved when the
  // user presses \n, it needs to be deleted)
  command[strcspn(command, "\n")] = '\0';
  printf("%s: command not found\n", command);
  return 0;
}
