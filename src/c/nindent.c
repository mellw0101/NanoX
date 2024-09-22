#include "../include/c_prototypes.h"


/* Stuff that needs to be shared with the rest of indent.  Documented in indent.h. */
char    *labbuf                = NULL;
char    *s_lab                 = NULL;
char    *e_lab                 = NULL;
char    *l_lab                 = NULL;
char    *codebuf               = NULL;
char    *s_code                = NULL;
char    *e_code                = NULL;
char    *l_code                = NULL;
char    *combuf                = NULL;
char    *s_com                 = NULL;
char    *e_com                 = NULL;
char    *l_com                 = NULL;
char    *s_code_corresponds_to = NULL;
char    *bp_save               = NULL;
char    *be_save               = NULL;
buf_t   save_com;
int      code_lines                 = 0;
int      line_no                    = 0;
int      break_comma                = 0;
int      n_real_blanklines          = 0;
int      prefix_blankline_requested = 0;
codes_t prefix_blankline_requested_code;
int      postfix_blankline_requested = 0;
codes_t postfix_blankline_requested_code;
/*!< Points to current input file name */
char *in_name = 0;
/*!< Points to the current input buffer */
file_buffer_t *current_input = 0;
/*!< True if there is an embedded comment on this code line */
int embedded_comment_on_line = 0;
int else_or_endif            = 0;
/* Structure indentation levels. */
int *di_stack = NULL;
/* Currently allocated size of di_stack.  */
int di_stack_alloc = 0;
/* When this is positive, we have seen a ? without the matching : in a [c]?[s]:[s] construct. */
int           squest       = 0;
unsigned long in_prog_size = 0U;
char         *in_prog      = NULL;

/**
 * The position that we will line the current line up with when it
 * comes time to print it (if we are lining up to parentheses).
 */
int paren_target = 0;

#ifdef DEBUG
int debug = 1;
#endif

static void
sw_buffer(void)
{
    parser_state_tos->search_brace = false;
    bp_save                        = buf_ptr;
    be_save                        = buf_end;
    need_chars(&save_com, 1);
    buf_ptr      = save_com.ptr;
    buf_end      = save_com.end;
    save_com.end = save_com.ptr; /* make save_com empty */
}

static bool
search_brace(codes_t *type_code, bool *force_nl, bool *flushed_nl, bool *last_else,
             bool *is_procname_definition, bool *pbreak_line)
{
    int cur_token;

    while (parser_state_tos->search_brace)
    {
        /* After scanning an if(), while (), etc., it might be necessary to
         * keep track of the text between the if() and the start of the
         * statement which follows.  Use save_com to do so.
         */

        /* The saved buffer has space at the beginning to hold a brace if
         * needed and otherwise collects comments, separating them with newlines
         * if there are more than one.
         *
         * The process stops if we find a left brace or the beginning of a statement.
         *
         * A left brace is moved before any comments in a -br situation.  Otherwise,
         * it comes after comments.
         *
         * At the moment any form feeds before we get to braces or a statement are just
         * dropped.
         */
        cur_token = *type_code;
        switch (cur_token)
        {
            case newline :
                ++line_no;
                *flushed_nl = true;
                break;
            case form_feed :
                break; /* form feeds and newlines found here will be
                        * ignored */
            case lbrace :
                /* Ignore buffering if no comment stored. */

                if (save_com.end == save_com.ptr)
                {
                    parser_state_tos->search_brace = false;
                    return true; /* RETURN */
                }

                /* We need to put the '{' back into save_com somewhere.  */

                if (settings.btype_2 && (parser_state_tos->last_token != rbrace))
                {
                    /* We are asking the brace to jump ahead of the comment.  In
                     * the event that it was between two comments, the regression
                     * examples want to insert a newline to put the comments on
                     * separate lines.  If it is followed by something not a
                     * comment, we don't want to add a newline.
                     *
                     * To make that work, we'll put the brace up front and let the
                     * process continue to pick up another comment or not.
                     */

                    save_com.ptr[0] = '{';
                    save_com.len    = 1;
                    save_com.column = current_column();
                }
                else
                {
                    /* Put the brace at the end of the saved buffer, after
                     * a newline character.  The newline char will cause
                     * a `dump_line' call, thus ensuring that the brace
                     * will go into the right column. */

                    need_chars(&save_com, 2);
                    *save_com.end++ = EOL;
                    *save_com.end++ = '{';
                    save_com.len += 2;

                    /* Go to common code to get out of this loop.  */

                    sw_buffer();
                }
                break;

            case comment :
                /* Save this comment in the `save_com' buffer, for
                 * possible re-insertion in the output stream later. */

                if (!*flushed_nl || (save_com.end != save_com.ptr))
                {
                    need_chars(&save_com, 10);

                    if (save_com.end == save_com.ptr)
                    {
                        /* if this is the first comment, we must set
                         * up the buffer */

                        save_com.start_column = current_column();
                        save_com.ptr[0] = save_com.ptr[1] = ' ';
                        save_com.end                      = save_com.ptr + 2;
                        save_com.len                      = 2;
                        save_com.column                   = current_column();
                    }
                    else
                    {
                        *save_com.end++ = EOL; /* add newline between
                                                * comments */
                        *save_com.end++ = ' ';
                        save_com.len += 2;
                        --line_no;
                    }

                    *save_com.end++ = '/'; /* copy in start of comment */
                    *save_com.end++ = '*';

                    for (;;)
                    {
                        /* loop until we get to the end of the
                         * comment */

                        /* make sure there is room for this character and
                         * (while we're at it) the '/' we might add at the end
                         * of the loop. */

                        need_chars(&save_com, 2);
                        *save_com.end = *buf_ptr++;
                        save_com.len++;

                        if (buf_ptr >= buf_end)
                        {
                            fill_buffer();

                            if (had_eof)
                            {
                                ERROR("EOF encountered in comment", NULL, NULL);
                                return indent_punt;
                            }
                        }

                        if (*save_com.end++ == '*' && *buf_ptr == '/')
                        {
                            break; /* we are at end of comment */
                        }
                    }

                    *save_com.end++ = '/';    /* add ending slash */
                    save_com.len++;

                    if (++buf_ptr >= buf_end) /* get past / in buffer */
                    {
                        fill_buffer();
                    }

                    break;
                }
                /* Just some statement Falls through */
            default :
                /* Some statement.  Unless it's special, arrange to break the line. */
                if (((*type_code == sp_paren) && (*token == 'i') && /* "if" statement */
                     *last_else) ||
                    ((*type_code == sp_else) &&                     /* "else" statement */
                     (e_code != s_code) && (e_code[-1] == '}') &&   /* The "else" follows '}' */
                     (save_com.end == save_com.ptr)))               /* And we haven't found an
                                                                     * intervening comment. */
                {
                    *force_nl = false;
                }
                else if (*flushed_nl)
                {
                    *force_nl = true;
                }
                else
                {
                    /* what ? */
                }
                if (save_com.end == save_com.ptr)
                {
                    /* ignore buffering if comment wasnt saved up */
                    parser_state_tos->search_brace = false;
                    return true; /* RETURN */
                }
                if (*force_nl)
                {
                    *force_nl = false;
                    --line_no; /* this will be re-increased when the nl is read from the buffer */
                    need_chars(&save_com, 2);
                    *save_com.end++ = EOL;
                    save_com.len++;
                    if (settings.verbose && !*flushed_nl)
                    {
                        WARNING("Line broken", NULL, NULL);
                    }
                    *flushed_nl = false;
                }
                /* Now copy this token we just found into the saved buffer. */
                *save_com.end++ = ' ';
                save_com.len++;
                buf_ptr = token;
                /* A total nightmare is created by trying to get the
                 * next token into this save buffer.  Rather than that,
                 * I've just backed up the buffer pointer to point
                 * at `token'. --jla 9/95 */
                parser_state_tos->procname      = "\0";
                parser_state_tos->procname_end  = "\0";
                parser_state_tos->classname     = "\0";
                parser_state_tos->classname_end = "\0";
                /* Switch input buffers so that calls to lexi() will read from our save buffer. */
                sw_buffer();
                break;
        } /* end of switch */

        if (*type_code != code_eof)
        {
            /* Read the next token */
            *type_code = lexi();
            /* Dump the line, if we just saw a newline, and:
             *
             * 1. The current token is a newline. - AND -
             * 2. The comment buffer is empty. - AND -
             * 3. The next token is a newline or comment. - AND -
             * 4. The previous token was a rbrace.
             *
             * This is needed to avoid indent eating newlines between
             * blocks like so:
             *
             * if (...) {
             *
             * }
             *
             * /comment here/
             * if (...)
             *
             * However, if there's a comment in the comment buffer, and the
             * next token is a newline, we'll just append a newline to the end
             * of the comment in the buffer, so that we don't lose it when
             * the comment is written out.
             */
            if (cur_token == newline && (*type_code == newline || *type_code == comment) &&
                parser_state_tos->last_token == rbrace)
            {
                if (!save_com.len)
                {
                    dump_line(true, &paren_target, pbreak_line);
                    *flushed_nl = true;
                }
                else if (*type_code == newline)
                {
                    *save_com.end++ = EOL;
                    save_com.len++;
                }
            }

            *is_procname_definition =
                ((parser_state_tos->procname[0] != '\0') && parser_state_tos->in_parameter_declaration);
        }

        if ((*type_code == ident) && *flushed_nl && !settings.procnames_start_line && parser_state_tos->in_decl &&
            (parser_state_tos->procname[0] != '\0'))
        {
            *flushed_nl = 0;
        }
    } /* end of while (search_brace) */

    *last_else = 0;

    return true;
}

static exit_values_t
indent_main_loop(bool *pbreak_line)
{
    codes_t       hd_type         = code_eof;
    char          *t_ptr           = NULL;
    codes_t       type_code       = start_token;
    exit_values_t file_exit_value = total_success;
    /* Current indentation for declarations. */
    int dec_ind = 0;
    /* 'true' when we've just see a "case"; determines what to do with the following colon. */
    bool scase = false;
    /* Used when buffering up comments to remember that a newline was passed over. */
    bool flushed_nl;
    /* 'true' when in the expression part of if(...), while(...), etc. */
    bool sp_sw    = false;
    bool force_nl = false;
    /**
     * last_token_ends_sp: True if we have just encountered the end of an if (...),
     * etc. (i.e. the ')' of the if (...) was the last token).  The variable is
     * set to 2 in the middle of the main token reading loop and is decremented
     * at the beginning of the loop, so it will reach zero when the second token
     * after the ')' is read.
     */
    bool last_token_ends_sp = false;
    /* 'true' if last keyword was an else. */
    bool last_else = false;
    /* This is the main loop.  it will go until we reach eof */
    for (;;)
    {
        bool    is_procname_definition;
        bb_code_t can_break = bb_none;
        if (type_code != newline)
        {
            can_break = parser_state_tos->can_break;
        }
        parser_state_tos->last_saw_nl = false;
        parser_state_tos->can_break   = bb_none;
        /* lexi reads one token. 'token' points to the actual characters.
         * lexi returns a code indicating the type of token */
        type_code = lexi();
        /**
         * If the last time around we output an identifier or a paren,
         * then consider breaking the line here if it's too long.
         *
         * A similar check is performed at the end of the loop, after
         * we've put the token on the line.
         */
        if ((settings.max_col > 0) && (buf_break != NULL) &&
            (((parser_state_tos->last_token == ident) && (type_code != comma) && (type_code != semicolon) &&
              (type_code != newline) && (type_code != form_feed) && (type_code != rparen) &&
              (type_code != struct_delim)) ||
             ((parser_state_tos->last_token == rparen) && (type_code != comma) && (type_code != rparen))) &&
            (output_line_length() > settings.max_col))
        {
            *pbreak_line = true;
        }
        if (last_token_ends_sp > 0)
        {
            last_token_ends_sp--;
        }
        is_procname_definition =
            (((parser_state_tos->procname[0] != '\0') && parser_state_tos->in_parameter_declaration) ||
             (parser_state_tos->classname[0] != '\0'));
        /**
         * The following code moves everything following an if (), while (),
         * else, etc. up to the start of the following stmt to a buffer. This
         * allows proper handling of both kinds of brace placement.
         */
        flushed_nl = false;
        /* Don't force a newline after an unbraced if, else, etc. */
        if (settings.allow_single_line_conditionals &&
            (parser_state_tos->last_token == rparen || parser_state_tos->last_token == sp_else))
        {
            force_nl = false;
        }
        /* Don't force a newline after '}' in a block initializer */
        if (parser_state_tos->block_init && parser_state_tos->last_token == rbrace && *token == ',')
        {
            force_nl = false;
        }
        /* Hit EOF unexpectedly in comment. */
        if (!search_brace(&type_code, &force_nl, &flushed_nl, &last_else, &is_procname_definition, pbreak_line))
        {
            return indent_punt;
        }
        /* We got eof. */
        if (type_code == code_eof)
        {
            /* Must dump end of line. */
            if (s_lab != e_lab || s_code != e_code || s_com != e_com)
            {
                dump_line(true, &paren_target, pbreak_line);
            }
            /* check for balanced braces */
            if (parser_state_tos->tos > 1)
            {
                ERROR("Unexpected end of file", NULL, NULL);
                file_exit_value = indent_error;
            }
            if (settings.verbose)
            {
                printf("There were %d non-blank output lines and %d comments\n", (int)out_lines, (int)com_lines);
                if (com_lines > 0 && code_lines > 0)
                {
                    printf("(Lines with comments)/(Lines with code): %6.3f\n", (1.0 * com_lines) / code_lines);
                }
            }
            flush_output();
            /* RETURN */
            return file_exit_value;
        }
        if ((type_code != comment) && (type_code != cplus_comment) && (type_code != newline) && (type_code != preesc) &&
            (type_code != form_feed))
        {
            if (force_nl && (type_code != semicolon) &&
                ((type_code != lbrace) || (!parser_state_tos->in_decl && !settings.btype_2) ||
                 (parser_state_tos->in_decl && !settings.braces_on_struct_decl_line) ||
                 (parser_state_tos->last_token == rbrace)))
            {
                if (settings.verbose && !flushed_nl)
                {
                    WARNING("Line broken 2", NULL, NULL);
                }
                flushed_nl = false;
                dump_line(true, &paren_target, pbreak_line);
                parser_state_tos->want_blank = false;
                force_nl                     = false;
            }
            parser_state_tos->in_stmt = true;
            /* turn on flag which causes an extra level of
             * indentation. this is turned off by a ; or } */
            if (s_com != e_com)
            {
                /**
                 * The code has an embedded comment in the line.
                 * Move it from the com buffer to the code buffer.
                 *
                 * Do not add a space before the comment if
                 * it is the first thing on the line.
                 */
                if (e_code != s_code)
                {
                    set_buf_break(bb_embedded_comment_start, paren_target);
                    *e_code++                = ' ';
                    embedded_comment_on_line = 2;
                }
                else
                {
                    embedded_comment_on_line = 1;
                }
                for (t_ptr = s_com; *t_ptr; ++t_ptr)
                {
                    check_code_size();
                    *e_code++ = *t_ptr;
                }
                set_buf_break(bb_embedded_comment_end, paren_target);
                *e_code++                    = ' ';
                /* null terminate code sect */
                *e_code                      = '\0';
                parser_state_tos->want_blank = false;
                e_com                        = s_com;
            }
        }
        else if ((type_code != comment) && (type_code != cplus_comment) &&
                 !(settings.break_function_decl_args && (parser_state_tos->last_token == comma)) &&
                 !((parser_state_tos->last_token == comma) && !settings.leave_comma))
        {
            /** 
             * Preserve force_nl thru a comment but cancel forced newline
             * after newline, form feed, etc.  However, don't cancel if
             * last thing seen was comma-newline and -bc flag is on.
             */
            force_nl = false;
        }
        else
        {
            /* what ? */
        }
        /* Main switch on type of token scanned. */
        check_code_size();
        /* Now, decide what to do with the token. */
        handle_the_token(type_code, &scase, &force_nl, &sp_sw, &flushed_nl, &hd_type, &dec_ind, &last_token_ends_sp,
                         &file_exit_value, can_break, &last_else, is_procname_definition, pbreak_line);
        /* Make sure code section is null terminated. */
        *e_code = '\0';
        if ((type_code != comment) && (type_code != cplus_comment) && (type_code != newline) && (type_code != preesc) &&
            (type_code != form_feed))
        {
            parser_state_tos->last_token = type_code;
        }
        /**
         * Now that we've put the token on the line (in most cases),
         * consider breaking the line because it's too long.
         *
         * Don't consider the cases of `unary_op', newlines,
         * declaration types (int, etc.), if, while, for,
         * identifiers (handled at the beginning of the loop),
         * periods, or preprocessor commands.
         */
        if ((settings.max_col > 0) && (buf_break != NULL))
        {
            if (((type_code == binary_op) || (type_code == postop) || (type_code == question) ||
                 ((type_code == colon) && (scase || (squest <= 0))) || (type_code == semicolon) ||
                 (type_code == sp_nparen) || (type_code == sp_else) || ((type_code == ident) && (*token == '\"')) ||
                 (type_code == struct_delim) || (type_code == comma)) &&
                (output_line_length() > settings.max_col))
            {
                *pbreak_line = true;
            }
        }
    }
    /* End of main infinite loop. */
}

static exit_values_t
indent(file_buffer_t *this_file)
{
    /* Whether or not we should break the line. */
    bool break_line          = false;
    in_prog                     = this_file->data;
    in_prog_pos                 = this_file->data;
    in_prog_size                = this_file->size;
    squest                      = false;
    n_real_blanklines           = 0;
    postfix_blankline_requested = 0;
    clear_buf_break_list(&break_line);
    /* If not specified by user, set this. */
    if (settings.decl_com_ind <= 0)
    {
        settings.decl_com_ind =
            settings.ljust_decl ? (settings.com_ind <= 10 ? 2 : settings.com_ind - 8) : settings.com_ind;
    }
    if (settings.continuation_indent == 0)
    {
        settings.continuation_indent = settings.ind_size;
    }
    if (settings.paren_indent == -1)
    {
        settings.paren_indent = settings.continuation_indent;
    }
    if (settings.case_brace_indent == -1)
    {
        /* This was the previous default. */
        settings.case_brace_indent = settings.ind_size;
    }
    /* Fill the input buffer */
    fill_buffer();
    /* Do the work. */
    return indent_main_loop(&break_line);
}

static char *
handle_profile(int argc, char *argv[])
{
    int   i;
    char *profile_pathname = NULL;
    for (i = 1; i < argc; ++i)
    {
        if ((strcmp(argv[i], "-npro") == 0) || (strcmp(argv[i], "--ignore-profile") == 0) ||
            (strcmp(argv[i], "+ignore-profile") == 0))
        {
            break;
        }
    }
    if (i >= argc)
    {
        profile_pathname = set_profile();
    }
    return profile_pathname;
}

/* Points to the name of the output file. */
static char *out_name = 0;
/* How many input files were specified. */
static int input_files = 0;
/* Names of all input files. */
static char **in_file_names = NULL;
/* Initial number of input filenames to allocate. */
static int max_input_files = 128;

static exit_values_t
process_args(int argc, char *argv[], bool *using_stdin)
{
    int            i;
    exit_values_t exit_status = total_success;
    for (i = 1; i < argc; ++i)
    {
        /* Filename. */
        if ((*argv[i] != '-') && (*argv[i] != '+'))
        {
            /* Last arg was "-o". */
            if (settings.expect_output_file == true)
            {
                if (out_name != 0)
                {
                    fprintf(stderr, "indent: only one output file (2nd was %s)\n", argv[i]);
                    exit_status = invocation_error;
                    break;
                }
                if (input_files > 1)
                {
                    fprintf(stderr, "indent: only one input file when output file is specified\n");
                    exit_status = invocation_error;
                    break;
                }
                out_name                    = argv[i];
                settings.expect_output_file = false;
                continue;
            }
            else
            {
                if (*using_stdin)
                {
                    fprintf(stderr, "indent: can't have filenames when specifying standard input\n");
                    exit_status = invocation_error;
                    break;
                }
                input_files++;
                if (input_files > 1)
                {
                    if (out_name != 0)
                    {
                        fprintf(stderr, "indent: only one input file when output file is specified\n");
                        exit_status = invocation_error;
                        break;
                    }
                    if (settings.use_stdout != 0)
                    {
                        fprintf(stderr, "indent: only one input file when stdout is used\n");
                        exit_status = invocation_error;
                        break;
                    }
                    if (input_files > max_input_files)
                    {
                        max_input_files = 2 * max_input_files;
                        in_file_names   = xrealloc(in_file_names, (max_input_files * sizeof(char *)));
                    }
                }
                in_file_names[input_files - 1] = argv[i];
            }
        }
        else
        {
            /* '-' as filename means stdin. */
            if (strcmp(argv[i], "-") == 0)
            {
                if (input_files > 0)
                {
                    fprintf(stderr, "indent: can't have filenames when specifying standard input\n");
                    exit_status = invocation_error;
                    break;
                }
                *using_stdin = true;
            }
            else
            {
                i += set_option(argv[i], (i < argc ? argv[i + 1] : 0), 1, "command line");
            }
        }
    }
    return exit_status;
}

static exit_values_t
indent_multiple_files(void)
{
    exit_values_t exit_status = total_success;
    int            i;
    /**
     * When multiple input files are specified, make a backup copy
     * and then output the indented code into the same filename.
     */
    for (i = 0; input_files; i++, input_files--)
    {
        exit_values_t status;
        struct stat    file_stats;
        in_name       = in_file_names[i];
        out_name      = in_file_names[i];
        current_input = read_file(in_file_names[i], &file_stats);
        open_output(out_name, "r+");
        /* Aborts on failure. */
        make_backup(current_input, &file_stats);
        /* We have safely made a backup so the open file can be truncated. */
        reopen_output_trunc(out_name);
        reset_parser();
        status = indent(current_input);
        if (status > exit_status)
        {
            exit_status = status;
        }
        if (settings.preserve_mtime)
        {
            close_output(&file_stats, out_name);
        }
        else
        {
            close_output(NULL, out_name);
        }
    }
    return exit_status;
}

static exit_values_t
indent_single_file(bool using_stdin)
{
    int            is_stdin    = false;
    exit_values_t exit_status = total_success;
    struct stat    file_stats;
    if ((input_files == 0) || using_stdin)
    {
        input_files      = 1;
        in_file_names[0] = "Standard input";
        in_name          = in_file_names[0];
        current_input    = read_stdin();
        is_stdin         = true;
    }
    else
    {
        /* 1 input file. */
        in_name       = in_file_names[0];
        current_input = read_file(in_file_names[0], &file_stats);
        if (!out_name && !settings.use_stdout)
        {
            out_name = in_file_names[0];
            make_backup(current_input, &file_stats);
        }
    }
    /* Use stdout if it was specified ("-st"), or neither input
     * nor output file was specified. */
    if (settings.use_stdout || !out_name)
    {
        open_output(NULL, NULL);
    }
    else
    {
        open_output(out_name, "w");
    }
    reset_parser();
    exit_status = indent(current_input);
    if (input_files > 0 && !using_stdin && settings.preserve_mtime)
    {
        close_output(&file_stats, out_name);
    }
    else
    {
        close_output(NULL, out_name);
    }
    if (current_input)
    {
        if (!is_stdin && current_input->name)
        {
            xfree(current_input->name);
        }
        xfree(current_input->data);
    }
    return exit_status;
}

static exit_values_t
indent_all(bool using_stdin)
{
    exit_values_t exit_status = total_success;
    if (input_files > 1)
    {
        exit_status = indent_multiple_files();
    }
    else
    {
        /* One input stream -- specified file, or stdin */
        exit_status = indent_single_file(using_stdin);
    }
    return exit_status;
}

int
main(int argc, char **argv)
{
    char          *tmp;
    char          *profile_pathname = 0;
    bool        using_stdin      = false;
    exit_values_t exit_status;
#if defined(HAVE_SETLOCALE)
    setlocale(LC_ALL, "");
#endif
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
#if defined(_WIN32) && !defined(__CYGWIN__)
    /* Wildcard expansion of commandline arguments, see 'wildexp.c'. */
    extern void wildexp(int *argc, char ***argv);
    wildexp(&argc, &argv);
#endif /* defined (_WIN32) && !defined (__CYGWIN__) */
    /* Initialize settings */
    memset(&settings, 0, sizeof(settings));
#ifdef DEBUG
    if (debug)
    {
        debug_init();
    }
#endif
    /*
     * 'size_t', 'wchar_t' and 'ptrdiff_t' are guarenteed to be available in 'ANSI C'.
     * These pointers will be freed in cleanup_user_specials().
     */
    tmp = xmalloc(7);
    memcpy(tmp, "size_t", 7);
    addkey(tmp, rw_decl);
    tmp = xmalloc(8);
    memcpy(tmp, "wchar_t", 8);
    addkey(tmp, rw_decl);
    tmp = xmalloc(10);
    memcpy(tmp, "ptrdiff_t", 10);
    addkey(tmp, rw_decl);
    init_parser();
    initialize_backups();
    exit_status   = total_success;
    input_files   = 0;
    in_file_names = xmalloc(max_input_files * sizeof(char *));
    set_defaults();
    profile_pathname = handle_profile(argc, argv);
    exit_status      = process_args(argc, argv, &using_stdin);
    if (exit_status == total_success)
    {
        if (settings.verbose && profile_pathname)
        {
            fprintf(stderr, "Read profile %s\n", profile_pathname);
        }
        set_defaults_after();
        exit_status = indent_all(using_stdin);
    }
    if (profile_pathname)
    {
        xfree(profile_pathname);
    }
    xfree(in_file_names);
    uninit_parser();
    cleanup_user_specials();
    return exit_status;
}
