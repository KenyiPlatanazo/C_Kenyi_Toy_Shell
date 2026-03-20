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
void handle_pwd(void);
void handle_cd(struct t_command *command);

void handle_redirections(struct t_command *command);

void create_file(char *name);
