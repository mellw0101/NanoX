#pragma once
/* clang-format off */

/* Include`s. */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <dirent.h>
#include <libintl.h>

/* Define`s. */

/* Values of special characters. */
#define TAB               '\t'
#define EOL               '\n'
#define EOS               '\0'
#define BACKSLASH         '\\'
#define NULL_CHAR         '\0'

#define VERSION "0.0.1"
#define PACKAGE "NIndent"
#define PACKAGE_STRING PACKAGE " " VERSION

#ifndef isascii
#    define ISDIGIT(c) (isdigit((unsigned char)(c)))
#else
#    define ISDIGIT(c) (isascii(c) && isdigit(c))
#endif

#define NAMLEN(dirent) strlen((dirent)->d_name)

#define RCSTAG_CC(string) static char rcs_ident[] __attribute__((unused)) = string
#ifdef __STDC__
#    define RCSTAG_H(name, string) static const char rcs_ident_##name##_h[] __attribute__((unused)) = string
#else
#    define RCSTAG_H(name, string) /**/
#endif

/* Round up P to be a multiple of SIZE. */
#ifndef ROUND_UP
#    define ROUND_UP(p, size) (((unsigned long)(p) + (size) - 1) & ~((size) - 1))
#endif

/* Some default values. */
#define DEFAULT_RIGHT_MARGIN         78
#define DEFAULT_RIGHT_COMMENT_MARGIN 78
#define DEFAULT_LABEL_INDENT         -2
#define HAVE_SETLOCALE 1
#define LOCALEDIR "/usr/share/locale"

/** Warning messages: indent continues. */
#define WARNING(s, a, b) message("Warning", s, (char *)(a), (char *)(b))
/** Error messages: indent stops processing the current file. */
#define ERROR(s, a, b) message("Error", s, (char *)(a), (char *)(b))

/* Struct`s. */

/**
 * Values that `indent' can return for exit status.
 *
 *  `total_success' means no errors or warnings were found during a successful
 *    invocation of the program.
 *
 *  `invocation_error' is returned if an invocation problem (like an incorrect
 *    option) prevents any formatting to occur.
 *
 *  `indent_error' is returned if errors occur during formatting which
 *    do not prevent completion of the formatting, but which appear to be
 *    manifested by incorrect code (i.e, code which wouldn't compile).
 *
 *  `indent_punt' is returned if formatting of a file is halted because of
 *    an error with the file which prevents completion of formatting.  If more
 *    than one input file was specified, indent continues to the next file.
 *
 *  `indent_fatal' is returned if a serious internal problem occurs and
 *    the entire indent process is terminated, even if all specified files
 *    have not been processed.
 */
typedef enum exit_values {
    total_success    = 0,
    invocation_error = 64, /* EX_USAGE */
    indent_error     = 2,
    indent_punt      = 3,
    indent_fatal     = 4,
    system_error     = 5
} exit_values_t;

typedef struct file_buffer {
    char  *name;
    size_t size;
    char  *data;
} file_buffer_t;

typedef enum bb_code {
    bb_none,
    bb_comma,
    bb_embedded_comment_start,
    bb_embedded_comment_end,
    bb_proc_call,
    bb_dec_ind,
    bb_unary_op,
    bb_binary_op,
    bb_before_boolean_binary_op,
    bb_after_boolean_binary_op,
    bb_after_equal_sign,
    bb_comparisation,
    bb_question,
    bb_colon,
    bb_label,
    bb_semicolon,
    bb_lbrace,
    bb_rbrace,
    bb_overloaded,
    bb_const_qualifier,
    bb_ident,
    bb_attribute,
    bb_struct_delim,
    bb_operator2, /* <-- Member selection (bb_struct_delim `.' or `->') */
    bb_operator4, /* <-- Member selection (bb_struct_delim `.*' or `->*') */
    bb_operator5, /* <-- Multiply, divide or modulo */
    bb_operator6, /* <-- Add or subtract */
    bb_doublecolon,
    bb_cast
} bb_code_t;

/** Used to keep track of buffers. */
typedef struct buf {
    char *ptr;          /* <-- Points to the start of the buffer. */
    char *end;          /* <-- Points to the character beyond the last one (e.g. is equal to ptr if the buffer is (empty)). */
    int   size;         /* <-- How many chars are currently allocated.  */
    int   len;          /* <-- How many chars we're actually using. */
    int   start_column; /* <-- Corresponding column of first character in the buffer. */
    int   column;       /* <-- Column we were in when we switched buffers. */
} buf_t;

typedef enum rwcodes {
    rw_none,
    rw_operator,    /* <-- For C++ operator overloading. */
    rw_break,
    rw_switch,
    rw_case,
    rw_struct_like, /* <-- struct, union */
    rw_enum,
    rw_decl,
    rw_sp_paren,    /* <-- if, while, for */
    rw_sp_nparen,   /* <-- do */
    rw_sp_else,     /* <-- else */
    rw_sizeof,
    rw_return
} rwcodes_t;

typedef struct {
    char      *rwd;
    rwcodes_t rwcode;
} templ_t;

typedef enum codes {
    code_eof = 0,   /* <-- End of file. */
    newline,
    lparen,         /* <-- '(' or '['.  Also '{' in an initialization. */
    rparen,         /* <-- ')' or ']'.  Also '}' in an initialization. */
    start_token,
    unary_op,
    binary_op,
    postop,
    question,
    casestmt,
    colon,
    doublecolon,    /* <-- For C++ class methods. */
    semicolon,
    lbrace,
    rbrace,
    ident,          /* <-- string or char literal, identifier, number */
    overloaded,     /* <-- For C++ overloaded operators (like +) */
    cpp_operator,
    comma,
    comment,        /* <-- A 'slash-star' comment. */
    cplus_comment,  /* <-- A C++ 'slash-slash'/'//'. */
    swstmt,
    preesc,         /* <-- '#'. */
    form_feed,
    decl,
    sp_paren,       /* <-- if, for, or while token */
    sp_nparen,      /* <-- do */
    sp_else,        /* <-- else */
    ifstmt,
    elseifstmt,
    whilestmt,
    forstmt,
    stmt,
    stmtl,
    elselit,
    dolit,
    dohead,
    dostmt,
    ifhead,
    elsehead,
    struct_delim,   /* <-- '.' or "->" */
    attribute,      /* <-- The '__attribute__' qualifier */
    number_of_codes
} codes_t;

/* This structure stores all the user options that come from (e.g.) command line flags. */
typedef struct user_options_st {
    /* When true, non-essential error messages are printed. */
    int verbose;
    /* Does indent convert tabs to spaces or not. */
    int use_tabs;
    /* The number of columns a tab character generates. */
    int tabsize;
    /* Where does output go. */
    int use_stdout;
    /* If true, a space is inserted between if, while, or for, and a semicolon for example while (*p++ == ' ') ; */
    int space_sp_semicolon;
    int swallow_optional_blanklines;
    /*!<  true if comment continuation lines should have stars at the beginning of each line. */
    int star_comment_cont;
    /*!<  Indentation level to be used for a '{' directly following a struct, union or enum */
    int struct_brace_indent;
    /*!<  Put a space after `while' */
    int space_after_while;
    /*!<  Put a space after `if' */
    int space_after_if;
    /*!<  Put a space after `for' */
    int space_after_for;
    /* if true, the names of procedures being defined get placed in column 1
     * (ie. a newline is placed between the type of the procedure and its name) */
    int procnames_start_line;
    /*!<  If true, parentheses will look like: ( foo ) rather than (foo) */
    int parentheses_space;
    /*!<  True when the modification time of the files should  be preserved. */
    int preserve_mtime;
    /*!<  set to the indentation per open parens */
    int paren_indent;
    /*!<  If true, procedure calls look like: foo (bar) rather than foo(bar) */
    int proc_calls_space;
    /*!<  if true, leave the spaces between  '#' and preprocessor commands. */
    int leave_preproc_space;
    /*!<  if this is >0, this becomes the preproc indent-level */
    int force_preproc_width;
    /*!<  if true, continued code within parens will be lined up to the open paren */
    int lineup_to_parens;
    /* True when positions at which we read a newline in the input
     * file, should get a high priority to break long lines at. */
    int honour_newlines;
    int fix_nested_comments;           /*!<  If nested comments are to be fixed */
    int format_comments;               /*!<  If any comments are to be reformatted */
    int format_col1_comments;          /*!<  If comments which start in column 1 are to be magically reformatted */
    int extra_expression_indent;       /*!<  True if continuation lines from the expression part of "if(e)",
                                        * "while(e)", "for(e;e;e)" should be indented an extra tab stop so that
                                        * they don't conflict with the code that follows */
    int ljust_decl;                    /*!<  true if declarations should be left justified */
    int cast_space;                    /*!<  If true, casts look like: (char *) bar rather than (char *)bar */
    int cuddle_else;                   /*!<  true if else should cuddle up to '}' */
    int cuddle_do_while;               /*!<  true if '}' should cuddle up to while in do loop */
    int comment_delimiter_on_blankline;
    int blank_after_sizeof;            /*!<  true iff a blank should always be inserted after sizeof */
    int break_function_decl_args;      /*!<  true if declarations should have args on new lines */
    int break_function_decl_args_end;  /*!<  true if declarations should have
                                        * ")" after args on new lines */
    int leave_comma;                   /*!<  if true, never break declarations after commas */
    int break_before_boolean_operator; /*!<  True when we prefer to break a long line
                                        * before a '&&' or '||', instead of behind it.
                                        */
    int blanklines_before_blockcomments;
    int blanklines_after_declarations;
    int blanklines_after_procs;
    int blanklines_after_declarations_at_proctop; /*!<  This is vaguely  similar to blanklines_after_declarations except
                                                   * that it only applies to the first set of declarations in a
                                                   * procedure (just after the first '{') and it causes a blank line to
                                                   * be generated even if there are no declarations
                                                   */
    int blanklines_around_conditional_compilation;
    int comment_max_col;
    int max_col;                                  /*!<  the maximum allowable line length */
    int label_offset;                             /* offset of labels */
    int ind_size;                                 /* !< The size of one indentation level in spaces.  */
    int indent_parameters;                        /*!<  Number of spaces to indent parameters.  */
    int decl_indent;                              /*!< column to indent declared identifiers to */
    int unindent_displace;                        /*!< comments not to the right of code will be
                                                   * placed this many indentation levels to the
                                                   * left of code */
    int else_endif_col;      /*!< The column in which comments to the right of # else and # endif should start. */
    int case_indent;         /*!< The distance to indent case labels from the switch statement */
    int continuation_indent; /*!< set to the indentation between the edge of code and continuation lines in spaces */
    int decl_com_ind;        /*!< the column in which comments after declarations should be put */
    int case_brace_indent;   /*!< Indentation level to be used for a '{'
                              * directly following a case label. */
    int c_plus_plus;         /*!< True if we're handling C++ code. */
    int com_ind;             /*!< the column in which comments to the right of code should start */
    int braces_on_struct_decl_line; /*!< when true, brace should be on same line as the struct declaration */
    int braces_on_func_def_line;    /*!< when true, brace should be on same line as the function definition */
    int btype_2;                    /* when true, brace should be on same line as if, while, etc */
    /*!< number of spaces to indent braces from the suround if, while, etc. in -bl * (bype_2 == 0) code */
    int brace_indent;
    /* Means "-o" was specified. */
    int expect_output_file;
    /* true: "char *a", false: "char* a" */
    int pointer_align_right;
    /* true: _("...") is a string, false: it's a function */
    int gettext_strings;
    /* Don't indent the body of an unbraced if, else, etc. */
    int allow_single_line_conditionals;
    /* Align with spaces if indenting with tabs. */
    int align_with_spaces;
    /* Place spaces after { and before } in initializers. */
    int spaces_around_initializers;
    /* Don't align comments to the nearest tabstop. */
    int dont_tab_align_comments;
} user_options_t;

/**
 * This \struct contains information relating to the state of parsing the code.
 * The difference is that the state is saved on # \if and restored on # \else.
 */
typedef struct parser_state {
    struct parser_state *next;
    codes_t last_token;
    codes_t *p_stack;     /* <-- This is the parsers stack. */
    int p_stack_size;     /* <-- Current allocated size.  */
    int *il;              /* <-- This stack stores indentation levels currently allocated size is stored in p_stack_size. */
    rwcodes_t last_rw;    /* <-- If the last token was an ident and is a reserved word, remember the type. */
    int last_rw_depth;    /* <-- Also, remember its depth in parentheses. */
    int *cstk;            /* <-- Used to store case stmt indentation levels.  Currently allocated size is stored in p_stack_size. */
    int tos;              /* <-- Pointer to the top of stack of the p_stack, il and cstk arrays. */
    int box_com;          /* <-- Set to true when we are in a 'boxed' comment.  In that case, the first non-blank char should be lined up with the / in the comment closing delimiter. */
    int cast_mask;        /* <-- Indicates which close parens close off casts. */
    int noncast_mask;     /* <-- A bit for each paren level, set if the open paren was in a context which indicates that this pair of parentheses is not a cast. */
    int sizeof_mask;      /* <-- Indicates which close parens close off sizeof''s */
    int block_init;       /* <-- Set to 1 if inside a block initialization set to 2 if inside an enum declaration and 3 for C99 compound literals. */
    int block_init_level; /* <-- The level of brace nesting in an initialization (0 in an enum decl) */
    int last_nl;          /* <-- This is true if the last thing scanned was a newline */
    int last_saw_nl;      /* <-- This is true if the last non white space scanned was a newline */
    int saw_double_colon; /* <-- Set when we see a ::, reset at first semi-colon or left brace */
    int is_func_ptr_decl; /* <-- Set when we see a decl, followed by lparen and '*'. */
    int broken_at_non_nl; /* <-- True when a line was broken at a place where there was no newline in the input file */
    int in_or_st;         /* <-- Will be true iff there has been a  declarator (e.g. int or char) and no left paren since the last semicolon. When true, a '{' is starting a structure definition or an initialization list */
    int col_1;            /* <-- Set to true if the last token started in column 1 */
    int com_col;          /* <-- This is the column in which the current coment should start */
    int dec_nest;         /* <-- Current nesting level for structure or init */
    int decl_on_line;     /* <-- Set to true if this line of code has part of a declaration on it */
    int i_l_follow;       /* <-- The level in spaces to which ind_level  should be set after the current line is printed */
    bool in_decl;         /* <-- Set to true when we are in a declaration statement.  The processing of braces is then slightly different */
    int in_stmt;          /* <-- Set to 1 while in a stmt */
    int in_parameter_declaration;
    int ind_level;        /* <-- The current indentation level in spaces */
    int ind_stmt;         /* <-- Set to 1 if next line should have an extra indentation level because we are in the middle of a stmt */
    int last_u_d;         /* <-- Set to true after scanning a token which forces a following operator to be unary */
    int p_l_follow;       /* <-- Used to remember how to indent following statement */
    int paren_level;      /* <-- Parenthesization level. used to indent within stmts */
    int paren_depth;      /* <-- Depth of paren nesting anywhere. */
    /**
     * Column positions of paren at each level.  If positive, it contains just the number of characters
     * of code on the line up to and including the right parenthesis character.  If negative, it contains
     * the opposite of the actual level of indentation in characters (that is, the indentation of the
     * line has been added to the number of characters and the sign has been reversed to indicate that
     * this has been done).
     */
    short *paren_indents;   /* <-- Column positions of each paren. */
    int paren_indents_size; /* <-- Currently allocated size.  */
    int pcase;              /* <-- Set to 1 if the current line label is a case.  It is printed differently from a regular label */
    int search_brace;       /* <-- Set to true by parse when it is necessary to buffer up all info up to the start of a stmt after an if, while, etc */
    int use_ff;             /* <-- Set to one if the current line should be terminated with a form feed */
    int want_blank;         /* <-- Set to true when the following token should be prefixed by a blank. (Said prefixing is ignored in some cases.) */
    bb_code_t can_break;    /* <-- Set when a break is ok before the following token (is automatically implied by `want_blank'. */
    int   its_a_keyword;
    int   sizeof_keyword;
    char *procname;       /* <-- The name of the current procedure. */
    char *procname_end;   /* <-- One char past the last one in procname. */
    char *classname;      /* <-- The name of the current C++ class. */
    char *classname_end;  /* <-- One char past the last one in classname. */
    int   just_saw_decl;
    int   matching_brace_on_same_line; /*!<  Set to a value >= 0 if the the current '}' has a matching '{' on the same input line */
} parser_state_t;

typedef struct buf_break_st{
    struct buf_break_st *next;  /* <-- The first possible break point to the right, if any. */
    struct buf_break_st *prev;  /* <-- The first possible break point to the left, if any. */
    int   offset;               /* <-- The break point: the first character in the buffer that will not be put on this line any more. */
    char *corresponds_to;       /* <-- If ptr equals s_code and this equals s_code_corresponds_to, then parser_state_toc->procname is valid. */
    int   target_col;           /* <-- Indentation column if we would break the line here. */
    int   first_level;
    int   level;                /* <-- Number of open '(' and '['. */
    int   col;                  /* <-- The number of columns left of the break point, before the break. */
    int   priority_code_length; /* <-- Used to calculate the priority of this break point: */
    int   priority_newline;     /* <-- Set when in the input file there was a newline at this place. */
    int   priority;
    bb_code_t  priority_code;
} buf_break_st_t;

/* When to make backup files.  Analagous to 'version-control' in Emacs. */
typedef enum {
    unknown,           /* <-- Uninitialized or indeterminate value */
    none,              /* <-- Never make backups. */
    simple,            /* <-- Make simple backups of every file. */
    numbered_existing, /* <-- Make numbered backups of files that already have numbered backups, and simple backups of the others. */
    numbered           /* <-- Make numbered backups of every file. */
} backup_mode_t;

typedef struct {
    backup_mode_t value;
    char *name;
} version_control_values_t;


/* clang-format on */