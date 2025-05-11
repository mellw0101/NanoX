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
  gui->promptmenu->element->flag.unset<GUIELEMENT_HIDDEN>();
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
  gui->promptmenu->element->flag.set<GUIELEMENT_HIDDEN>();
  cvec_clear(gui->promptmenu->completions);
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
    answer = STRLTR_COPY_OF("");
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
  if (!allow_outside && (mousepos.y < gui->promptmenu->element->pos.y || mousepos.y > (gui->promptmenu->element->pos.y + gui->promptmenu->element->size.h))) {
    return -1;
  }
  if (no_linenums) {
    SET(LINE_NUMBERS);
  }
  ret = index_from_mouse_x(answer, gui_font_get_font(gui->uifont), (gui->promptmenu->element->pos.x + (pixbreadth(gui_font_get_font(gui->uifont), " ") / 2) + pixbreadth(gui_font_get_font(gui->uifont), prompt)));
  if (no_linenums) {
    UNSET(LINE_NUMBERS);
  }
  return ret;
}

_UNUSED static GuiPromptMenuEntry *gui_promptmenu_entry_create(GuiPromptMenuEntry *const prev) {
  GuiPromptMenuEntry *entry;
  MALLOC_STRUCT(entry);
  entry->description = NULL;
  entry->next = NULL;
  entry->prev = prev;
  return entry;
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
  ASSERT(gui->uifont);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->element);
  ASSERT(gui->promptmenu->buffer);
  line_add_cursor(
    0,
    gui->uifont,
    gui->promptmenu->buffer,
    1,
    (gui->promptmenu->element->pos.x + pixbreadth(gui_font_get_font(gui->uifont), " ") + pixbreadth(gui_font_get_font(gui->uifont), prompt) + string_pixel_offset(answer, " ", typing_x, gui_font_get_font(gui->uifont))),
    gui->promptmenu->element->pos.y
  );
}

/* Add the promptmenu prompt and answer. */
static inline void gui_promptmenu_add_prompt_line_text(void) {
  ASSERT(answer);
  ASSERT(prompt);
  ASSERT(gui);
  ASSERT(gui->uifont);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->element);
  ASSERT(gui->promptmenu->buffer);
  vec2 penpos((gui->promptmenu->element->pos.x + pixbreadth(gui_font_get_font(gui->uifont), " ")), (/* row_baseline_pixel(0, gui->uifont) */ gui_font_row_baseline(gui->uifont, 0) + gui->promptmenu->element->pos.y));
  vertex_buffer_add_string(gui->promptmenu->buffer, prompt, strlen(prompt), NULL, gui_font_get_font(gui->uifont), vec4(1), &penpos);
  vertex_buffer_add_string(gui->promptmenu->buffer, answer, strlen(answer), " ", gui_font_get_font(gui->uifont), vec4(1), &penpos);
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
          (gui->promptmenu->element->pos.x + pixbreadth(gui_font_get_font(gui->uifont), " ")),
          (gui_font_row_baseline(gui->uifont, (row + 1)) + gui->promptmenu->element->pos.y)
          // (row_baseline_pixel((row + 1), gui->uifont) + gui->promptmenu->element->pos.y)
        );
        vertex_buffer_add_string(gui->promptmenu->buffer, text, strlen(text), NULL, gui_font_get_font(gui->uifont), 1, &textpen);
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
        writef("\nPrompt menu: open file: result:\n");
        for (int i=0, len=cvec_len(gui->promptmenu->completions); i<len; ++i) {
          writef("%s\n", (char *)cvec_get(gui->promptmenu->completions, i));
        }
      }
    }
    directory_data_free(&dir);
  }
  gui_scrollbar_refresh_needed(gui->promptmenu->sb);
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
      show_statusmsg(AHEM, 2, "'%s' is a block device", answer);
    }
    /* If there is no file at the given path. */
    else if (!file_exists(answer)) {
      show_statusmsg(AHEM, 2, "'%s' does not exist", answer);
    }
    else {
      pwd = getpwd();
      pwdlen = strlen(pwd);
      if (strncmp(answer, pwd, pwdlen) == 0 && file_exists(answer + pwdlen + 1)) {
        gui_editor_open_buffer(answer + pwdlen + 1);
      }
      else {
        gui_editor_open_buffer(answer);
      }
      free(pwd);
    }
  }
  gui_promptmode_leave();
}

/* ----------------------------- Set font ----------------------------- */

static void gui_promptmenu_set_font_search(void) {
# define FONT_DIR_PATH  "/usr/share/fonts"
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

/* ----------------------------- Scroll bar ----------------------------- */

/* The update routine for the promptmenu scrollbar. */
static void gui_promptmenu_scrollbar_update_routine(void *arg, float *total_length, Uint *start, Uint *total, Uint *visible, Uint *current, float *top_offset, float *right_offset) {
  ASSERT(arg);
  GuiPromptMenu *pm = (__TYPE(pm))arg;
  ASSIGN_IF_VALID(total_length, (pm->element->size.h - gui_font_height(gui->uifont) /* FONT_HEIGHT(gui->uifont) */));
  ASSIGN_IF_VALID(start, 0);
  ASSIGN_IF_VALID(total, (cvec_len(pm->completions) - pm->rows));
  ASSIGN_IF_VALID(visible, pm->rows);
  ASSIGN_IF_VALID(current, pm->viewtop);
  ASSIGN_IF_VALID(top_offset, gui_font_height(gui->uifont) /* FONT_HEIGHT(gui->uifont) */);
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
  gui->promptmenu->sb = gui_scrollbar_create(gui->promptmenu->element, gui->promptmenu, gui_promptmenu_scrollbar_update_routine, gui_promptmenu_scrollbar_moving_routine);
}


/* ---------------------------------------------------------- Promptmenu global function's ---------------------------------------------------------- */


/* Create the gui `prompt-menu` struct. */
void gui_promptmenu_create(void) {
  ASSERT(gui);
  MALLOC_STRUCT(gui->promptmenu);
  gui->promptmenu->text_refresh_needed = FALSE;
  gui->promptmenu->size_refresh_needed = FALSE;
  gui->promptmenu->buffer  = make_new_font_buffer();
  gui->promptmenu->element = gui_element_create(gui->root);
  gui_element_move_resize(
    gui->promptmenu->element,
    0.0f,
    vec2(gui->width, gui_font_height(gui->uifont))
  );
  gui->promptmenu->element->color = VEC4_VS_CODE_RED;
  gui->promptmenu->element->flag.set<GUIELEMENT_RELATIVE_WIDTH>();
  gui->promptmenu->element->relative_size = 0;
  gui->promptmenu->element->flag.set<GUIELEMENT_ABOVE>();
  gui->promptmenu->element->flag.set<GUIELEMENT_HIDDEN>();
  gui->promptmenu->search_vec  = cvec_create_setfree(free);
  gui->promptmenu->completions = cvec_create_setfree(free);
  gui->promptmenu->maxrows  = 8;
  gui->promptmenu->selected = 0;
  gui->promptmenu->viewtop  = 0;
  gui_promptmenu_scrollbar_create();
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
  vec2 size;
  int len;
  if (gui->promptmenu->size_refresh_needed) {
    len = cvec_len(gui->promptmenu->completions);
    if (len > gui->promptmenu->maxrows) {
      gui->promptmenu->rows = gui->promptmenu->maxrows;
    }
    else {
      gui->promptmenu->rows = len;
    }
    size.w = gui->promptmenu->element->size.w;
    size.h = ((gui->promptmenu->rows + 1) * gui_font_height(gui->uifont) /* FONT_HEIGHT(gui->uifont) */);
    gui_element_resize(gui->promptmenu->element, size);
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
  // upload_texture_atlas(gui->uiatlas);
  upload_texture_atlas(gui_font_get_atlas(gui->uifont));
  render_vertex_buffer(gui->font_shader, gui->promptmenu->buffer);
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
      pos.x = gui->promptmenu->element->pos.x;
      size.w = gui->promptmenu->element->size.w;
      gui_font_row_top_bot(gui->uifont, (selected_row + 1), &pos.y, &size.h);
      size.h -= pos.y;
      pos.y  += gui->promptmenu->element->pos.y;
      draw_rect(pos, size, vec4(vec3(1.0f), 0.4f));
    }
  }
}

_UNUSED static bool gui_promptmenu_selected_is_above_screen(void) {
  return (gui->promptmenu->selected < gui->promptmenu->viewtop);
}

_UNUSED static bool gui_promptmenu_selected_is_below_screen(void) {
  return (gui->promptmenu->selected < gui->promptmenu->viewtop);
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
    gui_scrollbar_refresh_needed(gui->promptmenu->sb);
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
    gui_scrollbar_refresh_needed(gui->promptmenu->sb);
  }
}

void gui_promptmenu_enter_action(void) {
  char *full_path;
  /* If there is a completion available, then copy the completion into answer before we proccess it. */
  if (cvec_len(gui->promptmenu->completions)) {
    answer = free_and_assign(answer, copy_of((char *)cvec_get(gui->promptmenu->completions, gui->promptmenu->selected)));
    typing_x = strlen(answer);
  }
  switch (gui_prompt_type) {
    case GUI_PROMPT_SAVEFILE: {
      if (*answer) {
        /* Get the full path to of the answer. */
        full_path = get_full_path(answer);
        /* If the currently open file has a name, and that name does not match the answer. */
        if (*openfile->filename && strcmp(answer, openfile->filename) != 0) {
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
          show_statusmsg(INFO, 5, "Saving file: %s", answer);
          /* Free the openfile filename, and assign answer to it. */
          openfile->filename = free_and_assign(openfile->filename, copy_of(answer));
          // gui_editor_from_file(openfile)->flag.set<GUIEDITOR_TOPBAR_REFRESH_NEEDED>();
          gui_editor_topbar_entries_refresh_needed(gui_editor_from_file(openfile)->etb);
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
  gui_scrollbar_refresh_needed(gui->promptmenu->sb);
  gui->promptmenu->size_refresh_needed = TRUE;
}

void gui_promptmenu_hover_action(float y_pos) {
  ASSERT(gui);
  ASSERT(gui->uifont);
  ASSERT(gui->promptmenu);
  ASSERT(gui->promptmenu->element);
  ASSERT(gui->promptmenu->completions);
  long row;
  float top;
  float bot;
  if (cvec_len(gui->promptmenu->completions)) {
    /* Top of the completions menu. */
    top = (gui->promptmenu->element->pos.y + gui_font_height(gui->uifont));
    /* Bottom of the completions menu. */
    bot = (gui->promptmenu->element->pos.y + gui->promptmenu->element->size.h);
    /* If `y_pos` relates to a valid row in the promptmenu completion menu, then adjust the selected to that row. */
    if (gui_font_row_from_pos(gui->uifont, top, bot, y_pos, &row)) {
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
      gui_scrollbar_refresh_needed(gui->promptmenu->sb);
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
    top = (gui->promptmenu->element->pos.y + gui_font_height(gui->uifont));
    bot = (gui->promptmenu->element->pos.y + gui->promptmenu->element->size.h);
    if (y_pos >= top && y_pos <= bot) {
      gui_promptmenu_hover_action(y_pos);
      gui_promptmenu_enter_action();
    }
  }
}

/* ----------------------------- Open file ----------------------------- */

void gui_promptmenu_open_file(void) {
  gui_ask_user("File to open", GUI_PROMPT_OPEN_FILE);
  /* Make the answer already consist of the current working directory. */
  answer = free_and_assign(answer, getpwd());
  typing_x = strlen(answer);
  /* If the current working directory does not end with a '/' char, then add it. */
  if (typing_x && answer[typing_x - 1] != '/') {
    answer = xnstrncat(answer, typing_x, S__LEN("/"));
    ++typing_x;
  }
  gui_promptmenu_open_file_search();
}

/* ----------------------------- Set font ----------------------------- */

void gui_promptmenu_set_font(void) {
  gui_ask_user("Select font", GUI_PROMPT_FONT);
  gui_promptmenu_set_font_search();
}
