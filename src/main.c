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
enum Command {
  SHELL_UNKNOWN,
  SHELL_ECHO,
  SHELL_EXIT,
  SHELL_TYPE,
  SHELL_PWD,
  SHELL_CD,
};
void process_command(char *command);
void dispatch(int argc, char *argv[]);
enum Command parse_command(const char *command);
bool does_exec_exists(const char *command, const char *dir);

int tokenize(char *line, char *argv[]);
void handle_str_quotes(char *token);
void delete_char(char *token, char ch);

void handle_type(int argc, char *argv[]);
void check_type(const char *command);
void handle_echo(int argc, char *argv[]);
void handle_exec(int argc, char *argv[]);
void handle_pwd(int argc, char *argv[]);
void handle_cd(int argc, char *argv[]);

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
  char *save_ptr = NULL;
  char *token = strtok_r(line, " ", &save_ptr);
  while (token != NULL) {
    argv[argc++] = token;
    token = strtok_r(NULL, " ", &save_ptr);
    if (token != NULL) {
      handle_str_quotes(token);
    }
  }
  argv[argc] = NULL;
  return argc;
}

void handle_str_quotes(char *token) {
  int token_length = strlen(token);
  if (token[0] != '\'' && token[token_length - 1] != '\'') {
    return;
  }
  delete_char(token, '\'');
}

void delete_char(char *token, char ch) {
  int i, j;
  int len = strlen(token);
  for (i = j = 0; i < len; i++) {
    if (token[i] != ch) {
      token[j++] = token[i];
    }
  }
  token[j] = '\0';
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
