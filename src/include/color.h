#include <Mlib/constexpr.hpp>

#define round_short(x)        ((x) >= 0 ? (short)((x) + 0.5) : (short)((x) - 0.5))

/* Return`s a rounded xterm-256 scale value from a 8-bit rgb value.  */
#define xterm_byte_scale(bit) round_short(((double)bit / 255) * 5)
/* Return the xterm-256 index for a given 8bit rgb value. */
#define xterm_color_index(r, g, b) \
  short(16 + (36 * xterm_byte_scale(r)) + (6 * xterm_byte_scale(g)) + xterm_byte_scale(b))

#define xterm_std_color_index(r, g, b) xterm_color_index(((double)r / 6), ((double)g / 6), ((double)b / 6)) - 16

#define grayscale_xterm_byte(r, g, b)  round_short(((0.299 * (r) + 0.587 * (g) + 0.114 * (b)) / 255.0) * 5)

#define grayscale_xterm_color_index(r, g, b) \
  (16 + 36 * grayscale_xterm_byte(r, g, b) + 6 * grayscale_xterm_byte(r, g, b) + grayscale_xterm_byte(r, g, b))

#define xterm_grayscale_color_index(r, g, b) grayscale_xterm_color_index(r, g, b)

/* Some color indexes. */
#define COLOR_LAGOON                         38
#define COLOR_PINK                           204
#define COLOR_TEAL                           35
#define COLOR_MINT                           48
#define COLOR_PURPLE                         163
#define COLOR_MAUVE                          134
/* Some base constexpr xterm index colors. */
static constexpr short XTERM_GREY_1 = xterm_grayscale_color_index(80,  80,  80);
static constexpr short XTERM_GREY_2 = xterm_grayscale_color_index(130, 130, 130);
static constexpr short XTERM_GREY_3 = xterm_grayscale_color_index(180, 180, 180);
/* Vs-code colors. */
static constexpr short VS_CODE_RED            = xterm_color_index(205, 49, 49);
static constexpr short VS_CODE_GREEN          = xterm_color_index(13, 188, 121);
static constexpr short VS_CODE_YELLOW         = xterm_color_index(229, 229, 16);
static constexpr short VS_CODE_BLUE           = xterm_color_index(36, 114, 200);
static constexpr short VS_CODE_MAGENTA        = xterm_color_index(188, 63, 188);
static constexpr short VS_CODE_CYAN           = xterm_color_index(17, 168, 205);
static constexpr short VS_CODE_WHITE          = xterm_color_index(229, 229, 229);
static constexpr short VS_CODE_BRIGHT_RED     = xterm_color_index(241, 76, 76);
static constexpr short VS_CODE_BRIGHT_GREEN   = xterm_color_index(35, 209, 139);
static constexpr short VS_CODE_BRIGHT_YELLOW  = xterm_color_index(245, 245, 67);
static constexpr short VS_CODE_BRIGHT_BLUE    = xterm_color_index(59, 142, 234);
static constexpr short VS_CODE_BRIGHT_MAGENTA = xterm_color_index(214, 112, 214);
static constexpr short VS_CODE_BRIGHT_CYAN    = xterm_color_index(41, 184, 219);
static constexpr short COMMENT_GREEN          = xterm_color_index(0, 77, 0);
/* Grayscale vs-code colors. */
static constexpr short VS_CODE_RED_GRAYSCALE            = xterm_grayscale_color_index(205, 49, 49);
static constexpr short VS_CODE_GREEN_GRAYSCALE          = xterm_grayscale_color_index(13, 188, 121);
static constexpr short VS_CODE_YELLOW_GRAYSCALE         = xterm_grayscale_color_index(229, 229, 16);
static constexpr short VS_CODE_BLUE_GRAYSCALE           = xterm_grayscale_color_index(36, 114, 200);
static constexpr short VS_CODE_MAGENTA_GRAYSCALE        = xterm_grayscale_color_index(188, 63, 188);
static constexpr short VS_CODE_CYAN_GRAYSCALE           = xterm_grayscale_color_index(17, 168, 205);
static constexpr short VS_CODE_WHITE_GRAYSCALE          = xterm_grayscale_color_index(229, 229, 229);
static constexpr short VS_CODE_BRIGHT_RED_GRAYSCALE     = xterm_grayscale_color_index(241, 76, 76);
static constexpr short VS_CODE_BRIGHT_GREEN_GRAYSCALE   = xterm_grayscale_color_index(35, 209, 139);
static constexpr short VS_CODE_BRIGHT_YELLOW_GRAYSCALE  = xterm_grayscale_color_index(245, 245, 67);
static constexpr short VS_CODE_BRIGHT_BLUE_GRAYSCALE    = xterm_grayscale_color_index(59, 142, 234);
static constexpr short VS_CODE_BRIGHT_MAGENTA_GRAYSCALE = xterm_grayscale_color_index(214, 112, 214);
static constexpr short VS_CODE_BRIGHT_CYAN_GRAYSCALE    = xterm_grayscale_color_index(41, 184, 219);

/* Base colors. */
#define FG_BLUE                              12
#define FG_GREEN                             13
#define FG_MAGENTA                           14
#define FG_LAGOON                            15
#define FG_YELLOW                            16
#define FG_RED                               17
#define FG_PINK                              18
#define FG_TEAL                              19
#define FG_MINT                              20
#define FG_PURPLE                            21
#define FG_MAUVE                             22
/* Vs-code colors. */
#define FG_VS_CODE_RED                       23
#define FG_VS_CODE_GREEN                     24
#define FG_VS_CODE_YELLOW                    25
#define FG_VS_CODE_BLUE                      26
#define FG_VS_CODE_MAGENTA                   27
#define FG_VS_CODE_CYAN                      28
#define FG_VS_CODE_WHITE                     29
#define FG_VS_CODE_BRIGHT_RED                30
#define FG_VS_CODE_BRIGHT_GREEN              31
#define FG_VS_CODE_BRIGHT_YELLOW             32
#define FG_VS_CODE_BRIGHT_BLUE               33
#define FG_VS_CODE_BRIGHT_MAGENTA            34
#define FG_VS_CODE_BRIGHT_CYAN               35
#define FG_COMMENT_GREEN                     36
#define FG_SUGGEST_GRAY                      37
/* Bg vs-code colors. */
#define BG_VS_CODE_RED                       38
#define BG_VS_CODE_BLUE                      39
#define BG_VS_CODE_GREEN                     40
/* Total elements. */
#define NUMBER_OF_ELEMENTS                   41

#define FG_VS_CODE_START   FG_VS_CODE_RED
#define FG_VS_CODE_END     (BG_VS_CODE_RED - 1)

#define BG_VS_CODE_START  BG_VS_CODE_RED
#define BG_VS_CODE_END    BG_VS_CODE_GREEN

constexpr short color_array[] {
  VS_CODE_RED,
  VS_CODE_GREEN,
  VS_CODE_YELLOW,
  VS_CODE_BLUE,
  VS_CODE_MAGENTA,
  VS_CODE_CYAN,
  VS_CODE_WHITE,
  VS_CODE_BRIGHT_RED,
  VS_CODE_BRIGHT_GREEN,
  VS_CODE_BRIGHT_YELLOW,
  VS_CODE_BRIGHT_BLUE,
  VS_CODE_BRIGHT_MAGENTA,
  VS_CODE_BRIGHT_CYAN,
  COMMENT_GREEN,
  XTERM_GREY_1
};

constexpr short bg_vs_code_color_array[] {
  VS_CODE_RED,
  VS_CODE_BLUE,
  VS_CODE_GREEN
};
#define BG_COLOR(index) bg_vs_code_color_array[index - BG_VS_CODE_START]