#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "util.h"

int exec_command(char *line) {
  char *save_ptr = NULL;
  char *tok = strtok_r(line, " \n", &save_ptr);
  if (tok == NULL) {
    return 0;
  }
  inode *buffer = fs.working_dir;

  if (strcmp(tok, "exit") == 0) { // EXIT
    return 1;
  } else if (strcmp(tok, "cd") == 0) { // CD
    tok = strtok_r(NULL, " \n", &save_ptr);
    if (tok == NULL) {
      fs.working_dir = fs.root;
      return 0;
    }
    resolve_path(tok, &fs.working_dir);
  } else if (strcmp(tok, "ls") == 0) { // LS
    tok = strtok_r(NULL, " \n", &save_ptr);
    if (tok != NULL) {
      resolve_path(tok, &buffer);
    }

    // List all subdirs of buffer
    list_dir(buffer);
  } else if (strcmp(tok, "cat") == 0) { // CAT
    tok = strtok_r(NULL, " \n", &save_ptr);
    // if no path is specified, then return
    if (tok == NULL) {
      return 0;
    }
    // Resolve path
    int err = resolve_path(tok, &buffer);

    // On resolve path error exit because something is very wrong
    if (err == ENOENT) {
      printf("cat: %s: No such file or directory\n", tok);
      return 0;
    }

    if (err != 0) {
      exit(err);
    }

    read_file(buffer);
  } else if (strcmp(tok, "find") == 0) { // FIND
    recursive_list(buffer, ".");
  } else if (strcmp(tok, "touch") == 0) { // TOUCH
    tok = strtok_r(NULL, " \n", &save_ptr);
    while (tok != NULL) {
      int err = create_file(tok);
      if (err != 0) {
        exit(err);
      }
      tok = strtok_r(NULL, " \n", &save_ptr);
    }
  } else if (strcmp(tok, "echo") == 0) { // ECHO
    parse_echo(line + 6);
  } else if (strcmp(tok, "mkdir") == 0) { // MKDIR
    tok = strtok_r(NULL, " \n", &save_ptr);
    if (strcmp(tok, "-p") == 0) { // discard -p, since behavior doesnt differ
      tok = strtok_r(NULL, " \n", &save_ptr);
    }

    while (tok != NULL) {
      create_dir(tok);
      tok = strtok_r(NULL, " \n", &save_ptr);
    }
  } else if (strcmp(tok, "mv") == 0) { // MV
    char *src = strtok_r(NULL, " \n", &save_ptr);
    char *dest = strtok_r(NULL, " \n", &save_ptr);

    move(src, dest);
  } else if (strcmp(tok, "cp") == 0) { // CP
    char *src = strtok_r(NULL, " \n", &save_ptr);
    if (strcmp(src, "-r") == 0) {
      src = strtok_r(NULL, " \n", &save_ptr);
    }
    char *dest = strtok_r(NULL, " \n", &save_ptr);

    int err = copy(src, dest);
    if (err != 0) {
      exit(err);
    }
  } else if (strcmp(tok, "rm") == 0) { // RM
    tok = strtok_r(NULL, " \n", &save_ptr);
    if (strcmp(tok, "-r") == 0) {
      tok = strtok_r(NULL, " \n", &save_ptr);
    }

    while (tok != NULL) {
      delete_g(tok);

      tok = strtok_r(NULL, " \n", &save_ptr);
    }
  } else if (strcmp(tok, "ln") == 0) { // LN
    char *src = strtok_r(NULL, " \n", &save_ptr);
    if (src == NULL) {
      exit(1);
    }
    char *dest = strtok_r(NULL, " \n", &save_ptr);
    if (dest == NULL) {
      exit(1);
    }
    create_hardlink(dest, src);
    /* Symbolic links disabled
    if (strcmp(tok, "-s") == 0) { // Symbolic
      src = strtok_r(NULL, " \n", &save_ptr);
      char *dest = strtok_r(NULL, " \n", &save_ptr);
      create_symlink(src, dest);
    } else { // HARD
      char *dest = strtok_r(NULL, " \n", &save_ptr);
      create_hardlink(src, dest);
    } */
  }

  return 0;
}

int main(void) {
  int end = 0;

  char *line = malloc(200000);
  size_t line_size = 200000;

  init_fs();

  while (!end) {
    if (fgets(line, line_size, stdin) == NULL) {
      free(line);
      return -1;
    }

    end = exec_command(line);
  }

  free(line);
  clear_fs();
  return 0;
}
