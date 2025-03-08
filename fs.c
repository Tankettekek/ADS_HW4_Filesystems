#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"

void create_file(inode *parent, char *name) {
  inode new;
  strcpy(new.name, name);
  new.type = S_IFREG;
  new.reference_count = 1;
  new.size = 0;

  parent->size += sizeof(DIR_ENTRY);

  // Reallocate directory entry array
  if (realloc(parent->data, parent->size) == NULL) {
    fprintf(stderr, "Reallocation failed");
  }

  create_entry(parent->data, name, &new);
}

void create_directory(inode *parent, char *name) {
  inode new;
  strcpy(new.name, name);
  new.type = S_IFDIR;
  new.reference_count = 1;
  new.size = 2;

  new.data = malloc(sizeof(DIR_ENTRY) * new.size);

  parent->size += sizeof(DIR_ENTRY);

  // Reallocate directory entry array
  if (realloc(parent->data, parent->size) == NULL) {
    fprintf(stderr, "Reallocation failed");
  }

  create_entry(parent->data, name, &new);
  create_entry(new.data, ".", &new);
  create_entry(new.data, "..", parent);
}

void create_hardlink(inode *parent, char *name, inode *target) {
  parent->size += sizeof(DIR_ENTRY);

  // Reallocate directory entry array
  if (realloc(parent->data, parent->size) == NULL) {
    fprintf(stderr, "Reallocation failed");
  }

  create_entry(parent->data, name, target);
}

void create_symlink(inode *parent, char *name, char *path, int size) {
  inode new;
  strcpy(new.name, name);
  new.type = S_IFREG;
  new.reference_count = 1;
  new.size = size;
  new.data = malloc(sizeof(char) * size);
  strcpy(new.data, path);

  parent->size += sizeof(DIR_ENTRY);

  // Reallocate directory entry array
  if (realloc(parent->data, parent->size) == NULL) {
    fprintf(stderr, "Reallocation failed");
  }

  create_entry(parent->data, name, &new);
}

void delete_file(inode *root, char *path) {
  inode *target = resolve_path(root, path);
  char name[61];
  strcpy(name, target->name);

  target->size--;
  if (target->size == 0) {
    free(target->data);
    free(target);
  }

  delete_entry();
}
