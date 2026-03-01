#!/bin/bash

clang parser.c main.c -o main -Wall -Werror -Wimplicit-fallthrough
