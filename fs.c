#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"

int create_dir(char *path) {
  // Take the parent of target creation
  char *parent = parent_of(path);
  inode *dir;

  // If the parent of the target does not exist, create it
  if (resolve_path(parent, dir, true) == ENOENT) {
    create_dir(parent);
  }

  // If one of the parents is not a directory, then abbort with ENOTDIR
  if (dir->filetype != S_IFDIR) {
    return ENOTDIR;
  }

  // Allocate a new inode and set its properties properly
  inode *new_dir = (inode *)malloc(sizeof(inode));
  // Validate memory allocation
  if (new_dir == NULL) {
    return ENOMEM;
  }

  new_dir->filetype = S_IFDIR;
  new_dir->reference_count = 1;
  new_dir->data_size = sizeof(DIR_ENTRY) * 2;
  new_dir->data = malloc(new_dir->data_size);
  if (new_dir->data == NULL) {
    return ENOMEM;
  }

  // Get the name of the target dir from the path
  char *name = filename(path);

  // Add new directory entry to the parent
  add_entry(parent, name, new_dir);

  // Add itself to the directory list.
  add_entry(path, ".", new_dir);
  // Add parent to the directory list, to enable proper traversal.
  add_entry(path, "..", dir);

  // Free temporary variables
  free(parent);
  free(name);

  return 0;
}

int create_file(char *path) {
  char *parent = parent_of(path);
  inode *dir;

  // Check wether the promised conditions are completed
  int err = resolve_path(parent, dir, true);
  if (err != 0) {
    return err;
  }

  char *name = filename(path);
  if (dir->filetype != S_IFDIR) {
    return ENOTDIR;
  }

  if (entry_exists(dir, name) != -1) {
    return EEXIST;
  }

  inode *new_file = (inode *)malloc(sizeof(inode));
  // Memory allocation check
  if (new_file == NULL) {
    return ENOMEM;
  }

  new_file->filetype = S_IFREG;
  new_file->reference_count = 1;
  new_file->data_size = 0;
  new_file->data = NULL;

  add_entry(parent, name, new_file);

  // Free temp vars
  free(parent);
  free(name);

  return 0;
}

int create_hardlink(char *dest, char *target) {
  char *dest_parent = parent_of(dest);
  inode *parent_dir;

  // Destination validity checks
  int err = resolve_path(dest_parent, parent_dir, true);
  if (err != 0) {
    return err;
  }

  char *dest_name = filename(dest);
  if (parent_dir->filetype != S_IFDIR) {
    return ENOTDIR;
  }

  if (entry_exists(parent_dir, dest_name) != -1) {
    return EEXIST;
  }

  // Target validity checks
  inode *target_inode;
  err = resolve_path(target, target_inode, true);
  if (err != 0) {
    return err;
  }

  // Increment hardlink count
  target_inode->reference_count++;

  // Add entry to parent directory
  add_entry(dest_parent, dest_name, target_inode);

  // Free temp vars
  free(dest_parent);
  free(dest_name);

  return 0;
}

int create_symlink(char *dest, char *target) {
  char *dest_parent = parent_of(dest);
  inode *parent_dir;

  // Destination validity check
  int err = resolve_path(dest_parent, parent_dir, true);
  if (err != 0) {
    return err;
  }

  char *dest_name = filename(dest);
  if (parent_dir->filetype != S_IFDIR) {
    return ENOTDIR;
  }

  if (entry_exists(parent_dir, dest_name) != -1) {
    return EEXIST;
  }

  // New symlink inode, however no validity check is performed on supplied path
  inode *new_file = (inode *)malloc(sizeof(inode));
  // Memory allocation check
  if (new_file == NULL) {
    return ENOMEM;
  }

  new_file->filetype = S_IFLNK;
  new_file->reference_count = 1;
  new_file->data_size = strlen(target);
  new_file->data = NULL;

  // Add the symlink to its parent
  add_entry(dest_parent, dest_name, new_file);

  // free temp vars
  free(dest_parent);
  free(dest_name);

  return 0;
}

int delete_dir(char *path) {
  char *parent_path = parent_of(path);
  inode *parent_inode;

  // Path validation
  int err = resolve_path(parent_path, parent_inode, false);
  if (err != 0) {
    return err;
  }

  inode *directory;
  err = resolve_path(path, directory, false);
  if (err != 0) {
    return err;
  }

  // Recursively delete all subentities
  if (directory->data_size > 2) {
    for (int i = 2; i < directory->data_size / sizeof(DIR_ENTRY); i++) {
      err =
          delete_g(append(path, ((DIR_ENTRY *)directory->data)[i].name), false);
      if (err != 0) {
        return err;
      }
    }
  }

  // Decrement element count
  directory->reference_count--;

  // If through this deletion, the count reaches 0, deallocate inode
  // It could however still be hardlinked elsewhere so check before
  // nuking the data from memory
  if (directory->reference_count == 0) {
    free(directory->data);
    free(directory);
  }

  // Either way, we need to remove this element from the parent dir
  remove_entry(parent_path, filename(path));

  // Free temporary variables
  free(parent_path);

  return 0;
}

int delete_file(char *path) {
  inode *file;

  // Check for potential errors
  int err = resolve_path(path, file, false);
  if (err != 0) {
    return err;
  }

  // Again decrease count of hardlinks
  file->reference_count--;

  // If hardlink count is 0, nuke the data
  if (file->reference_count == 0) {
    free(file->data);
    free(file);
  }

  // Remove entry from parent directory anyway
  remove_entry(parent_of(path), filename(path));

  return 0;
}

int delete_g(char *path, bool recursive) {
  inode *target;

  // Error checking for path validation
  int err = resolve_path(path, target, true);
  if (err != 0) {
    return err;
  }

  // Decide on deletion method
  // Given the specification, recursive will always be true
  // But too late, I am not refactoring this
  if (target->filetype == S_IFDIR && recursive) {
    delete_dir(path);
  } else {
    delete_file(path);
  }

  return 0;
}

int write_file(char *path, char *data) {
  inode *target;

  // Error checking
  int err = resolve_path(path, target, true);
  if (err != 0) {
    return err;
  }

  if (target->filetype == S_IFDIR) {
    return EISDIR;
  }

  // Overwrite the existing data
  target->data_size = strlen(data) + 1;
  free(target->data);
  target->data = malloc(target->data_size);
  if (target->data == NULL) {
    return ENOMEM;
  }

  // Copy the current text into the inode data field
  strcpy((char *)target->data, data);

  return 0;
}

int append_file(char *path, char *data) {
  inode *target;

  // Error checking
  int err = resolve_path(path, target, true);
  if (err != 0) {
    return err;
  }

  if (target->filetype == S_IFDIR) {
    return EISDIR;
  }

  int str1_size = target->data_size - 1;
  target->data_size += strlen(data);
  // Memory check
  target->data = realloc(target->data, target->data_size);
  if (target->data == NULL) {
    return ENOMEM;
  }

  memcpy(target->data + str1_size, data, target->data_size - str1_size);

  return 0;
}

int add_entry(char *path, char *name, inode *target) {
  inode *dir;

  // Error checking
  int err = resolve_path(path, dir, true);
  if (err != 0) {
    return err;
  }

  if (dir->filetype != S_IFDIR) {
    return ENOTDIR;
  }

  if (entry_exists(dir, name) != -1) {
    return EEXIST;
  }

  // Reallocate proper size
  dir->data_size += sizeof(DIR_ENTRY);
  dir->data = realloc(dir->data, dir->data_size);
  // Memory realloc check
  if (dir->data == NULL) {
    return ENOMEM;
  }

  // Add entry (yes this looks absolutely horendous)
  strcpy(((DIR_ENTRY *)dir->data)[dir->data_size / sizeof(DIR_ENTRY) - 1].name,
         name);
  ((DIR_ENTRY *)dir->data)[dir->data_size / sizeof(DIR_ENTRY) - 1].item =
      target;

  return 0;
}

int remove_entry(char *path, char *name) {
  inode *target;

  // Error checking
  int err = resolve_path(path, target, true);
  if (err != 0) {
    return err;
  }

  // Iterate over entries
  for (int i = 2; i < target->data_size / sizeof(DIR_ENTRY); i++) {
    // If names match, delete entry, then reallocate the array to reflect
    // current size
    if (strcmp(((DIR_ENTRY *)target->data)[i].name, name) == 0) {
      ((DIR_ENTRY *)target->data)[i] =
          ((DIR_ENTRY *)
               target->data)[target->data_size / sizeof(DIR_ENTRY) - 1];
      target->data_size -= sizeof(DIR_ENTRY);

      target->data = realloc(target->data, target->data_size);

      // If reallocation fails, then return ENOMEM
      if (target->data == NULL) {
        return ENOMEM;
      }
    }
  }

  return 0;
}

int entry_exists(inode *dir, char *name) {
  // Iterate over entries until found
  for (int i = 0; i < dir->data_size / sizeof(DIR_ENTRY); i++) {
    if (strcmp(((DIR_ENTRY *)dir->data)[i].name, name) == 0) {
      return i;
    }
  }
  return -1;
}

int resolve_path(char *path, inode *result, bool follow_symlink) {
  // Check whether path begins at root, or if it is a relative path.
  if (path[0] == '/') {
    result = fs.root;
  } else {
    result = fs.working_dir; // Otherwise set it to the current WD
  }

  char *path_copy = (char *)malloc(strlen(path));
  if (path_copy == NULL) {
    return ENOMEM;
  }

  strcpy(path_copy, path);

  char *token = strtok(path_copy, "/");
  while (token != NULL) {
    // Check wether current node is a directory
    // Otherwise, path is invalid, since by this point it is evident that there
    // are further tokens
    if (result->filetype != S_IFDIR) {
      return ENOTDIR;
    }

    // Figure out wether the next node actually exists.
    // Otherwise return the fact that this directory does not exist
    int tok_index = entry_exists(result, token);
    if (tok_index == -1) {
      return ENOENT;
    }

    // Set the new current dir to the inode that is the result
    result = ((DIR_ENTRY *)result->data)[tok_index].item;

    // Get the new token in order
    token = strtok(path_copy, "/");
  }

  // If we got to this point, we have resolved the path.
  free(path_copy);
  return 0;
}

char *parent_of(char *path) {
  char *last_tok = strrchr(path, '/');
  if (last_tok == NULL) {
    return ".";
  }

  char *copy = (char *)malloc(last_tok - path);
  if (copy == NULL) {
    exit(ENOMEM);
  }

  memcpy(copy, path, last_tok - path);

  return copy;
}

char *filename(char *path) {
  char *last_tok = strrchr(path, '/');
  if (last_tok) {
    return last_tok + 1;
  }
  return path;
}

char *append(char *path, char *name) {
  const size_t len1 = strlen(path);
  const size_t len2 = strlen(name);
  char *result = malloc(len1 + len2 + 1);

  if (result == NULL) {
    exit(ENOMEM);
  }

  memcpy(result, path, len1);
  memcpy(result + len1, name, len2 + 1);
  return result;
}

_fs init_fs() {
  _fs filesystem;
  filesystem.root = (inode *)malloc(sizeof(inode));
  filesystem.working_dir = filesystem.root;
  return filesystem;
}

int clear_fs(_fs *filesystem) {
  int err = delete_dir("/");
  if (err != NULL) {
    return err;
  }
  return 0;
}
