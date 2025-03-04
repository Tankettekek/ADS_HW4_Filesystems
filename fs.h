#pragma once

typedef enum filetypes { S_IFDIR, S_IFREG, S_IFLNK } ftype;

typedef struct inode {
  int ref_count;
  int byte_count;
  void *data;
  ftype type;
} inode;

typedef struct filesystem {
  int size; // in inodes
  int ninodes;
  inode *inode_table;
} fs;

typedef struct DIR_ENTRY {
  int inode;
  ftype type;
  int name_len; // in bytes
  char *name;
} DIR_ENTRY;

// FS FUNCTIONS
void init_fs(fs *filesystem);
void free_fs(fs *filesystem);
inode new_root();

// INODE FUNCTIONS
inode init_inode();
void free_inode(inode *inode);
inode copy_inode(inode *src);

// Directory functions
DIR_ENTRY *offset(void *DIR, int entry);
void new_entry(DIR_ENTRY *entry, int inode, ftype type, int name_len,
               char *name);
void free_entry(DIR_ENTRY entry);
