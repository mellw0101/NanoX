/** @file loading.c

  @author  Melwin Svensson.
  @date    25-3-2025.

 */
#include "../../../include/prototypes.h"


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
static void free_atlas(texture_atlas_t *atlas) {
  ASSERT(atlas);
  glDeleteTextures(1, &atlas->id);
  atlas->id = 0;
  texture_atlas_delete(atlas);
}

/* Free the current uifont.  Note that this funtion is `NULL-SAFE`. */
static void free_current_uifont(void) {
  ASSERT(gui);
  if (gui->uiatlas) {
    free_atlas(gui->uiatlas);
    gui->uiatlas = NULL;
  }
  if (gui->uifont) {
    texture_font_delete(gui->uifont);
    gui->uifont = NULL;
  }
}

/* Free the current font.  Note that this function is `NULL-SAFE`. */
static void free_current_font(void) {
  ASSERT(gui);
  if (gui->atlas) {
    free_atlas(gui->atlas);
    gui->atlas = NULL;
  }
  if (gui->font) {
    texture_font_delete(gui->font);
    gui->font = NULL;
  }
}

/* Set the font the gui uses to the fallback font, this should never fail unless the fallback font file is removed. */
static void set_fallback_font(Uint size) {
  ASSERT(size);
  char *path;
  /* Get the current homedir. */
  get_homedir();
  /* Ensure it assigned a valid string to the ptr. */
  ALWAYS_ASSERT(homedir);
  /* Construct the path to the fallback font file. */
  path = fmtstr("%s/%s", homedir, ".config/nanox/fonts/unifont.ttf");
  /* Free the existing font, if any. */
  free_current_font();
  /* Set the font. */
  set_font(path, size, &gui->font, &gui->atlas);
  /* Free the constructed path. */
  free(path);
}

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
  free_current_uifont();
  /* Set the font. */
  set_font(path, size, &gui->uifont, &gui->uiatlas);
  /* Free the constructed path. */
  free(path);
}

/* Set the gui font using `path` with `size`.  If the provided file does not exist, the fallback font will be set. */
void set_gui_font(const char *const restrict path, Uint size) {
  ASSERT(gui);
  ASSERT(path);
  ASSERT(size);
  /* Ensure the fontsize parameter in the gui structure aligns with the actual size. */
  gui->font_size = size;
  /* When the provided path does not exist, revert to the fallback font. */
  if (!file_exists(path)) {
    set_fallback_font(size);
  }
  /* Otherwise, set the provided  */
  else {
    free_current_font();
    set_font(path, size, &gui->font, &gui->atlas);
  }
}

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
    free_current_uifont();
    set_font(path, size, &gui->uifont, &gui->uiatlas);
  }
}

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
