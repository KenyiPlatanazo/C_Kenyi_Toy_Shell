#include "parser.h"
#include <asm-generic/errno-base.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
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

bool does_exec_exists(const char *command, const char *dir);

void handle_type(struct t_command *command);
void check_type(const char *command);
void handle_echo(struct t_command *command);
void handle_exec(struct t_command *command);
void handle_pwd(struct t_command *command);
void handle_cd(struct t_command *command);

void handle_redirections(struct t_command *command);

void create_file(char *name);

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

void handle_type(struct t_command *command) {
  if (command->argc < 2)
    return;
  for (int i = 1; i < command->argc; i++) {
    check_type(command->argv[i]);
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

void handle_echo(struct t_command *command) {
  for (int i = 1; i < command->argc && command->argv[i] != NULL; i++) {
    printf("%s", command->argv[i]);
    if (i < command->argc - 1) {
      printf(" ");
    }
  }
  printf("\n");
}

void handle_exec(struct t_command *command) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return;
  }
  if (pid == 0) {
    handle_redirections(command);
    execvp(command->argv[0], command->argv);
    if (errno == ENOENT) {
      fprintf(stderr, "%s: command not found\n", command->argv[0]);
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

void handle_pwd(struct t_command *command) {
  // We keep the argc and argv to add flag detection later
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
    return;
  } else {
    perror("getcwd() error");
  }
}
void handle_cd(struct t_command *command) {
  char *usr_home = getenv("HOME");
  if (usr_home == NULL) {
    return;
  }
  if (command->argc < 2) {
    chdir(usr_home);
    return;
  }
  // We haven't implemented flags yet, that's why we'll keep this error here for
  // the mean time.
  if (command->argc > 2) {
    printf("bash: cd: too many arguments\n");
    return;
  }
  if (strcmp(command->argv[1], "~") == 0) {
    chdir(usr_home);
    return;
  }
  if (chdir(command->argv[1]) != 0)
    fprintf(stderr, "cd: %s: No such file or directory\n", command->argv[1]);
  // perror("chdir() failed: ");
}

void handle_redirections(struct t_command *command) {
  if (command->redir_count == 0 || command->redirs == NULL)
    return;
  for (int i = 0; i < command->redir_count; i++) {
    int file_fd = -1;

    switch (command->redirs[i].type) {
    case REDIR_OUT:
      file_fd =
          open(command->redirs[i].filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      break;
    case REDIR_APPEND:
      file_fd = open(command->redirs[i].filename, O_WRONLY | O_CREAT | O_APPEND,
                     0644);
      break;
    case REDIR_IN:
      file_fd = open(command->redirs[i].filename, O_RDONLY, 0644);
      break;
    }

    if (file_fd < 0) {
      perror("Open/create file");
      return;
    }

    // printf("type = %d fd = %d file = %s\n", command->redirs[i].type,
    //       command->redirs[i].fd, command->redirs[i].filename);

    dup2(file_fd, command->redirs[i].fd);
    close(file_fd);
  }
}
