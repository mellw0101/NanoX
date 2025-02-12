/** @file dirs.c

  @author  Melwin Svensson.
  @date    10-2-2025.

 */
#include "../include/c_proto.h"

/* Return `TRUE` when path exists and is a directory and when we have access to the directory. */
bool dir_exists(const char *const __restrict path) {
  ASSERT(path);
  struct stat st;
  /* If the path exists, but we do not have permissions to access it return false. */
  if (access(path, R_OK) != 0) {
    return FALSE;
  }
  return (stat(path, &st) != -1 && S_ISDIR(st.st_mode));
}

/* Create a new blank allocated directory entry. */
directory_entry_t *directory_entry_make(void) {
  directory_entry_t *entry = xmalloc(sizeof(*entry));
  entry->type       = 0;
  entry->name       = NULL;
  entry->path       = NULL;
  entry->ext        = NULL;
  entry->clean_name = NULL;
  entry->stat       = NULL;
  return entry;
}

/* Retrieve a `directory_entry_t *` from a `directory_t` structure, note that this removes the ptr in `dir` to this entry. */
directory_entry_t *directory_entry_extract(directory_t *const dir, Ulong idx) {
  ASSERT(dir);
  ASSERT_MSG(dir->entries,
    "The passed directory_t structure has not been initilized, if using a stack "
    "based directory_t structure initilize it with 'directory_data_init(&dir).'"
  );
  /* Ensure this is a valid index before anything else. */
  ALWAYS_ASSERT(idx < dir->len);
  directory_entry_t *retentry = dir->entries[idx];
  /* Move the entries over by one including the NULL-TERMINATOR. */
  memmove((dir->entries + idx), (dir->entries + idx + 1), ((dir->len - idx) * sizeof(void *)));
  --dir->len;
  return retentry;
}

/* Using the available internal data of `entry`, perform the same operation as `file_exsists()` using less operation. */
bool directory_entry_is_file(directory_entry_t *const entry) {
  ASSERT(entry);
  if (access(entry->path, R_OK) != 0) {
    return FALSE;
  }
  return (entry->stat && !(S_ISDIR(entry->stat->st_mode) || S_ISCHR(entry->stat->st_mode) || S_ISBLK(entry->stat->st_mode)));
}

/* Using the available internal data of `entry`, perform the same operation as `non_exec_file_exsists()` using less operation. */
bool directory_entry_is_non_exec_file(directory_entry_t *const entry) {
  ASSERT(entry);
  if (access(entry->path, R_OK) != 0) {
    return FALSE;
  }
  return (entry->stat && !(S_ISDIR(entry->stat->st_mode) || S_ISCHR(entry->stat->st_mode) || S_ISBLK(entry->stat->st_mode)) && !(entry->stat->st_mode & S_IXUSR));
}

/* Free the data of an `directory_entry_t`, then free the entry itself. */
void directory_entry_free(directory_entry_t *const entry) {
  ASSERT(entry);
  ASSERT(entry->name);
  ASSERT(entry->path);
  /* Free the data assosiated with entry. */
  free(entry->name);
  free(entry->path);
  free(entry->ext);
  free(entry->clean_name);
  free(entry->stat);
  /* Then free entry itself. */
  free(entry);
}

/* Init a directory_t structure. */
void directory_data_init(directory_t *const dir) {
  ASSERT(dir);
  dir->len = 0;
  dir->cap = 10;
  dir->entries = xmalloc(sizeof(void *) * dir->cap);
  init_mutex(&dir->mutex, NULL);
}

/* Free the internal data of a `directory_t` structure. */
void directory_data_free(directory_t *const dir) {
  ASSERT(dir);
  /* Ensure thread-safe destruction of the 'directory_t' structure. */
  under_mutex(&dir->mutex,
    /* If another thread freed the internal data of dir while we were waiting on the mutex lock, just leave. */
    if (!dir->entries) {
      unlock_mutex(&dir->mutex);
      return;
    }
    /* Iter until len reatches zero. */
    while (dir->len) {
      directory_entry_free(dir->entries[--dir->len]);
    }
    free(dir->entries);
    dir->entries = NULL;
  );
  destroy_mutex(&dir->mutex);
}

/* Get all entries in `path` and append them onto `output->entries`.  Return `-1` on error.  Otherwise, `0`. */
int directory_get(const char *const __restrict path, directory_t *const output) {
  ASSERT(path);
  ASSERT(output);
  const char *fileext = NULL;
  struct dirent *direntry;
  DIR *dir;
  directory_entry_t *entry;
  /* If the path does not exist, or is not a directory.  Return early. */
  if (!dir_exists(path)) {
    return -1;
  }
  /* Open the directory. */
  ALWAYS_ASSERT_MSG((dir = opendir(path)), strerror(errno));
  /* Iter over all entries in opened dir. */
  while ((direntry = readdir(dir))) {
    /* Skip directory trevarsal entries. */
    if (direntry->d_type == DT_DIR && _D_ALLOC_NAMLEN(direntry) <= 5 && direntry->d_name[0] == '.'
     && (direntry->d_name[1] == '\0' || (direntry->d_name[1] == '.' && direntry->d_name[2] == '\0'))) {
      continue;
    }
    /* Allocate the directory_entry_t structure. */
    entry = directory_entry_make();
    /* Allocate the internal data. */
    entry->type = direntry->d_type;
    entry->name = measured_copy(direntry->d_name, (_D_ALLOC_NAMLEN(direntry) - 1));
    entry->path = concatpath(path, entry->name);
    fileext = ext(entry->name);
    if (fileext) {
      entry->ext = copy_of(fileext + 1);
      entry->clean_name = measured_copy(entry->name, (fileext - entry->name));
    }
    statalloc(entry->path, &entry->stat);
    /* Insure thread-safe insertion of the entry. */
    under_mutex(&output->mutex,
      /* Insert the entry into output. */
      ENSURE_PTR_ARRAY_SIZE(output->entries, output->cap, output->len);
      output->entries[output->len++] = entry;
    );
  }
  /* Make the trimming of the entries array thread-safe. */
  under_mutex(&output->mutex,
    /* Trim the entries array and NULL-TERMINATE it to same memory and ensure safe iteration even without a len. */
    TRIM_PTR_ARRAY(output->entries, output->cap, output->len);
    output->entries[output->len] = NULL;
  );
  closedir(dir);
  return 0;
}

/* Recursivly get all entries in `path`. */
int directory_get_recurse(const char *const __restrict path, directory_t *const output) {
  ASSERT(path);
  ASSERT(output);
  ASSERT(output->entries);
  /* The subdir, if any exists. */
  char *subdir;
  /* Used to scope the recursive nature of this function.  As it will modify the same structure. */
  Ulong waslen, newlen;
  /* Set waslen before running directory_get(). */
  waslen = output->len;
  if (directory_get(path, output) == -1) {
    return -1;
  }
  /* Then set newlen after. */
  newlen = output->len;
  /* Now we have a set scope to perform the recursive calls. */
  for (Ulong i = waslen; i < newlen; ++i) {
    if (output->entries[i]->type == DT_DIR) {
      subdir = concatpath(path, output->entries[i]->name);
      directory_get_recurse(subdir, output);
      free(subdir);
    }
  }
  return 0;
}


/* ------------- Tests ------------- */


void test_directory_t(const char *const dirpath) {
  ASSERT(dirpath);
  if (!dir_exists(dirpath)) {
    return;
  }
  directory_t dir;
  directory_entry_t *entry;
  directory_data_init(&dir);
  /* Start timer. */
  TIMER_START(timer);
  ASSERT(directory_get_recurse(dirpath, &dir) != -1);
  /* Stop timer. */
  TIMER_END(timer, ms);
  for (Ulong i = 0; i < dir.len; ++i) {
    entry = dir.entries[i];
    printf("%s\n", entry->path);
  }
  directory_data_free(&dir);
  printf("%s: Time: %.5f ms\n", __func__, (double)ms);
}
