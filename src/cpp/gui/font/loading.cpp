/** @file gui/font/loading.cpp

  @author  Melwin Svensson.
  @date    25-3-2025.

 */
#include "../../../include/prototypes.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_GUI_FONT  \
  ASSERT(f);             \
  ASSERT(f->path);       \
  ASSERT(f->atlas);      \
  ASSERT(f->font)

#define FALLBACK_FONT_PATH  ".config/nanox/fonts/unifont.ttf"

/* Use this to fully avoid devision by zero in a easy way. */
#define GF_HALF_LH  (!f->line_height ? 0 : ((float)f->line_height / 2))

#define GF_HEIGHT   (FONT_HEIGHT(f->font) + f->line_height)

#define GF_ROW_BASELINE(row)  (((row) * GF_HEIGHT) + f->font->ascender + GF_HALF_LH)

#define GF_ROW_TOP(r)  (GF_ROW_BASELINE(r) - f->font->ascender  - GF_HALF_LH)
#define GF_ROW_BOT(r)  (GF_ROW_BASELINE(r) - f->font->descender + GF_HALF_LH)


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


struct GuiFont {
  Uint size;
  Uint atlas_size;
  char *path;
  texture_atlas_t *atlas;
  texture_font_t  *font;

  /* This represents a deviation from the base separation between rows.  This can be negative or positive. */
  long line_height;
};


/* ---------------------------------------------------------- Function's ---------------------------------------------------------- */


/* ----------------------------- Static gui font ----------------------------- */

/* A simple wrapper of `texture_font_delete` that makes it a `NO-OP` function, meaning passing `NULL` to it is safe. */
static inline void texture_font_free(texture_font_t *const font) {
  if (!font) {
    return;
  }
  texture_font_delete(font);
}

/* A simple wrapper of `texture_atlas_delete` that makes it a `NO-OP` function, meaning passing `NULL` to it is safe. */
static inline void texture_atlas_free(texture_atlas_t *const atlas) {
  if (!atlas) {
    return;
  }
  glDeleteTextures(1, &atlas->id);
  atlas->id = 0;
  texture_atlas_delete(atlas);
}

/* Free the internal `texture_atlas_t` of `f` if it exists and create a new one using the internal `atlas_size`. */
static inline void gui_font_load_atlas(GuiFont *const f) {
  ASSERT(f);
  texture_atlas_free(f->atlas);
  f->atlas = texture_atlas_new(f->atlas_size, f->atlas_size, 1);
}

/* Free the internal `texture_font_t` of `f` if it exists and create a new one using the internal `path`, `size` and `atlas`. */
static inline void gui_font_load_font(GuiFont *const f) {
  ASSERT(f);
  ASSERT(f->path);
  ASSERT(f->atlas);
  texture_font_free(f->font);
  f->font = texture_font_new_from_file(f->atlas, f->size, f->path);
}

/* Free's both the internal atlas and font and recreates them based on the internal `path`, `size`, `atlas_size`. */
static inline void gui_font_reload(GuiFont *const f) {
  ASSERT(f);
  ASSERT(f->path);
  gui_font_load_atlas(f);
  gui_font_load_font(f);  
}

/* Free the internal font path of `f` if it exists and replace it with a copy of `path`. */
static inline void gui_font_set_path(GuiFont *const f, const char *const restrict path) {
  ASSERT(f);
  /* The path passed to this function should always be a valid path to a file. */
  ALWAYS_ASSERT(file_exists(path));
  f->path = free_and_assign(f->path, copy_of(path));
}

/* ----------------------------- Global gui font ----------------------------- */

GuiFont *gui_font_create(void) {
  GuiFont *f;
  MALLOC_STRUCT(f);
  /* Zero init the base attributes needed to load a font. */
  f->size = 0;
  f->path = NULL;
  /* Zero init the atlas and font ptr's that make up the font. */
  f->atlas = NULL;
  f->font  = NULL;
  /* Zero init the extra config options of the font. */
  f->line_height = 0;
  return f;
}

void gui_font_free(GuiFont *const f) {
  /* Make this function a `NO-OP` when `f` is `NULL`. */
  if (!f) {
    return;
  }
  free(f->path);
  texture_atlas_free(f->atlas);
  texture_font_free(f->font);
}

void gui_font_load(GuiFont *const f, const char *const restrict path, Uint size, Uint atlas_size) {
  ASSERT(f);
  f->size       = size;
  f->atlas_size = atlas_size;
  gui_font_set_path(f, path);
  gui_font_reload(f);
}

void gui_font_change_size(GuiFont *const f, Uint new_size) {
  ASSERT_GUI_FONT;
  if (f->size == new_size) {
    return;
  }
  f->size = new_size;
  gui_font_reload(f);
}

void gui_font_increase_size(GuiFont *const f) {
  ASSERT_GUI_FONT;
  ++f->size;
  gui_font_reload(f);
}

void gui_font_decrease_size(GuiFont *const f) {
  ASSERT_GUI_FONT;
  --f->size;
  gui_font_reload(f);
}

texture_font_t *gui_font_get_font(GuiFont *const f) {
  ASSERT_GUI_FONT;
  return f->font;
}

texture_atlas_t *gui_font_get_atlas(GuiFont *const f) {
  ASSERT_GUI_FONT;
  return f->atlas;
}

bool gui_font_is_mono(GuiFont *const f) {
  ASSERT_GUI_FONT;
  return texture_font_is_mono(f->font);
}

float gui_font_height(GuiFont *const f) {
  ASSERT_GUI_FONT;
  return GF_HEIGHT;
}

float gui_font_row_baseline(GuiFont *const f, long row) {
  ASSERT_GUI_FONT;
  return GF_ROW_BASELINE(row);
}

void gui_font_row_top_bot(GuiFont *const f, long row, float *const top, float *const bot) {
  ASSERT_GUI_FONT;
  /* Ensure at least one of the passed ptr's are valid. */
  ALWAYS_ASSERT(top || bot);
  ASSIGN_IF_VALID(top, GF_ROW_TOP(row));
  ASSIGN_IF_VALID(bot, GF_ROW_BOT(row));
}

/* ----------------------------- General ----------------------------- */

/* Create a new font and atlas from `path`, and assign them to `*outfont` and `*outatlas`. */
static void set_font(const char *const restrict path, Uint size, texture_font_t **const outfont, texture_atlas_t **const outatlas) {
  ASSERT(path);
  ASSERT(size);
  ASSERT(file_exists(path));
  texture_atlas_t *atlas;
  texture_font_t *font;
  /* Generate the new texture atlas. */
  atlas = texture_atlas_new(512, 512, 1);
  glGenTextures(1, &atlas->id);
  /* Load the font file. */
  font = texture_font_new_from_file(atlas, size, path);
  /* Assign the new atlas and font to the passed ones. */
  *outfont = font;
  *outatlas = atlas;
}

/* Free an allocated `texture_atlas_t` strucure properly.  Note that this function is `NOT NULL-SAFE`. */
void free_atlas(texture_atlas_t *atlas) {
  if (!atlas) {
    return;
  }
  glDeleteTextures(1, &atlas->id);
  atlas->id = 0;
  texture_atlas_delete(atlas);
}

/* When `uifont` is `TRUE` free the uifont, otherwise free the textfont. */
void free_gui_font(bool uifont) {
  ASSERT(gui);
  /* Delete the current uifont. */
  if (uifont) {
    texture_atlas_free(gui->uiatlas);
    texture_font_free(gui->uifont);
    gui->uiatlas = NULL;
    gui->uifont  = NULL;
    // if (gui->uiatlas) {
    //   free_atlas(gui->uiatlas);
    //   gui->uiatlas = NULL;
    // }
    // if (gui->uifont) {
    //   texture_font_delete(gui->uifont);
    //   gui->uifont = NULL;
    // }
  }
  /* Otherwise, delete the textfont. */
  else {
    // if (gui->atlas) {
    //   free_atlas(gui->atlas);
    //   gui->atlas = NULL;
    // }
    // if (gui->font) {
    //   texture_font_delete(gui->font);
    //   gui->font = NULL;
    // }
  }
}

/* Set the font the gui uses to the fallback font, this should never fail unless the fallback font file is removed. */
// static void set_fallback_font(Uint size) {
//   ASSERT(size);
//   char *path;
//   /* Get the current homedir. */
//   get_homedir();
//   /* Ensure it assigned a valid string to the ptr. */
//   ALWAYS_ASSERT(homedir);
//   /* Construct the path to the fallback font file. */
//   path = fmtstr("%s/%s", homedir, ".config/nanox/fonts/unifont.ttf");
//   /* Set the font path to the fallback font path. */
//   gui->font_path = free_and_assign(gui->font_path, copy_of(path));
//   /* Free the existing font, if any. */
//   free_gui_font(FALSE);
//   /* Set the font. */
//   set_font(path, size, &gui->font, &gui->atlas);
//   /* Free the constructed path. */
//   free(path);
// }

/* Set the uifont the gui uses to the fallback font, this should never fail unless the fallback font file is removed. */
static void set_fallback_uifont(Uint size) {
  ASSERT(size);
  char *path;
  /* Get the current homedir. */
  get_homedir();
  /* Ensure it assigned a valid string to the ptr. */
  ALWAYS_ASSERT(homedir);
  /* Construct the path to the fallback font file. */
  path = fmtstr("%s/%s", homedir, ".config/nanox/fonts/unifont.ttf");
  /* Free the existing font, if any. */
  free_gui_font(TRUE);
  /* Set the font. */
  set_font(path, size, &gui->uifont, &gui->uiatlas);
  /* Free the constructed path. */
  free(path);
}

/* Set the gui font using `path` with `size`.  If the provided file does not exist, the fallback font will be set. */
// void set_gui_font(const char *const restrict path, Uint size) {
//   ASSERT(gui);
//   ASSERT(path);
//   ASSERT(size);
//   /* Ensure the fontsize parameter in the gui structure aligns with the actual size. */
//   gui->font_size = size;
//   /* When the provided path does not exist, revert to the fallback font. */
//   if (!file_exists(path)) {
//     set_fallback_font(size);
//   }
//   /* Otherwise, set the provided  */
//   else {
//     /* Set the path of the loaded font. */
//     gui->font_path = free_and_assign(gui->font_path, copy_of(path));
//     free_gui_font(FALSE);
//     set_font(path, size, &gui->font, &gui->atlas);
//   }
//   /* If the editor's have been initilazed, then recheck the size of each editor. */
//   if (openeditor) {
//     ITER_OVER_ALL_OPENEDITORS(starteditor, editor,
//       guieditor_resize(editor);
//     );
//   }
//   /* If the completion menu is active.  Ensure that it is updated. */
//   if (gui->suggestmenu && cvec_len(gui->suggestmenu->completions)) {
//     gui->suggestmenu->pos_refresh_needed  = TRUE;
//     gui->suggestmenu->text_refresh_needed = TRUE;
//   }
// }

/* Set the gui uifont using `path` with `size`.  If the provided file does not exist, the fallback font will be set. */
void set_gui_uifont(const char *const restrict path, Uint size) {
  ASSERT(gui);
  ASSERT(path);
  ASSERT(size);
  /* Ensure the uifontsize parameter in the gui structure aligns with the actual size. */
  gui->uifont_size = size;
  /* When the provided path does not exist, revert to the fallback font. */
  if (!file_exists(path)) {
    set_fallback_uifont(size);
  }
  /* Otherwise, set the provided  */
  else {
    free_gui_font(TRUE);
    set_font(path, size, &gui->uifont, &gui->uiatlas);
  }
}

/* Set the font and uifont for the gui using the same font file. */
// void set_all_gui_fonts(const char *const restrict path, Uint size, Uint uisize) {
//   ASSERT(gui);
//   ASSERT(path);
//   ASSERT(size);
//   ASSERT(uisize);
//   set_gui_font(path, size);
//   set_gui_uifont(path, uisize);
// }

/* Change the size of the currently loaded font. */
// void change_gui_font_size(Uint size) {
//   ASSERT(size);
//   char *path;
//   if (!gui->font_path) {
//     return;
//   }
//   /* Make a copy of the current font path, as otherwise there will be issues with free'ing and assigning `gui->font_path` using itself. */
//   path = copy_of(gui->font_path);
//   free_gui_font(FALSE);
//   set_gui_font(path, size);
//   free(path); 
// }

void list_available_fonts(void) {
  static const char *const fontdir = "/usr/share/fonts";
  directory_t dir;
  /* If the font directory does not exist, just return. */
  if (!dir_exists(fontdir)) {
    return;
  }
  directory_data_init(&dir);
  ALWAYS_ASSERT(directory_get_recurse(fontdir, &dir) != -1);
  DIRECTORY_ITER(dir, i, entry,
    if (entry->ext && strcmp(entry->ext, "ttf") == 0) {
      writef("%s\n", entry->clean_name);
    }
  );
  directory_data_free(&dir);
}
