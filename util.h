#pragma once

#include "fs.h"
#include <stdio.h>

void list_dir(inode *dir);
void read_file(inode *file);
void recursive_list(inode *dir, char *prefix);

void move(const char *src, const char *dst);

void parse_echo(char *line);

int compare_entries(const void *a, const void *b);
