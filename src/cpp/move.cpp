#include "../include/prototypes.h"

#include <Mlib/Debug.h>
#include <Mlib/Profile.h>

#include <cstring>

//  Move to the first line of the file.
void
to_first_line()
{
    openfile->current     = openfile->filetop;
    openfile->current_x   = 0;
    openfile->placewewant = 0;
    refresh_needed        = true;
}

//  Move to the last line of the file.
void
to_last_line()
{
    openfile->current     = openfile->filebot;
    openfile->current_x   = (inhelp) ? 0 : strlen(openfile->filebot->data);
    openfile->placewewant = xplustabs();
    //  Set the last line of the screen as the target for the cursor.
    openfile->cursor_row = editwinrows - 1;
    refresh_needed       = true;
    recook |= perturbed;
    focusing = false;
}

//
//  Determine the actual current chunk and the target column.
//
void
get_edge_and_target(size_t *leftedge, size_t *target_column)
{
    if ISSET (SOFTWRAP)
    {
        size_t shim    = editwincols * (1 + (tabsize / editwincols));
        *leftedge      = leftedge_for(xplustabs(), openfile->current);
        *target_column = (openfile->placewewant + shim - *leftedge) % editwincols;
    }
    else
    {
        *leftedge      = 0;
        *target_column = openfile->placewewant;
    }
}

/* Return the index in line->data that corresponds to the given column on the
 * chunk that starts at the given leftedge.  If the target column has landed
 * on a tab, prevent the cursor from falling back a row when moving forward,
 * or from skipping a row when moving backward, by incrementing the index. */
size_t
proper_x(linestruct *line, size_t *leftedge, bool forward, size_t column, bool *shifted)
{
    size_t index = actual_x(line->data, column);
    if (ISSET(SOFTWRAP) && line->data[index] == '\t' &&
        ((forward && wideness(line->data, index) < *leftedge) ||
         (!forward && column / tabsize == (*leftedge - 1) / tabsize &&
          column / tabsize < (*leftedge + editwincols - 1) / tabsize)))
    {
        index++;
        if (shifted != nullptr)
        {
            *shifted = true;
        }
    }
    if ISSET (SOFTWRAP)
    {
        *leftedge = leftedge_for(wideness(line->data, index), line);
    }
    return index;
}

/* Adjust the values for current_x and placewewant in case we have landed in
 * the middle of a tab that crosses a row boundary. */
void
set_proper_index_and_pww(size_t *leftedge, size_t target, bool forward)
{
    unsigned long was_edge = *leftedge;
    bool          shifted  = false;
    openfile->current_x =
        proper_x(openfile->current, leftedge, forward, actual_last_column(*leftedge, target), &shifted);
    //
    //  If the index was incremented, try going to the target column.
    //
    if (shifted || *leftedge < was_edge)
    {
        openfile->current_x =
            proper_x(openfile->current, leftedge, forward, actual_last_column(*leftedge, target), &shifted);
    }
    openfile->placewewant = *leftedge + target;
}

/* Move up almost one screenful. */
void
do_page_up()
{
    int    mustmove = (editwinrows < 3) ? 1 : editwinrows - 2;
    size_t leftedge, target_column;
    //
    //  If we're not in smooth scrolling mode, put the cursor at the
    //  beginning of the top line of the edit window, as Pico does.
    //
    if ISSET (JUMPY_SCROLLING)
    {
        openfile->current    = openfile->edittop;
        leftedge             = openfile->firstcolumn;
        openfile->cursor_row = 0;
        target_column        = 0;
    }
    else
    {
        get_edge_and_target(&leftedge, &target_column);
    }
    //
    //  Move up the required number of lines or chunks.
    //  If we can't, we're at the top of the file,
    //  so put the cursor there and get out.
    //
    if (go_back_chunks(mustmove, &openfile->current, &leftedge) > 0)
    {
        to_first_line();
        return;
    }
    set_proper_index_and_pww(&leftedge, target_column, false);
    //
    //  Move the viewport so that the cursor stays immobile, if possible.
    //
    adjust_viewport(STATIONARY);
    refresh_needed = true;
}

//
//  Move down almost one screenful.
//
void
do_page_down(void)
{
    int           mustmove = (editwinrows < 3) ? 1 : editwinrows - 2;
    unsigned long leftedge, target_column;
    /* If we're not in smooth scrolling mode, put the cursor at the
     * beginning of the top line of the edit window, as Pico does. */
    if ISSET (JUMPY_SCROLLING)
    {
        openfile->current    = openfile->edittop;
        leftedge             = openfile->firstcolumn;
        openfile->cursor_row = 0;
        target_column        = 0;
    }
    else
    {
        get_edge_and_target(&leftedge, &target_column);
    }
    /* Move down the required number of lines or chunks.  If we can't, we're
     * at the bottom of the file, so put the cursor there and get out. */
    if (go_forward_chunks(mustmove, &openfile->current, &leftedge) > 0)
    {
        to_last_line();
        return;
    }
    set_proper_index_and_pww(&leftedge, target_column, true);
    /* Move the viewport so that the cursor stays immobile, if possible. */
    adjust_viewport(STATIONARY);
    refresh_needed = true;
}

//
//  Place the cursor on the first row in the viewport.
//
void
to_top_row()
{
    size_t leftedge, offset;
    get_edge_and_target(&leftedge, &offset);
    openfile->current = openfile->edittop;
    leftedge          = openfile->firstcolumn;
    set_proper_index_and_pww(&leftedge, offset, false);
    place_the_cursor();
}

/* Place the cursor on the last row in the viewport, when possible. */
void
to_bottom_row()
{
    size_t leftedge, offset;
    get_edge_and_target(&leftedge, &offset);
    openfile->current = openfile->edittop;
    leftedge          = openfile->firstcolumn;
    go_forward_chunks(editwinrows - 1, &openfile->current, &leftedge);
    set_proper_index_and_pww(&leftedge, offset, true);
    place_the_cursor();
}

//
//  Put the cursor line at the center, then the top, then the bottom.
//
void
do_cycle()
{
    if (cycling_aim == 0)
    {
        adjust_viewport(CENTERING);
    }
    else
    {
        openfile->cursor_row = (cycling_aim == 1) ? 0 : editwinrows - 1;
        adjust_viewport(STATIONARY);
    }
    cycling_aim = (cycling_aim + 1) % 3;
    draw_all_subwindows();
    full_refresh();
}

//
//  Scroll the line with the cursor to the center of the screen.
//
void
do_center()
{
    //
    //  The main loop has set 'cycling_aim' to zero.
    //
    do_cycle();
}

//
//  Move to the first beginning of a paragraph before the current line.
//
void
do_para_begin(linestruct **line)
{
    if ((*line)->prev != nullptr)
    {
        *line = (*line)->prev;
    }
    while (!begpar(*line, 0))
    {
        *line = (*line)->prev;
    }
}

//
//  Move down to the last line of the first found paragraph.
//
void
do_para_end(linestruct **line)
{
    while ((*line)->next != nullptr && !inpar(*line))
    {
        *line = (*line)->next;
    }
    while ((*line)->next != nullptr && inpar((*line)->next) && !begpar((*line)->next, 0))
    {
        *line = (*line)->next;
    }
}

//
//  Move up to first start of a paragraph before the current line.
//
void
to_para_begin()
{
    linestruct *was_current = openfile->current;
    do_para_begin(&openfile->current);
    openfile->current_x = 0;
    edit_redraw(was_current, CENTERING);
}

//
//  Move down to just after the first found end of a paragraph.
//
void
to_para_end()
{
    linestruct *was_current = openfile->current;

    do_para_end(&openfile->current);

    //
    //  Step beyond the last line of the paragraph, if possible.
    //  Otherwise, move to the end of the line.
    //
    if (openfile->current->next != nullptr)
    {
        openfile->current   = openfile->current->next;
        openfile->current_x = 0;
    }
    else
    {
        openfile->current_x = constexpr_strlen(openfile->current->data);
    }

    edit_redraw(was_current, CENTERING);
    recook |= perturbed;
}

//
//  Move to the preceding block of text.
//  TODO : (to_prev_block) - Make this stop at indent as well.
//
void
to_prev_block()
{
    int         cur_indent, was_indent = -1;
    linestruct *was_current = openfile->current;
    bool        is_text = false, seen_text = false;
    //
    //  Skip backward until first blank line after some nonblank line(s).
    //
    while (openfile->current->prev != nullptr && (!seen_text || is_text))
    {
        openfile->current = openfile->current->prev;
        /**
            This is experimental and will be improved.
            TODO: (to_prev_block) - Make better with more rules.
         */
        for (cur_indent = 0; openfile->current->data[cur_indent]; cur_indent++)
        {
            if (openfile->current->data[cur_indent] != '\t')
            {
                if (was_indent == -1)
                {
                    was_indent = cur_indent;
                }
                else if (was_indent != cur_indent)
                {
                    if (openfile->current->next != nullptr)
                    {
                        openfile->current   = openfile->current->next;
                        openfile->current_x = was_indent;
                        edit_redraw(was_current, CENTERING);
                        return;
                    }
                }
                break;
            }
        }
        is_text   = !white_string(openfile->current->data);
        seen_text = seen_text || is_text;
    }
    //
    //  Step forward one line again if we passed text but this line is blank.
    //
    if (seen_text && openfile->current->next != nullptr && white_string(openfile->current->data))
    {
        openfile->current = openfile->current->next;
    }
    openfile->current_x = 0;
    edit_redraw(was_current, CENTERING);
}

/* Move to the next block of text. */
void
to_next_block()
{
    int         cur_indent, was_indent = -1;
    linestruct *was_current = openfile->current;
    bool        is_white    = white_string(openfile->current->data);
    bool        seen_white  = is_white;
    /* Skip forward until first nonblank line after some blank line(s). */
    while (openfile->current->next != nullptr && (!seen_white || is_white))
    {
        openfile->current = openfile->current->next;
        /* This is experimental and will be improved.
         * TODO: (to_next_block) - Implement more complex rules. */
        for (cur_indent = 0; openfile->current->data[cur_indent]; cur_indent++)
        {
            if (openfile->current->data[cur_indent] != '\t')
            {
                if (was_indent == -1)
                {
                    was_indent = cur_indent;
                }
                else if (was_indent != cur_indent)
                {
                    if (openfile->current->prev != nullptr)
                    {
                        openfile->current   = openfile->current->prev;
                        openfile->current_x = was_indent;
                        edit_redraw(was_current, CENTERING);
                        recook |= perturbed;
                        return;
                    }
                }
                break;
            }
        }
        is_white   = white_string(openfile->current->data);
        seen_white = seen_white || is_white;
    }
    openfile->current_x = 0;
    edit_redraw(was_current, CENTERING);
    recook |= perturbed;
}

/* Move to the previous word. */
void
do_prev_word()
{
    PROFILE_FUNCTION;
    bool punctuation_as_letters = ISSET(WORD_BOUNDS);
    bool seen_a_word            = false;
    bool step_forward           = false;
    /* Move backward until we pass over the start of a word. */
    while (true)
    {
        /* If at the head of a line, move to the end of the preceding one. */
        if (openfile->current_x == 0)
        {
            if (openfile->current->prev == nullptr)
            {
                break;
            }
            openfile->current   = openfile->current->prev;
            openfile->current_x = constexpr_strlen(openfile->current->data);
        }
        /* Step back one character. */
        openfile->current_x = step_left(openfile->current->data, openfile->current_x);
        if (is_word_char(openfile->current->data + openfile->current_x, punctuation_as_letters))
        {
            seen_a_word = true;
            //
            //  If at the head of a line now,
            //  this surely is a word start.
            //
            if (openfile->current_x == 0)
            {
                break;
            }
        }
        else if (isCppSyntaxChar(openfile->current->data[openfile->current_x]))
        {
            break;
        }
        else if (is_zerowidth(openfile->current->data + openfile->current_x))
        {
            ;  //  Do nothing.
        }
        else if (seen_a_word)
        {
            //
            //  This is space now: we've overshot the start of the word.
            //
            step_forward = true;
            break;
        }
    }
    if (step_forward)
    {
        /* Move one character forward again to sit on the start of the word. */
        openfile->current_x = step_right(openfile->current->data, openfile->current_x);
    }
}

/* Move to the next word.  If after_ends is 'true',
 * stop at the ends of words instead of at their beginnings.
 * If @param 'after_ends' is 'true', stop at the ends of words instead of at their beginnings.
 * Return 'true' if we started on a word. */
bool
do_next_word(bool after_ends)
{
    PROFILE_FUNCTION;
    bool punct_as_letters, started_on_word, seen_space, seen_word;
    punct_as_letters = ISSET(WORD_BOUNDS);
    started_on_word  = is_word_char(openfile->current->data + openfile->current_x, punct_as_letters);
    seen_space       = !started_on_word;
    seen_word        = started_on_word;
    /* Move forward until we reach the start of a word. */
    while (true)
    {
        /* If at the end of a line, move to the beginning of the next one. */
        if (openfile->current->data[openfile->current_x] == '\0')
        {
            /* When at end of file, stop. */
            if (openfile->current->next == nullptr)
            {
                break;
            }
            openfile->current   = openfile->current->next;
            openfile->current_x = 0;
            seen_space          = true;
        }
        else
        {
            /* Step forward one character. */
            openfile->current_x = step_right(openfile->current->data, openfile->current_x);
        }
        if (after_ends)
        {
            /* If this is a word character, continue.  Else, it's a separator,
             * and if we've already seen a word, then it's a word end. */
            if (is_word_char(openfile->current->data + openfile->current_x, punct_as_letters))
            {
                seen_word = true;
            }
            /* Skip */
            else if (is_zerowidth(openfile->current->data + openfile->current_x))
            {
                ;
            }
            else if (seen_word)
            {
                break;
            }
        }
        else
        {
            if (is_zerowidth(openfile->current->data + openfile->current_x))
            {
                ;
            }
            /* Checks for cpp syntax characters, and if true then break. */
            else if (isCppSyntaxChar(openfile->current->data[openfile->current_x]))
            {
                break;
            }
            else
            {
                /* If this is not a word character, then it's a separator.
                 * Else, if we've already seen a separator, then it's a word start. */
                if (!is_word_char(openfile->current->data + openfile->current_x, punct_as_letters))
                {
                    seen_space = true;
                    break;
                }
                else if (seen_space)
                {
                    break;
                }
            }
        }
    }
    return started_on_word;
}

/* Move to the previous word in the file, and update the screen afterwards. */
void
to_prev_word(void)
{
    linestruct *was_current = openfile->current;
    do_prev_word();
    edit_redraw(was_current, FLOWING);
}

/* Move to the next word in the file.
 * If the AFTER_ENDS flag is set, stop at word ends instead of beginnings.
 * Update the screen afterwards. */
void
to_next_word(void)
{
    linestruct *was_current = openfile->current;
    do_next_word(ISSET(AFTER_ENDS));
    edit_redraw(was_current, FLOWING);
}

/* Move to the beginning of the current line (or softwrapped chunk).
 * When enabled, do a smart home.  When softwrapping, go the beginning
 * of the full line when already at the start of a chunk. */
void
do_home(void)
{
    linestruct *was_current = openfile->current;
    bool        moved_off_chunk, moved;
    size_t      was_column, leftedge, left_x;
    moved_off_chunk = true;
    moved           = false;
    was_column      = xplustabs();
    if ISSET (SOFTWRAP)
    {
        leftedge = leftedge_for(was_column, openfile->current);
        left_x   = proper_x(openfile->current, &leftedge, false, leftedge, nullptr);
    }
    if ISSET (SMART_HOME)
    {
        size_t indent_x = indent_length(openfile->current->data);
        if (openfile->current->data[indent_x] != '\0')
        {
            //
            //  If we're exactly on the indent, move fully home.  Otherwise,
            //  when not softwrapping or not after the first nonblank chunk,
            //  move to the first nonblank character.
            //
            if (openfile->current_x == indent_x)
            {
                openfile->current_x = 0;
                moved               = true;
            }
            else if (left_x <= indent_x)
            {
                openfile->current_x = indent_x;
                moved               = true;
            }
        }
    }
    if (!moved && ISSET(SOFTWRAP))
    {
        //
        //  If already at the left edge of the screen, move fully home.
        //  Otherwise, move to the left edge.
        //
        if (openfile->current_x == left_x)
        {
            openfile->current_x = 0;
        }
        else
        {
            openfile->current_x   = left_x;
            openfile->placewewant = leftedge;
            moved_off_chunk       = false;
        }
    }
    else if (!moved)
    {
        openfile->current_x = 0;
    }
    if (moved_off_chunk)
    {
        openfile->placewewant = xplustabs();
    }
    /* If we changed chunk, we might be offscreen.
     * Otherwise, update current if the mark is on or we changed 'page'. */
    if (ISSET(SOFTWRAP) && moved_off_chunk)
    {
        edit_redraw(was_current, FLOWING);
    }
    else if (line_needs_update(was_column, openfile->placewewant))
    {
        update_line(openfile->current, openfile->current_x);
    }
}

/* Move to the end of the current line (or softwrapped 'chunk').
 * When softwrapping and already at the end of a 'chunk'(?? TODO: what does a 'chunk' in this context),
 * go to the end of the full line. */
void
do_end(void)
{
    bool          moved_off_chunk, kickoff, last_chunk;
    unsigned long was_column, line_len, leftedge, rightedge, right_x;
    linestruct   *was_current;
    was_current     = openfile->current;
    was_column      = xplustabs();
    line_len        = constexpr_strlen(openfile->current->data);
    moved_off_chunk = true;
    if ISSET (SOFTWRAP)
    {
        kickoff    = true;
        last_chunk = false;
        leftedge   = leftedge_for(was_column, openfile->current);
        rightedge  = get_softwrap_breakpoint(openfile->current->data, leftedge, kickoff, last_chunk);
        /* If we're on the last chunk, we're already at the end of the line.
         * Otherwise, we're one column past the end of the line.  Shifting
         * backwards one column might put us in the middle of a multi-column
         * character, but actual_x() will fix that. */
        if (!last_chunk)
        {
            rightedge--;
        }
        right_x = actual_x(openfile->current->data, rightedge);
        /* If already at the right edge of the screen, move fully to
         * the end of the line.  Otherwise, move to the right edge. */
        if (openfile->current_x == right_x)
        {
            openfile->current_x = line_len;
        }
        else
        {
            openfile->current_x   = right_x;
            openfile->placewewant = rightedge;
            moved_off_chunk       = false;
        }
    }
    else
    {
        openfile->current_x = line_len;
    }
    if (moved_off_chunk)
    {
        openfile->placewewant = xplustabs();
    }
    /* If we changed chunk, we might be offscreen.  Otherwise,
     * update current if the mark is on or we changed "page". */
    if (ISSET(SOFTWRAP) && moved_off_chunk)
    {
        edit_redraw(was_current, FLOWING);
    }
    else if (line_needs_update(was_column, openfile->placewewant))
    {
        update_line(openfile->current, openfile->current_x);
    }
}

/* Move the cursor to the preceding line or chunk. */
void
do_up(void)
{
    linestruct   *was_current = openfile->current;
    unsigned long leftedge, target_column;
    get_edge_and_target(&leftedge, &target_column);
    /* If we can't move up one line or chunk, we're at top of file. */
    if (go_back_chunks(1, &openfile->current, &leftedge) > 0)
    {
        return;
    }
    set_proper_index_and_pww(&leftedge, target_column, false);
    if (openfile->cursor_row == 0 && !ISSET(JUMPY_SCROLLING) && (tabsize < editwincols || !ISSET(SOFTWRAP)))
    {
        edit_scroll(BACKWARD);
    }
    else
    {
        edit_redraw(was_current, FLOWING);
    }
    /* <Up> should not change placewewant, so restore it. */
    openfile->placewewant = leftedge + target_column;
}

/* Move the cursor to next line or chunk. */
void
do_down(void)
{
    linestruct   *was_current = openfile->current;
    unsigned long leftedge, target_column;
    get_edge_and_target(&leftedge, &target_column);
    /* If we can't move down one line or chunk, we're at bottom of file. */
    if (go_forward_chunks(1, &openfile->current, &leftedge) > 0)
    {
        return;
    }
    set_proper_index_and_pww(&leftedge, target_column, true);
    if (openfile->cursor_row == editwinrows - 1 && !ISSET(JUMPY_SCROLLING) &&
        (tabsize < editwincols || !ISSET(SOFTWRAP)))
    {
        edit_scroll(FORWARD);
    }
    else
    {
        edit_redraw(was_current, FLOWING);
    }
    /* <Down> should not change placewewant, so restore it. */
    openfile->placewewant = leftedge + target_column;
}

/* Scroll up one line or chunk without moving the cursor textwise. */
void
do_scroll_up(void)
{
    //
    //  When the top of the file is onscreen, we can't scroll.
    //
    if (openfile->edittop->prev == nullptr && openfile->firstcolumn == 0)
    {
        return;
    }
    if (openfile->cursor_row == editwinrows - 1)
    {
        do_up();
    }
    if (editwinrows > 1)
    {
        edit_scroll(BACKWARD);
    }
}

/* Scroll down one line or chunk without moving the cursor textwise. */
void
do_scroll_down(void)
{
    if (openfile->cursor_row == 0)
    {
        do_down();
    }
    if (editwinrows > 1 && (openfile->edittop->next != nullptr ||
                            (ISSET(SOFTWRAP) && (extra_chunks_in(openfile->edittop) >
                                                 chunk_for(openfile->firstcolumn, openfile->edittop)))))
    {
        edit_scroll(FORWARD);
    }
}

/* Move left one character. */
void
do_left(void)
{
    linestruct *was_current = openfile->current;
    if (openfile->current_x > 0)
    {
        openfile->current_x = step_left(openfile->current->data, openfile->current_x);
        while (openfile->current_x > 0 && is_zerowidth(openfile->current->data + openfile->current_x))
        {
            openfile->current_x = step_left(openfile->current->data, openfile->current_x);
        }
    }
    else if (openfile->current != openfile->filetop)
    {
        openfile->current   = openfile->current->prev;
        openfile->current_x = constexpr_strlen(openfile->current->data);
    }
    edit_redraw(was_current, FLOWING);
}

/* Move right one character. */
void
do_right(void)
{
    linestruct *was_current = openfile->current;
    if (openfile->current->data[openfile->current_x] != '\0')
    {
        openfile->current_x = step_right(openfile->current->data, openfile->current_x);
        while (openfile->current->data[openfile->current_x] != '\0' &&
               is_zerowidth(openfile->current->data + openfile->current_x))
        {
            openfile->current_x = step_right(openfile->current->data, openfile->current_x);
        }
    }
    else if (openfile->current != openfile->filebot)
    {
        openfile->current   = openfile->current->next;
        openfile->current_x = 0;
    }
    edit_redraw(was_current, FLOWING);
}
