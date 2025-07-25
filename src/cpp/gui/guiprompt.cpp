/** @file guiprompt.cpp

  @author  Melwin Svensson.
  @date    19-1-2025.

 */
#include "../../include/prototypes.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_GUI_PROMPTMENU            \
  ASSERT(gui);                           \
  ASSERT(gui->promptmenu);               \
  ASSERT(gui->promptmenu->buffer);       \
  ASSERT(gui->promptmenu->element);      \
  ASSERT(gui->promptmenu->search_vec);   \
  ASSERT(gui->promptmenu->completions);  \
  ASSERT(gui->promptmenu->sb)

#define FONT_DIR_PATH  "/usr/share/fonts"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* If `gui_prompt_mark` is set, then this holds the `x position` of the mark. */
Ulong gui_prompt_mark_x = 0;
/* Whether or not the prompt has its mark set, to be able to select words or the entire `gui_answer`. */
bool gui_prompt_mark = FALSE;
/* The type of prompt this is. */
int gui_prompt_type = 0;


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


/* Enter the gui prompt mode, by setting the prompt mode flag and showing the topbar. */
void gui_promptmode_enter(void) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->element);
  statusbar_discard_all_undo_redo();
  gui->flag.set<GUI_PROMPT>();
  // gui->promptmenu->element->flag.unset<GUIELEMENT_HIDDEN>();
  // gui->promptmenu->element->hidden     = FALSE;
  gui->promptmenu->element->xflags &= ~ELEMENT_HIDDEN;
  gui->promptmenu->text_refresh_needed = TRUE;
  gui->promptmenu->size_refresh_needed = TRUE;
  refresh_needed = TRUE;
}

/* Leave the gui prompt mode, by unsetting the prompt mode flag and hiding the topbar. */
void gui_promptmode_leave(void) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->element);
  ASSERT(gui->promptmenu->completions);
  statusbar_discard_all_undo_redo();
  gui->flag.unset<GUI_PROMPT>();
  // gui->promptmenu->element->flag.set<GUIELEMENT_HIDDEN>();
  // gui->promptmenu->element->hidden = TRUE;
  gui->promptmenu->element->xflags |= ELEMENT_HIDDEN;
  cvec_clear(gui->promptmenu->completions);
  cvec_clear(gui->promptmenu->search_vec);
  gui->promptmenu->selected = 0;
  gui->promptmenu->viewtop  = 0;
  refresh_needed = TRUE;
}

/* Set up the prompt. */
void gui_ask_user(const char *question, guiprompt_type type) {
  /* Set this flag so that the key and char callbacks both go into prompt-mode. */
  gui_promptmode_enter();
  prompt = free_and_assign(prompt, copy_of(question));
  prompt = xstrncat(prompt, S__LEN(": "));
  /* Reset the gui answer string. */
  if (answer) {
    *answer = '\0';
  }
  else {
    answer = COPY_OF("");
  }
  typing_x = 0;
  /* Set the type so that we know what to do upon a responce. */
  gui_prompt_type = type;
}

/* Return's the index of a mouse press, but only when inside the scope of the `top-bar`, in the case it's not inside the scope, then return `-1`. */
long prompt_index_from_mouse(bool allow_outside) {
  bool no_linenums = !ISSET(LINE_NUMBERS);
  long ret = 0;
  /* When we dont allow a valid return value when outside the confinement of top-bar, just return -1. 
   * This is usefull for when we are tracking a hold after the user has pressed inside the top-bar, then drags outside it. */
  if (!allow_outside && (mouse_gui_get_y() < (double)gui->promptmenu->element->y || mouse_gui_get_y() > (double)(gui->promptmenu->element->y + gui->promptmenu->element->height))) {
    return -1;
  }
  if (no_linenums) {
    SET(LINE_NUMBERS);
  }
  ret = index_from_mouse_x(answer, font_get_font(uifont), (gui->promptmenu->element->x + (font_breadth(uifont, " ") / 2) + font_breadth(uifont, prompt)));
  if (no_linenums) {
    UNSET(LINE_NUMBERS);
  }
  return ret;
}


/* ---------------------------------------------------------- PromptMenu static function's ---------------------------------------------------------- */


/* Perform the searching throue all openfiles and all lines. */
static void gui_promptmenu_find_completions(void) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  TIMER_START(timer);
  /* We use our hash map for fast lookup. */
  HashMap *hash_map;
  char *completion;
  Ulong j;
  char *search_path;
  Ulong answer_len;
  cvec_clear(gui->promptmenu->completions);
  /* If the search vector is empty there is nothing to search for. */
  if (!cvec_len(gui->promptmenu->search_vec)) {
    return;
  }
  hash_map = hashmap_create();
  answer_len = strlen(answer);
  for (int pathno=0, len=cvec_len(gui->promptmenu->search_vec); pathno<len; ++pathno) {
    search_path = (char *)cvec_get(gui->promptmenu->search_vec, pathno);
    /* If the first byte does not match, move on. */
    if (search_path[0] != answer[0]) {
      continue;
    }
    /* When it does match, check the rest of the bytes. */
    for (j=1; j<answer_len; ++j) {
      if (search_path[j] != answer[j]) {
        break;
      }
    }
    /* Continue if all bytes did not match. */
    if (j < answer_len) {
      continue;
    }
    /* Look for duplicates in the already found completions. */
    if (hashmap_get(hash_map, search_path)) {
      continue;
    }
    completion = copy_of(search_path);
    /* Add to the hashmap, using the ptr to the word as it will live longer then this hashmap. */
    hashmap_insert(hash_map, completion, completion);
    cvec_push(gui->promptmenu->completions, completion);
  }
  hashmap_free(hash_map);
  cvec_qsort(gui->promptmenu->completions, qsort_strlen);
  /* Set the view top and the selected to the first suggestion. */
  gui->promptmenu->viewtop  = 0;
  gui->promptmenu->selected = 0;
  TIMER_END(timer, ms);
  TIMER_PRINT(ms);
}

/* Add the cursor the the promptmenu vertex buffer. */
static inline void gui_promptmenu_add_cursor(void) {
  ASSERT(answer);
  ASSERT(prompt);
  ASSERT(gui);
  ASSERT(uifont);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->element);
  ASSERT(gui->promptmenu->buffer);
  line_add_cursor(
    0,
    uifont,
    gui->promptmenu->buffer,
    1,
    (gui->promptmenu->element->x + pixbreadth(font_get_font(uifont), " ") + pixbreadth(font_get_font(uifont), prompt) + string_pixel_offset(answer, " ", typing_x, font_get_font(uifont))),
    gui->promptmenu->element->y
  );
}

/* Add the promptmenu prompt and answer. */
static inline void gui_promptmenu_add_prompt_line_text(void) {
  ASSERT(answer);
  ASSERT(prompt);
  ASSERT(gui);
  ASSERT(uifont);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->element);
  ASSERT(gui->promptmenu->buffer);
  vec2 penpos((gui->promptmenu->element->x + pixbreadth(font_get_font(uifont), " ")), (font_row_baseline(uifont, 0) + gui->promptmenu->element->y));
  vertex_buffer_add_string(gui->promptmenu->buffer, prompt, strlen(prompt), NULL, font_get_font(uifont), vec4(1), &penpos);
  vertex_buffer_add_string(gui->promptmenu->buffer, answer, strlen(answer), " ", font_get_font(uifont), vec4(1), &penpos);
}

/* If there are any completions then draw them to the promptmenu.  TODO: Make a function that decides the text color of the completions based on the current prompt type. */
static inline void gui_promptmenu_add_completions_text(void) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->completions);
  static int was_viewtop = gui->promptmenu->viewtop;
  int row = 0;
  char *text;
  vec2 textpen;
  /* Only perfomn any action if there are any current completions available. */
  if (cvec_len(gui->promptmenu->completions)) {
    /* Only add the text to the vertex buffer if asked or if the current viewtop has changed. */
    if (gui->promptmenu->text_refresh_needed || was_viewtop != gui->promptmenu->viewtop) {
      while (row < gui->promptmenu->rows) {
        text = (char *)cvec_get(gui->promptmenu->completions, (gui->promptmenu->viewtop + row));
        textpen = vec2(
          (gui->promptmenu->element->x + pixbreadth(font_get_font(uifont), " ")),
          (font_row_baseline(uifont, (row + 1)) + gui->promptmenu->element->y)
          // (row_baseline_pixel((row + 1), uifont) + gui->promptmenu->element->pos.y)
        );
        vertex_buffer_add_string(gui->promptmenu->buffer, text, strlen(text), NULL, font_get_font(uifont), 1, &textpen);
        ++row;
      }
      was_viewtop = gui->promptmenu->viewtop;
    }
  }
}

/* ----------------------------- Open file ----------------------------- */

/* Routine to put all entries in the current dir into the search vector of the prompt menu. */
static void gui_promptmenu_open_file_search(void) {
  ASSERT(gui);
  directory_t dir;
  char *last_slash;
  char *dirpath;
  cvec_clear(gui->promptmenu->search_vec);
  if (*answer) {
    directory_data_init(&dir);
    /* If the current answer is a dir itself. */
    if (dir_exists(answer)) {
      directory_get(answer, &dir);
    }
    else {
      last_slash = strrchr(answer, '/');
      if (last_slash && last_slash != answer) {
        dirpath = measured_copy(answer, (last_slash - answer));
        directory_get(dirpath, &dir);
        free(dirpath);
      }
    }
    if (dir.len) {
      DIRECTORY_ITER(dir, i, entry,
        cvec_push(gui->promptmenu->search_vec, (void *)copy_of(entry->path));
      );
      gui_promptmenu_find_completions();
      if (cvec_len(gui->promptmenu->completions)) {
        for (int i=0, len=cvec_len(gui->promptmenu->completions); i<len; ++i) {
          writef("%s\n", (char *)cvec_get(gui->promptmenu->completions, i));
        }
      }
    }
    directory_data_free(&dir);
  }
  scrollbar_refresh_needed(gui->promptmenu->sb);
  gui->promptmenu->size_refresh_needed = TRUE;
}

/* Routine for when enter is pressed in open file mode for the gui promptmode. */
static void gui_promptmenu_open_file_enter_action(void) {
  char *pwd;
  Ulong pwdlen;
  Ulong len;
  if (*answer) {
    /* Path is a directory. */
    if (dir_exists(answer)) {
      len = strlen(answer);
      if (answer[len - 1] != '/') {
        answer = xstrncat(answer, S__LEN("/"));
      }
      typing_x = strlen(answer);
      gui_promptmenu_open_file_search();
      return;
    }
    /* Path is a block device. */
    else if (blkdev_exists(answer)) {
      statusline(AHEM, "'%s' is a block device", answer);
    }
    /* If there is no file at the given path. */
    else if (!file_exists(answer)) {
      statusline(AHEM, "'%s' does not exist", answer);
    }
    else {
      pwd = getpwd();
      pwdlen = strlen(pwd);
      if (strncmp(answer, pwd, pwdlen) == 0 && file_exists(answer + pwdlen + 1)) {
        editor_open_buffer(answer + pwdlen + 1);
      }
      else {
        editor_open_buffer(answer);
      }
      free(pwd);
    }
  }
  gui_promptmode_leave();
}

/* ----------------------------- Set font ----------------------------- */

static void gui_promptmenu_set_font_search(void) {
  directory_t dir;
  cvec_clear(*answer ? gui->promptmenu->search_vec : gui->promptmenu->completions);
  /* If the font dir does not exist, just return. */
  if (!dir_exists(FONT_DIR_PATH)) {
    return;
  }
  directory_data_init(&dir);
  ALWAYS_ASSERT(directory_get_recurse(FONT_DIR_PATH, &dir) != -1);
  DIRECTORY_ITER(dir, i, entry,
    if (entry->ext && strcmp(entry->ext, "ttf") == 0) {
      if (*answer) {
        cvec_push(gui->promptmenu->search_vec, copy_of(entry->clean_name));
      }
      else {
        cvec_push(gui->promptmenu->completions, copy_of(entry->clean_name));
      }
    }
  );
  directory_data_free(&dir);
  if (*answer) {
    gui_promptmenu_find_completions();
  }
  else {
    cvec_qsort(gui->promptmenu->completions, qsort_strlen);
  }
}

static void gui_promptmenu_set_font_enter_action(void) {
  Uint size       = font_get_size(textfont);
  Uint atlas_size = font_get_atlas_size(textfont);
  directory_t dir;
  char *path;
  if (!dir_exists(FONT_DIR_PATH) || !*answer) {
    return;
  }
  path = copy_of(answer);
  directory_data_init(&dir);
  ALWAYS_ASSERT(directory_get_recurse(FONT_DIR_PATH, &dir) != -1);
  DIRECTORY_ITER(dir, i, entry,
    if (entry->ext && strcmp(entry->ext, "ttf") == 0 && strcmp(answer, entry->clean_name) == 0) {
      path = free_and_assign(path, copy_of(entry->path));
    }
  );
  directory_data_free(&dir);
  font_load(textfont, path, size, atlas_size);
  free(path);
  CLIST_ITER(starteditor, editor,
    editor_set_rows_cols(editor, editor->text->width, editor->text->height);
  );
  refresh_needed = TRUE;
  gui_promptmode_leave();
}

/* ----------------------------- Scroll bar ----------------------------- */

/* The update routine for the promptmenu scrollbar. */
static void gui_promptmenu_scrollbar_update_routine(void *arg, float *total_length, Uint *start, Uint *total, Uint *visible, Uint *current, float *top_offset, float *right_offset) {
  ASSERT(arg);
  GuiPromptMenu *pm = (__TYPE(pm))arg;
  ASSIGN_IF_VALID(total_length, (pm->element->height - font_height(uifont) /* FONT_HEIGHT(uifont) */));
  ASSIGN_IF_VALID(start, 0);
  ASSIGN_IF_VALID(total, (cvec_len(pm->completions) - pm->rows));
  ASSIGN_IF_VALID(visible, pm->rows);
  ASSIGN_IF_VALID(current, pm->viewtop);
  ASSIGN_IF_VALID(top_offset, font_height(uifont) /* FONT_HEIGHT(uifont) */);
  ASSIGN_IF_VALID(right_offset, 0);
}

/* The moving routine for the promptmenu scrollbar. */
static void gui_promptmenu_scrollbar_moving_routine(void *arg, long index) {
  ASSERT(arg);
  GuiPromptMenu *pm = (__TYPE(pm))arg;
  pm->viewtop = index;
  pm->text_refresh_needed = TRUE;
}

/* Create and init the scrollbar for the prompmenu. */
static void gui_promptmenu_scrollbar_create(void) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  gui->promptmenu->sb = scrollbar_create(gui->promptmenu->element, gui->promptmenu, gui_promptmenu_scrollbar_update_routine, gui_promptmenu_scrollbar_moving_routine);
}


/* ---------------------------------------------------------- Promptmenu global function's ---------------------------------------------------------- */


/* Create the gui `prompt-menu` struct. */
void gui_promptmenu_create(void) {
  ASSERT(gui);
  MALLOC_STRUCT(gui->promptmenu);
  gui->promptmenu->text_refresh_needed = FALSE;
  gui->promptmenu->size_refresh_needed = FALSE;
  gui->promptmenu->buffer  = vertex_buffer_new(FONT_VERTBUF);
  gui->promptmenu->element = element_create(0, 0, gl_window_width(), font_height(uifont), TRUE);
  gl_window_add_root_child(gui->promptmenu->element);
  gui->promptmenu->element->color = PACKED_UINT_VS_CODE_RED;
  gui->promptmenu->element->xflags |= (ELEMENT_HIDDEN | ELEMENT_REL_WIDTH);
  gui->promptmenu->search_vec  = cvec_create_setfree(free);
  gui->promptmenu->completions = cvec_create_setfree(free);
  gui->promptmenu->maxrows  = 8;
  gui->promptmenu->selected = 0;
  gui->promptmenu->viewtop  = 0;
  gui_promptmenu_scrollbar_create();
  gui->promptmenu->closing_file = NULL;
}

/* Delete the gui `prompt-menu` struct. */
void gui_promptmenu_free(void) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->buffer);
  vertex_buffer_delete(gui->promptmenu->buffer);
  cvec_free(gui->promptmenu->search_vec);
  cvec_free(gui->promptmenu->completions);
  free(gui->promptmenu);
}

/* Resize the promptmenu element based on how meny completions there are available. */
void gui_promptmenu_resize(void) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->completions);
  // vec2 size;
  float width;
  float height;
  int len;
  if (gui->promptmenu->size_refresh_needed) {
    len = cvec_len(gui->promptmenu->completions);
    if (len > gui->promptmenu->maxrows) {
      gui->promptmenu->rows = gui->promptmenu->maxrows;
    }
    else {
      gui->promptmenu->rows = len;
    }
    width  = gui->promptmenu->element->width;
    height = ((gui->promptmenu->rows + 1) * font_height(uifont) /* FONT_HEIGHT(uifont) */);
    // gui_element_resize(gui->promptmenu->element, size);
    element_resize(gui->promptmenu->element, width, height);
    gui->promptmenu->size_refresh_needed = FALSE;
  }
}

/* Draw all text related to the promptmenu. */
void gui_promptmenu_draw_text(void) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->completions);
  if (gui->promptmenu->text_refresh_needed) {
    vertex_buffer_clear(gui->promptmenu->buffer);
    gui_promptmenu_add_prompt_line_text();
    gui_promptmenu_add_cursor();
    gui_promptmenu_add_completions_text();
    gui->promptmenu->text_refresh_needed = FALSE;
  }
  upload_texture_atlas(font_get_atlas(uifont));
  render_vertex_buffer(font_shader, gui->promptmenu->buffer);
}

void gui_promptmenu_draw_selected(void) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->completions);
  int selected_row;
  vec2 pos, size;
  /* Only draw the currently selected row when there are completions available. */
  if (cvec_len(gui->promptmenu->completions)) {
    selected_row = (gui->promptmenu->selected - gui->promptmenu->viewtop);
    if (selected_row >= 0 && selected_row < gui->promptmenu->rows) {
      pos.x  = gui->promptmenu->element->x;
      size.w = gui->promptmenu->element->width;
      font_row_top_bot(uifont, (selected_row + 1), &pos.y, &size.h);
      size.h -= pos.y;
      pos.y  += gui->promptmenu->element->y;
    }
  }
}

_UNUSED static bool gui_promptmenu_selected_is_above_screen(void) {
  return (gui->promptmenu->selected < gui->promptmenu->viewtop);
}

_UNUSED static bool gui_promptmenu_selected_is_below_screen(void) {
  return (gui->promptmenu->selected > (gui->promptmenu->viewtop + gui->promptmenu->rows));
}

/* Move the currently selected promptmenu entry up once or when at the very top move it to the bottom.  This also ensures that viewtop moves as well when moving from the top of it. */
void gui_promptmenu_selected_up(void) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->completions);
  int len = cvec_len(gui->promptmenu->completions);
  if (len) {
    /* If we are at the first entry. */
    if (gui->promptmenu->selected == 0) {
      gui->promptmenu->selected = (len - 1);
      if (len > gui->promptmenu->maxrows) {
        gui->promptmenu->viewtop = (len - gui->promptmenu->maxrows);
      }
    }
    else {
      if (gui->promptmenu->viewtop == gui->promptmenu->selected) {
        --gui->promptmenu->viewtop;
      }
      --gui->promptmenu->selected;
    }
    scrollbar_refresh_needed(gui->promptmenu->sb);
  }
}

void gui_promptmenu_selected_down(void) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->completions);
  int len = cvec_len(gui->promptmenu->completions);
  if (len) {
    /* If we are at the last entry. */
    if (gui->promptmenu->selected == (len - 1)) {
      gui->promptmenu->selected = 0;
      gui->promptmenu->viewtop  = 0;
    }
    else {
      if (gui->promptmenu->selected == (gui->promptmenu->viewtop + gui->promptmenu->maxrows - 1)) {
        ++gui->promptmenu->viewtop;
      }
      ++gui->promptmenu->selected;
    }
    scrollbar_refresh_needed(gui->promptmenu->sb);
  }
}

void gui_promptmenu_enter_action(void) {
  char *full_path;
  /* If there is a completion available, then copy the completion into answer before we proccess it. */
  if (cvec_len(gui->promptmenu->completions)) {
    answer = free_and_assign(answer, copy_of((char *)cvec_get(gui->promptmenu->completions, gui->promptmenu->selected)));
    typing_x = strlen(answer);
    gui->promptmenu->text_refresh_needed = TRUE;
  }
  switch (gui_prompt_type) {
    case GUI_PROMPT_SAVEFILE: {
      if (*answer) {
        /* Get the full path to of the answer. */
        full_path = get_full_path(answer);
        /* If the currently open file has a name, and that name does not match the answer. */
        if (*GUI_OF->filename && strcmp(answer, GUI_OF->filename) != 0) {
          /* When the given path does not exist. */
          if (!full_path) {
            NETLOG("Save file using diffrent name?\n");
          }
          else {
            NETLOG("Overwrite file: '%s'?\n", answer);
          }
        }
        /* Otherwise if the current file has no name, then just save using the answer. */
        else {
          statusline(INFO, "Saving file: %s", answer);
          /* Copy the answer as the new filename. */
          GUI_OF->filename = xstrcpy(GUI_OF->filename, answer);
          etb_entries_refresh_needed(openeditor->tb);
          /* Then save the file. */
          if (write_it_out(FALSE, FALSE) == 2) {
            logE("Failed to save file, this needs fixing and the reason needs to be found out.");
            close_and_go();
          }
          gui_promptmode_leave();
        }
        free(full_path);
      }
      break;
    }
    case GUI_PROMPT_MENU: {
      /* Open a new file. */
      if (strcasecmp(answer, "open file") == 0) {
        gui_promptmenu_open_file();
      }
      else if (strcasecmp(answer, "set font") == 0) {
        gui_promptmenu_set_font();
      }
      else {
        gui_promptmode_leave();
      }
      break;
    }
    case GUI_PROMPT_OPEN_FILE: {
      gui_promptmenu_open_file_enter_action();
      break;
    }
    case GUI_PROMPT_FONT: {
      gui_promptmenu_set_font_enter_action();
      break;
    }
  }
}

void gui_promptmenu_completions_search(void) {
  switch (gui_prompt_type) {
    case GUI_PROMPT_OPEN_FILE: {
      gui_promptmenu_open_file_search();
      break;
    }
    case GUI_PROMPT_FONT: {
      gui_promptmenu_set_font_search();
      break;
    }
  } 
  scrollbar_refresh_needed(gui->promptmenu->sb);
  gui->promptmenu->size_refresh_needed = TRUE;
}

void gui_promptmenu_hover_action(float y_pos) {
  ASSERT(gui);
  ASSERT(uifont);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->element);
  ASSERT(gui->promptmenu->completions);
  long row;
  float top;
  float bot;
  if (cvec_len(gui->promptmenu->completions)) {
    /* Top of the completions menu. */
    top = (gui->promptmenu->element->y + font_height(uifont));
    /* Bottom of the completions menu. */
    bot = (gui->promptmenu->element->y + gui->promptmenu->element->height);
    /* If `y_pos` relates to a valid row in the promptmenu completion menu, then adjust the selected to that row. */
    if (font_row_from_pos(uifont, top, bot, y_pos, &row)) {
      gui->promptmenu->selected = lclamp((gui->promptmenu->viewtop + row), gui->promptmenu->viewtop, (gui->promptmenu->viewtop + gui->promptmenu->rows));
    }
  }
}

void gui_promptmenu_scroll_action(bool direction, float y_pos) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->completions);
  int len = cvec_len(gui->promptmenu->completions);
  /* Only do anything if there are more entries then rows in the promptmenu. */
  if (len > gui->promptmenu->maxrows) {
    /* Only scroll when not already at the top or bottom. */
    if ((direction == BACKWARD && gui->promptmenu->viewtop > 0) || (direction == FORWARD && gui->promptmenu->viewtop < (len - gui->promptmenu->maxrows))) {
      gui->promptmenu->viewtop += (!direction ? -1 : 1);
      gui->promptmenu->text_refresh_needed = TRUE;
      /* Ensure that the currently selected entry gets correctly set based on where the mouse is. */
      gui_promptmenu_hover_action(y_pos);
      scrollbar_refresh_needed(gui->promptmenu->sb);
    }
  }
}

void gui_promptmenu_click_action(float y_pos) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->completions);
  float top;
  float bot;
  /* Only perform any action when there are completions available. */
  if (cvec_len(gui->promptmenu->completions)) {
    top = (gui->promptmenu->element->y + font_height(uifont));
    bot = (gui->promptmenu->element->y + gui->promptmenu->element->height);
    if (y_pos >= top && y_pos <= bot) {
      gui_promptmenu_hover_action(y_pos);
      gui_promptmenu_enter_action();
    }
  }
}

void gui_promptmenu_char_action(char input) {
  ASSERT(gui);
  ASSERT(gui->promptmenu);
  /* Prompt when closing a single file that has not been saved. */
  if (gui_prompt_type == GUI_PROMPT_EXIT_NO_SAVE) {
    ALWAYS_ASSERT(gui->promptmenu->closing_file);
    if (isconeof(input, "Yy")) {
      editor_close_a_open_buffer(gui->promptmenu->closing_file);
      gui->promptmenu->closing_file = NULL;
      gui_promptmode_leave();
    }
    else if (isconeof(input, "Nn")) {
      gui->promptmenu->closing_file = NULL;
      gui_promptmode_leave();
    }
  }
  else if (gui_prompt_type == GUI_PROMPT_EXIT_OTHERS_NO_SAVE) {
    // ALWAYS_ASSERT(gui->promptmenu->closing_file);
    // if (isconeof(input, "Yy")) {
    //   editor_close_a_open_buffer(gui->promptmenu->closing_file->next);
    //   gui_promptmode_leave();
    //   gui_editor_close_other_files(gui->promptmenu->closing_file);
    // }
    // else if (isconeof(input, "Nn")) {
    //   gui->promptmenu->closing_file = NULL;
    //   gui_promptmode_leave();
    // }
  }
  else if (gui_prompt_type == GUI_PROMPT_EXIT_ALL_NO_SAVE) {
    // ALWAYS_ASSERT(gui->promptmenu->closing_file);
    // if (isconeof(input, "Yy")) {
    //   gui_editor_close_a_open_buffer(gui->promptmenu->closing_file->next);
    //   gui_promptmode_leave();
    //   gui_editor_close_all_files(gui->promptmenu->closing_file);
    // }
    // else if (isconeof(input, "Nn")) {
    //   gui->promptmenu->closing_file = NULL;
    //   gui_promptmode_leave();
    // }
  }
  else {
    inject_into_answer(&input, 1);
    gui_promptmenu_completions_search();
  }
  gui->promptmenu->text_refresh_needed = TRUE;
}

/* ----------------------------- Open file ----------------------------- */

void gui_promptmenu_open_file(void) {
  gui_ask_user("File to open", GUI_PROMPT_OPEN_FILE);
  /* Make the answer already consist of the current working directory. */
  answer = free_and_assign(answer, getpwd());
  typing_x = strlen(answer);
  /* If the current working directory does not end with a '/' char, then add it. */
  if (typing_x && answer[typing_x - 1] != '/') {
    answer = xnstrncat(answer, typing_x++, S__LEN("/"));
  }
  gui_promptmenu_open_file_search();
}

/* ----------------------------- Set font ----------------------------- */

void gui_promptmenu_set_font(void) {
  gui_ask_user("Select font", GUI_PROMPT_FONT);
  gui_promptmenu_set_font_search();
}
