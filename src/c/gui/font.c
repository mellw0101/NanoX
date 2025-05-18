/** @file gui/font.c

  @author  Melwin Svensson.
  @date    14-5-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define ASSERT_GUI_FONT  \
  ASSERT(f);             \
  ASSERT(f->path);       \
  ASSERT(f->atlas);      \
  ASSERT(f->font)

#define FONT_WIDTH(font)  (font->face->max_advance_width >> 6)
#define FONT_HEIGHT(font)  ((font->ascender - font->descender) + font->linegap)

#define FALLBACK_FONT_PATH  "/etc/nanox/fonts/unifont.ttf"

/* Use this to fully avoid devision by zero in a easy way. */
#define GF_HALF_LH  (!f->line_height ? 0 : ((float)f->line_height / 2))

#define GF_HEIGHT   (FONT_HEIGHT(f->font) + f->line_height)

#define GF_ROW_BASELINE(row)  (((row) * GF_HEIGHT) + f->font->ascender + GF_HALF_LH)

#define GF_ROW_TOP(r)  (GF_ROW_BASELINE(r) - f->font->ascender  - GF_HALF_LH)
#define GF_ROW_BOT(r)  (GF_ROW_BASELINE(r) - f->font->descender + GF_HALF_LH)


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


struct Font {
  Uint             size;        /* The size of the font. */
  Uint             atlas_size;  /* This is how big the base of the atlas is, so when this is the `512` the atlas size is `512x512`. */
  char            *path;        /* The path of the loaded font, this is useful when changing size. */
  texture_atlas_t *atlas;       /* The texture atlas ptr. */
  texture_font_t  *font;        /* The freetype font type. */

  /* This represents a deviation from the base separation between rows.  Note that this can be negative or positive. */
  long line_height;
};


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


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

/* Free the internal font path of `f` if it exists and replace it with a copy of `path`. */
static inline void gui_font_set_path(Font *const f, const char *const restrict path) {
  ASSERT(f);
  /* The path passed to this function should always be a valid path to a file. */
  ALWAYS_ASSERT(file_exists(path));
  f->path = free_and_assign(f->path, copy_of(path));
}

/* Free the internal `texture_atlas_t` of `f` if it exists and create a new one using the internal `atlas_size`. */
static inline void gui_font_load_atlas(Font *const f) {
  ASSERT(f);
  texture_atlas_free(f->atlas);
  f->atlas = texture_atlas_new(f->atlas_size, f->atlas_size, 1);
}

/* Free the internal `texture_font_t` of `f` if it exists and create a new one using the internal `path`, `size` and `atlas`. */
static inline void gui_font_load_font(Font *const f) {
  ASSERT(f);
  ASSERT(f->path);
  ASSERT(f->atlas);
  texture_font_free(f->font);
  f->font = texture_font_new_from_file(f->atlas, f->size, f->path);
}

static inline void font_load_fallback(Font *const f) {
  ASSERT_GUI_FONT;
  ALWAYS_ASSERT_MSG(
    file_exists(FALLBACK_FONT_PATH),
    "\nPlease re-compile NanoX using the install script.\n "
    "  The fallback gui font does not exist in the correct location `/etc/nanox/fonts/unifont.ttf`.\n"
    "  If the error persists after re-compiling you can manualy copy the font from\n"
    "  \"[NanoX-Source-Directory]/fonts/unifont.ttf\" to \"/etc/nanox/fonts/unifont.ttf\"."
  );
  gui_font_set_path(f, FALLBACK_FONT_PATH);
  gui_font_load_atlas(f);
  gui_font_load_font(f);
  ALWAYS_ASSERT_MSG(f->font, "Failed to load the fallback font");
}

/* Free's both the internal atlas and font and recreates them based on the internal `path`, `size`, `atlas_size`. */
static inline void gui_font_reload(Font *const f) {
  ASSERT(f);
  ASSERT(f->path);
  gui_font_load_atlas(f);
  gui_font_load_font(f);
  if (!f->font) {
    font_load_fallback(f);
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */

/* Create a blank allocated `Font` structure. */
Font *gui_font_create(void) {
  Font *f;
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

/* Free a allocated `Font` structure as well as loaded font if there was any. */
void gui_font_free(Font *const f) {
  /* Make this function a `NO-OP` when `f` is `NULL`. */
  if (!f) {
    return;
  }
  free(f->path);
  texture_atlas_free(f->atlas);
  texture_font_free(f->font);
  free(f);
}

/* Load `path` in `f` at `size` and using an atlas of size `atlas_size`. */
void gui_font_load(Font *const f, const char *const restrict path, Uint size, Uint atlas_size) {
  ASSERT(f);
  // char *fallback_path;
  f->size       = size;
  f->atlas_size = atlas_size;
  /* If the given path does not exist, set the fallback font path as the path. */
  if (!file_exists(path)) {
    // get_homedir();
    // fallback_path = fmtstr("%s/%s", homedir, FALLBACK_FONT_PATH);
    // gui_font_set_path(f, fallback_path);
    // ALWAYS_ASSERT_MSG(
    //   file_exists(FALLBACK_FONT_PATH),
    //   "\nPlease re-compile NanoX using the install script.\n "
    //   "  The fallback gui font does not exist in the correct location `/etc/nanox/fonts/unifont.ttf`.\n"
    //   "  If the error persists after re-compiling you can manualy copy the font from\n"
    //   "  \"[NanoX-Source-Directory]/fonts/unifont.ttf\" to \"/etc/nanox/fonts/unifont.ttf\"."
    // );
    // gui_font_set_path(f, FALLBACK_FONT_PATH);
    font_load_fallback(f);
  }
  /* Otherwise, just use the given path. */
  else {
    gui_font_set_path(f, path);
    gui_font_reload(f);
  }
}

/* Return's the internal `texture_font_t *` of `f`. */
texture_font_t *gui_font_get_font(Font *const f) {
  ASSERT_GUI_FONT;
  return f->font;
}

/* Return's the internal `texture_atlas_t *` of `f`. */
texture_atlas_t *gui_font_get_atlas(Font *const f) {
  ASSERT_GUI_FONT;
  return f->atlas;
}

/* Return's the glyph assisiated with `codepoint`. */
texture_glyph_t *gui_font_get_glyph(Font *const f, const char *const restrict codepoint) {
  ASSERT_GUI_FONT;
  texture_glyph_t *glyph = texture_font_get_glyph(f->font, codepoint);
  ALWAYS_ASSERT(glyph);
  return glyph;
}

/* Get the current size of `f`. */
Uint gui_font_get_size(Font *const f) {
  ASSERT_GUI_FONT;
  return f->size;
}

/* Return's the current `atlas_size` of `f`. */
Uint gui_font_get_atlas_size(Font *const f) {
  ASSERT_GUI_FONT;
  return f->atlas_size;
}

/* Get the current `line height modifier` of `f`. */
long gui_font_get_line_height(Font *const f) {
  ASSERT_GUI_FONT;
  return f->line_height;
}

/* Return's `TRUE` if the currently loaded font in `f` is a `mono` font. */
bool gui_font_is_mono(Font *const f) {
  ASSERT_GUI_FONT;
  return texture_font_is_mono(f->font);
}

/* Return's the full height of a line. */
float gui_font_height(Font *const f) {
  ASSERT_GUI_FONT;
  return GF_HEIGHT;
}

float gui_font_row_baseline(Font *const f, long row) {
  ASSERT_GUI_FONT;
  return GF_ROW_BASELINE(row);
}

void gui_font_row_top_bot(Font *const f, long row, float *const top, float *const bot) {
  ASSERT_GUI_FONT;
  /* Ensure at least one of the passed ptr's are valid. */
  ALWAYS_ASSERT(top || bot);
  ASSIGN_IF_VALID(top, GF_ROW_TOP(row));
  ASSIGN_IF_VALID(bot, GF_ROW_BOT(row));
}

void gui_font_change_size(Font *const f, Uint new_size) {
  ASSERT_GUI_FONT;
  if (f->size == new_size) {
    return;
  }
  f->size = new_size;
  gui_font_reload(f);
}

void gui_font_increase_size(Font *const f) {
  ASSERT_GUI_FONT;
  ++f->size;
  gui_font_reload(f);
}

void gui_font_decrease_size(Font *const f) {
  ASSERT_GUI_FONT;
  --f->size;
  gui_font_reload(f);
}

void gui_font_decrease_line_height(Font *const f) {
  ASSERT_GUI_FONT;
  --f->line_height;
}

void gui_font_increase_line_height(Font *const f) {
  ASSERT_GUI_FONT;
  ++f->line_height;
}

void gui_font_rows_cols(Font *const f, float width, float height, int *const outrows, int *const outcols) {
  ASSERT_GUI_FONT;
  /* Ensure at least one passed output ptr is valid, otherwise this should not be called. */
  ALWAYS_ASSERT(outrows || outcols);
  int rows;
  int cols;
  texture_glyph_t *glyph;
  if (outcols) {
    if (texture_font_is_mono(f->font)) {
      glyph = gui_font_get_glyph(f, " ");
      cols = (width / glyph->advance_x);
    }
    else {
      cols = ((width / FONT_WIDTH(f->font)) * 0.9f);
    }
    *outcols = cols;
  }
  if (outrows) {
    rows = (height / gui_font_height(f));
    *outrows = rows;
  }
}

/* Return's `FALSE` when outside `y_top` and `y_bot`, otherwise return's `TRUE`, it always assigns the correct row
 * even when outside that be it the first (0) or the last ('rows' - 1).  Note that `outrow` must always be valid. */
bool gui_font_row_from_pos(Font *const f, float y_top, float y_bot, float y_pos, long *outrow) {
  ASSERT_GUI_FONT;
  ASSERT(outrow);
  ALWAYS_ASSERT(y_top < y_bot);
  /* Calculate the number of `full` lines that fit in the length between `y_top` and `y_bot`. */
  long rows = ((y_bot - y_top) / GF_HEIGHT);
  long low = 0;
  long high = (rows - 1);
  long mid;
  float top;
  float bot;
  /* The position is above the passed range. */
  if (y_pos < y_top) {
    *outrow = low;
    return FALSE;
  }
  /* The position is below the passed range. */
  else if (y_pos > y_bot) {
    *outrow = high;
    return FALSE;
  }
  /* The position is still inside the passed range, but its below all full rows. */
  else if (y_pos > ((rows * GF_HEIGHT) + y_top)) {
    *outrow = high;
    return TRUE;
  }
  /* Otherwise, we go hunting. */
  else {
    while (low <= high) {
      mid = ((low + high) / 2);
      gui_font_row_top_bot(f, mid, &top, &bot);
      if (y_pos < (top + y_top)) {
        high = (mid - 1);
      }
      else if (y_pos > (bot + y_top)) {
        low = (mid + 1);
      }
      else {
        break;
      }
    }
    *outrow = mid;
    return TRUE;
  }
}

Ulong gui_font_index_from_pos(Font *const f, const char *const restrict string, Ulong len, float rawx, float normx) {
  ASSERT_GUI_FONT;
  return strpx_str_index(f->font, string, len, rawx, normx);
}

float font_breadth(Font *const f, const char *const restrict string) {
  ASSERT_GUI_FONT;
  ASSERT(string);
  float ret = 0;
  for (const char *ch=string, *prev=NULL; *ch; ch += char_length(ch)) {
    ret += strpx_glyphlen(ch, prev, f->font);
    prev = ch;
  }
  return ret;
}

float font_wideness(Font *const f, const char *const restrict string, Ulong maxlen) {
  ASSERT_GUI_FONT;
  ASSERT(string);
  float ret = 0;
  for (const char *ch=string, *prev=NULL; *ch && ch<(string + maxlen); ch += char_length(ch)) {
    ret += strpx_glyphlen(ch, prev, f->font);
    prev = ch;
  }
  return ret;
}

/* Add one glyph to 'buffer' to be rendered.  At position pen. */
void font_add_glyph(Font *const f, vertex_buffer_t *const buf, const char *const restrict current, const char *const restrict prev, Color *const color, float *const pen_x, float *const pen_y) {
  ASSERT_GUI_FONT;
  ASSERT(current);
  ASSERT(buf);
  float x0, x1, y0, y1;
  Uint indices[] = { 0, 1, 2, 0, 2, 3 };
  vertex_t vertices[4];
  texture_glyph_t *glyph = gui_font_get_glyph(f, current);
  ALWAYS_ASSERT(glyph);
  if (prev) {
    (*pen_x) += texture_glyph_get_kerning(glyph, prev);
  }
  x0 = (int)((*pen_x) + glyph->offset_x);
  y0 = (int)((*pen_y) - glyph->offset_y);
  x1 = (int)(x0 + glyph->width);
  y1 = (int)(y0 + glyph->height);
  vertices[0] = (vertex_t){ x0,y0,0, glyph->s0,glyph->t0, color->r,color->g,color->b,color->a };
  vertices[1] = (vertex_t){ x0,y1,0, glyph->s0,glyph->t1, color->r,color->g,color->b,color->a };
  vertices[2] = (vertex_t){ x1,y1,0, glyph->s1,glyph->t1, color->r,color->g,color->b,color->a };
  vertices[3] = (vertex_t){ x1,y0,0, glyph->s1,glyph->t0, color->r,color->g,color->b,color->a };
  vertex_buffer_push_back(buf, vertices, 4, indices, 6);
  (*pen_x) += glyph->advance_x;
}

// void vertex_buffer_add_string(vertex_buffer_t *buf, const char *string, Ulong slen, const char *previous, texture_font_t *font, vec4 color, vec2 *penpos) {
//   ASSERT(buf);
//   ASSERT(string);
//   ASSERT(font);
//   ASSERT(penpos);
//   const char *cur;
//   const char *prev = previous;
//   for (Ulong i = 0; i < slen; ++i) {
//     cur = &string[i];
//     add_glyph(cur, prev, buf, font, color, penpos);
//     prev = cur;
//   }
// }

void font_vertbuf_add_mbstr(Font *const f, vertex_buffer_t *buf, const char *string, Ulong len, const char *previous, Color *const color, float *const pen_x, float *const pen_y) {
  ASSERT_GUI_FONT;
  ASSERT(buf);
  ASSERT(string);
  ASSERT(pen_x);
  ASSERT(pen_y);
  const char *cur = string;
  const char *prev = previous;
  while (*cur && cur < (string + len)) {
    font_add_glyph(f, buf, cur, prev, color, pen_x, pen_y);
    prev = cur;
    cur += charlen(cur);
  }
}

/* Upload a atlas texture. */
void font_upload_texture_atlas(Font *const f) {
  ASSERT_GUI_FONT;
  glBindTexture(GL_TEXTURE_2D, f->atlas->id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, f->atlas->width, f->atlas->height, 0, GL_RED, GL_UNSIGNED_BYTE, f->atlas->data);
}
