#include "../include/prototypes.h"

#include <Mlib/Profile.h>

/* Render the text of a given line.  Note that this function only renders the text and nothing else. */
void
render_line_text(const int row, const char *str, linestruct *line, const unsigned long from_col)
{
    if (margin > 0)
    {
        midwin_attr_on(interface_color_pair[LINE_NUMBERS]);
        if (ISSET(SOFTWRAP) && from_col != 0)
        {
            midwin_mv_printw(row, 0, "%*s", margin - 1, " ");
        }
        else
        {

            midwin_mv_printw(row, 0, "%*lu", margin - 1, line->lineno);
        }
        midwin_attr_off(interface_color_pair[LINE_NUMBERS]);
        if (line->has_anchor == TRUE && (from_col == 0 || !ISSET(SOFTWRAP)))
        {
            if (using_utf8())
            {
                midwin_printw("\xE2\xAC\xA5");
            }
            else
            {
                midwin_printw("+");
            }
        }
        else
        {
            midwin_printw(" ");
        }
    }
    midwin_mv_add_str(row, margin, str);
    if (is_shorter || ISSET(SOFTWRAP))
    {
        midwin_clear_to_eol();
    }
    if (sidebar)
    {
        midwin_mv_add_char(row, COLS - 1, bardata[row]);
    }
}
