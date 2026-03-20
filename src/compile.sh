#!/bin/bash

clang parser.c exec.c input.c -o main -Wall -Werror -Wextra -pedantic -std=c23
