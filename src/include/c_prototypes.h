#pragma once
#include <sys/cdefs.h>
__BEGIN_DECLS

#include "c_definitions.h"
/* clang-format off */

extern char *in_name;   /* <-- Name of input file. */
extern char *in_prog;   /* <-- pointer to the NULL-terminated input program. */
extern char *cur_line;  /* <-- Point to the start of the current line.  */
extern char *labbuf;    /* <-- Buffer for label */
extern char *s_lab;     /* <-- Start of stored label */
extern char *e_lab;     /* <-- End of stored label */
extern char *l_lab;     /* <-- Limit of label buffer */
extern char *codebuf;   /* <-- Buffer for code section */
extern char *s_code;    /* <-- Start  of stored code */
extern char *e_code;    /* <-- End of stored code */
extern char *l_code;    /* <-- Limit of code section */
extern char *combuf;    /* <-- Buffer for comments */
extern char *s_com;     /* <-- Start of stored comments */
extern char *e_com;     /* <-- End of stored comments */
extern char *l_com;     /* <-- Limit of comment buffer */
extern char *buf_ptr;   /* <-- Ptr to next character to be taken from in_buffer */
extern char *buf_end;   /* <-- Ptr to first after last char in in_buffer */
extern char *token;     /* <-- Pointer to the token that lexi() has just found */
extern char *token_end; /* <-- Points to the first char after the end of token */
extern char *bp_save;   /* <-- Saved value of 'buf_ptr' when taking input from 'save_com'. */
extern char *be_save;   /* <-- Similarly saved value of 'buf_end' */
extern int break_comma; /* <-- When true and not in parens, break after a comma. */
extern int  squest;
extern int  pointer_as_binop;
extern int  n_real_blanklines;
extern char *in_prog_pos;             /* <-- Point to the position in the input program which we are currently looking at. */
extern unsigned long in_prog_size;    /* <-- Size of the input program, not including the NEWLINE EOS we add at the end. */
extern char *s_code_corresponds_to;   /* <-- If set, start of corresponding code in token buffer... */
extern int  embedded_comment_on_line; /* <-- True if there is an embedded comment on this code line */
extern int prefix_blankline_requested;
extern int postfix_blankline_requested;
extern int  di_stack_alloc;
extern int *di_stack;
extern int  else_or_endif; /* <-- True if a # else or # endif has been encountered.  */
extern int  code_lines;    /* <-- count of lines with code */
extern int  out_lines;     /* <-- the number of lines written, set by dump_line */
extern int  com_lines;     /* <-- the number of lines with comments, set by dump_line */
extern bool had_eof;       /* <-- set to true when input is exhausted */
extern int  line_no;       /* <-- the current input line number. */
extern int use_stdinout;   /* <-- Nonzero if we should use standard input/output when files are not explicitly specified. */
extern int  paren_target;
extern int buf_break_used;
extern int prev_target_col_break;

/**
 * Buffer in which to save a comment which occurs between an if(), while(),
 * etc., and the statement following it.  Note: the fact that we point into
 * this buffer, and that we might realloc() it (via the need_chars macro) is
 * a bad thing (since when the buffer is realloc'd its address might change,
 * making any pointers into it point to garbage), but since the filling of
 * the buffer (hence the need_chars) and the using of the buffer (where
 * buf_ptr points into it) occur at different times, we can get away with it
 * (it would not be trivial to fix).
 */
extern buf_t save_com;
extern buf_break_st_t *buf_break;
extern codes_t prefix_blankline_requested_code;  /* <-- The code that caused the blank line to be requested. */
extern codes_t postfix_blankline_requested_code; /* <-- The code that caused the blank line to be requested. */
extern file_buffer_t *current_input;
extern user_options_t settings;

/**
 * All manipulations of the parser state occur at the top of stack (tos).
 * A stack is kept for conditional compilation (unrelated to the p_stack, il, &
 * cstk stacks)--it is implemented as a linked list via the next field.
 */
extern parser_state_t *parser_state_tos;

/* 'output.c'. */
extern  int compute_code_target(int paren_targ);
extern  int compute_label_target(void);
extern  int count_columns(int column, char *bp, int stop_char);
extern  int output_line_length(void);
extern void clear_buf_break_list(bool *pbreak_line);
extern void set_buf_break(bb_code_t code, int paren_targ);
extern void dump_line(int force_nl, int *paren_targ, bool *break_line);
extern void flush_output(void);
extern void open_output(const char *filename, const char *mode);
extern void reopen_output_trunc(const char *filename);
extern void close_output(struct stat *file_stats, const char *filename);
extern void inhibit_indenting(bool flag);

/* 'globs.c'. */
extern void *xmalloc(unsigned int size);
extern void *xrealloc(void *ptr, unsigned int size);
extern void  xfree(void *ptr);
extern void  do_exit(int code);
extern void  fatal(const char *string, const char *a0);
extern void  message(char *kind, char *string, char *a0, char *a1);

/* 'parse.c'. */
extern           int inc_pstack(void);
extern          void parse_lparen_in_decl(void);
extern exit_values_t parse(codes_t tk);
extern          void init_parser(void);
extern          void uninit_parser(void);
extern          void reset_parser(void);
extern          void reduce(void);
#ifdef DEBUG
extern          void debug_init(void);
#endif

/* 'lexi.c'. */
extern void    addkey(char *key, rwcodes_t val);
extern codes_t lexi(void);
extern void    cleanup_user_specials(void);

/* 'code_io.c'. */
extern          char *skip_horiz_space(const char * p);
extern file_buffer_t *read_file (char *filename, struct stat *);
extern file_buffer_t *read_stdin (void);
extern            int current_column (void);
extern           void fill_buffer (void);
extern           void skip_buffered_space(void);

/* 'handletoken.c'. */
extern void check_code_size(void);
extern void need_chars(buf_t * bp, size_t needed);
extern void handle_the_token(
    const codes_t   type_code,
    bool           *scase,
    bool           *force_nl,
    bool           *sp_sw,
    bool           *flushed_nl,
    codes_t        *hd_type,
    int            *dec_ind,
    bool           *last_token_ends_sp,
    exit_values_t  *file_exit_value,
    const bb_code_t can_break,
    bool           *last_else,
    bool            is_procname_definition,
    bool           *pbreak_line);

/* 'comment.c'. */
extern void print_comment(int *paren_targ, bool *pbreak_line);

/* 'args.c'. */
extern void set_defaults (void);
extern void set_defaults_after (void);
extern int  set_option (
   const char * option, 
   const char * param, 
   int          expl,
   const char * option_source);
extern char *set_profile (void);

/* 'utils.c'. */
extern void DieError(
   int          errval,
   const char * fmt,
   ...);

/* 'backup.c'. */
/**
 * Determine the value of `version_control' by looking in the environment
 * variable "VERSION_CONTROL".  Defaults to numbered_existing.
 */
extern backup_mode_t version_control_value(void);

/* Initialize information used in determining backup filenames. */
extern void initialize_backups(void);

/**
 * Make a backup copy of FILE, taking into account version-control.
 * See the description at the beginning of the file for details. 
 */
extern void make_backup(
    file_buffer_t * file, 
    const struct stat * file_stats);

__END_DECLS
/* clang-format on */