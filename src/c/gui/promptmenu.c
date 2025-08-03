/** @file gui/promptmenu.c

  @author  Melwin Svensson.
  @date    29-7-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_PM           \
  ASSERT(pm);               \
  ASSERT(pm->buf);          \
  ASSERT(pm->element);      \
  ASSERT(pm->menu);         \
  ASSERT(pm->completions)

#define PROMPTMENU_DEFAULT_WIDTH  600


/* ---------------------------------------------------------- Enum's ---------------------------------------------------------- */


typedef enum {
  PROMPTMENU_ACTIVE       = (1 << 0),
  PROMPTMENU_REFRESH_TEXT = (1 << 1),
  PROMPTMENU_REFRESH_POS  = (1 << 2),
  PROMPTMENU_YN_MODE      = (1 << 3),
# define PROMPTMENU_ACTIVE          PROMPTMENU_ACTIVE
# define PROMPTMENU_REFRESH_TEXT    PROMPTMENU_REFRESH_TEXT
# define PROMPTMENU_REFRESH_POS     PROMPTMENU_REFRESH_POS
# define PROMPTMENU_YN_MODE         PROMPTMENU_YN_MODE
# define PROMPTMENU_DEFAULT_XFLAGS  (0U)
} PromptMenuState;


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


typedef struct {
  Uint xflags;
  vertex_buffer_t *buf;
  Element *element;
  Menu *menu;
  CVec *completions;

  /* The current mode we are in. */
  PromptMenuType mode;

  /* Data used for actions. */
  void *data;
} PromptMenu;


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* The source file global ptr to the opaque prompt-menu structure. */
static PromptMenu *pm = NULL;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* ----------------------------- Promptmenu pos ----------------------------- */

static void promptmenu_pos(void *arg, float _UNUSED w, float _UNUSED h, float *const x, float *const y) {
  ASSERT(arg);
  ASSERT(x);
  ASSERT(y);
  PromptMenu *p = arg;
  *x = p->element->x;
  *y = (p->element->y + p->element->height);
}

/* ----------------------------- Promptmenu accept ----------------------------- */

/* `Menu accept routine`.  Note that this will simply replace the answer to the entry's lable. */
static void promptmenu_accept(void *arg, const char *const restrict lable, int _UNUSED index) {
  ASSERT(arg);
  ASSERT(lable);
  do_statusbar_replace(lable);
  pm->xflags |= PROMPTMENU_REFRESH_TEXT;
}

/* ----------------------------- Promptmenu add prompt text ----------------------------- */

static inline void promptmenu_add_prompt_text(void) {
  float x = (pm->element->x + font_breadth(uifont, " "));
  float y = (pm->element->y + font_row_baseline(uifont, 0));
  font_vertbuf_add_mbstr(uifont, pm->buf, prompt, strlen(prompt), NULL, PACKED_UINT(255, 255, 255, 255), &x, &y);
  font_vertbuf_add_mbstr(uifont, pm->buf, answer, strlen(answer), " ",  PACKED_UINT(255, 255, 255, 255), &x, &y);
}

/* ----------------------------- Promptmenu add cursor ----------------------------- */

static inline void promptmenu_add_cursor(void) {
  font_add_cursor(
    uifont,
    pm->buf,
    0,
    PACKED_UINT(255,255,255,255),
    (pm->element->x + font_breadth(uifont, " ") + font_breadth(uifont, prompt) + font_wideness(uifont, answer, typing_x)),
    pm->element->y
  );
}

/* ----------------------------- Promptmenu find completions ----------------------------- */

static void promptmenu_find_completions(void) {
  ASSERT_PM;
  HashMap *map;
  char *entry;
  Ulong j;
  Ulong len;
  menu_clear_entries(pm->menu);
  if (cvec_len(pm->completions)) {
    map = hashmap_create();
    len = strlen(answer);
    for (int i=0; i<cvec_len(pm->completions); ++i) {
      entry = cvec_get(pm->completions, i);
      /* When the first byte does not match, just continue. */
      if (*entry != *answer) {
        continue;
      }
      /* Otherwise, check the rest. */
      for (j=1; j<len && (entry[j] == answer[j]); ++j);
      /* All bytes did not match.  Or, the entry is already in the map. */
      if (j < len || hashmap_get(map, entry)) {
        continue;
      }
      hashmap_insert(map, entry, entry);
      menu_push_back(pm->menu, entry);
    }
    hashmap_free(map);
    menu_qsort(pm->menu, menu_qsort_cb_strlen);
    menu_refresh_text(pm->menu);
  }
  /* If we did add some entries to the menu, we should show it. */
  if (menu_len(pm->menu)) {
    menu_show(pm->menu, TRUE);
    menu_refresh_pos(pm->menu);
  }
  else {
    menu_show(pm->menu, FALSE);
  }
}

/* ----------------------------- Promptmenu open file search ----------------------------- */

static void promptmenu_open_file_search(void) {
  ASSERT_PM;
  directory_t dir;
  char *last_slash;
  char *dirpath;
  cvec_clear(pm->completions);
  if (*answer) {
    directory_data_init(&dir);
    /* If the current answer is a dir itself. */
    if (dir_exists(answer)) {
      directory_get(answer, &dir);
    }
    else {
      last_slash = STRRCHR(answer, '/');
      if (last_slash && last_slash != answer) {
        dirpath = measured_copy(answer, (last_slash - answer));
        directory_get(dirpath, &dir);
        free(dirpath);
      }
    }
    if (dir.len) {
      DIRECTORY_ITER(dir, i, entry,
        cvec_push(pm->completions, copy_of(entry->path));
      );
    }
    directory_data_free(&dir);
  }
}

/* ----------------------------- Promptmenu get x width ----------------------------- */

static inline void promptmenu_get_x_width(float *const x, float *const width) {
  ASSERT(x);
  ASSERT(width);
  float winwidth = gl_window_width();
  *width = FMINF(winwidth, PROMPTMENU_DEFAULT_WIDTH);
  *x = ((winwidth / 2) - (*width / 2));
}

/* ----------------------------- Promptmenu extra routine rect ----------------------------- */

static void promptmenu_extra_routine_rect(Element *const e, void *arg) {
  ASSERT(e);
  ASSERT(arg);
  PromptMenu *p = arg;
  p->xflags |= PROMPTMENU_REFRESH_TEXT;
  menu_refresh_text(p->menu);
  menu_refresh_pos(p->menu);
}

/* ----------------------------- Promptmenu check completion ----------------------------- */

/* Check if there is a active completion, and if there is, then run the menu accept function. */
static inline void promptmenu_should_accept_completion(void) {
  ASSERT_PM;
  if (menu_get_active() && menu_len(pm->menu)) {
    ALWAYS_ASSERT(menu_get_active() == pm->menu);
    menu_action_accept(pm->menu);
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* ----------------------------- Promptmenu create ----------------------------- */

void promptmenu_create(void) {
  float x;
  float width;
  promptmenu_get_x_width(&x, &width);
  pm = xmalloc(sizeof *pm);
  pm->xflags = PROMPTMENU_DEFAULT_XFLAGS;
  pm->buf = vertex_buffer_new(FONT_VERTBUF);
  pm->element = element_create(x, 0, width, font_height(uifont), TRUE);
  gl_window_add_root_child(pm->element);
  pm->element->color   = PACKED_UINT_VS_CODE_RED;
  /* Ensure that when the root element is resized, the element keeps in the center. */
  pm->element->xflags |= (ELEMENT_HIDDEN | ELEMENT_CENTER_X | ELEMENT_CONSTRAIN_WIDTH);
  /* And also ensure that the menu and text of the prompt-menu is fully updated. */
  element_set_data_callback(pm->element, pm);
  element_set_extra_routine_rect(pm->element, promptmenu_extra_routine_rect);
  pm->completions = cvec_create_setfree(free);
  pm->menu = menu_create(pm->element, uifont, pm, promptmenu_pos, promptmenu_accept);
  menu_set_lable_offset(pm->menu, font_breadth(uifont, " "));
  pm->mode = PROMPTMENU_TYPE_NONE;
  pm->data = NULL;
  if (!prompt) {
    prompt = COPY_OF("");
  }
  if (!answer) {
    answer = COPY_OF("");
  }
}

/* ----------------------------- Promptmenu free ----------------------------- */

void promptmenu_free(void) {
  ASSERT_PM;
  vertex_buffer_delete(pm->buf);
  cvec_free(pm->completions);
  menu_free(pm->menu);
  free(pm);
}

/* ----------------------------- Promptmenu draw ----------------------------- */

void promptmenu_draw(void) {
  ASSERT_PM;
  if (pm->xflags & PROMPTMENU_ACTIVE) {
    menu_set_static_width(pm->menu, pm->element->width);
    /* Resize/Reposition */
    element_draw(pm->element);
    if (pm->xflags & PROMPTMENU_REFRESH_TEXT) {
      vertex_buffer_clear(pm->buf);
      promptmenu_add_prompt_text();
      promptmenu_add_cursor();
      pm->xflags &= ~PROMPTMENU_REFRESH_TEXT;
    }
    render_vertbuf(uifont, pm->buf);
    menu_draw(pm->menu);
  }
}

/* ----------------------------- Promptmenu open ----------------------------- */

void promptmenu_open(void) {
  ASSERT_PM;
  statusbar_discard_all_undo_redo();
  pm->xflags |= (PROMPTMENU_ACTIVE | PROMPTMENU_REFRESH_POS | PROMPTMENU_REFRESH_TEXT);
  pm->element->xflags &= ~ELEMENT_HIDDEN;
  menu_show(pm->menu, TRUE);
  refresh_needed = TRUE;
}

/* ----------------------------- Promptmenu close ----------------------------- */

void promptmenu_close(void) {
  ASSERT_PM;
  statusbar_discard_all_undo_redo();
  pm->xflags &= ~PROMPTMENU_ACTIVE;
  pm->element->xflags |= ELEMENT_HIDDEN;
  menu_clear_entries(pm->menu);
  cvec_clear(pm->completions);
  pm->data = NULL;
  refresh_needed = TRUE;
}

/* ----------------------------- Promptmenu active ----------------------------- */

bool promptmenu_active(void) {
  ASSERT_PM;
  return !!(pm->xflags & PROMPTMENU_ACTIVE);
}

/* ----------------------------- Promptmenu yn mode ----------------------------- */

/* Returns `TRUE` when the current mode only wants a `Y/y` or `N/n`. */
bool promptmenu_yn_mode(void) {
  ASSERT_PM;
  return !!(pm->xflags & PROMPTMENU_YN_MODE);
}

/* ----------------------------- Promptmenu refresh text ----------------------------- */

void promptmenu_refresh_text(void) {
  ASSERT_PM;
  pm->xflags |= PROMPTMENU_REFRESH_TEXT;
  refresh_needed = TRUE;
}

/* ----------------------------- Promptmenu completions search ----------------------------- */

void promptmenu_completions_search(void) {
  ASSERT_PM;
  switch (pm->mode) {
    case PROMPTMENU_TYPE_FILE_OPEN: {
      promptmenu_open_file_search();
      break;
    }
    default: {
      break;
    }
  }
  promptmenu_find_completions();
}

/* ----------------------------- Promptmenu action enter ----------------------------- */

/* The routine that is performed when `enter` is pressed. */
void promptmenu_action_enter(void) {
  ASSERT_PM;
  char *pwd;
  char *full_path;
  Ulong pwdlen;
  promptmenu_should_accept_completion();
  switch (pm->mode) {
    case PROMPTMENU_TYPE_FILE_SAVE: {
      if (*answer) {
        full_path = get_full_path(answer);
        if (*GUI_OF->filename && STRCMP(answer, GUI_OF->filename) != 0) {
          /* TODO: Add the [Y/N] prompt type. */
        }
        else {
          statusline(INFO, "Saving file: %s", answer);
          GUI_OF->filename = xstrcpy(GUI_OF->filename, answer);
          etb_entries_refresh_needed(openeditor->tb);
          if (write_it_out(FALSE, FALSE) == 2) {
            log_ERR_FA("Failed to write file %s to disk", full_path);
          }
          promptmenu_close();
        }
        free(full_path);
      }
      break;
    }
    case PROMPTMENU_TYPE_FILE_OPEN: {
      if (*answer) {
        /* The answer is a directory. */
        if (dir_exists(answer)) {
          typing_x = strlen(answer);
          if (answer[typing_x - 1] != '/') {
            answer = xnstrncat(answer, typing_x++, S__LEN("/"));
          }
          promptmenu_open_file_search();
          return;
        }
        else if (blkdev_exists(answer)) {
          statusline(AHEM, "'%s' is a block device", answer);
        }
        else if (!file_exists(answer)) {
          statusline(AHEM, "'%s' does not exist", answer);
        }
        else {
          pwd    = getpwd();
          pwdlen = strlen(pwd);
          if (STRNCMP(answer, pwd, pwdlen) && file_exists(answer + pwdlen + 1)) {
            editor_open_buffer(answer + pwdlen + 1);
          }
          else {
            editor_open_buffer(answer);
          }
          FREE(pwd);
        }
        promptmenu_close();
      }
      break;
    }
    default: {
      if (STRCASECMP(answer, "open file") == 0) {
        promptmenu_ask(PROMPTMENU_TYPE_FILE_OPEN);
      }
      else {
        promptmenu_close();
      }
    }
  }
}

/* ----------------------------- Promptmenu action tab ----------------------------- */

/* The routine that is performed when `tab` is pressed. */
void promptmenu_action_tab(void) {
  ASSERT_PM;
  promptmenu_should_accept_completion();
  switch (pm->mode) {
    case PROMPTMENU_TYPE_FILE_OPEN: {
      if (dir_exists(answer) && typing_x && answer[typing_x - 1] != '/') {
        answer = xnstrncat(answer, typing_x++, S__LEN("/"));
        pm->xflags |= PROMPTMENU_REFRESH_TEXT;
        refresh_needed = TRUE;
      }
      break;
    }
    default: {
      break;
    }
  }
  promptmenu_completions_search();
}

/* ----------------------------- Promptmenu action yes ----------------------------- */

/* This will be called when prompting for a single `y/n` char and the user presses `y`. */
void promptmenu_routine_yes(void) {
  ASSERT_PM;
  switch (pm->mode) {
    case PROMPTMENU_TYPE_FILE_SAVE_MODIFIED: {
      /* TODO: Fix this.  Extend/Modify do_savefile_for() to save any file in the given context. */
      break;
    }
    default: {
      break;
    }
  }
  pm->xflags &= ~PROMPTMENU_YN_MODE;
  promptmenu_close();
}

/* ----------------------------- Promptmenu action no ----------------------------- */

/* This will be called when prompting for a single `y/n` char and the user presses `n`. */
void promptmenu_routine_no(void) {
  ASSERT_PM;
  switch (pm->mode) {
    case PROMPTMENU_TYPE_FILE_SAVE_MODIFIED: {
      ALWAYS_ASSERT(pm->data);
      editor_close_a_open_buffer((openfilestruct *)pm->data);
      break;
    }
    default: {
      break;
    }
  }
  pm->xflags &= ~PROMPTMENU_YN_MODE;
  promptmenu_close();
}

/* ----------------------------- Promptmenu ask ----------------------------- */

void promptmenu_ask(PromptMenuType type) {
  ASSERT_PM;
  promptmenu_open();
  pm->mode = type;
  *answer = NUL;
  typing_x = 0;
  switch (type) {
    case PROMPTMENU_TYPE_NONE: {
      prompt = xstrncpy(prompt, S__LEN("> "));
      break;
    }
    case PROMPTMENU_TYPE_FILE_SAVE: {
      prompt = xstrncpy(prompt, S__LEN("Save file: "));
      if (*GUI_OF->filename) {
        answer   = xstrcpy(answer, GUI_OF->filename);
        typing_x = STRLEN(answer);
      }
      break;
    }
    case PROMPTMENU_TYPE_FILE_OPEN: {
      prompt = xstrncpy(prompt, S__LEN("File to open: "));
      answer = free_and_assign(answer, getpwd());
      typing_x = STRLEN(answer);
      if (typing_x && answer[typing_x - 1] != '/') {
        answer = xnstrncat(answer, typing_x++, S__LEN("/"));
      }
      promptmenu_completions_search();
      break;
    }
    case PROMPTMENU_TYPE_FILE_SAVE_MODIFIED: {
      prompt   = xstrncpy(prompt, S__LEN("Save modified buffer? "));
      pm->data = GUI_OF;
      pm->xflags |= PROMPTMENU_YN_MODE;
      break;
    }
  }
}
