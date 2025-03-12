#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"

int exec_command(char *line, size_t line_size) {
  char *tok = strtok(line, " \n");

  while (tok != NULL) {
    if (strcmp(tok, "exit") == 0) {
      return 1;
    }

    tok = strtok(NULL, " \n");
  }

  return 0;
}

int main(void) {
  int end = 0;

  char *line = NULL;
  size_t line_size = 0;

  init_fs();

  while (!end) {
    if (getline(&line, &line_size, stdin) == -1) {
      return -1;
    }

    end = exec_command(line, line_size);
  }

  clear_fs();
  free(line);
  return 0;
}
