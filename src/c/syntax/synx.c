/** @file synx.c

  @author  Melwin Svensson.
  @date    10-2-2025.

 */
#include "../../include/c_proto.h"


/* -------------------------------------------------------- Struct's -------------------------------------------------------- */


/* A structure that reprecents a position inside a `SyntaxFile` structure. */
struct SyntaxFilePos {
  int row;
  int column;
};

/* The values in the hashmap that `syntax_file_t` uses. */
struct SyntaxObject {
  /* The color this should be draw as. */
  SyntaxColor color;

  /* Type of syntax this object this is. */
  SyntaxObjectType type;
  
  /* Only used when there is more then one object with the same name. */
  SyntaxObject *next;
};


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

/* Free a entire double linked list of `SyntaxFileLine` structures. */
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
void syntaxfileline_from_str(const char *const string, SyntaxFileLine **const head, SyntaxFileLine **const tail) {
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
    filebot->data = measured_copy(start, (end - start));
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


/* Create a allocated blank `SyntaxObject` structure. */
SyntaxObject *syntaxobject_create(void) {
  SyntaxObject *object = xmalloc(sizeof(*object));
  object->color = SYNTAX_COLOR_NONE;
  object->type  = SYNTAX_OBJECT_TYPE_NONE;
  return object;
}

/* Free a `SyntaxObject` structure. */
void syntaxobject_free(void *ptr) {
  ASSERT(ptr);
  SyntaxObject *object = ptr;
  free(object);
}


/* -------------------------------------------------------- SyntaxFile -------------------------------------------------------- */


/* Create a allocated blank `SyntaxFile` structure. */
SyntaxFile *syntaxfile_create(void) {
  SyntaxFile *sfile = xmalloc(sizeof(*sfile));
  sfile->path    = NULL;
  sfile->objects = hashmap_create();
  sfile->filetop = NULL;
  sfile->filebot = NULL;
  sfile->stat    = NULL;
  hashmap_set_free_value_callback(sfile->objects, syntaxobject_free);
  return sfile;
}

/* Free a `SyntaxFile` structure. */
void syntaxfile_free(SyntaxFile *const sfile) {
  ASSERT(sfile);
  ASSERT(sfile->objects);
  free(sfile->path);
  hashmap_free(sfile->objects);
  syntaxfileline_free_lines(sfile->filetop);
  free(sfile);
}

/* Read the file at `path` into the `SyntaxFile` struct `sfile`. */
void syntaxfile_read(SyntaxFile *const sfile, const char *const __restrict path) {
  ASSERT(sfile);
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
  sfile->path = copy_of(path);
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
  syntaxfileline_from_str(data, &sfile->filetop, &sfile->filebot);
}


/* -------------------------------------------------------- Tests -------------------------------------------------------- */


static void syntaxfile_test_read_one_file(const char *path, Ulong *nlines) {
  ASSERT(path);
  SyntaxFile *sfile = syntaxfile_create();
  syntaxfile_read(sfile, path);
  ASSIGN_IF_VALID(nlines, sfile->filebot->lineno);
  syntaxfile_free(sfile);
}

void syntaxfile_test_read(void) {
  TIMER_START(timer);
  Ulong num_lines, total_lines=0, files_read=0;
  directory_t dir;
  directory_entry_t *entry;
  directory_data_init(&dir);
  if (directory_get_recurse("/usr/include", &dir) != -1) {
    for (Ulong i = 0; i < dir.len; ++i) {
      entry = dir.entries[i];
      if (directory_entry_is_non_exec_file(entry) && entry->ext && (strcmp(entry->ext, "xml") == 0 || strcmp(entry->ext, "c") == 0 || strcmp(entry->ext, "cpp") == 0
       || strcmp(entry->ext, "h") == 0 || strcmp(entry->ext, "hpp") == 0 || strcmp(entry->ext, "txt") == 0 || strcmp(entry->ext, "log") == 0)) {
        syntaxfile_test_read_one_file(entry->path, &num_lines);
        total_lines += num_lines;
        ++files_read;
      }
      free(entry->stat);
      entry->stat = NULL;
    }
  }
  directory_data_free(&dir);
  TIMER_END(timer, ms);
  printf("\n%s: Files read: %lu: Lines read: %lu: Time: %.5f ms\n\n", __func__, files_read, total_lines, (double)ms);
}
