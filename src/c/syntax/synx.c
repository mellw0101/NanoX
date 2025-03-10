/** @file synx.c

  @author  Melwin Svensson.
  @date    10-2-2025.

 */
#include "../../include/c_proto.h"


/* -------------------------------------------------------- SyntaxFileError -------------------------------------------------------- */


/* Create a new allocated `SyntaxFilePos` structure. */
SyntaxFilePos *syntaxfilepos_create(int row, int column) {
  SyntaxFilePos *pos = xmalloc(sizeof(*pos));
  pos->row    = row;
  pos->column = column;
  return pos;
}

/* Free a `SyntaxFilePos` structure. */
void syntaxfilepos_free(SyntaxFilePos *const pos) {
  free(pos);
}

/* Set the values for a allocated `SyntaxFilePos` structure. */
void syntaxfilepos_set(SyntaxFilePos *const pos, int row, int column) {
  ASSERT(pos);
  pos->row    = row;
  pos->column = column;
}


/* -------------------------------------------------------- SyntaxFileLine -------------------------------------------------------- */


/* Create a new `SyntaxFileLine *` and append it to the end of a double linked list of lines, or NULL for first line. */
SyntaxFileLine *syntaxfileline_create(SyntaxFileLine *const prev) {
  SyntaxFileLine *node = xmalloc(sizeof(*node));
  node->prev   = prev;
  node->next   = NULL;
  node->data   = NULL;
  node->lineno = (prev ? (prev->lineno + 1) : 1);
  return node;
}

/* Free the internal data of a `SyntaxFileLine` structure, then free the structure itself. */
void syntaxfileline_free(SyntaxFileLine *const line) {
  ASSERT(line);
  free(line->data);
  free(line);
}

/* Unlink `line` from the double linked list it belongs to. */
void syntaxfileline_unlink(SyntaxFileLine *const line) {
  ASSERT(line);
  /* If this is not the first line in the double linked list. */
  if (line->prev) {
    /* Set the previous line's next line to the line we want to unlink's next. */
    line->prev->next = line->next;
  }
  /* If this is not the last line in the double linked list. */
  if (line->next) {
    /* Set the next line's previous line to the previous line of the line we want to unlink. */
    line->next->prev = line->prev;
  }
  syntaxfileline_free(line);
}

/* Free a entire double linked list of `SyntaxFileLine` structures.  This function is `NULL-SAFE`. */
void syntaxfileline_free_lines(SyntaxFileLine *head) {
  if (!head) {
    return;
  }
  while (head->next) {
    head = head->next;
    syntaxfileline_free(head->prev);
  }
  syntaxfileline_free(head);
}

/* Create a double linked list from string. */
void syntaxfileline_from_str(const char *const __restrict string, SyntaxFileLine **const head, SyntaxFileLine **const tail) {
  ASSERT(string);
  ASSERT(head);
  ASSERT(tail);
  SyntaxFileLine *filetop = syntaxfileline_create(NULL);
  SyntaxFileLine *filebot = filetop;
  /* Initilaze both start and end to string. */
  const char *start = string;
  const char *end   = string;
  /* Iterate until the end of the string. */
  while (*end) {
    /* Advance end until we se the delimiter. */
    while (*end && *end != '\n') {
      ++end;
    }
    filebot->len  = (end - start);
    filebot->data = measured_copy(start, filebot->len);
    /* If we have retched the end of the file, just break. */
    if (!*end) {
      break;
    }
    /* Otherwise, make the next line and set filebot to it. */
    else {
      ++end;
      filebot->next = syntaxfileline_create(filebot);
      filebot = filebot->next;
    }
    /* Set start to the first char that is not delim. */
    start = end;
  }
  /* When the file ends with just a '\n' char, the last line->data will
   * be NULL, this is because we make the new line last in the loop. */
  if (!filebot->data) {
    filebot->data = copy_of("");
  }
  /* Assign filetop and filebot to head and tail. */
  *head = filetop;
  *tail = filebot;
}


/* -------------------------------------------------------- SyntaxObject -------------------------------------------------------- */


// /* Create a allocated blank `SyntaxObject` structure. */
// SyntaxObject *syntaxobject_create(void) {
//   SyntaxObject *object = xmalloc(sizeof(*object));
//   object->prev     = NULL;
//   object->next     = NULL;
//   object->color    = SYNTAX_COLOR_NONE;
//   object->type     = SYNTAX_OBJECT_TYPE_NONE;
//   object->pos      = syntaxfilepos_create(0, 0);
//   object->data     = NULL;
//   object->freedata = NULL;
//   return object;
// }

// /* Free a `SyntaxObject` structure. */
// void syntaxobject_free(SyntaxObject *const obj) {
//   ASSERT(obj);
//   ASSERT(obj->pos);
//   /* Then free the last one. */
//   if (obj->data) {
//     CALL_IF_VALID(obj->freedata, obj->data);
//   }
//   syntaxfilepos_free(obj->pos);
//   free(obj);
// }

// /* Unlink and free a `SyntaxObject` structure from its double linked list. */
// void syntaxobject_unlink(SyntaxObject *const obj) {
//   ASSERT(obj);
//   ASSERT(obj->pos);
//   if (obj->prev) {
//     obj->prev->next = obj->next;
//   }
//   if (obj->next) {
//     obj->next->prev = obj->prev;
//   }
//   syntaxobject_free(obj);
// }

// /* Free a entire double linked list of `SyntaxObject` structure's. */
// void syntaxobject_free_objects(void *ptr) {
//   SyntaxObject *head = ptr;
//   ASSERT(head);
//   ASSERT(head->pos);
//   /* If there are any stacked objects, free all of them. */
//   while (head->next) {
//     head = head->next;
//     syntaxobject_free(head->prev);
//   }
//   syntaxobject_free(head);
// }


/* Create a allocated blank SyntaxObject structure. */
SyntaxObject *syntaxobject_create(void) {
  SyntaxObject *node = xmalloc(sizeof(*node));
  /* Make this the only object in the double circular list. */
  node->prev     = node;
  node->next     = node;
  node->color    = SYNTAX_COLOR_NONE;
  node->type     = SYNTAX_OBJECT_TYPE_NONE;
  node->pos      = syntaxfilepos_create(0, 0);
  node->data     = NULL;
  node->freedata = NULL;
  return node;
}

/* Free a SyntaxObject structure. */
void syntaxobject_free(SyntaxObject *const obj) {
  ASSERT(obj);
  ASSERT(obj->pos);
  CALL_IF_VALID(obj->freedata, obj->data);
  syntaxfilepos_free(obj->pos);
  free(obj);
}

/* Unlink and free a SyntaxObject structure from its double linked list. */
void syntaxobject_unlink(SyntaxObject *const obj) {
  ASSERT(obj);
  ASSERT(obj->pos);
  obj->prev->next = obj->next;
  obj->next->prev = obj->prev;
  syntaxobject_free(obj);
}

/* Free a entire double linked list of SyntaxObject structure's. */
void syntaxobject_free_objects(void *ptr) {
  SyntaxObject *head = ptr;
  ASSERT(head);
  ASSERT(head->pos);
  /* If there are any stacked objects, free all of them. */
  while (head != head->next) {
    syntaxobject_unlink(head->next);
  }
  syntaxobject_free(head);
}

/* Set a `SyntaxObject`'s data, and the function that will be used to free it, or `NULL` when not needed. */
void syntaxobject_setdata(SyntaxObject *const obj, void *const data, FreeFuncPtr freedatafunc) {
  ASSERT(obj);
  ASSERT(obj->pos);
  ASSERT(data);
  obj->data     = data;
  obj->freedata = freedatafunc;
}

/* Set the row and column of this `SyntaxObject`. */
void syntaxobject_setpos(SyntaxObject *const obj, int row, int column) {
  ASSERT(obj);
  ASSERT(obj->pos);
  obj->pos->row    = row;
  obj->pos->column = column;
}

/* Set the color of a `SyntaxObject`.  NOTE: we will probebly change the internal strucure later to just have a 4 float or 4 char based color. */
void syntaxobject_setcolor(SyntaxObject *const obj, SyntaxColor color) {
  ASSERT(obj);
  ASSERT(obj->pos);
  obj->color = color;
}

/* Set the `type` of a `SyntaxObject`. */
void syntaxobject_settype(SyntaxObject *const obj, SyntaxObjectType type) {
  ASSERT(obj);
  ASSERT(obj->pos);
  obj->type = type;
}


/* -------------------------------------------------------- SyntaxFileError -------------------------------------------------------- */


/* Create a new `SyntaxFileError *` and append it to the end of a double linked list of errors, or `NULL` for first error. */
SyntaxFileError *syntaxfileerror_create(SyntaxFileError *const prev) {
  SyntaxFileError *node = xmalloc(sizeof(*node));
  node->prev = prev;
  node->next = NULL;
  node->msg  = NULL;
  node->pos  = syntaxfilepos_create(0, 0);
  return node;
}

/* Free a allocated `SyntaxFileError` struct. */
void syntaxfileerror_free(SyntaxFileError *const err) {
  ASSERT(err);
  ASSERT(err->pos);
  syntaxfilepos_free(err->pos);
  free(err->msg);
  free(err);
}

/* Unlink `error` from the double linked list. */
void syntaxfileerror_unlink(SyntaxFileError *const err) {
  ASSERT(err);
  ASSERT(err->pos);
  if (err->prev) {
    err->prev->next = err->next;
  }
  if (err->next) {
    err->next->prev = err->prev;
  }
  syntaxfileerror_free(err);
}

/* Free a entire double linked list of `SyntaxFileError` structure's. */
void syntaxfileerror_free_errors(SyntaxFileError *head) {
  /* Make this function NULL-SAFE. */
  if (!head) {
    return;
  }
  while (head->next) {
    head = head->next;
    syntaxfileerror_free(head->prev);
  }
  syntaxfileerror_free(head);
}


/* -------------------------------------------------------- SyntaxFile -------------------------------------------------------- */


/* Create a allocated blank `SyntaxFile` structure. */
SyntaxFile *syntaxfile_create(void) {
  SyntaxFile *sfile = xmalloc(sizeof(*sfile));
  sfile->path    = NULL;
  sfile->filetop = NULL;
  sfile->filebot = NULL;
  sfile->errtop  = NULL;
  sfile->errbot  = NULL;
  sfile->stat    = NULL;
  sfile->objects = hashmap_create();
  hashmap_set_free_value_callback(sfile->objects, syntaxobject_free_objects);
  return sfile;
}

/* Free a `SyntaxFile` structure. */
void syntaxfile_free(SyntaxFile *const sf) {
  ASSERT(sf);
  ASSERT(sf->objects);
  free(sf->path);
  hashmap_free(sf->objects);
  syntaxfileline_free_lines(sf->filetop);
  syntaxfileerror_free_errors(sf->errtop);
  free(sf);
}

/* Read the file at `path` into the `SyntaxFile` struct `sfile`. */
void syntaxfile_read(SyntaxFile *const sf, const char *const restrict path) {
  ASSERT(sf);
  ASSERT(path);
  char *data;
  char  buffer[4096];
  Ulong bytes_read, total_bytes=0;
  int   fd;
  /* If the file does not exist, return early. */
  if (!file_exists(path)) {
    return;
  }
  /* If the file exists, set the syntaxfile path to its path. */
  sf->path = copy_of(path);
  /* Init the data ptr. */
  data = xmalloc(1);
  /* Open the fd as a read only file-descriptor. */
  ALWAYS_ASSERT_MSG(((fd = open(path, O_RDONLY)) >= 0), strerror(errno));
  /* Lock the file-descriptor. */
  ALWAYS_ASSERT(lock_fd(fd, F_RDLCK));
  /* Then read all data from fd. */
  while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
    data = xrealloc(data, (bytes_read + total_bytes + 1));
    memcpy((data + total_bytes), buffer, bytes_read);
    total_bytes += bytes_read;
  }
  /* Unlock the file-descriptor. */
  ALWAYS_ASSERT(unlock_fd(fd));
  /* Null terminate the data buffer and close the fd. */
  data[total_bytes] = '\0';
  close(fd);
  /* Split the data into lines. */
  syntaxfileline_from_str(data, &sf->filetop, &sf->filebot);
  /* Free the data after we create the lines. */
  free(data);
}

/* Add a error to the `SyntaxFile`. */
void syntaxfile_adderror(SyntaxFile *const sf, int row, int column, const char *const restrict msg) {
  ASSERT(sf);
  /* If this is the first error, both errtop and errbot will point to the same data. */
  if (!sf->errtop) {
    sf->errtop = syntaxfileerror_create(NULL);
    sf->errbot = sf->errtop;
  }
  /* Otherwise, append a new error to errbot, and set errbot to the new error. */
  else {
    sf->errbot->next = syntaxfileerror_create(sf->errbot);
    sf->errbot = sf->errbot->next;
  }
  /* Set the position data for the error. */
  sf->errbot->pos->row    = row;
  sf->errbot->pos->column = column;
  /* Set the message of the error. */
  sf->errbot->msg = copy_of(msg);
}

/* Add a SyntaxObject to the hashmap of a SyntaxFile structure, and if it exists, append it to the double linked list that is the object's. */
void syntaxfile_addobject(SyntaxFile *const sf, const char *const restrict key, SyntaxObject *const obj) {
  ASSERT(sf);
  ASSERT(key);
  ASSERT(obj);
  SyntaxObject *existing = hashmap_get(sf->objects, key);
  /* If there does not exist any objects with the same key, insert it. */
  if (!existing) {
    hashmap_insert(sf->objects, key, obj);
  }
  /* Otherwise, append the object to the double linked list. */
  else {
    /* Add the new object before the first one, i.e: in the back. */
    obj->next = existing;
    obj->prev = existing->prev;
    existing->prev->next = obj;
    existing->prev       = obj;
  }
}

// /* Add a `SyntaxObject` to the hashmap of a `SyntaxFile` structure, and if it exists, append it to the double linked list that is the object's. */
// void syntaxfile_addobject(SyntaxFile *const sf, const char *const __restrict key, SyntaxObject *const value) {
//   ASSERT(sf);
//   ASSERT(key);
//   ASSERT(value);
//   SyntaxObject *existing = hashmap_get(sf->objects, key);
//   /* If there does not exist any objects with the same key, insert it. */
//   if (!existing) {
//     hashmap_insert(sf->objects, key, value);
//   }
//   /* Otherwise, append the object to the double linked list. */
//   else {
//     existing->next = value;
//     value->prev = existing;
//   }
// }


/* -------------------------------------------------------- Tests -------------------------------------------------------- */


static void syntaxfile_test_read_one_file(const char *path, Ulong *nlines) {
  ASSERT(path);
  SyntaxFile *sfile = syntaxfile_create();
  syntaxfile_read(sfile, path);
  process_syntaxfile_c(sfile);
  ASSIGN_IF_VALID(nlines, sfile->filebot->lineno);
  syntaxfile_free(sfile);
}

void syntaxfile_test_read(void) {
  Ulong num_lines, total_lines=0, files_read=0;
  directory_t dir;
  directory_entry_t *entry;
  timer_action(ms,
    directory_data_init(&dir);
    if (directory_get_recurse("/usr/include", &dir) != -1) {
      for (Ulong i = 0; i < dir.len; ++i) {
        entry = dir.entries[i];
        if (directory_entry_is_non_exec_file(entry) && entry->ext && (strcmp(entry->ext, "h") == 0 || strcmp(entry->ext, "c") == 0)) {
          syntaxfile_test_read_one_file(entry->path, &num_lines);
          total_lines += num_lines;
          ++files_read;
        }
        free(entry->stat);
        entry->stat = NULL;
      }
    }
  );
  directory_data_free(&dir);
  printf("\n%s: Files read: %lu: Lines read: %lu: Time: %.5f ms\n\n", __func__, files_read, total_lines, (double)ms);
}
