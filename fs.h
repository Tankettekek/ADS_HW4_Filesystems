#pragma once

typedef struct inode inode;
typedef union data data;

typedef enum ftype { S_IFDIR, S_IFREG, S_IFLNK } ftype;

typedef struct inode {
  ftype type;
  char name[61];
  int reference_count;
  int size;
  void *data;
} inode;

typedef struct DIR_ENTRY {
  char name[61];
  inode *inode;
} DIR_ENTRY;

// Create functions
void create_file(inode *parent, char *name);
void create_directory(inode *parent, char *name);
void create_hardlink(inode *parent, char *name, inode *target);
void create_symlink(inode *parent, char *name, char *path, int size);

// Delete functions
void delete_file(inode *root, char *path);
void delete_directory(inode *root, char *path);

// READ/WRITE functions
inode *resolve_path(inode *root, char *path);
void read_file(inode *root, char *path);
void write_file(inode *root, char *path, char *content);
void append_file(inode *root, char *path, char *content);
void list_dir(inode *root, char *path);

// DIR_ENTRY helpers
void create_entry(void *data, char *name, inode *inode);
void delete_entry(void *data, char *name);
