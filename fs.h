#pragma once

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define INODE_TABLE_SIZE 10000

typedef enum filetype { S_IFDIR, S_IFREG, S_IFLNK } ftype;

typedef struct inode {
  uint8_t reference_count;
  ftype filetype;
  bool sorted;
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
int create_dir(const char *path);
int create_file(const char *path);
int create_hardlink(const char *dest, const char *target);
int create_symlink(const char *dest, const char *target);

// Delete file
int delete_dir(const char *path);
int delete_file(const char *path);
int delete_g(const char *path);

// Write data to file
int write_file(const char *path, char *data);
int append_file(const char *path, const char *data);

// Add directory entry
int add_entry(const char *path, const char *name, inode *target);
int remove_entry(const char *path, const char *name);
int entry_exists(inode *dir, const char *name);

// Path utilities
int resolve_path(const char *path, inode **result);
char *parent_of(const char *path);
char *filename(const char *path);
char *append(const char *path, const char *path_complement);

// Copy util... Bordel de merde qu'est ce que ca me soule ca
int copy_file(const char *src, const char *dest);
int copy_dir(const char *src, const char *dest);
int copy(const char *src, const char *dest);

// FS Utilities
void init_fs(void);
int clear_fs(void);

extern _fs fs;
