#!/usr/bin/env bash
set -xe
gcc -o forth -Wall -Wextra -Wall compiler.c lexer.c main.c
