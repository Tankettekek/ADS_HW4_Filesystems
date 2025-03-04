#include <stdlib.h>

#include "fs.h"

void init_filesystem(fs *filesystem) {
  filesystem->size = 10000;
  filesystem->inode_table = (inode *)malloc(10000 * sizeof(inode));
  filesystem->inode_table[0] = new_root();
}

void free_fs(fs *filesystem) {
  for (int i = 0; i < filesystem->ninodes; i++) {
    free_inode(&filesystem->inode_table[i]);
  }
  free(filesystem->inode_table);
}

inode new_root() {
  inode root;

  root.ref_count = 2;
  root.byte_count = sizeof(DIR_ENTRY) * 2;
  root.type = S_IFDIR;
  root.data = malloc(sizeof(DIR_ENTRY) * 2);

  new_entry(offset(root.data, 0), 0, S_IFDIR, 1, ".");
  new_entry(offset(root.data, 1), 0, S_IFDIR, 2, "..");
  return root;
}
