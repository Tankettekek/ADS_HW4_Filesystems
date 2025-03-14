#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "util.h"

int compare_entries(const void *a, const void *b) {
  return strcmp(((DIR_ENTRY *)a)->name, ((DIR_ENTRY *)b)->name);
}

void list_dir(inode *dir) { // Used for printing directories
  qsort(dir->data + (sizeof(DIR_ENTRY) * 2),
        dir->data_size / sizeof(DIR_ENTRY) - 2, sizeof(DIR_ENTRY),
        compare_entries);

  for (size_t i = 2; i < dir->data_size / sizeof(DIR_ENTRY); i++) {
    printf("%s\n", ((DIR_ENTRY *)dir->data)[i].name);
  }

  return;
}

void read_file(inode *file) {
  if (file->data_size > 0) {
    printf("%s\n", (char *)file->data);
    return;
  }
}

void recursive_list(inode *dir, char *prefix) {
  qsort(dir->data + (sizeof(DIR_ENTRY) * 2),
        dir->data_size / sizeof(DIR_ENTRY) - 2, sizeof(DIR_ENTRY),
        compare_entries);

  printf("%s\n", prefix);

  for (size_t i = 2; i < dir->data_size / sizeof(DIR_ENTRY); i++) {
    if (((DIR_ENTRY *)dir->data)[i].item->filetype == S_IFDIR) {
      char *p_asdir = append(prefix, "/");
      char *new_pre = append(p_asdir, ((DIR_ENTRY *)dir->data)[i].name);
      recursive_list(((DIR_ENTRY *)dir->data)[i].item, new_pre);
      free(p_asdir);
      free(new_pre);
    } else {
      printf("%s/%s\n", prefix, ((DIR_ENTRY *)dir->data)[i].name);
    }
  }

  return;
}

void move(const char *src, const char *dst) {
  inode *src_inode;
  int err = resolve_path(src, &src_inode);
  if (err != 0) {
    clear_fs();
    exit(-1);
  }

  char *src_parent = parent_of(src);
  char *src_name = filename(src);

  err = remove_entry(src_parent, src_name);
  if (err != 0) {
    clear_fs();
    exit(-1);
  }

  char *dst_parent = parent_of(dst);
  char *dst_name = filename(dst);

  err = add_entry(dst_parent, dst_name, src_inode);
  if (err != 0) {
    clear_fs();
    exit(-1);
  }

  free(src_parent);
  free(dst_parent);
  return;
}

void parse_echo(char *line) {
  int end_data = 0;

  char *w_ptr = line; // Line should begin on a "

  int redir_sign_count = 0;

  while (*w_ptr != '\0') {
    if (*w_ptr == '"') {
      end_data = (int)(w_ptr - line);
    }

    if (*w_ptr == '>') {
      redir_sign_count++;
    }

    if (redir_sign_count > 0 && (*w_ptr != '>' && *w_ptr != ' ')) {
      break;
    }

    w_ptr++;
  }
  char *data = (char *)malloc(end_data + 1);
  memcpy(data, line, end_data);
  data[end_data] = '\0';

  w_ptr = strtok(w_ptr, " \n");
  create_file(w_ptr);

  if (redir_sign_count == 1) {
    write_file(w_ptr, data);
  } else {
    append_file(w_ptr, data);
  }

  free(data);
  return;
}
