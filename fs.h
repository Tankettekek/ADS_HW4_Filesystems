#pragma once

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum filetype { S_IFDIR, S_IFREG, S_IFLNK } ftype;

typedef struct inode {
  uint8_t reference_count;
  ftype filetype;
  size_t data_size; // Data Size in Bytes
  void *data;
} inode;

typedef struct filesystem {
  inode *root;
  inode *working_dir;
} _fs;

typedef struct DIR_ENTRY {
  char name[61];
  inode *item;
} DIR_ENTRY;

// Create files
int create_dir(char *path);
int create_file(char *path);
int create_hardlink(char *dest, char *target);
int create_symlink(char *dest, char *target);

// Delete file
int delete_dir(char *path);
int delete_file(char *path);
int delete_g(char *path, bool recursive);

// Write data to file
int write_file(char *path, char *data);
int append_file(char *path, char *data);

// Add directory entry
int add_entry(char *path, char *name, inode *target);
int remove_entry(char *path, char *name);
int entry_exists(inode *dir, char *name);

// Path utilities
int resolve_path(char *path, inode *result, bool follow_symlink);
char *parent_of(char *path);
char *filename(char *path);
char *append(char *path, char *path_complement);

// FS Utilities
_fs init_fs();
int clear_fs(_fs *filesystem);

extern _fs fs;
