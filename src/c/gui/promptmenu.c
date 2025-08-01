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
# define PROMPTMENU_ACTIVE          PROMPTMENU_ACTIVE
# define PROMPTMENU_REFRESH_TEXT    PROMPTMENU_REFRESH_TEXT
# define PROMPTMENU_REFRESH_POS     PROMPTMENU_REFRESH_POS
# define PROMPTMENU_DEFAULT_XFLAGS  (0U)
} PromptMenuState;


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


typedef struct {
  Uint xflags;
  vertex_buffer_t *buf;
  Element *element;
  Menu *menu;
  CVec *completions;

  PromptMenuType mode;

  /* Data used for actions. */
  void *data;
} PromptMenu;


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


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
    menu_qsort(pm->menu, menu_entry_qsort_strlen_cb);
    menu_text_refresh_needed(pm->menu);
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
  if (cvec_len(pm->completions)) {
    menu_show(pm->menu, TRUE);
    menu_pos_refresh_needed(pm->menu);
    menu_text_refresh_needed(pm->menu);
  }
  else {
    menu_show(pm->menu, FALSE);
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
  pm->element->xflags |= ELEMENT_HIDDEN;
  pm->completions = cvec_create_setfree(free);
  pm->menu = menu_create(pm->element, uifont, pm, promptmenu_pos, promptmenu_accept);
  menu_set_tab_accept_behavior(pm->menu, TRUE);
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
  refresh_needed = TRUE;
}

/* ----------------------------- Promptmenu active ----------------------------- */

bool promptmenu_active(void) {
  ASSERT_PM;
  return !!(pm->xflags & PROMPTMENU_ACTIVE);
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

/* ----------------------------- Promptmenu enter action ----------------------------- */

void promptmenu_enter_action(void) {
  ASSERT_PM;
  char *pwd;
  Ulong pwdlen;
  if (menu_get_active() && menu_len(pm->menu)) {
    ALWAYS_ASSERT(menu_get_active() == pm->menu);
    menu_accept_action(pm->menu);
  }
  switch (pm->mode) {
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
        promptmenu_open_file();
      }
      else {
        promptmenu_close();
      }
    }
  }
}

/* ----------------------------- Promptmenu tab action ----------------------------- */

void promptmenu_tab_action(void) {
  ASSERT_PM;
  if (menu_get_active() && menu_len(pm->menu)) {
    ALWAYS_ASSERT(menu_get_active() == pm->menu);
    menu_accept_action(pm->menu);
  }
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

/* ----------------------------- Promptmenu ask ----------------------------- */

void promptmenu_ask(const char *const restrict question, PromptMenuType type) {
  ASSERT_PM;
  ASSERT(question);
  promptmenu_open();
  prompt = free_and_assign(prompt, fmtstr("%s: ", question));
  if (answer) {
    *answer = NUL;
  }
  else {
    answer = COPY_OF("");
  }
  typing_x = 0;
  pm->mode = type;
}

/* ----------------------------- Promptmenu open file ----------------------------- */

void promptmenu_open_file(void) {
  ASSERT_PM;
  promptmenu_ask("File to open", PROMPTMENU_TYPE_FILE_OPEN);
  answer   = free_and_assign(answer, getpwd());
  typing_x = strlen(answer);
  if (typing_x && answer[typing_x - 1] != '/') {
    answer = xnstrncat(answer, typing_x++, S__LEN("/"));
  }
  promptmenu_completions_search();
}
