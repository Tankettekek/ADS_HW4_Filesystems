#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "util.h"

_fs fs;

int create_dir(const char *path) {
  // Take the parent of target creation
  char *parent = parent_of(path);
  inode *dir = NULL;

  // If the parent of the target does not exist, create it
  if (resolve_path(parent, &dir) == ENOENT) {
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
  new_dir->data_size = 0;
  new_dir->data = NULL;
  new_dir->sorted = true;

  // Get the name of the target dir from the path
  char *name = filename(path);

  // Add new directory entry to the parent
  int err = add_entry(parent, name, new_dir);
  if (err != 0) {
    free(new_dir);
    return err;
  }

  // Add itself to the directory list.
  err = add_entry(path, ".", new_dir);
  if (err != 0) {
    return err;
  }
  // Add parent to the directory list, to enable proper traversal.
  err = add_entry(path, "..", dir);
  if (err != 0) {
    return err;
  }

  // Free temporary variables
  free(parent);
  parent = NULL;
  return 0;
}

int create_file(const char *path) {
  char *parent = parent_of(path);
  inode *dir = NULL;

  // Check wether the promised conditions are completed
  int err = resolve_path(parent, &dir);
  if (err != 0) {
    return err;
  }

  char *name = filename(path);
  if (dir->filetype != S_IFDIR) {
    free(parent);
    parent = NULL;
    return ENOTDIR;
  }

  if (entry_exists(dir, name) != -1) {
    free(parent);
    parent = NULL;
    return EEXIST;
  }

  inode *new_file = NULL;
  new_file = (inode *)malloc(sizeof(inode));
  // Memory allocation check
  if (new_file == NULL) {
    free(parent);
    parent = NULL;
    return ENOMEM;
  }

  new_file->filetype = S_IFREG;
  new_file->reference_count = 1;
  new_file->data_size = 0;
  new_file->data = NULL;
  new_file->sorted = true;

  err = add_entry(parent, name, new_file);
  if (err != 0) {
    free(new_file);
    return err;
  }
  // Free temp vars
  free(parent);
  parent = NULL;
  return 0;
}

int create_hardlink(const char *dest, const char *target) {
  char *dest_parent = parent_of(dest);
  inode *parent_dir = NULL;

  // Destination validity checks
  int err = resolve_path(dest_parent, &parent_dir);
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
  inode *target_inode = NULL;
  err = resolve_path(target, &target_inode);
  if (err != 0) {
    return err;
  }

  // Increment hardlink count
  target_inode->reference_count++;

  // Add entry to parent directory
  err = add_entry(dest_parent, dest_name, target_inode);
  if (err != 0) {
    return err;
  }

  // Free temp vars
  free(dest_parent);
  dest_parent = NULL;
  return 0;
}

/* Commented out for debugging
int create_symlink(const char *dest, const char *target) {
  char *dest_parent = parent_of(dest);
  inode *parent_dir = NULL;

  // Destination validity check
  int err = resolve_path(dest_parent, &parent_dir);
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
  err = add_entry(dest_parent, dest_name, new_file);
  if (err != 0) {
    return err;
  }

  // free temp vars
  free(dest_parent);
  dest_parent = NULL;
  return 0;
} */

int delete_dir(const char *path) {
  char *parent_path = parent_of(path);
  inode *parent_inode = NULL;

  // Path validation
  int err = resolve_path(parent_path, &parent_inode);
  if (err != 0) {
    free(parent_path);
    return err;
  }

  inode *directory = NULL;
  err = resolve_path(path, &directory);
  if (err != 0) {
    free(parent_path);
    return err;
  }

  // Recursively delete all subentities
  while (directory->data_size / sizeof(DIR_ENTRY) > 2) {
    char *dir = append(path, "/");
    char *appended = append(dir, ((DIR_ENTRY *)directory->data)[2].name);
    err = delete_g(appended);
    free(dir);
    free(appended);
    appended = NULL;
    if (err != 0) {
      free(parent_path);
      return err;
    }
  }

  // Decrement element count
  directory->reference_count--;

  // If through this deletion, the count reaches 0, deallocate inode
  // It could however still be hardlinked elsewhere so check before
  // nuking the data from memory
  if (directory->reference_count == 0) {
    free(directory->data);
    directory->data = NULL;
    free(directory);
    directory = NULL;
  }

  // Either way, we need to remove this element from the parent dir
  err = remove_entry(parent_path, filename(path));
  if (err != 0) {
    free(parent_path);
    parent_path = NULL;
    return err;
  }

  // Free temporary variables
  free(parent_path);
  parent_path = NULL;
  return 0;
}

int delete_file(const char *path) {
  inode *file = NULL;

  // Check for potential errors
  int err = resolve_path(path, &file);
  if (err != 0) {
    return err;
  }

  // Again decrease count of hardlinks
  file->reference_count--;

  // If hardlink count is 0, nuke the data
  if (file->reference_count == 0) {
    free(file->data);
    file->data = NULL;
    free(file);
    file = NULL;
  }

  // Remove entry from parent directory anyway
  char *parent = parent_of(path);
  err = remove_entry(parent, filename(path));
  if (err != 0) {
    free(parent);
    parent = NULL;
    return err;
  }

  free(parent);
  parent = NULL;
  return 0;
}

int delete_g(const char *path /*, bool recursive */) { // Param deleted for
                                                       // error suppression
  inode *target = NULL;

  // Error checking for path validation
  int err = resolve_path(path, &target);
  if (err != 0) {
    return err;
  }

  // Decide on deletion method
  // Given the specification, recursive will always be true
  // But too late, I am not refactoring this
  if (target->filetype == S_IFDIR) {
    delete_dir(path);
  } else {
    // Attempt to fix valgrind issue
    delete_file(path);
  }

  return 0;
}

int write_file(const char *path, char *data) {
  inode *target = NULL;

  // Error checking
  int err = resolve_path(path, &target);
  if (err != 0) {
    return err;
  }

  if (target->filetype == S_IFDIR) {
    return EISDIR;
  }

  // Overwrite the existing data
  target->data_size = strlen(data) + 1;
  free(target->data);
  target->data = NULL;
  target->data = malloc(target->data_size);
  memset(target->data, 0, target->data_size);
  if (target->data == NULL) {
    return ENOMEM;
  }

  // Copy the current text into the inode data field
  strcpy((char *)target->data, data);

  return 0;
}

int append_file(const char *path, const char *data) {
  inode *target = NULL;

  // Error checking
  int err = resolve_path(path, &target);

  if (err != 0) {
    return err;
  }

  if (target->filetype == S_IFDIR) {
    return EISDIR;
  }

  if (target->data_size == 0 || target->data == NULL) {
    target->data_size = strlen(data) + 1;
    target->data = malloc(target->data_size);
    if (target->data == NULL) {
      return ENOMEM; // Handle malloc failure
    }
    strcpy(target->data, data);
    return 0;
  }

  size_t old_size = target->data_size;
  size_t data_len = strlen(data);
  target->data_size += data_len;

  char *new_data = realloc(target->data, target->data_size);
  if (new_data == NULL) {
    return ENOMEM; // Handle realloc failure
  }
  target->data = new_data;

  strcpy(target->data + old_size - 1,
         data); // Append at the end (before null terminator)
  ((char *)target->data)[target->data_size - 1] =
      '\0'; // Ensure null termination

  return 0;
}

int add_entry(const char *path, const char *name, inode *target) {
  inode *dir = NULL;

  // Error checking
  int err = resolve_path(path, &dir);
  if (err != 0) {
    return err;
  }

  if (dir->filetype != S_IFDIR) {
    return ENOTDIR;
  }

  if (name[0] != '.') {
    dir->sorted = false;
  }

  void *data_copy = dir->data;
  // Reallocate proper size
  dir->data_size += sizeof(DIR_ENTRY);
  dir->data = realloc(dir->data, dir->data_size);
  // Memory realloc check
  if (dir->data == NULL) {
    free(data_copy);
    data_copy = NULL;
    return ENOMEM;
  }

  // Add entry (yes this looks absolutely horendous) and apparently worked
  // horrendously too, due to allocating over previously used space which led to
  // what can only be qualified as crimes against humanity
  strcpy(((DIR_ENTRY *)dir->data)[dir->data_size / sizeof(DIR_ENTRY) - 1].name,
         name);
  ((DIR_ENTRY *)dir->data)[dir->data_size / sizeof(DIR_ENTRY) - 1].item =
      target;

  if (dir->sorted == false) {
    qsort(dir->data + (sizeof(DIR_ENTRY) * 2),
          dir->data_size / sizeof(DIR_ENTRY) - 2, sizeof(DIR_ENTRY),
          compare_entries);
    dir->sorted = true;
  }

  return 0;
}

int remove_entry(const char *path, const char *name) {
  inode *target = NULL;

  // Error checking
  int err = resolve_path(path, &target);
  if (err != 0) {
    return err;
  }

  // Iterate over entries
  for (size_t i = 2; i < target->data_size / sizeof(DIR_ENTRY); i++) {
    // If names match, delete entry, then reallocate the array to reflect
    // current size
    if (strcmp(((DIR_ENTRY *)target->data)[i].name, name) == 0) {
      target->sorted = false;
      ((DIR_ENTRY *)target->data)[i] =
          ((DIR_ENTRY *)
               target->data)[target->data_size / sizeof(DIR_ENTRY) - 1];
      target->data_size -= sizeof(DIR_ENTRY);

      void *old_data = target->data;
      target->data = realloc(target->data, target->data_size);
      // If reallocation fails, then return ENOMEM
      if (target->data == NULL) {
        free(old_data);
        old_data = NULL;
        return ENOMEM;
      }
    }
  }

  if (target->sorted == false) {
    qsort(target->data + (sizeof(DIR_ENTRY) * 2),
          target->data_size / sizeof(DIR_ENTRY) - 2, sizeof(DIR_ENTRY),
          compare_entries);
  }

  return 0;
}

int entry_exists(inode *dir, const char *name) {
  size_t low = 0;
  size_t high = dir->data_size / sizeof(DIR_ENTRY) - 1;

  while (low <= high) {
    size_t mid = low + (high - low) / 2;

    DIR_ENTRY *mid_element = &((DIR_ENTRY *)dir->data)[mid];

    int comparison = strcmp(name, mid_element->name);
    if (comparison == 0) {
      return mid;
    } else if (comparison > 0) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }

  return -1;
}

// TODO: Implement the actual symlink logic, because as of now, this only
// traverses directories.
int resolve_path(const char *path, inode **result) {
  // Check whether path begins at root, or if it is a relative path.
  if (path[0] == '/') {
    *result = fs.root;
  } else {
    *result = fs.working_dir; // Otherwise set it to the current WD
  }

  char *path_copy = (char *)malloc(strlen(path) + 1);
  if (path_copy == NULL) {
    free(path_copy);
    path_copy = NULL;
    return ENOMEM;
  }

  memset(path_copy, 0, strlen(path) + 1);

  strcpy(path_copy, path);

  char *token = strtok(path_copy, "/");
  while (token != NULL) {
    // Check wether current node is a directory
    // Otherwise, path is invalid, since by this point it is evident that there
    // are further tokens
    if ((*result)->filetype != S_IFDIR) {
      free(path_copy);
      path_copy = NULL;
      return ENOTDIR;
    }

    // Figure out wether the next node actually exists.
    // Otherwise return the fact that this directory does not exist
    int tok_index = entry_exists(*result, token);
    if (tok_index == -1) {
      free(path_copy);
      path_copy = NULL;
      return ENOENT;
    }

    // Set the new current dir to the inode that is the result
    *result = ((DIR_ENTRY *)(*result)->data)[tok_index].item;

    // Get the new token in order
    token = strtok(NULL, "/");
  }

  // If we got to this point, we have resolved the path.
  free(path_copy);
  path_copy = NULL;
  return 0;
}

char *parent_of(const char *path) {
  char *last_tok = strrchr(path, '/');
  if (last_tok == NULL) {
    char *current_dir = malloc(2);
    if (current_dir == NULL) {
      exit(ENOMEM);
    }
    memset(current_dir, 0, 2);
    current_dir[0] = '.';
    current_dir[1] = '\0';
    return current_dir;
  }

  if (last_tok == path && path[0] == '/') { // Handle root directory
    char *root_copy = malloc(2); // Allocate space for "/" and null terminator
    if (root_copy == NULL) {
      exit(ENOMEM);
    }
    memset(root_copy, 0, 2);
    root_copy[0] = '/';
    root_copy[1] = '\0';
    return root_copy;
  }

  char *copy = (char *)malloc((size_t)(last_tok - path) + 1);
  if (copy == NULL) {
    exit(ENOMEM);
  }
  memset(copy, 0, (size_t)(last_tok - path) + 1);

  memcpy(copy, path, (size_t)(last_tok - path));
  copy[last_tok - path] = '\0';

  return copy;
}

char *filename(const char *path) {
  char *last_tok = strrchr(path, '/');
  if (last_tok) {
    return last_tok + 1;
  }
  return (char *)path;
}

char *append(const char *path, const char *name) {
  const size_t len1 = strlen(path);
  const size_t len2 = strlen(name);
  char *result = malloc(len1 + len2 + 1);
  memset(result, 0, len1 + len2 + 1);

  if (result == NULL) {
    exit(ENOMEM);
  }

  memcpy(result, path, len1);
  memcpy(result + len1, name, len2 + 1);
  return result;
}

int copy_file(const char *src, const char *dest) {
  inode *dest_file = NULL;
  int err = resolve_path(dest, &dest_file);
  if (err == 0) {
    err = delete_g(dest);
    if (err != 0) {
      return err;
    }
  } else if (err != ENOENT) {
    return err;
  }

  err = create_file(dest);
  if (err != 0) {
    return err;
  }
  err = resolve_path(dest, &dest_file);
  if (err != 0) {
    return err;
  }

  inode *src_file = NULL;
  err = resolve_path(src, &src_file);
  if (err != 0) {
    return err;
  }

  dest_file->data_size = src_file->data_size;
  dest_file->data = malloc(dest_file->data_size);
  memcpy(dest_file->data, src_file->data, dest_file->data_size);

  return 0;
}

int copy_dir(const char *src, const char *dest) {
  int err = create_dir(dest);
  if (err != 0) {
    return err;
  }

  inode *dest_dir = NULL;
  err = resolve_path(dest, &dest_dir);
  if (err != 0 || dest_dir == NULL) {
    return err != 0 ? err : ENOENT;
  }

  inode *src_dir = NULL;
  err = resolve_path(src, &src_dir);
  if (err != 0 || src_dir == NULL) {
    return err != 0 ? err : ENOENT;
  }

  char *parent = parent_of(dest);
  inode *parent_dir = NULL;
  err = resolve_path(parent, &parent_dir);
  if (err != 0 || parent_dir == NULL) {
    return err != 0 ? err : ENOENT;
  }

  for (size_t i = 2; i < src_dir->data_size / sizeof(DIR_ENTRY); i++) {
    char *src_buf = NULL;
    char *src_file = NULL;
    char *dst_buf = NULL;
    char *dst_file = NULL;

    src_buf = append(src, "/");
    dst_buf = append(dest, "/");
    src_file = append(src_buf, ((DIR_ENTRY *)src_dir->data)[i].name);
    dst_file = append(dst_buf, ((DIR_ENTRY *)src_dir->data)[i].name);

    if (src_file == NULL || dst_file == NULL) {
      free(src_buf);
      src_buf = NULL;
      free(dst_buf);
      dst_buf = NULL;
      free(src_file);
      src_file = NULL;
      free(dst_file);
      src_file = NULL;
      return ENOMEM;
    }

    copy(src_file, dst_file);
    free(src_buf);
    src_buf = NULL;
    free(dst_buf);
    dst_buf = NULL;
    free(src_file);
    src_file = NULL;
    free(dst_file);
    src_file = NULL;
  }

  free(parent);
  parent = NULL;
  return 0;
}

int copy(const char *src, const char *dest) {
  inode *src_entity = NULL;
  int err = resolve_path(src, &src_entity);
  if (err != 0 || src_entity == NULL) {
    return err != 0 ? err : ENOENT;
  }

  if (src_entity->filetype == S_IFDIR) {
    copy_dir(src, dest);
  } else {
    copy_file(src, dest);
  }

  return 0;
}

void init_fs(void) {
  fs.root = (inode *)malloc(sizeof(inode));
  memset(fs.root, 0, sizeof(inode));
  fs.working_dir = fs.root;

  // Add proper information to root ;
  fs.root->reference_count = 0;
  fs.root->filetype = S_IFDIR;
  fs.root->data_size = 0;
  fs.root->data = NULL;
  fs.root->sorted = true;

  int err = add_entry("/", ".", fs.root);
  if (err != 0) {
    free(fs.root->data);
    free(fs.root);
    exit(err);
  }
  err = add_entry("/", "..", fs.root);
  if (err != 0) {
    free(fs.root->data);
    free(fs.root);
    exit(err);
  }
}

int clear_fs(void) {
  int err = delete_dir("/");
  if (err != 0) {
    return err;
  }

  free(fs.root->data);
  fs.root->data = NULL;
  fs.root->data_size = 0;
  free(fs.root);
  fs.root = NULL;

  return 0;
}
