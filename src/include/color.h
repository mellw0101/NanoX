#include <Mlib/constexpr.hpp>

#define round_short(x)        ((x) >= 0 ? (short)((x) + 0.5) : (short)((x) - 0.5))

/* Return`s a rounded xterm-256 scale value from a 8-bit rgb value.  */
#define xterm_byte_scale(bit) round_short(((double)bit / 255) * 5)
/* Return the xterm-256 index for a given 8bit rgb value. */
#define xterm_color_index(r, g, b) \
    short(16 + (36 * xterm_byte_scale(r)) + (6 * xterm_byte_scale(g)) + xterm_byte_scale(b))

#define xterm_std_color_index(r, g, b) \
    xterm_color_index(((double)r / 6), ((double)g / 6), ((double)b / 6)) - 16

#define grayscale_xterm_byte(r, g, b) round_short(((0.299 * (r) + 0.587 * (g) + 0.114 * (b)) / 255.0) * 5)

#define grayscale_xterm_color_index(r, g, b)                                       \
    (16 + 36 * grayscale_xterm_byte(r, g, b) + 6 * grayscale_xterm_byte(r, g, b) + \
     grayscale_xterm_byte(r, g, b))

#define xterm_grayscale_color_index(r, g, b) grayscale_xterm_color_index(r, g, b)

/* Some color indexes. */
#define COLOR_LAGOON                         38
#define COLOR_PINK                           204
#define COLOR_TEAL                           35
#define COLOR_MINT                           48
#define COLOR_PURPLE                         163
#define COLOR_MAUVE                          134
/* Vs-code colors. */
#define COLOR_VS_CODE_RED                    xterm_color_index(205, 49, 49)
#define COLOR_VS_CODE_GREEN                  xterm_color_index(13, 188, 121)
#define COLOR_VS_CODE_YELLOW                 xterm_color_index(229, 229, 16)
#define COLOR_VS_CODE_BLUE                   xterm_color_index(36, 114, 200)
#define COLOR_VS_CODE_MAGENTA                xterm_color_index(188, 63, 188)
#define COLOR_VS_CODE_CYAN                   xterm_color_index(17, 168, 205)
#define COLOR_VS_CODE_WHITE                  xterm_color_index(229, 229, 229)
#define COLOR_VS_CODE_BRIGHT_RED             xterm_color_index(241, 76, 76)
#define COLOR_VS_CODE_BRIGHT_GREEN           xterm_color_index(35, 209, 139)
#define COLOR_VS_CODE_BRIGHT_YELLOW          xterm_color_index(245, 245, 67)
#define COLOR_VS_CODE_BRIGHT_BLUE            xterm_color_index(59, 142, 234)
#define COLOR_VS_CODE_BRIGHT_MAGENTA         xterm_color_index(214, 112, 214)
#define COLOR_VS_CODE_BRIGHT_CYAN            xterm_color_index(41, 184, 219)

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
/* Total elements. */
#define NUMBER_OF_ELEMENTS                   37

constexpr_map<int, short, 15> color_index_map = {
    {{FG_VS_CODE_RED, COLOR_VS_CODE_RED},
     {FG_VS_CODE_GREEN, COLOR_VS_CODE_GREEN},
     {FG_VS_CODE_YELLOW, COLOR_VS_CODE_YELLOW},
     {FG_VS_CODE_BLUE, COLOR_VS_CODE_BLUE},
     {FG_VS_CODE_MAGENTA, COLOR_VS_CODE_MAGENTA},
     {FG_VS_CODE_CYAN, COLOR_VS_CODE_CYAN},
     {FG_VS_CODE_WHITE, COLOR_VS_CODE_WHITE},
     {FG_VS_CODE_BRIGHT_RED, COLOR_VS_CODE_BRIGHT_RED},
     {FG_VS_CODE_BRIGHT_GREEN, COLOR_VS_CODE_BRIGHT_GREEN},
     {FG_VS_CODE_BRIGHT_YELLOW, COLOR_VS_CODE_BRIGHT_YELLOW},
     {FG_VS_CODE_BRIGHT_BLUE, COLOR_VS_CODE_BRIGHT_BLUE},
     {FG_VS_CODE_BRIGHT_MAGENTA, COLOR_VS_CODE_BRIGHT_MAGENTA},
     {FG_VS_CODE_BRIGHT_CYAN, COLOR_VS_CODE_BRIGHT_CYAN},
     {FG_COMMENT_GREEN, xterm_color_index(0, 102, 0)},
     {FG_SUGGEST_GRAY, xterm_color_index(80, 80, 80)}}
};
