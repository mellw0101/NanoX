/** @file winio.c

  @author  Melwin Svensson.
  @date    18-5-2025.

 */
#include "../include/c_proto.h"


/* ---------------------------------------------------------- Define's ---------------------------------------------------------- */


#define BRANDING        PACKAGE_STRING
#define ISO8859_CHAR    FALSE
#define ZEROWIDTH_CHAR  (is_zerowidth(text))
#define SHIM            (ISSET(ZERO) && (currmenu == MREPLACEWITH || currmenu == MYESNO) ? 1 : 0)

/* The number of bytes after which to stop painting, to avoid major slowdowns.  Note that this is only for the regex based painting system nano had. */
#define PAINT_LIMIT 2000

#define PROCEED -44

#define INVALID_DIGIT -77


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


/* From where in the relevant line the current row is drawn. */
Ulong from_x = 0;
/* Until where in the relevant line the current row is drawn. */
Ulong till_x = 0;
/* Whether a row's text is narrower than the screen's width. */
bool is_shorter = TRUE;
/* The number of key codes waiting in the keystroke buffer. */
Ulong waiting_codes = 0;
/* The number of keystrokes left before we blank the status bar. */
int countdown = 0;
/* Whether we are in the process of recording a macro. */
bool recording = FALSE;

/* A buffer for the keystrokes that haven't been handled. */
static int *key_buffer = NULL;
/* A pointer pointing at the next keycode in the keystroke buffer. */
static int *nextcodes = NULL;
/* The size of the keystroke buffer; gets doubled whenever needed. */
static Ulong key_capacity = 32;
/* A buffer where the recorded key codes are stored. */
static int *macro_buffer = NULL;
/* The current length of the macro. */
static Ulong macro_length = 0;
/* Where the last burst of recorded keystrokes started. */
static Ulong milestone = 0;
/* Whether the cursor should be shown when waiting for input. */
static bool reveal_cursor = FALSE;
/* Whether to give ncurses some time to get the next code. */
static bool linger_after_escape = FALSE;
/* Points into the expansion string for the current implantation. */
static const char *plants_pointer = NULL;
/* How many digits of a three-digit character code we've eaten. */
static int digit_count = 0;

/* Whether the current line has more text after the displayed part. */
static bool has_more = FALSE;
/* The starting column of the next chunk when softwrapping. */
static Ulong sequel_column = 0;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


/* Determine the sequence number of the given buffer in the circular list. */
static int buffer_number(openfilestruct *buffer) {
  int count = 1;
  while (buffer != startfile) {
    buffer = buffer->prev;
    ++count;
  }
  return count;
}

/* Add the given code to the macro buffer. */
static void add_to_macrobuffer(int code) {
  ++macro_length;
  macro_buffer = xrealloc(macro_buffer, (macro_length * sizeof(int)));
  macro_buffer[macro_length - 1] = code;
}

/**
 * Control character compatibility:
 *
 *   Ctrl-H is Backspace under ASCII, ANSI, VT100, and VT220.
 *   Ctrl-I is Tab under ASCII, ANSI, VT100, VT220, and VT320.
 *   Ctrl-M is Enter under ASCII, ANSI, VT100, VT220, and VT320.
 *   Ctrl-Q is XON under ASCII, ANSI, VT100, VT220, and VT320.
 *   Ctrl-S is XOFF under ASCII, ANSI, VT100, VT220, and VT320.
 *   Ctrl-? is Delete under ASCII, ANSI, VT100, and VT220, but is Backspace under VT320.
 *
 * Note: the VT220 and VT320 also generate Esc [ 3 ~ for Delete.  By default,
 * xterm assumes it's running on a VT320 and generates Ctrl-? for Backspace
 * and Esc [ 3 ~ for Delete.  This causes problems for VT100-derived terminals
 * such as the FreeBSD console, which expect Ctrl-H for Backspace and Ctrl-?
 * for Delete, and on which ncurses translates the VT320 sequences to KEY_DC
 * and [nothing].  We work around this conflict via the REBIND_DELETE flag:
 * if it's set, we assume VT100 compatibility, and VT320 otherwise.
 *
 * Escape sequence compatibility:
 *
 * We support escape sequences for ANSI, VT100, VT220, VT320, the Linux
 * console, the FreeBSD console, the Mach console, xterm, and Terminal,
 * and some for Konsole, rxvt, Eterm, and iTerm2.  Among these sequences,
 * there are some conflicts:
 *
 * - PageUp on FreeBSD console == Tab on ANSI; the latter is omitted.
 *   (Ctrl-I is also Tab on ANSI, which we already support.)
 * - PageDown on FreeBSD console == Center (5) on numeric keypad with
 *   NumLock off on Linux console; the latter is useless and omitted.
 * - F1 on FreeBSD console == the mouse sequence on xterm/rxvt/Eterm;
 *   the latter is omitted.  (Mouse input works only when KEY_MOUSE
 *   is generated on mouse events, not with the raw escape sequence.)
 * - F9 on FreeBSD console == PageDown on Mach console; the former is
 *   omitted.  (Moving the cursor is more important than a function key.)
 * - F10 on FreeBSD console == PageUp on Mach console; the former is
 *   omitted.  (Same as above.)
 */
/* Read in at least one keystroke from the given window and save it (or them) in the keystroke buffer. */
static void read_keys_from(WINDOW *const frame) {
  int   input    = ERR;
  Ulong errcount = 0;
  bool  timed    = FALSE;
  /* Before reading the first keycode, display any pending screen updates. */
  doupdate();
  if (reveal_cursor && (!spotlighted || ISSET(SHOW_CURSOR) || currmenu == MSPELL) && (LINES > 1 || lastmessage <= HUSH)) {
    curs_set(1);
  }
  if (currmenu == MMAIN && (((ISSET(MINIBAR) || ISSET(ZERO) || LINES == 1) && lastmessage > HUSH && lastmessage < ALERT && lastmessage != INFO) || spotlighted)) {
    timed = TRUE;
    halfdelay(ISSET(QUICK_BLANK) ? 8 : 15);
    /* Counteract a side effect of half-delay mode. */
    disable_kb_interrupt();
  }
  /* Read in the first keycode, waiting for it to arrive. */
  while (input == ERR) {
    input = wgetch(frame);
    if (the_window_resized) {
      regenerate_screen();
      input = KEY_WINCH;
    }
    if (timed) {
      timed = FALSE;
      /* Leave half-delay mode. */
      raw();
      if (input == ERR) {
        if (spotlighted || ISSET(ZERO) || LINES == 1) {
          if (ISSET(ZERO) && lastmessage > VACUUM) {
            wredrawln(midwin, (editwinrows - 1), 1);
          }
          lastmessage = VACUUM;
          spotlighted = FALSE;
          update_line_curses(openfile->current, openfile->current_x);
          wnoutrefresh(midwin);
          curs_set(1);
        }
        if (ISSET(MINIBAR) && !ISSET(ZERO) && LINES > 1) {
          minibar();
        }
        as_an_at = TRUE;
        place_the_cursor();
        doupdate();
        continue;
      }
    }
    /* When we've failed to get a keycode millions of times in a row, assume our input source
     * is gone and die gracefully.  We could check if errno is set to EIO ("Input/output error")
     * and die in that case, but it's not always set properly.  Argh. */
    if (input == ERR && ++errcount == 12345678) {
      die(_("Too many errors from stdin\n"));
    }
  }
  curs_set(0);
  /* When there is no keystroke buffer yet, allocate one. */
  if (!key_buffer) {
    reserve_space_for(key_capacity);
  }
  key_buffer[0] = input;
  nextcodes     = key_buffer;
  waiting_codes = 1;
  /* Cancel the highlighting of a search match, if there still is one. */
  if (currmenu == MMAIN) {
    refresh_needed |= spotlighted;
    spotlighted = FALSE;
  }
  /* If we got a SIGWINCH, get out as the frame argument is no longer valid. */
  if (input == KEY_WINCH) {
    return;
  }
  /* Remember where the recording of this keystroke (or burst of them) started. */
  milestone = macro_length;
  /* Read in any remaining key codes using non-blocking input. */
  nodelay(frame, TRUE);
  /* After an ESC, when ncurses does not translate escape sequences, give the keyboard some time to bring the next code to ncurses. */
  if (input == ESC_CODE && (linger_after_escape || ISSET(RAW_SEQUENCES))) {
    napms(20);
  }
  while (TRUE) {
    if (recording) {
      add_to_macrobuffer(input);
    }
    input = wgetch(frame);
    /* If there aren't any more characters, stop reading. */
    if (input == ERR) {
      break;
    }
    /* When the keystroke buffer is full, extend it. */
    if (waiting_codes == key_capacity) {
      reserve_space_for(2 * key_capacity);
    }
    key_buffer[waiting_codes++] = input;
  }
  /* Restore blocking-input mode. */
  nodelay(frame, FALSE);
#ifdef DEBUG
  fprintf(stderr, "\nSequence of hex codes:");
  for (Ulong i=0; i<waiting_codes; ++i) {
    fprintf(stderr, " %3x", key_buffer[i]);
  }
  fprintf(stderr, "\n");
#endif
}

/* Add the given keycode to the front of the keystroke buffer. */
static void put_back(int keycode) {
  /* If there is no room at the head of the keystroke buffer, make room. */
  if (nextcodes == key_buffer) {
    if (waiting_codes == key_capacity) {
      reserve_space_for(2 * key_capacity);
    }
    memmove((key_buffer + 1), key_buffer, (waiting_codes * sizeof(int)));
  }
  else {
    --nextcodes;
  }
  *nextcodes = keycode;
  ++waiting_codes;
}

/* Continue processing an expansion string.  Returns either an error code, a plain character byte, or a placeholder for a command shortcut. */
static int get_code_from_plantation(void) {
  char *closing;
  char *opening;
  Uchar firstbyte;
  int length;
  if (*plants_pointer == '{') {
    closing = strchr((plants_pointer + 1), '}');
    if (!closing) {
      return MISSING_BRACE;
    }
    if (plants_pointer[1] == '{' && plants_pointer[2] == '}') {
      plants_pointer += 3;
      if (*plants_pointer) {
        put_back(MORE_PLANTS);
      }
      return '{';
    }
    free(commandname);
    free(planted_shortcut);
    commandname      = measured_copy((plants_pointer + 1), (closing - plants_pointer - 1));
    planted_shortcut = strtosc(commandname);
    if (!planted_shortcut) {
      return NO_SUCH_FUNCTION;
    }
    plants_pointer = (closing + 1);
    if (*plants_pointer) {
      put_back(MORE_PLANTS);
    }
    return PLANTED_A_COMMAND;
  }
  else {
    opening = _(strchr(plants_pointer, '{'));
    firstbyte = *plants_pointer;
    if (opening) {
      length = (opening - plants_pointer);
      put_back(MORE_PLANTS);
    }
    else {
      length = strlen(plants_pointer);
    }
    for (int index = (length - 1); index > 0; --index) {
      put_back((Uchar)plants_pointer[index]);
    }
    plants_pointer += length;
    return ((firstbyte) ? firstbyte : ERR);
  }
}

/* Return the arrow-key code that corresponds to the given letter.  (This mapping is common to a handful of escape sequences). */
static int arrow_from_ABCD(int letter) {
  /* This is how it was done before.
   *
   * if (letter < 'C') {
   *   return ((letter == 'A') ? KEY_UP : KEY_DOWN);
   * }
   * return ((letter == 'D') ? KEY_LEFT : KEY_RIGHT);
   *
   * And this is the new way we are doing this. */
  return ((letter < 'C') ? ((letter == 'A') ? KEY_UP : KEY_DOWN) : ((letter == 'D') ? KEY_LEFT : KEY_RIGHT));
}

/* Translate a sequence that began with "Esc O" to its corresponding key code. */
static int convert_SS3_sequence(const int *const seq, Ulong length, int *const consumed) {
  switch (seq[0]) {
    case '1': {
      if (length > 3 && seq[1] == ';') {
        *consumed = 4;
        switch (seq[2]) {
          case '2': { /* Shift */
            if ('A' <= seq[3] && seq[3] <= 'D') {
              /* 'Esc O 1 ; 2 A' == Shift-Up    on old Terminal.
               * 'Esc O 1 ; 2 B' == Shift-Down  on old Terminal.
               * 'Esc O 1 ; 2 C' == Shift-Right on old Terminal.
               * 'Esc O 1 ; 2 D' == Shift-Left  on old Terminal. */
              shift_held = TRUE;
              return arrow_from_ABCD(seq[3]);
            }
            break;
          }
          case '5': { /* Ctrl */
            switch (seq[3]) {
              case 'A': { /* 'Esc O 1 ; 5 A' == 'Ctrl-Up' on old Terminal. */ 
                return CONTROL_UP;
              }
              case 'B': { /* 'Esc O 1 ; 5 B' == 'Ctrl-Down' on old Terminal. */ 
                return CONTROL_DOWN;
              }
              case 'C': { /* 'Esc O 1 ; 5 C' == 'Ctrl-Right' on old Terminal. */
                return CONTROL_RIGHT;
              }
              case 'D': { /* 'Esc O 1 ; 5 D' == 'Ctrl-Left' on old Terminal. */
                return CONTROL_LEFT;
              }
            }
            break;
          }
        }
      }
      break;
    }
    case '2':   /* Shift */
    case '3':   /* Alt */
    case '4':   /* Shift+Alt */
    case '5':   /* Ctrl */
    case '6':   /* Shift+Ctrl */
    case '7':   /* Alt+Ctrl */
    case '8': { /* Shift+Alt+Ctrl */
      if (length > 1) {
        *consumed = 2;
        /* Do not accept multiple modifiers. */
        if (seq[0] == '4' || seq[0] > '5') {
          return FOREIGN_SEQUENCE;
        }
        switch (seq[1]) {
          case 'A': { /* Esc O 5 A == Ctrl-Up on Haiku. */
            return CONTROL_UP;
          }
          case 'B': { /* Esc O 5 B == Ctrl-Down on Haiku. */
            return CONTROL_DOWN;
          }
          case 'C': { /* Esc O 5 C == Ctrl-Right on Haiku. */
            return CONTROL_RIGHT;
          }
          case 'D': { /* Esc O 5 D == Ctrl-Left on Haiku. */
            return CONTROL_LEFT;
          }
        }
        /* Translate 'Shift+digit' on the keypad to the digit (Esc O 2 p == Shift-0, ...),
         * 'modifier+operator' to the operator, and 'modifier+Enter' to CR. */
        return (seq[1] - 0x40);
      }
      break;
    }
    case 'A': { /* 'Esc O A' == 'Up'    on 'VT100/VT320'. */
      return KEY_UP;
    }
    case 'B': { /* 'Esc O B' == 'Down'  on 'VT100/VT320'. */
      return KEY_DOWN;
    }
    case 'C': { /* 'Esc O C' == 'Right' on 'VT100/VT320'. */
      return KEY_RIGHT;
    }
    case 'D': { /* 'Esc O D' == 'Left'  on 'VT100/VT320'. */
      return KEY_LEFT;
    }
    case 'F': { /* 'Esc O F' == 'End'   on 'old xterm'. */
      return KEY_END;
    }
    case 'H': { /* 'Esc O H' == 'Home'  on 'old xterm'. */
      return KEY_HOME;
    }
    case 'M': { /* 'Esc O M' == 'Enter' on numeric keypad with NumLock off on 'VT100/VT220/VT320'. */
      return KEY_ENTER;
    }
    case 'P':   /* 'Esc O P' == 'F1' on 'VT100/VT220/VT320/xterm/Mach' console. */
    case 'Q':   /* 'Esc O Q' == 'F2' on 'VT100/VT220/VT320/xterm/Mach' console. */
    case 'R':   /* 'Esc O R' == 'F3' on 'VT100/VT220/VT320/xterm/Mach' console. */
    case 'S': { /* 'Esc O S' == 'F4' on 'VT100/VT220/VT320/xterm/Mach' console. */
      return KEY_F(seq[0] - 'O');
    }
    case 'T':   /* 'Esc O T' == 'F5'  on 'Mach' console. */
    case 'U':   /* 'Esc O U' == 'F6'  on 'Mach' console. */
    case 'V':   /* 'Esc O V' == 'F7'  on 'Mach' console. */
    case 'W':   /* 'Esc O W' == 'F8'  on 'Mach' console. */
    case 'X':   /* 'Esc O X' == 'F9'  on 'Mach' console. */
    case 'Y': { /* 'Esc O Y' == 'F10' on 'Mach' console. */ 
      return KEY_F(seq[0] - 'O');
    }
    case 'a': { /* 'Esc O a' == 'Ctrl-Up'    on 'rxvt/Eterm'. */
      return CONTROL_UP;
    }
    case 'b': { /* 'Esc O b' == 'Ctrl-Down'  on 'rxvt/Eterm'. */
      return CONTROL_DOWN;
    }
    case 'c': { /* 'Esc O c' == 'Ctrl-Right' on 'rxvt/Eterm'. */
      return CONTROL_RIGHT;
    }
    case 'd': { /* 'Esc O d' == 'Ctrl-Left'  on 'rxvt/Eterm'. */
      return CONTROL_LEFT;
    }
    case 'j': { /* 'Esc O j' == '*' on numeric keypad with NumLock off on 'xterm/rxvt/Eterm'. */
      return '*';
    }
    case 'k': { /* 'Esc O k' == '+' on the same. */
      return '+';
    }
    case 'l': { /* 'Esc O l' == ',' on VT100/VT220/VT320. */
      return ',';
    }
    case 'm': { /* 'Esc O m' == '-' on numeric keypad with NumLock off on VTnnn/xterm/rxvt/Eterm. */
      return '-';
    }
    case 'n': { /* 'Esc O n' == Delete (.) on numeric keypad with NumLock off on rxvt/Eterm. */
      return KEY_DC;
    }
    case 'o': { /* 'Esc O o' == '/' on numeric keypad with NumLock off on VTnnn/xterm/rxvt/Eterm. */
      return '/';
    }
    case 'p': { /* 'Esc O p' == Insert (0) on numeric keypad with NumLock off on rxvt/Eterm. */
      return KEY_IC;
    }
    case 'q': { /* 'Esc O q' == End (1) on the same. */
      return KEY_END;
    }
    case 'r': { /* 'Esc O r' == Down (2) on the same. */
      return KEY_DOWN;
    }
    case 's': { /* 'Esc O s' == PageDown (3) on the same. */
      return KEY_NPAGE;
    }
    case 't': { /* 'Esc O t' == Left (4) on the same. */
      return KEY_LEFT;
    }
    case 'v': { /* 'Esc O v' == Right (6) on the same. */
      return KEY_RIGHT;
    }
    case 'w': { /* 'Esc O w' == Home (7) on the same. */
      return KEY_HOME;
    }
    case 'x': { /* 'Esc O x' == Up (8) on the same. */
      return KEY_UP;
    }
    case 'y': { /* 'Esc O y' == PageUp (9) on the same. */
      return KEY_PPAGE;
    }
  }
  return FOREIGN_SEQUENCE;
}

/* Interpret an escape sequence that has the given post-ESC starter byte and with the rest of the sequence still in the keystroke buffer. */
static int parse_escape_sequence(int starter) {
  int consumed = 1;
  int keycode  = 0;
  if (starter == 'O') {
    keycode = convert_SS3_sequence(nextcodes, waiting_codes, &consumed);
  }
  else if (starter == '[') {
    keycode = convert_CSI_sequence(nextcodes, waiting_codes, &consumed);
  }
  /* Skip the consumed sequence elements. */
  waiting_codes -= consumed;
  nextcodes += consumed;
  return keycode;
}

/* For each consecutive call, gather the given digit into a three-digit decimal byte
 * code (from 000 to 255).  Return the assembled code when it is complete, but until
 * then return PROCEED when the given digit is valid, and the given digit itself otherwise. */
static int assemble_byte_code(int keycode) {
  static int byte = 0;
  digit_count++;
  /* The first digit is either 0, 1, or 2 (checked before the call). */
  if (digit_count == 1) {
    byte = (keycode - '0') * 100;
    return PROCEED;
  }
  /* The second digit may be at most 5 if the first was 2. */
  if (digit_count == 2) {
    if (byte < 200 || keycode <= '5') {
      byte += (keycode - '0') * 10;
      return PROCEED;
    }
    else {
      return keycode;
    }
  }
  /* The third digit may be at most 5 if the first two were 2 and 5. */
  if (byte < 250 || keycode <= '5') {
    return (byte + keycode - '0');
  }
  else {
    return keycode;
  }
}

/* Translate a normal ASCII character into its corresponding control code.
 * The following groups of control keystrokes are EQUVILENT:
 * - Ctrl-2 == Ctrl-@ == Ctrl-` == Ctrl-Space
 * - Ctrl-3 == Ctrl-[ == <Esc>
 * - Ctrl-4 == Ctrl-\ == Ctrl-|
 * - Ctrl-5 == Ctrl-]
 * - Ctrl-6 == Ctrl-^ == Ctrl-~
 * - Ctrl-7 == Ctrl-/ == Ctrl-_
 * - Ctrl-8 == Ctrl-? */
static int convert_to_control(int kbinput) {
  if ('@' <= kbinput && kbinput <= '_') {
    return kbinput - '@';
  }
  if ('`' <= kbinput && kbinput <= '~') {
    return kbinput - '`';
  }
  if ('3' <= kbinput && kbinput <= '7') {
    return kbinput - 24;
  }
  if (kbinput == '?' || kbinput == '8') {
    return DEL_CODE;
  }
  if (kbinput == ' ' || kbinput == '2') {
    return 0;
  }
  if (kbinput == '/') {
    return 31;
  }
  return kbinput;
}

/* Extract one keystroke from the input stream.  Translate escape sequences and possibly keypad codes into
 * their corresponding values. Set meta_key to TRUE when appropriate. Supported keypad keystrokes are:
 *   The arrow keys, Insert, Delete, Home, End, PageUp, PageDown, Enter, and Backspace.
 * (Many of them also when modified with Shift, Ctrl, Alt, Shift+Ctrl, or Shift+Alt). The function keys
 * (F1-F12), and the numeric keypad with NumLock off. The function also handles UTF-8 sequences, and
 * converts them to Unicode.  The function returns the corresponding value for the given keystroke. */
static int parse_kbinput(WINDOW *frame) {
  static bool first_escape_was_alone = FALSE;
  static bool last_escape_was_alone  = FALSE;
  static int  escapes = 0;
  int keycode;
  meta_key   = FALSE;
  shift_held = FALSE;
  /* Get one code from the input stream. */
  keycode = get_input(frame);
# ifdef KEY_DEBUG
    // NETLOG("%d\n", keycode);
# endif
  /* Check for '^Bsp'. */
  if (term_env_var) {
    /* First we check if we are running in xterm.  And if so then check if the appropriet key was pressed. */
    if (strcmp(term_env_var, "xterm") == 0) {
      if (ISSET(REBIND_DELETE) && keycode == DEL_CODE) {
        return CONTROL_BSP;
      }
      else if (!ISSET(REBIND_DELETE) && keycode == KEY_BACKSPACE) {
        return CONTROL_BSP;
      }
      else if (keycode == 8) {
        return KEY_BACKSPACE;
      }
    }
    else {
      if (term_program_env_var && (strcmp(term_program_env_var, "vscode") == 0) && (keycode == 23)) {
        return CONTROL_BSP;
      }
      else if (keycode == 8) {
        return CONTROL_BSP;
      }
    }
  }
  /* For an Esc, remember whether the last two arrived by themselves.  Then increment the counter, rolling around on three escapes. */
  if (keycode == ESC_CODE) {
    first_escape_was_alone = last_escape_was_alone;
    last_escape_was_alone  = (waiting_codes == 0);
    if (digit_count > 0) {
      digit_count = 0;
      escapes     = 1;
    }
    else if (++escapes > 2) {
      escapes = (last_escape_was_alone ? 0 : 1);
    }
    return ERR;
  }
  else if (keycode == ERR) {
    return ERR;
  }
  if (!escapes) {
    /* Most key codes in byte range cannot be special keys. */
    if (keycode < 0xFF && keycode != '\t' && keycode != DEL_CODE) {
      return keycode;
    }
  }
  else if (escapes == 1) {
    escapes = 0;
    /* Codes out of ASCII printable range cannot form an escape sequence. */
    if (keycode < 0x20 || 0x7E < keycode) {
      if (keycode == '\t') {
        return SHIFT_TAB;
      }
      else if (keycode == KEY_BACKSPACE || keycode == '\b' || keycode == DEL_CODE) {
        return CONTROL_SHIFT_DELETE;
      }
      else if (0xC0 <= keycode && keycode <= 0xFF && using_utf8()) {
        while (waiting_codes && 0x80 <= nextcodes[0] && nextcodes[0] <= 0xBF) {
          get_input(NULL);
        }
        return FOREIGN_SEQUENCE;
      }
      else if (keycode < 0x20 && !last_escape_was_alone) {
        meta_key = TRUE;
      }
    }
    else if (!waiting_codes || nextcodes[0] == ESC_CODE || (keycode != 'O' && keycode != '[')) {
      if (!shifted_metas) {
        keycode = tolower(keycode);
      }
      meta_key = TRUE;
    }
    else {
      keycode = parse_escape_sequence(keycode);
    }
  }
  else {
    escapes = 0;
    if (keycode == '[' && waiting_codes && (('A' <= nextcodes[0] && nextcodes[0] <= 'D') || ('a' <= nextcodes[0] && nextcodes[0] <= 'd'))) {
      /* An iTerm2/Eterm/rxvt double-escape sequence: Esc Esc [ X for Option+arrow, or Esc Esc [ x for Shift+Alt+arrow. */
      switch (get_input(NULL)) {
        case 'A' : {
          return KEY_HOME;
        }
        case 'B' : {
          return KEY_END;
        }
        case 'C' : {
          return CONTROL_RIGHT;
        }
        case 'D' : {
          return CONTROL_LEFT;
        }
        case 'a' : {
          shift_held = TRUE;
          return KEY_PPAGE;
        }
        case 'b' : {
          shift_held = TRUE;
          return KEY_NPAGE;
        }
        case 'c' : {
          shift_held = TRUE;
          return KEY_HOME;
        }
        case 'd' : {
          shift_held = TRUE;
          return KEY_END;
        }
      }
    }
    else if (waiting_codes && nextcodes[0] != ESC_CODE && (keycode == '[' || keycode == 'O')) {
      keycode  = parse_escape_sequence(keycode);
      meta_key = TRUE;
    }
    else if ('0' <= keycode && (keycode <= '2' || (keycode <= '9' && digit_count > 0))) {
      /* Two escapes followed by one digit: byte sequence mode. */
      int byte = assemble_byte_code(keycode);
      /* If the decimal byte value is not yet complete, return nothing. */
      if (byte == PROCEED) {
        escapes = 2;
        return ERR;
      }
      else if (byte > 0x7F && using_utf8()) {
        /* Convert the code to the corresponding Unicode, and
         * put the second byte back into the keyboard buffer. */
        if (byte < 0xC0) {
          put_back((Uchar)byte);
          return 0xC2;
        }
        else {
          put_back((Uchar)(byte - 0x40));
          return 0xC3;
        }
      }
      else if (byte == '\t' || byte == DEL_CODE) {
        keycode = byte;
      }
      else {
        return byte;
      }
    }
    else if (!digit_count) {
      /* If the first escape arrived alone but not the second, then it is a Meta keystroke; otherwise, it is an "Esc Esc control". */
      if (first_escape_was_alone && !last_escape_was_alone) {
        if (!shifted_metas) {
          keycode = tolower(keycode);
        }
        meta_key = TRUE;
      }
      else {
        keycode = convert_to_control(keycode);
      }
    }
  }
  if (keycode == controlleft) {
    return CONTROL_LEFT;
  }
  else if (keycode == controlright) {
    return CONTROL_RIGHT;
  }
  else if (keycode == controlup) {
    return CONTROL_UP;
  }
  else if (keycode == controldown) {
    return CONTROL_DOWN;
  }
  else if (keycode == controlhome) {
    return CONTROL_HOME;
  }
  else if (keycode == controlend) {
    return CONTROL_END;
  }
  else if (keycode == controldelete) {
    return CONTROL_DELETE;
  }
  else if (keycode == controlshiftdelete) {
    return CONTROL_SHIFT_DELETE;
  }
  else if (keycode == shiftup) {
    shift_held = TRUE;
    return KEY_UP;
  }
  else if (keycode == shiftdown) {
    shift_held = TRUE;
    return KEY_DOWN;
  }
  else if (keycode == shiftcontrolleft) {
    shift_held = TRUE;
    return CONTROL_LEFT;
  }
  else if (keycode == shiftcontrolright) {
    shift_held = TRUE;
    return CONTROL_RIGHT;
  }
  else if (keycode == shiftcontrolup) {
    shift_held = TRUE;
    return CONTROL_UP;
  }
  else if (keycode == shiftcontroldown) {
    shift_held = TRUE;
    return CONTROL_DOWN;
  }
  else if (keycode == shiftcontrolhome) {
    shift_held = TRUE;
    return CONTROL_HOME;
  }
  else if (keycode == shiftcontrolend) {
    shift_held = TRUE;
    return CONTROL_END;
  }
  else if (keycode == altleft) {
    return ALT_LEFT;
  }
  else if (keycode == altright) {
    return ALT_RIGHT;
  }
  else if (keycode == altup) {
    return ALT_UP;
  }
  else if (keycode == altdown) {
    return ALT_DOWN;
  }
  else if (keycode == althome) {
    return ALT_HOME;
  }
  else if (keycode == altend) {
    return ALT_END;
  }
  else if (keycode == altpageup) {
    return ALT_PAGEUP;
  }
  else if (keycode == altpagedown) {
    return ALT_PAGEDOWN;
  }
  else if (keycode == altinsert) {
    return ALT_INSERT;
  }
  else if (keycode == altdelete) {
    return ALT_DELETE;
  }
  else if (keycode == shiftaltleft) {
    shift_held = TRUE;
    return KEY_HOME;
  }
  else if (keycode == shiftaltright) {
    shift_held = TRUE;
    return KEY_END;
  }
  else if (keycode == shiftaltup) {
    shift_held = TRUE;
    return KEY_PPAGE;
  }
  else if (keycode == shiftaltdown) {
    shift_held = TRUE;
    return KEY_NPAGE;
  }
  else if ((KEY_F0 + 24) < keycode && keycode < (KEY_F0 + 64)) {
    return FOREIGN_SEQUENCE;
  }
#ifdef __linux__
  /* When not running under X, check for the bare arrow keys whether Shift/Ctrl/Alt are being held together with them. */
  Uchar modifiers = 6;
  /* Modifiers are: Alt (8), Ctrl (4), Shift (1). */
  if (on_a_vt && !mute_modifiers && ioctl(0, TIOCLINUX, &modifiers) >= 0) {
    /* Is Shift being held? */
    if (modifiers & 0x01) {
      if (keycode == '\t') {
        return SHIFT_TAB;
      }
      if (keycode == KEY_DC && modifiers == 0x01) {
        return SHIFT_DELETE;
        #ifdef KEY_DEBUG
          NETLOG("keycode == KEY_DC && modifiers == 0x01\n");
        #endif
      }
      if (keycode == KEY_DC && modifiers == 0x05) {
        return CONTROL_SHIFT_DELETE;
        #ifdef KEY_DEBUG
          NETLOG("keycode == KEY_DC && modifiers == 0x05\n");
        #endif
      }
      if (!meta_key) {
        shift_held = TRUE;
      }
    }
    /* Is only Alt being held? */
    if (modifiers == 0x08) {
      switch (keycode) {
        case KEY_UP : {
          return ALT_UP;
        }
        case KEY_DOWN : {
          return ALT_DOWN;
        }
        case KEY_HOME : {
          return ALT_HOME;
        }
        case KEY_END : {
          return ALT_END;
        }
        case KEY_PPAGE : {
          return ALT_PAGEUP;
        }
        case KEY_NPAGE : {
          return ALT_PAGEDOWN;
        }
        case KEY_DC : {
          return ALT_DELETE;
        }
        case KEY_IC : {
          return ALT_INSERT;
        }
      }
    }
    /* Is Ctrl being held? */
    if (modifiers & 0x04) {
      switch (keycode) {
        case KEY_UP : {
          return CONTROL_UP;
        }
        case KEY_DOWN : {
          return CONTROL_DOWN;
        }
        case KEY_LEFT : {
          return CONTROL_LEFT;
        }
        case KEY_RIGHT : {
          return CONTROL_RIGHT;
        }
        case KEY_HOME : {
          return CONTROL_HOME;
        }
        case KEY_END : {
          return CONTROL_END;
        }
        case KEY_DC : {
          return CONTROL_DELETE;
        }
        case KEY_BACKSPACE : /* ADDED: TESTING */ {
          return CONTROL_BSP;
        }
      }
    }
    /* Are both Shift and Alt being held? */
    if ((modifiers & 0x09) == 0x09) {
      switch (keycode) {
        case KEY_UP : {
          return KEY_PPAGE;
        }
        case KEY_DOWN : {
          return KEY_NPAGE;
        }
        case KEY_LEFT : {
          return KEY_HOME;
        }
        case KEY_RIGHT : {
          return KEY_END;
        }
      }
    }
  }
#endif
  /* Spurious codes from VTE -- see https://sv.gnu.org/bugs/?64578. */
  if (keycode == mousefocusin || keycode == mousefocusout) {
    return ERR;
  }
  switch (keycode) {
    case KEY_SLEFT : {
      shift_held = TRUE;
      return KEY_LEFT;
    }
    case KEY_SRIGHT : {
      shift_held = TRUE;
      return KEY_RIGHT;
    }
#ifdef KEY_SR
#  ifdef KEY_SUP    /* Ncurses doesn't know Shift+Up. */
    case KEY_SUP :
#  endif
    case KEY_SR : { /* Scroll backward, on Xfce4-terminal. */
      shift_held = TRUE;
      return KEY_UP;
    }
#endif
#ifdef KEY_SF
#  ifdef KEY_SDOWN  /* Ncurses doesn't know Shift+Down. */
    case KEY_SDOWN :
#  endif
    case KEY_SF : { /* Scroll forward, on Xfce4-terminal. */
      shift_held = TRUE;
      return KEY_DOWN;
    }
#endif
#ifdef KEY_SHOME /* HP-UX 10-11 doesn't know Shift+Home. */
    case KEY_SHOME :
#endif
    case SHIFT_HOME : {
      shift_held = TRUE;
      _FALLTHROUGH;
    }
    case KEY_A1 : { /* Home (7) on keypad with NumLock off. */
      return KEY_HOME;
    }
#ifdef KEY_SEND     /* HP-UX 10-11 doesn't know Shift+End. */
    case KEY_SEND :
#endif
    case SHIFT_END : {
      shift_held = TRUE;
      _FALLTHROUGH;
    }
    case KEY_C1 : { /* End (1) on keypad with NumLock off. */
      return KEY_END;
    }
#ifdef KEY_EOL
    case KEY_EOL : { /* Ctrl+End on rxvt-unicode. */
      return CONTROL_END;
    }
#endif
#ifdef KEY_SPREVIOUS
    case KEY_SPREVIOUS :
#endif
    case SHIFT_PAGEUP : { /* Fake key, from Shift+Alt+Up. */
      shift_held = TRUE;
      _FALLTHROUGH;
    }
    case KEY_A3 : {       /* PageUp (9) on keypad with NumLock off. */
      return KEY_PPAGE;
    }
#ifdef KEY_SNEXT
    case KEY_SNEXT :
#endif
    case SHIFT_PAGEDOWN : { /* Fake key, from Shift+Alt+Down. */
      shift_held = TRUE;
      _FALLTHROUGH;
    }
    case KEY_C3 : {         /* PageDown (3) on keypad with NumLock off. */
      return KEY_NPAGE;
    }
    /* When requested, swap meanings of keycodes for <Bsp> and <Del>. */
    case DEL_CODE :
    case KEY_BACKSPACE : {
      return (ISSET(REBIND_DELETE) ? KEY_DC : KEY_BACKSPACE);
    }
    case KEY_DC : {
      return (ISSET(REBIND_DELETE) ? KEY_BACKSPACE : KEY_DC);
    }
    case KEY_SDC : {
      return SHIFT_DELETE;
    }
    case KEY_SCANCEL : {
      return KEY_CANCEL;
    }
    case KEY_SSUSPEND :
    case KEY_SUSPEND : {
      return 0x1A; /* The ASCII code for Ctrl+Z. */
    }
    case KEY_BTAB : {
      return SHIFT_TAB;
    }
    case KEY_SBEG :
    case KEY_BEG :
    case KEY_B2 : /* Center (5) on keypad with NumLock off. */
#ifdef PDCURSES   /* TODO: (PDCURSES) - Find out if this can be used. */
    case KEY_SHIFT_L :
    case KEY_SHIFT_R :
    case KEY_CONTROL_L :
    case KEY_CONTROL_R :
    case KEY_ALT_L :
    case KEY_ALT_R :
#endif
#ifdef KEY_RESIZE /* SunOS 5.7-5.9 doesn't know KEY_RESIZE. */
    case KEY_RESIZE :
#endif
    case KEY_FRESH : {
      return ERR; /* Ignore this keystroke. */
    }
  }
  return keycode;
}

/* For each consecutive call, gather the given symbol into a Unicode code point.  When it's complete
 * (with six digits, or when Space or Enter is typed), return the assembled code. Until then, return
 * PROCEED when the symbol is valid, or an error code for anything other than hexadecimal, Space, and Enter. */
static long assemble_unicode(int symbol) {
  static long unicode = 0;
  static int  digits  = 0;
  int outcome = PROCEED;
  char partial[7];
  if ('0' <= symbol && symbol <= '9') {
    unicode = ((unicode << 4) + symbol - '0');
  }
  else if ('a' <= (symbol | 0x20) && (symbol | 0x20) <= 'f') {
    unicode = ((unicode << 4) + (symbol | 0x20) - 'a' + 10);
  }
  else if (symbol == '\r' || symbol == ' ') {
    outcome = unicode;
  }
  else {
    outcome = INVALID_DIGIT;
  }
  /* If also the sixth digit was a valid hexadecimal value, then the Unicode sequence is complete, so return it (when it's valid). */
  if (++digits == 6 && outcome == PROCEED) {
    outcome = (unicode < 0x110000) ? unicode : INVALID_DIGIT;
  }
  /* Show feedback only when editing, not when at a prompt. */
  if (outcome == PROCEED && currmenu == MMAIN) {
    // char partial[7] = "      ";
    memcpy(partial, "      ", 7);
    sprintf((partial + 6 - digits), "%0*lX", digits, unicode);
    /* TRANSLATORS: This is shown while a six-digit hexadecimal Unicode character code (%s) is being typed in. */
    statusline(INFO, _("Unicode Input: %s"), partial);
  }
  /* If we have an end result, reset the value and the counter. */
  if (outcome != PROCEED) {
    unicode = 0;
    digits  = 0;
  }
  return outcome;
}

/* Read in one control character (or an iTerm/Eterm/rxvt double Escape), or convert a series of six digits into a Unicode codepoint.
 * Return in count either 1 (for a control character or the first byte of a multibyte sequence), or 2 (for an iTerm/Eterm/rxvt double Escape). */
static int *parse_verbatim_kbinput(WINDOW *const frame, Ulong *const count) {
  int keycode;
  int *yield;
  long unicode;
  char multibyte[MB_CUR_MAX];
  reveal_cursor = TRUE;
  keycode       = get_input(frame);
  /* When the window was resized, abort and return nothing. */
  if (keycode == KEY_WINCH) {
    *count = 999;
    return NULL;
  }
  /* Reserve ample space for the possible result. */
  yield = xmalloc(6 * sizeof(int));
  /* If the key code is a hexadecimal digit, commence Unicode input. */
  if (using_utf8() && isxdigit(keycode)) {
    unicode = assemble_unicode(keycode);
    reveal_cursor = FALSE;
    /* Gather at most six hexadecimal digits. */
    while (unicode == PROCEED) {
      keycode = get_input(frame);
      unicode = assemble_unicode(keycode);
    }
    if (keycode == KEY_WINCH) {
      *count = 999;
      free(yield);
      return NULL;
    }
    /* For an invalid keystroke, discard its possible continuation bytes. */
    if (unicode == INVALID_DIGIT) {
      if (keycode == ESC_CODE && waiting_codes) {
        get_input(NULL);
        while (waiting_codes && 0x1F < nextcodes[0] && nextcodes[0] < 0x40) {
          get_input(NULL);
        }
        if (waiting_codes && 0x3F < nextcodes[0] && nextcodes[0] < 0x7F) {
          get_input(NULL);
        }
      }
      else if (0xC0 <= keycode && keycode <= 0xFF) {
        while (waiting_codes && 0x7F < nextcodes[0] && nextcodes[0] < 0xC0) {
          get_input(NULL);
        }
      }
    }
    /* Convert the Unicode value to a multibyte sequence. */
    *count = wctomb(multibyte, unicode);
    if (*count > MAXCHARLEN) {
      *count = 0;
    }
    /* Change the multibyte character into a series of integers. */
    for (Ulong i=0; i<(*count); ++i) {
      yield[i] = (int)multibyte[i];
    }
    return yield;
  }
  yield[0] = keycode;
  /* In case of an escape, take also a second code, as it might be another
   * escape (on iTerm2/rxvt) or a control code (for M-Bsp and M-Enter). */
  if (keycode == ESC_CODE && waiting_codes) {
    yield[1] = get_input(NULL);
    *count   = 2;
  }
  return yield;
}

/* ----------------------------- Curses ----------------------------- */

static void show_state_at_curses(WINDOW *const window) {
  ASSERT(window);
  waddnstr(window, (ISSET(AUTOINDENT) ? "I" : " "), 1);
  waddnstr(window, (openfile->mark ? "M" : " "), 1);
  waddnstr(window, (ISSET(BREAK_LONG_LINES) ? "L" : " "), 1);
  waddnstr(window, (recording ? "R" : " "), 1);
  waddnstr(window, (ISSET(SOFTWRAP) ? "S" : " "), 1);
}

_UNUSED static inline int mvwaddnstr_curses(WINDOW *const window, int row, int column, const char *const restrict string, int len) {
  return mvwaddnstr(window, row, column, string, len);
}

static void place_the_cursor_for_internal(openfilestruct *const file, int cols, Ulong *const out_column) {
  ASSERT(file);
  ASSERT(out_column);
  long row = 0;
  Ulong column = xplustabs_for(file);
  linestruct *line;
  Ulong leftedge;
  if (ISSET(SOFTWRAP)) {
    line = file->filetop;
    row -= chunk_for(cols, file->firstcolumn, file->edittop);
    /* Calculate how meny rows from edittop the current line is. */
    while (line && line != file->edittop) {
      row += (1 + extra_chunks_in(cols, line));
      CLIST_ADV_NEXT(line);
    }
    /* Add the number of wraps in the current line before the cursor. */
    row    += get_chunk_and_edge(cols, column, file->current, &leftedge);
    column -= leftedge;
  }
  else {
    row     = (file->current->lineno - file->edittop->lineno);
    column -= get_page_start(column, cols);
  }
  file->cursor_row = row;
  *out_column = column;
}

/* Draw a `scroll bar` on the righthand side of the edit window. */
static void draw_scrollbar_curses(void) {
  int fromline;
  int totallines;
  int coveredlines;
  linestruct *line;
  int lowest;
  int highest;
  int extras;
  if (!IN_CURSES_CTX) {
    return;
  }
  fromline     = (openfile->edittop->lineno - 1);
  totallines   = openfile->filebot->lineno;
  coveredlines = editwinrows;
  if (ISSET(SOFTWRAP)) {
    line = openfile->edittop;
    extras = (extra_chunks_in(editwincols, line) - chunk_for(editwincols, openfile->firstcolumn, line));
    while ((line->lineno + extras) < (fromline + editwinrows) && line->next) {
      line = line->next;
      extras += extra_chunks_in(editwincols, line);
    }
    coveredlines = (line->lineno - fromline);
  }
  lowest  = ((fromline * editwinrows) / totallines);
  highest = (lowest + (editwinrows * coveredlines) / totallines);
  if (editwinrows > totallines && !ISSET(SOFTWRAP)) {
    highest = editwinrows;
  }
  for (int row=0; row<editwinrows; ++row) {
    bardata[row] = (' ' | interface_color_pair[SCROLL_BAR] | ((row < lowest || row > highest) ? A_NORMAL : A_REVERSE));
    mvwaddch(midwin, row, (COLS - 1), bardata[row]);
  }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


/* Start or stop the recording of keystrokes. */
void record_macro(void) { 
  recording = !recording;
  if (recording) {
    macro_length = 0;
    statusline(REMARK, _("Recording a macro..."));
  }
  else {
    /* Snip the keystroke that invoked this function. */
    macro_length = milestone;
    statusline(REMARK, _("Stopped recording"));
  }
  if (ISSET(STATEFLAGS)) {
    titlebar(NULL);
  }
}

/* Copy the stored sequence of codes into the regular key buffer, so they will be "executed" again. */
void run_macro(void) {
  if (recording) {
    statusline(AHEM, _("Cannot run macro while recording"));
    macro_length = milestone;
    return;
  }
  if (!macro_length) {
    statusline(AHEM, _("Macro is empty"));
    return;
  }
  if (macro_length > key_capacity) {
    reserve_space_for(macro_length);
  }
  for (Ulong i = 0; i < macro_length; ++i) {
    key_buffer[i] = macro_buffer[i];
  }
  waiting_codes  = macro_length;
  nextcodes      = key_buffer;
  mute_modifiers = TRUE;
}

/* Allocate the requested space for the keystroke. */
void reserve_space_for(Ulong newsize) {
  if (newsize < key_capacity) {
    die(_("Too much input at once\n"));
  }
  key_buffer   = xrealloc(key_buffer, (newsize * sizeof(int)));
  nextcodes    = key_buffer;
  key_capacity = newsize;
}

/* Set up the given expansion string to be ingested by the keyboard routines. */
void implant(const char *const string) {
  plants_pointer = string;
  put_back(MORE_PLANTS);
  mute_modifiers = TRUE;
}

/* Return one code from the keystroke buffer.  If the buffer is empty but frame is given, first read more codes from the keyboard. */
int get_input(WINDOW *const frame) {
  if (waiting_codes) {
    spotlighted = FALSE;
  }
  else if (frame) {
    read_keys_from(frame);
  }
  if (waiting_codes) {
    --waiting_codes;
    if (*nextcodes == MORE_PLANTS) {
      ++nextcodes;
      return get_code_from_plantation();
    }
    else {
      return *(nextcodes++);
    }
  }
  else {
    return ERR;
  }
}

/* Translate a sequence that began with "Esc [" to its corresponding key code. */
int convert_CSI_sequence(const int *const seq, Ulong length, int *const consumed) {
  if (seq[0] < '9' && length > 1) {
    *consumed = 2;
  }
  switch (seq[0]) {
    case '1': {
      /* Esc [ 1 ~ == Home on VT320/Linux console. */
      if (length > 1 && seq[1] == '~') {
        return KEY_HOME;
      }
      else if (length > 2 && seq[2] == '~') {
        *consumed = 3;
        switch (seq[1]) {
          case '1':   /* 'Esc [ 1 1 ~' == 'F1' on 'rxvt/Eterm'. */
          case '2':   /* 'Esc [ 1 2 ~' == 'F2' on 'rxvt/Eterm'. */
          case '3':   /* 'Esc [ 1 3 ~' == 'F3' on 'rxvt/Eterm'. */
          case '4':   /* 'Esc [ 1 4 ~' == 'F4' on 'rxvt/Eterm'. */
          case '5': { /* 'Esc [ 1 5 ~' == 'F5' on 'xterm/rxvt/Eterm'. */
            return KEY_F(seq[1] - '0');
          }
          case '7':   /* 'Esc [ 1 7 ~' == 'F6' on 'VT220/VT320/Linux-console/xterm/rxvt/Eterm'. */
          case '8':   /* 'Esc [ 1 8 ~' == 'F7' on 'VT220/VT320/Linux-console/xterm/rxvt/Eterm'. */
          case '9': { /* 'Esc [ 1 9 ~' == 'F8' on 'VT220/VT320/Linux-console/xterm/rxvt/Eterm'. */
            return KEY_F(seq[1] - '1');
          }
        }
      }
      else if (length > 3 && seq[1] == ';') {
        *consumed = 4;
        switch (seq[2]) {
          case '2': { /* Shift */
            switch (seq[3]) {
              case 'A':   /* 'Esc [ 1 ; 2 A' == 'Shift-Up'    on 'xterm'. */
              case 'B':   /* 'Esc [ 1 ; 2 B' == 'Shift-Down'  on 'xterm'. */
              case 'C':   /* 'Esc [ 1 ; 2 C' == 'Shift-Right' on 'xterm'. */
              case 'D': { /* 'Esc [ 1 ; 2 D' == 'Shift-Left'  on 'xterm'. */
                shift_held = TRUE;
                return arrow_from_ABCD(seq[3]);
              }
              case 'F': { /* 'Esc [ 1 ; 2 F' == 'Shift-End'   on 'xterm'. */
                return SHIFT_END;
              }
              case 'H': { /* 'Esc [ 1 ; 2 H' == 'Shift-Home'  on 'xterm'. */
                return SHIFT_HOME;
              }
            }
            break;
          }
          case '9':   /* To accommodate iTerm2 in "xterm mode". */
          case '3': { /* Alt */
            switch (seq[3]) {
              case 'A': { /* 'Esc [ 1 ; 3 A' == 'Alt-Up'    on 'xterm'. */
                return ALT_UP;
              }
              case 'B': { /* 'Esc [ 1 ; 3 B' == 'Alt-Down'  on 'xterm'. */
                return ALT_DOWN;
              }
              case 'C': { /* 'Esc [ 1 ; 3 C' == 'Alt-Right' on 'xterm'. */
                return ALT_RIGHT;
              }
              case 'D': { /* 'Esc [ 1 ; 3 D' == 'Alt-Left'  on 'xterm'. */
                return ALT_LEFT;
              }
              case 'F': { /* 'Esc [ 1 ; 3 F' == 'Alt-End'   on 'xterm'. */
                return ALT_END;
              }
              case 'H': { /* 'Esc [ 1 ; 3 H' == 'Alt-Home'  on 'xterm'. */
                return ALT_HOME;
              }
            }
            break;
          }
          case '4': { /* When the arrow keys are held together with Shift+Meta, act as if they are Home/End/PgUp/PgDown with Shift. */
            switch (seq[3]) {
              case 'A': { /* 'Esc [ 1 ; 4 A' == 'Shift-Alt-Up'    on 'xterm'. */
                return SHIFT_PAGEUP;
              }
              case 'B': { /* 'Esc [ 1 ; 4 B' == 'Shift-Alt-Down'  on 'xterm'. */
                return SHIFT_PAGEDOWN;
              }
              case 'C': { /* 'Esc [ 1 ; 4 C' == 'Shift-Alt-Right' on 'xterm'. */
                return SHIFT_END;
              }
              case 'D': { /* 'Esc [ 1 ; 4 D' == 'Shift-Alt-Left'  on 'xterm'. */
                return SHIFT_HOME;
              }
            }
            break;
          }
          case '5': {
            switch (seq[3]) {
              case 'A': { /* 'Esc [ 1 ; 5 A' == 'Ctrl-Up'    on 'xterm'. */
                return CONTROL_UP;
              }
              case 'B': { /* 'Esc [ 1 ; 5 B' == 'Ctrl-Down'  on 'xterm'. */
                return CONTROL_DOWN;
              }
              case 'C': { /* 'Esc [ 1 ; 5 C' == 'Ctrl-Right' on 'xterm'. */
                return CONTROL_RIGHT;
              }
              case 'D': { /* 'Esc [ 1 ; 5 D' == 'Ctrl-Left'  on 'xterm'. */
                return CONTROL_LEFT;
              }
              case 'F': { /* 'Esc [ 1 ; 5 F' == 'Ctrl-End'   on 'xterm'. */
                return CONTROL_END;
              }
              case 'H': { /* 'Esc [ 1 ; 5 H' == 'Ctrl-Home'  on 'xterm'. */
                return CONTROL_HOME;
              }
            }
            break;
          }
          case '6': {
            switch (seq[3]) {
              case 'A' : { /* 'Esc [ 1 ; 6 A' == 'Shift-Ctrl-Up'    on 'xterm'. */
                return shiftcontrolup;
              }
              case 'B' : { /* 'Esc [ 1 ; 6 B' == 'Shift-Ctrl-Down'  on 'xterm'. */
                return shiftcontroldown;
              }
              case 'C' : { /* 'Esc [ 1 ; 6 C' == 'Shift-Ctrl-Right' on 'xterm'. */
                return shiftcontrolright;
              }
              case 'D' : { /* 'Esc [ 1 ; 6 D' == 'Shift-Ctrl-Left'  on 'xterm'. */
                return shiftcontrolleft;
              }
              case 'F' : { /* 'Esc [ 1 ; 6 F' == 'Shift-Ctrl-End'   on 'xterm'. */
                return shiftcontrolend;
              }
              case 'H' : { /* 'Esc [ 1 ; 6 H' == 'Shift-Ctrl-Home'  on 'xterm'. */
                return shiftcontrolhome;
              }
            }
            break;
          }
        }
      }
      /* 'Esc [ 1 n ; 2 ~' == 'F17...F20' on some terminals. */
      else if (length > 4 && seq[2] == ';' && seq[4] == '~') {
        *consumed = 5;
      }
      break;
    }
    case '2': {
      if (length > 2 && seq[2] == '~') {
        *consumed = 3;
        switch (seq[1]) {
          case '0': { /* Esc [ 2 0 ~ == F9 on VT220/VT320/Linux console/xterm/rxvt/Eterm. */
            return KEY_F(9);
          }
          case '1': { /* Esc [ 2 1 ~ == F10 on the same. */
            return KEY_F(10);
          }
          case '3': { /* Esc [ 2 3 ~ == F11 on the same. */
            return KEY_F(11);
          }
          case '4': { /* Esc [ 2 4 ~ == F12 on the same. */
            return KEY_F(12);
          }
          case '5': { /* Esc [ 2 5 ~ == F13 on the same. */
            return KEY_F(13);
          }
          case '6': { /* Esc [ 2 6 ~ == F14 on the same. */
            return KEY_F(14);
          }
          case '8': { /* Esc [ 2 8 ~ == F15 on the same. */
            return KEY_F(15);
          }
          case '9': { /* Esc [ 2 9 ~ == F16 on the same. */
            return KEY_F(16);
          }
        }
      }
      /* Esc [ 2 ~ == Insert on VT220/VT320/Linux console/xterm/Terminal. */
      else if (length > 1 && seq[1] == '~') {
        return KEY_IC;
      }
      /* Esc [ 2 ; x ~ == modified Insert on xterm. */
      else if (length > 3 && seq[1] == ';' && seq[3] == '~') {
        *consumed = 4;
        if (seq[2] == '3') {
          return ALT_INSERT;
        }
      }
      /* Esc [ 2 n ; 2 ~ == F21...F24 on some terminals. */
      else if (length > 4 && seq[2] == ';' && seq[4] == '~') {
        *consumed = 5;
      }
      /* Esc [ 2 0 0 ~ == start of a bracketed paste, Esc [ 2 0 1 ~ == end of a bracketed paste. */
      else if (length > 3 && seq[1] == '0' && seq[3] == '~') {
        *consumed = 4;
        if (seq[2] == '0') {
          bracketed_paste = TRUE;
          return BRACKETED_PASTE_MARKER;
        }
        else if (seq[2] == '1') {
          bracketed_paste = FALSE;
          return BRACKETED_PASTE_MARKER;
        }
      }
      /* When invalid, assume it's a truncated end-of-paste sequence,
       * in order to avoid a hang -- https://sv.gnu.org/bugs/?64996. */
      else {
        bracketed_paste = FALSE;
        *consumed       = length;
        return ERR;
      }
      break;
    }
    case '3': { /* Esc [ 3 ~ == Delete on VT220/VT320/Linux console/xterm/Terminal. */
      if (length > 1 && seq[1] == '~') {
        return KEY_DC;
      }
      if (length > 3 && seq[1] == ';' && seq[3] == '~') {
        *consumed = 4;
        /* Esc [ 3 ; 2 ~ == Shift-Delete on xterm/Terminal. */
        if (seq[2] == '2') {
          return SHIFT_DELETE;
        }
        /* Esc [ 3 ; 3 ~ == Alt-Delete on xterm/rxvt/Eterm/Terminal. */
        if (seq[2] == '3') {
          return ALT_DELETE;
        }
        /* Esc [ 3 ; 5 ~ == Ctrl-Delete on xterm. */
        if (seq[2] == '5') {
          return CONTROL_DELETE;
        }
        /* Esc [ 3 ; 6 ~ == Ctrl-Shift-Delete on xterm. */
        if (seq[2] == '6') {
          return controlshiftdelete;
        }
      }
      /* Esc [ 3 $ == Shift-Delete on urxvt. */
      if (length > 1 && seq[1] == '$') {
        return SHIFT_DELETE;
      }
      /* Esc [ 3 ^ == Ctrl-Delete on urxvt. */
      if (length > 1 && seq[1] == '^') {
        return CONTROL_DELETE;
      }
      /* Esc [ 3 @ == Ctrl-Shift-Delete on urxvt. */
      if (length > 1 && seq[1] == '@') {
        return controlshiftdelete;
      }
      /* Esc [ 3 n ~ == F17...F20 on some terminals. */
      if (length > 2 && seq[2] == '~') {
        *consumed = 3;
      }
      break;
    }
    case '4': { /* Esc [ 4 ~ == End on VT220/VT320/Linux console/xterm. */
      if (length > 1 && seq[1] == '~') {
        return KEY_END;
      }
      break;
    }
    case '5': { /* Esc [ 5 ~ == PageUp on VT220/VT320/Linux console/xterm/Eterm/urxvt/Terminal */
      if (length > 1 && seq[1] == '~') {
        return KEY_PPAGE;
      }
      else if (length > 3 && seq[1] == ';' && seq[3] == '~') {
        *consumed = 4;
        if (seq[2] == '2') {
          return shiftaltup;
        }
        if (seq[2] == '3') {
          return ALT_PAGEUP;
        }
      }
      break;
    }
    case '6': { /* Esc [ 6 ~ == PageDown on VT220/VT320/Linux console/xterm/Eterm/urxvt/Terminal. */
      if (length > 1 && seq[1] == '~') {
        return KEY_NPAGE;
      }
      else if (length > 3 && seq[1] == ';' && seq[3] == '~') {
        *consumed = 4;
        if (seq[2] == '2') {
          return shiftaltdown;
        }
        if (seq[2] == '3') {
          return ALT_PAGEDOWN;
        }
      }
      break;
    }
    case '7': {
      if (length > 1 && seq[1] == '~') { /* Esc [ 7 ~ == Home on Eterm/rxvt; */
        return KEY_HOME;
      }
      else if (length > 1 && seq[1] == '$') { /* Esc [ 7 $ == Shift-Home on Eterm/rxvt; */
        return SHIFT_HOME;
      }
      else if (length > 1 && seq[1] == '^') { /* Esc [ 7 ^ == Control-Home on Eterm/rxvt; */
        return CONTROL_HOME;
      }
      else if (length > 1 && seq[1] == '@') { /* Esc [ 7 @ == Shift-Control-Home on same. */
        return shiftcontrolhome;
      }
      break;
    }
    case '8': {
      /* Esc [ 8 ~ == End on Eterm/rxvt; */
      if (length > 1 && seq[1] == '~') {
        return KEY_END;
      }
      /* Esc [ 8 $ == Shift-End on Eterm/rxvt; */
      else if (length > 1 && seq[1] == '$') {
        return SHIFT_END;
      }
      /* Esc [ 8 ^ == Control-End on Eterm/rxvt; */
      else if (length > 1 && seq[1] == '^') {
        return CONTROL_END;
      }
      /* Esc [ 8 @ == Shift-Control-End on same. */
      else if (length > 1 && seq[1] == '@') {
        return shiftcontrolend;
      }
      break;
    }
    case '9': { /* Esc [ 9 == Delete on Mach console. */
      return KEY_DC;
    }
    case '@': { /* Esc [ @ == Insert on Mach console. */
      return KEY_IC;
    }
    case 'A':   /* Esc [ A == Up on ANSI/VT220/Linux console/
                  * FreeBSD console/Mach console/xterm/Eterm/
                  * urxvt/Gnome and Xfce Terminal. */
    case 'B':   /* Esc [ B == Down on the same. */
    case 'C':   /* Esc [ C == Right on the same. */
    case 'D': { /* Esc [ D == Left on the same. */
      return arrow_from_ABCD(seq[0]);
    }
    case 'F': { /* Esc [ F == End on FreeBSD console/Eterm. */
      return KEY_END;
    }
    case 'G': { /* Esc [ G == PageDown on FreeBSD console. */
      return KEY_NPAGE;
    }
    case 'H': { /* Esc [ H == Home on ANSI/VT220/FreeBSD console/Mach console/Eterm. */
      return KEY_HOME;
    }
    case 'I': { /* Esc [ I == PageUp on FreeBSD console. */
      return KEY_PPAGE;
    }
    case 'L': { /* Esc [ L == Insert on ANSI/FreeBSD console. */
      return KEY_IC;
    }
    case 'M':   /* Esc [ M == F1 on FreeBSD console. */
    case 'N':   /* Esc [ N == F2 on FreeBSD console. */
    case 'O':   /* Esc [ O == F3 on FreeBSD console. */
    case 'P':   /* Esc [ P == F4 on FreeBSD console. */
    case 'Q':   /* Esc [ Q == F5 on FreeBSD console. */
    case 'R':   /* Esc [ R == F6 on FreeBSD console. */
    case 'S':   /* Esc [ S == F7 on FreeBSD console. */
    case 'T': { /* Esc [ T == F8 on FreeBSD console. */
      return KEY_F(seq[0] - 'L');
    }
    case 'U': { /* Esc [ U == PageDown on Mach console. */
      return KEY_NPAGE;
    }
    case 'V': { /* Esc [ V == PageUp on Mach console. */
      return KEY_PPAGE;
    }
    case 'W': { /* Esc [ W == F11 on FreeBSD console. */
      return KEY_F(11);
    }
    case 'X': { /* Esc [ X == F12 on FreeBSD console. */
      return KEY_F(12);
    }
    case 'Y': { /* Esc [ Y == End on Mach console. */
      return KEY_END;
    }
    case 'Z': { /* Esc [ Z == Shift-Tab on ANSI/Linux console/ FreeBSD console/xterm/rxvt/Terminal. */
      return SHIFT_TAB;
    }
    case 'a':   /* Esc [ a == Shift-Up on rxvt/Eterm. */
    case 'b':   /* Esc [ b == Shift-Down on rxvt/Eterm. */
    case 'c':   /* Esc [ c == Shift-Right on rxvt/Eterm. */
    case 'd': { /* Esc [ d == Shift-Left on rxvt/Eterm. */
      shift_held = TRUE;
      return arrow_from_ABCD(seq[0] - 0x20);
    }
    case '[': {
      if (length > 1) {
        *consumed = 2;
        if ('@' < seq[1] && seq[1] < 'F') {
          /* 'Esc [ [ A' == 'F1' on 'Linux-console'.
           * 'Esc [ [ B' == 'F2' on 'Linux-console'.
           * 'Esc [ [ C' == 'F3' on 'Linux-console'.
           * 'Esc [ [ D' == 'F4' on 'Linux-console'.
           * 'Esc [ [ E' == 'F5' on 'Linux-console'. */
          return KEY_F(seq[1] - '@');
        }
      }
      break;
    }
  }
  return FOREIGN_SEQUENCE;
}

/* Read in a single keystroke, ignoring any that are invalid. */
int get_kbinput(WINDOW *const frame, bool showcursor) {
  int kbinput = ERR;
  if (IN_CURSES_CTX) {
    ASSERT(frame);
    reveal_cursor = showcursor;
    /* Extract one keystroke from the input stream. */
    while (kbinput == ERR) {
      kbinput = parse_kbinput(frame);
      #ifdef KEY_DEBUG
      // NETLOG("kbinput: %d\n", kbinput);
      #endif
    }
    /* If we read from the edit window, blank the status bar when it's time. */
    if (frame == midwin) {
      blank_it_when_expired();
    }
  }
  return kbinput;
}

/* Read in one control code, one character byte, or the leading escapes of
 * an escape sequence, and return the resulting number of bytes in count. */
char *get_verbatim_kbinput(WINDOW *const frame, Ulong *const count) {
  char *bytes = xmalloc(MAXCHARLEN + 2);
  int  *input;
  /* Turn off flow control characters if necessary so that we can type them in verbatim,
   * and turn the keypad off if necessary so that we don't get extended keypad values. */
  if (ISSET(PRESERVE)) {
    disable_flow_control();
  }
  if (!ISSET(RAW_SEQUENCES)) {
    keypad(frame, FALSE);
  }
  /* Turn bracketed-paste mode off. */
  printf("\033[?2004l");
  fflush(stdout);
  linger_after_escape = TRUE;
  /* Read in a single byte or two escapes. */
  input = parse_verbatim_kbinput(frame, count);
  /* If the byte is invalid in the current mode, discard it; if it is an incomplete Unicode sequence, stuff it back. */
  if (input && *count) {
    if (*input >= 0x80 && *count == 1) {
      put_back(*input);
      *count = 999;
    }
    else if ((*input == '\n' && as_an_at) || (!*input && !as_an_at)) {
      *count = 0;
    }
  }
  linger_after_escape = FALSE;
  /* Turn bracketed-paste mode back on. */
  printf("\033[?2004h");
  fflush(stdout);
  /* Turn flow control characters back on if necessary and turn the keypad back on if necessary now that we're done. */
  if (ISSET(PRESERVE)) {
    enable_flow_control();
  }
  /* Use the global window pointers, because a resize may have freed the data that the frame parameter points to. */
  if (!ISSET(RAW_SEQUENCES)) {
    keypad(midwin, TRUE);
    keypad(footwin, TRUE);
  }
  if (*count < 999) {
    for (Ulong i=0; i<(*count); ++i) {
      bytes[i] = (char)input[i];
    }
    bytes[*count] = '\0';
  }
  free(input);
  return bytes;
}

/* Handle any mouse event that may have occurred.  We currently handle
 * releases/clicks of the first mouse button.  If allow_shortcuts is
 * 'TRUE', releasing/clicking on a visible shortcut will put back the
 * keystroke associated with that shortcut.  If ncurses supports them,
 * we also handle presses of the fourth mouse button (upward rolls of
 * the mouse wheel) by putting back keystrokes to scroll up, and presses
 * of the fifth mouse button (downward rolls of the mouse wheel) by
 * putting back keystrokes to scroll down.  We also store the coordinates
 * of a mouse event that needs further handling in mouse_x and mouse_y.
 * Return -1 on error, 0 if the mouse event needs to be handled, 1 if it's
 * been handled by putting back keystrokes, or 2 if it's been ignored. */
int get_mouseinput(int *const my, int *const mx, bool allow_shortcuts) {
  ASSERT(my);
  ASSERT(mx);
  bool in_middle;
  bool in_footer;
  MEVENT event;
  const keystruct *shortcut;
  /* The width of each shortcut item, except the last two. */
  int width;
  /* The calculated index of the clicked item. */
  int index;
  /* The number of shortcut items that get displayed. */
  Ulong number;
  /* First, get the actual mouse event. */
  if (getmouse(&event) == ERR) {
    return -1;
  }
  in_middle = wenclose(midwin, event.y, event.x);
  in_footer = wenclose(footwin, event.y, event.x);
  /* Copy (and possibly adjust) the coordinates of the mouse event. */
  *mx = (event.x - (in_middle ? margin : 0));
  *my = event.y;
  /* Handle releases/clicks of the first mouse button. */
  if (event.bstate & (BUTTON1_RELEASED | BUTTON1_CLICKED)) {
    /* If we're allowing shortcuts, and the current shortcut list is being displayed on the last two
     * lines of the screen, and the first mouse button was released on/clicked inside it, we need to
     * figure out which shortcut was released on/clicked and put back the equivalent keystroke(s) for it. */
    if (allow_shortcuts && !ISSET(NO_HELP) && in_footer) {
      /* Shift the coordinates to be relative to the bottom window. */
      wmouse_trafo(footwin, my, mx, FALSE);
      /* Clicks on the status bar are handled elsewhere, so restore the untranslated mouse-event coordinates. */
      if (!*my) {
        *mx = event.x;
        *my = event.y;
        return 0;
      }
      /* Determine how many shortcuts are being shown. */
      number = shown_entries_for(currmenu);
      /* Calculate the clickable width of each menu item. */
      if (number < 5) {
        width = (COLS / 2);
      }
      else {
        width = (COLS / ((number + 1) / 2));
      }
      /* Calculate the one-based index in the shortcut list. */
      index = ((*mx / width) * 2 + *my);
      /* Adjust the index if we hit the last two wider ones. */
      if ((index > (int)number) && (*mx % width < COLS % width)) {
        index -= 2;
      }
      /* Ignore clicks beyond the last shortcut. */
      if (index > (int)number) {
        return 2;
      }
      /* Search through the list of functions to determine which shortcut in the current menu
       * the user clicked on; then put the corresponding keystroke into the keyboard buffer. */
      for (funcstruct *f = allfuncs; f; f = f->next) {
        if (!(f->menus & currmenu)) {
          continue;
        }
        if (!first_sc_for(currmenu, f->func)) {
          continue;
        }
        if (--index == 0) {
          shortcut = first_sc_for(currmenu, f->func);
          put_back(shortcut->keycode);
          if (0x20 <= shortcut->keycode && shortcut->keycode <= 0x7E) {
            put_back(ESC_CODE);
          }
          break;
        }
      }
      return 1;
    }
    else {
      /* Clicks outside of the bottom window are handled elsewhere. */
      return 0;
    }
  }
# if NCURSES_MOUSE_VERSION >= 2
  /* Handle "presses" of the fourth and fifth mouse buttons (upward and downward rolls of the mouse wheel). */
  else if (event.bstate & (BUTTON4_PRESSED | BUTTON5_PRESSED)) {
    if (in_footer) {
      /* Shift the coordinates to be relative to the bottom window. */
      wmouse_trafo(footwin, my, mx, FALSE);
    }
    if (in_middle || (in_footer && *my == 0)) {
      int keycode = ((event.bstate & BUTTON4_PRESSED) ? ALT_UP : ALT_DOWN);
      /* One bump of the mouse wheel should scroll two lines. */
      put_back(keycode);
      put_back(keycode);
      return 1;
    }
    else {
      /* Ignore "presses" of the fourth and fifth mouse buttons that aren't on the edit window or the status bar. */
      return 2;
    }
  }
# endif
  /* Ignore all other mouse events. */
  return 2;
}

/* Get the column number after leftedge where we can break the given linedata,
 * and return it.  (This will always be at most `total_cols` after leftedge)
 * When kickoff is TRUE, start at the beginning of the linedata; otherwise,
 * continue from where the previous call left off.  Set end_of_line to TRUE
 * when end-of-line is reached while searching for a possible breakpoint. */
Ulong get_softwrap_breakpoint(int cols, const char *const restrict linedata, Ulong leftedge, bool *kickoff, bool *end_of_line) {
  /* Pointer at the current character in this line's data. */
  static const char *text;
  /* Column position that corresponds to the above pointer. */
  static Ulong column;
  /* The place at or before which text must be broken. */
  Ulong rightside = (leftedge + cols);
  /* The column where text can be broken, when there's no better. */
  Ulong breaking_col = rightside;
  /* The column position of the last seen whitespace character. */
  Ulong last_blank_col = 0;
  /* A pointer to the last seen whitespace character in text. */
  const char *farthest_blank = NULL;
  /* Initialize the static variables when it's another line. */
  if (*kickoff) {
    text = linedata;
    column = 0;
    *kickoff = FALSE;
  }
  /* First find the place in text where the current chunk starts. */
  while (*text && column < leftedge) {
    text += advance_over(text, &column);
  }
  /* Now find the place in text where this chunk should end. */
  while (*text && column <= rightside) {
    /* When breaking at blanks, do it *before* the target column. */
    if (ISSET(AT_BLANKS) && is_blank_char(text) && column < rightside) {
      farthest_blank = text;
      last_blank_col = column;
    }
    breaking_col = ((*text == '\t') ? rightside : column);
    text += advance_over(text, &column);
  }
  /* If we didn't overshoot the limit, we've found a breaking point;
   * and we've reached EOL if we didn't even *reach* the limit. */
  if (column <= rightside) {
    *end_of_line = (column < rightside);
    return column;
  }
  /* If we're softwrapping at blanks and we found at least one blank, break
   * after that blank -- if it doesn't overshoot the screen's edge. */
  if (farthest_blank) {
    Ulong aftertheblank = last_blank_col;
    Ulong onestep = advance_over(farthest_blank, &aftertheblank);
    if (aftertheblank <= rightside) {
      text = (farthest_blank + onestep);
      column = aftertheblank;
      return aftertheblank;
    }
    /* If it's a tab that overshoots, break at the screen's edge. */
    if (*farthest_blank == '\t') {
      breaking_col = rightside;
    }
  }
  /* Otherwise, break at the last character that doesn't overshoot. */
  return ((cols > 1) ? breaking_col : (column - 1));
}

/* Return the row number of the softwrapped chunk in the given line that the given column is on, relative
 * to the first row (zero-based).  If leftedge isn't NULL, return in it the leftmost column of the chunk. */
Ulong get_chunk_and_edge(int cols, Ulong column, linestruct *line, Ulong *leftedge) {
  Ulong end_col;
  Ulong current_chunk = 0;
  Ulong start_col     = 0;
  bool end_of_line    = FALSE;
  bool kickoff        = TRUE;
  while (TRUE) {
    end_col = get_softwrap_breakpoint(line->data, start_col, &kickoff, &end_of_line, cols);
    /* When the column is in range or we reached end-of-line, we're done. */
    if (end_of_line || (start_col <= column && column < end_col)) {
      if (leftedge) {
        *leftedge = start_col;
      }
      return current_chunk;
    }
    start_col = end_col;
    ++current_chunk;
  }
}

/* Return how many extra rows the given line needs when softwrapping. */
Ulong extra_chunks_in(int cols, linestruct *const line) {
  return get_chunk_and_edge(cols, (Ulong)-1, line, NULL);
}

/* Return the row of the softwrapped chunk of the given line that column is on, relative to the first row (zero-based). */
Ulong chunk_for(int cols, Ulong column, linestruct *const line) {
  return get_chunk_and_edge(cols, column, line, NULL);
}

/* Return the leftmost column of the softwrapped chunk of the given line that the given column is on. */
Ulong leftedge_for(int cols, Ulong column, linestruct *const line) {
  Ulong leftedge;
  get_chunk_and_edge(cols, column, line, &leftedge);
  return leftedge;
}

/* ----------------------------- Go back chunks ----------------------------- */

/* Try to move up nrows softwrapped chunks from the given line and the given column (leftedge).
 * After moving, leftedge will be set to the starting column of the current chunk.  Return the
 * number of chunks we couldn't move up, which will be zero if we completely succeeded. */
int go_back_chunks_for(openfilestruct *const file, int cols, int nrows, linestruct **const line, Ulong *const leftedge) {
  int i;
  Ulong chunk;
  if (ISSET(SOFTWRAP)) {
    /* Recede through the requested number of chunks. */
    for (i=nrows; i>0; --i) {
      chunk = chunk_for(cols, *leftedge, *line);
      *leftedge = 0;
      if (GE(chunk, i)) {
        return go_forward_chunks_for(file, cols, (chunk - i), line, leftedge);
      }
      else if (*line == file->filetop) {
        break;
      }
      i -= chunk;
      DLIST_ADV_PREV(*line);
      *leftedge = HIGHEST_POSITIVE;
    }
    if (*leftedge == HIGHEST_POSITIVE) {
      *leftedge = leftedge_for(cols, *leftedge, *line);
    }
  }
  else {
    for (i=nrows; i>0 && (*line)->prev; --i) {
      DLIST_ADV_PREV(*line);
    }
  }
  return i;
}

/* Try to move up nrows softwrapped chunks from the given line and the given column (leftedge).
 * After moving, leftedge will be set to the starting column of the current chunk.  Return the
 * number of chunks we couldn't move up, which will be zero if we completely succeeded. */
int go_back_chunks(int nrows, linestruct **const line, Ulong *const leftedge) {
  if (IN_GUI_CTX) {
    return go_back_chunks_for(GUI_OF, GUI_COLS, nrows, line, leftedge);
  }
  else {
    return go_back_chunks_for(TUI_OF, TUI_COLS, nrows, line, leftedge);
  }
}

/* ----------------------------- Go forward chunk ----------------------------- */

/* Try to move down nrows softwrapped chunks from the given line and the given column (leftedge).
 * After moving, leftedge will be set to the starting column of the current chunk.  Return the
 * number of chunks we couldn't move down, which will be zero if we completely succeeded. */
int go_forward_chunks_for(openfilestruct *const file, int cols, int nrows, linestruct **const line, Ulong *const leftedge) {
  int i;
  Ulong current_leftedge;
  bool kickoff;
  bool eol;
  if (ISSET(SOFTWRAP)) {
    current_leftedge = (*leftedge);
    kickoff = TRUE;
    /* Advance thrue the requested number of chunks. */
    for (i=nrows; i>0; --i) {
      eol = FALSE;
      current_leftedge = get_softwrap_breakpoint((*line)->data, current_leftedge, &kickoff, &eol, cols);
      if (!eol) {
        continue;
      }
      else if ((*line) == file->filebot) {
        break;
      }
      CLIST_ADV_NEXT(*line);
      current_leftedge = 0;
      kickoff = TRUE;
    }
    /* Only change leftedge when we actually could move. */
    if (i < nrows) {
      (*leftedge) = current_leftedge;
    }
  }
  else {
    for (i=nrows; i>0 && (*line)->next; --i) {
      CLIST_ADV_NEXT(*line);
    }
  }
  return i;
}

/* Try to move down nrows softwrapped chunks from the given line and the given column (leftedge).
 * After moving, leftedge will be set to the starting column of the current chunk.  Return the
 * number of chunks we couldn't move down, which will be zero if we completely succeeded. */
int go_forward_chunks(int nrows, linestruct **const line, Ulong *const leftedge) {
  if (IN_GUI_CTX) { 
    return go_forward_chunks_for(GUI_OF, GUI_COLS, nrows, line, leftedge);
  }
  else {
    return go_forward_chunks_for(TUI_OF, TUI_COLS, nrows, line, leftedge);
  }
}

void ensure_firstcolumn_is_aligned_for(openfilestruct *const file, int cols) {
  ASSERT(file);
  if (ISSET(SOFTWRAP)) {
    file->firstcolumn = leftedge_for(cols, file->firstcolumn, file->edittop);
  }
  else {
    file->firstcolumn = 0;
  }
  /* If smooth scrolling is on, make sure the viewport doesn't center. */
  focusing = FALSE;
}

/* Ensure that firstcolumn is at the starting column of the softwrapped chunk
 * it's on.  We need to do this when the number of columns of the edit window
 * has changed, because then the width of softwrapped chunks has changed. */
void ensure_firstcolumn_is_aligned(void) {
  if (IN_GUI_CTX) {
    ensure_firstcolumn_is_aligned_for(GUI_OF, GUI_COLS);
  }
  else {
    ensure_firstcolumn_is_aligned_for(TUI_OF, TUI_COLS);
  }
}

/* Convert text into a string that can be displayed on screen.
 * The caller wants to display text starting with the given column, and
 * extending for at most span columns. The returned string is dynamically
 * allocated, and should be freed. If isdata is TRUE, the caller might put "<"
 * at the beginning or ">" at the end of the line if it's too long. If isprompt
 * is TRUE, the caller might put ">" at the end of the line if it's too long. */
char *display_string(const char *text, Ulong column, Ulong span, bool isdata, bool isprompt) {
  /* The beginning of the text, to later determine the covered part. */
  const char *origin = text;
  /* The index of the first character that the caller wishes to show. */
  Ulong start_x = actual_x(text, column);
  /* The actual column where that first character starts. */
  Ulong start_col = wideness(text, start_x);
  /* The number of zero-width characters for which to reserve space. */
  Ulong stowaways = 20;
  /* The amount of memory to reserve for the displayable string. */
  Ulong allocsize = (((IN_GUI_CTX ? (span + 50) : COLS) + stowaways) * MAXCHARLEN + 1);
  /* The displayable string we will return. */
  char *converted = xmalloc(allocsize);
  /* Current position in converted. */
  Ulong index = 0;
  /* The column number just beyond the last shown character. */
  Ulong beyond = (column + span);
  text += start_x;
  if (span > HIGHEST_POSITIVE) {
    statusline(ALERT, "Span has underflowed -- please report a bug");
    converted[0] = '\0';
    return converted;
  }
  /* If the first character starts before the left edge, or would be overwritten by a "<" token, then show placeholders instead. */
  if ((start_col < column || (start_col > 0 && isdata && !ISSET(SOFTWRAP))) && *text && *text != '\t') {
    if (is_cntrl_char(text)) {
      if (start_col < column) {
        converted[index++] = control_mbrep(text, isdata);
        ++column;
        text += char_length(text);
      }
    }
    else if (is_doublewidth(text)) {
      if (start_col == column) {
        converted[index++] = ' ';
        ++column;
      }
      /* Display the right half of a two-column character as ']'. */
      converted[index++] = ']';
      ++column;
      text += char_length(text);
    }
  }
  while (*text && (column < beyond || ZEROWIDTH_CHAR)) {
    /* A plain printable ASCII character is one byte, one column. */
    if ((*text > 0x20 && *text != DEL_CODE) || ISO8859_CHAR) {
      converted[index++] = *(text++);
      ++column;
      continue;
    }
    /* Show a space as a visible character, or as a space. */
    if (*text == ' ') {
      if (ISSET(WHITESPACE_DISPLAY)) {
        for (int i=whitelen[0]; i<(whitelen[0] + whitelen[1]);) {
          converted[index++] = whitespace[i++];
        }
      }
      else {
        converted[index++] = ' ';
      }
      ++column;
      ++text;
      continue;
    }
    /* Show a tab as a visible character plus spaces, or as just spaces. */
    if (*text == '\t') {
      if (ISSET(WHITESPACE_DISPLAY) && (index > 0 || !isdata || !ISSET(SOFTWRAP) || column % tabsize == 0 || column == start_col)) {
        for (int i=0; i<whitelen[0];) {
          converted[index++] = whitespace[i++];
        }
      }
      else {
        converted[index++] = ' ';
      }
      ++column;
      /* Fill the tab up with the required number of spaces. */
      while ((column % tabsize) != 0 && column < beyond) {
        converted[index++] = ' ';
        ++column;
      }
      ++text;
      continue;
    }
    /* Represent a control character with a leading caret. */
    if (is_cntrl_char(text)) {
      converted[index++] = '^';
      converted[index++] = control_mbrep(text, isdata);
      text += char_length(text);
      column += 2;
      continue;
    }
    int charlength, charwidth;
    wchar wc;
    /* Convert a multibyte character to a single code. */
    charlength = mbtowide(&wc, text);
    /* Represent an invalid character with the Replacement Character. */
    if (charlength < 0) {
      converted[index++] = '\xEF';
      converted[index++] = '\xBF';
      converted[index++] = '\xBD';
      ++text;
      ++column;
      continue;
    }
    /* Determine whether the character takes zero, one, or two columns. */
    charwidth = wcwidth(wc);
    /* Watch the number of zero-widths, to keep ample memory reserved. */
    if (charwidth == 0 && --stowaways == 0) {
      stowaways  = 40;
      allocsize += (stowaways * MAXCHARLEN);
      converted  = xrealloc(converted, allocsize);
    }
    /* On a Linux console, skip zero-width characters, as it would show them WITH a width, thus messing up the display.  See bug #52954. */
    if (on_a_vt && charwidth == 0) {
      text += charlength;
      continue;
    }
    /* For any valid character, just copy its bytes. */
    for (; charlength > 0; --charlength) {
      converted[index++] = *(text++);
    }
    /* If the codepoint is unassigned, assume a width of one. */
    column += (charwidth < 0 ? 1 : charwidth);
  }
  /* If there is more text than can be shown, make room for the ">". */
  if (column > beyond || (*text && (isprompt || (isdata && !ISSET(SOFTWRAP))))) {
    do {
      index = step_left(converted, index);
    } while (is_zerowidth(converted + index));
    /* Display the left half of a two-column character as '['. */
    if (is_doublewidth(converted + index)) {
      converted[index++] = '[';
    }
    has_more = TRUE;
  }
  else {
    has_more = FALSE;
  }
  is_shorter = (column < beyond);
  /* Null-terminate the converted string. */
  converted[index] = '\0';
  /* Remember what part of the original text is covered by converted. */
  from_x = start_x;
  till_x = (text - origin);
  return converted;
}

/* Check whether the mark is on, or whether old_column and new_column are on different "pages"
 * (in softwrap mode, only the former applies), which means that the relevant line needs to be redrawn. */
bool line_needs_update_for(openfilestruct *const file, int cols, Ulong old_column, Ulong new_column) {
  ASSERT(file);
  if (file->mark) {
    return TRUE;
  }
  else {
    return (get_page_start(old_column, cols) != get_page_start(new_column, cols));
  }
}

/* Check whether the mark is on, or whether old_column and new_column are on different "pages"
 * (in softwrap mode, only the former applies), which means that the relevant line needs to be redrawn. */
bool line_needs_update(Ulong old_column, Ulong new_column) {
  if (IN_GUI_CTX) {
    return line_needs_update_for(GUI_OF, GUI_COLS, old_column, new_column);
  }
  else {
    return line_needs_update_for(TUI_OF, TUI_COLS, old_column, new_column);
  }
}

/* Return 'TRUE' if there are fewer than a screen's worth of lines between the line at line number
 * was_lineno (and column was_leftedge, if we're in softwrap mode) and the line at current[current_x]. */
bool less_than_a_screenful_for(CTX_ARGS, Ulong was_lineno, Ulong was_leftedge) {
  int rows_left;
  Ulong leftedge;
  linestruct *line;
  if (ISSET(SOFTWRAP)) {
    line = file->current;
    leftedge  = leftedge_for(cols, xplustabs_for(file), file->current);
    rows_left = go_back_chunks_for(file, cols, (rows - 1), &line, &leftedge);
    return (rows_left > 0 || line->lineno < (long)was_lineno || (line->lineno == (long)was_lineno && leftedge <= was_leftedge));
  }
  else {
    return ((int)(file->current->lineno - was_lineno) < rows);
  }
}

/* Return 'TRUE' if there are fewer than a screen's worth of lines between the line at line number was_lineno
 * (and column was_leftedge, if we're in softwrap mode) and the line at `openfile->current[openfile->current_x]`. */
bool less_than_a_screenful(Ulong was_lineno, Ulong was_leftedge) {
  if (IN_GUI_CTX) {
    return less_than_a_screenful_for(GUI_CTX, was_lineno, was_leftedge);
  }
  else {
    return less_than_a_screenful_for(TUI_CTX, was_lineno, was_leftedge);
  }
}

/* When in softwrap mode, and the given column is on or after the breakpoint of a softwrapped
 * chunk, shift it back to the last column before the breakpoint.  The given column is relative
 * to the given leftedge in current.  The returned column is relative to the start of the text. */
Ulong actual_last_column_for(openfilestruct *const file, int cols, Ulong leftedge, Ulong column) {
  ASSERT(file);
  bool  kickoff, last_chunk;
  Ulong end_col;
  if (ISSET(SOFTWRAP)) {
    kickoff    = TRUE;
    last_chunk = FALSE;
    end_col    = (get_softwrap_breakpoint(file->current->data, leftedge, &kickoff, &last_chunk, cols) - leftedge);
    /* If we're not on the last chunk, we're one column past the end of the row.  Shifting back one column
     * might put us in the middle of a multi-column character, but 'actual_x()' will fix that later. */
    if (!last_chunk) {
      --end_col;
    }
    if (column > end_col) {
      column = end_col;
    }
  }
  return (leftedge + column);
}

/* When in softwrap mode, and the given column is on or after the breakpoint of a softwrapped
 * chunk, shift it back to the last column before the breakpoint.  The given column is relative
 * to the given leftedge in current.  The returned column is relative to the start of the text. */
Ulong actual_last_column(Ulong leftedge, Ulong column) {
  if (IN_GUI_CTX) {
    return actual_last_column_for(GUI_OF, GUI_COLS, leftedge, column);
  }
  else {
    return actual_last_column_for(TUI_OF, TUI_COLS, leftedge, column);
  }
}

/* Return TRUE if current[current_x] is before the viewport. */
bool current_is_above_screen_for(openfilestruct *const file) {
  ASSERT(file);
  if (ISSET(SOFTWRAP)) {
    return (file->current->lineno < file->edittop->lineno || (file->current->lineno == file->edittop->lineno && xplustabs_for(file) < file->firstcolumn));
  }
  return (file->current->lineno < file->edittop->lineno);
}

/* Return TRUE if current[current_x] is before the viewport. */
bool current_is_above_screen(void) {
  return current_is_above_screen_for(CTX_OF);
}

/* Return `TRUE` if `file->current[file->current_x]` is beyond the viewport. */
bool current_is_below_screen_for(CTX_ARGS) {
  ASSERT(file);
  linestruct *line;
  Ulong leftedge;
  if (ISSET(SOFTWRAP)) {
    line     = file->edittop;
    leftedge = file->firstcolumn;
    /* If current[current_x] is more than a screen's worth of lines after edittop at column firstcolumn, it's below the screen. */
    return (go_forward_chunks_for(file, cols, (rows - 1 - SHIM), &line, &leftedge) == 0 && (line->lineno < file->current->lineno
     || (line->lineno == file->current->lineno && leftedge < leftedge_for(cols, xplustabs_for(file), file->current))));
  }
  return (file->current->lineno >= (file->edittop->lineno + rows - SHIM));
}

/* Return `TRUE` if `openfile->current[openfile->current_x]` is beyond the viewport. */
bool current_is_below_screen(void) {
  if (IN_GUI_CTX) {
    return current_is_below_screen_for(GUI_CTX);
  }
  else {
    return current_is_below_screen_for(TUI_CTX);
  }
}

/* Return TRUE if current[current_x] is outside the viewport. */
bool current_is_offscreen_for(CTX_ARGS) {
  ASSERT(file);
  return (current_is_above_screen_for(file) || current_is_below_screen_for(STACK_CTX));
}

/* Return TRUE if current[current_x] is outside the viewport. */
bool current_is_offscreen(void) {
  if (IN_GUI_CTX) {
    return current_is_offscreen_for(GUI_CTX);
  }
  else {
    return current_is_offscreen_for(TUI_CTX);
  }
}

/* Move edittop so that current is on the screen.  manner says how:  STATIONARY means that the cursor
 * should stay on the same screen row, CENTERING means that current should end up in the middle of the
 * screen, and FLOWING means that it should scroll no more than needed to bring current into view. */
void adjust_viewport_for(CTX_ARGS, update_type manner) {
  ASSERT(file);
  int goal = 0;
  if (manner == STATIONARY) {
    goal = file->cursor_row;
  }
  else if (manner == CENTERING) {
    goal = (rows / 2);
  }
  else if (!current_is_above_screen_for(file)) {
    goal = (rows - 1 - SHIM);
  }
  file->edittop = file->current;
  if (ISSET(SOFTWRAP)) {
    file->firstcolumn = leftedge_for(cols, xplustabs_for(file), file->current);
  }
  /* Move edittop back goal rows, starting at current[current_x]. */
  go_back_chunks_for(file, cols, goal, &file->edittop, &file->firstcolumn);
}

/* Move edittop so that current is on the screen.  manner says how:  STATIONARY means that the cursor
 * should stay on the same screen row, CENTERING means that current should end up in the middle of the
 * screen, and FLOWING means that it should scroll no more than needed to bring current into view. */
void adjust_viewport(update_type manner) {
  CTX_CALL_WARGS(adjust_viewport_for, manner);
}

/* Redetermine `cursor_row` from the position of current relative to edittop, and put the cursor in the edit window at (cursor_row, "current_x"). */
void place_the_cursor_for(openfilestruct *const file) {
  ASSERT(file);
  Ulong column;
  if (IN_GUI_CTX) {
    place_the_cursor_for_internal(file, editor_from_file(file)->cols, &column);
  }
  else if (IN_CURSES_CTX) {
    place_the_cursor_curses_for(openfile);
  }
}

/* Redetermine `cursor_row` from the position of current relative to edittop, and put the cursor in the edit window at (cursor_row, "current_x"). */
void place_the_cursor(void) {
  Ulong column;
  if (IN_GUI_CTX) {
    place_the_cursor_for_internal(GUI_OF, GUI_COLS, &column);
  }
  else if (IN_CURSES_CTX) {
    place_the_cursor_curses_for(TUI_OF);
  }
}

/* Ensure that the status bar will be wiped upon the next keystroke. */
void set_blankdelay_to_one(void) {
  countdown = 1;
}

/* Return the number of key codes waiting in the keystroke buffer. */
Ulong waiting_keycodes(void) {
  return waiting_codes;
}

/* Scroll the edit window one row in the given direction, and draw the relevant content on the resultant blank row. */
void edit_scroll_for(openfilestruct *const file, bool direction) {
  ASSERT(file);
  linestruct *line;
  Ulong leftedge;
  int nrows = 1;
  /* Move the top line of the edit window one row up or down. */
  if (direction == BACKWARD) {
    go_back_chunks(1, &file->edittop, &file->firstcolumn);
  }
  else {
    go_forward_chunks(1, &file->edittop, &file->firstcolumn);
  }
  /* If using the gui, return early. */
  if (ISSET(USING_GUI)) {
    return;
  }
  /* Otherwise, when in curses mode, scroll. */
  else if (!ISSET(NO_NCURSES)) {
    /* Actually scroll the text of the edit window one row up or down. */
    scrollok(midwin, TRUE);
    wscrl(midwin, ((direction == BACKWARD) ? -1 : 1));
    scrollok(midwin, FALSE);
  }
  /* If we're not on the first "page" (when not softwrapping), or the mark
   * is on, the row next to the scrolled region needs to be redrawn too. */
  if (line_needs_update_for(file, editwincols, file->placewewant, 0) && nrows < editwinrows) {
    ++nrows;
  }
  /* If we scrolled backward, the top row needs to be redrawn. */
  line     = file->edittop;
  leftedge = file->firstcolumn;
  /* If we scrolled forward, the bottom row needs to be redrawn. */
  if (direction == FORWARD) {
    go_forward_chunks_for(file, editwincols, (editwinrows - nrows), &line, &leftedge);
  }
  if (sidebar) {
    draw_scrollbar_curses();
  }
  if (ISSET(SOFTWRAP)) {
    /* Compensate for the earlier chunks of a softwrapped line. */
    nrows += chunk_for(editwincols, leftedge, line);
    /* Don't compensate for the chunks that are offscreen. */
    if (line == file->edittop) {
      nrows -= chunk_for(editwincols, file->firstcolumn, line);
    }
  }
  /* Draw new content on the blank row (and on the bordering row too when it was deemed necessary). */
  while (nrows > 0 && line) {
    nrows -= update_line_curses(line, ((line == file->current) ? file->current_x : 0));
    line = line->next;
  }
}

/* Scroll the edit window one row in the given direction, and draw the relevant content on the resultant blank row. */
void edit_scroll(bool direction) {
  edit_scroll_for(CTX_OF, direction);
}

/* ----------------------------- Edit redraw ----------------------------- */

/* Update any lines between old_current and current that need to be updated.  Use this if we've moved without changing any text. */
void edit_redraw_for(CTX_ARGS, linestruct *const old_current, update_type manner) {
  ASSERT(file);
  ASSERT(old_current);
  linestruct *line;
  Ulong was_pww = file->placewewant;
  set_pww_for(file);
  /* If the current line is offscreen, scroll until it's onscreen. */
  if (current_is_offscreen_for(STACK_CTX)) {
    adjust_viewport_for(STACK_CTX, (ISSET(JUMPY_SCROLLING) ? CENTERING : manner));
    refresh_needed = TRUE;
    return;
  }
  /* Return early if running in gui mode. */
  if (ISSET(USING_GUI)) {
    return;
  }
  /* If the mark is on, update all lines between old_current and current. */
  if (file->mark) {
    line = old_current;
    while (line != file->current) {
      update_line_curses_for(file, line, 0);
      line = ((line->lineno > file->current->lineno) ? line->prev : line->next);
    }
  }
  else {
    /* Otherwise, update old_current only if it differs from current and was horizontally scrolled. */
    if (old_current != file->current && get_page_start(was_pww, cols) > 0) {
      update_line_curses_for(file, old_current, 0);
    }
  }
  /* Update current if the mark is on or it has changed "page", or if it differs from old_current and needs to be horizontally scrolled. */
  if (line_needs_update_for(file, cols, was_pww, file->placewewant) || (old_current != file->current && get_page_start(file->placewewant, cols) > 0)) {
    update_line_curses_for(file, file->current, file->current_x);
  }
}

/* Update any lines between old_current and current that need to be updated.  Use this if we've moved without changing any text. */
void edit_redraw(linestruct *const old_current, update_type manner) {
  if (IN_GUI_CTX) {
    edit_redraw_for(GUI_CTX, old_current, manner);
  }
  else { 
    edit_redraw_for(TUI_CTX, old_current, manner);
  }
}

/* ----------------------------- Edit refresh ----------------------------- */

/* Refresh the screen without changing the position of lines.  Use this if we've moved and changed text. */
void edit_refresh_for(CTX_ARGS) {
  ASSERT(file);
  linestruct *line;
  int row = 0;
  /* If the current line in file is out of view, get it back on screen. */
  if (current_is_offscreen_for(STACK_CTX)) {
    adjust_viewport_for(STACK_CTX, ((focusing || ISSET(JUMPY_SCROLLING)) ? CENTERING : FLOWING));
  }
  if (IN_CURSES_CTX) {
    /* When needed and usefull, initialize the colors for the current syntax. */
    if (!ISSET(NO_SYNTAX) && file->syntax && !have_palette && has_colors()) {
      prepare_palette();
    }
    /* When the line above the viewport does not have multidata, recalculate it. */
    recook |= (ISSET(SOFTWRAP) && file->edittop->prev && !file->edittop->prev->multidata);
    if (recook) {
      precalc_multicolorinfo_for(file);
      perturbed = FALSE;
      recook    = FALSE;
    }
    /* Only draw sidebar when appropriet, ie: When there is more then one rows worth of data. */
    if (sidebar && file->filebot->lineno > rows) {
      /* TODO: Make this file specific. */
      draw_scrollbar_curses();
    }
    line = file->edittop;
    while (row < rows && line) {
      row += update_line_curses_for(file, line, ((line == file->current) ? file->current_x : 0));
      DLIST_ADV_NEXT(line);
    }
    while (row < rows) {
      blank_row_curses(midwin, row);
      /* If full linenumber bar is enabled, then draw it. */
      /*
       * if (config->linenumber.fullverticalbar) {
       *   mvwaddchcolor(midwin, row, (margin - 1), ACS_VLINE, config->linenumber.barcolor);
       * }
       */
      /* Only draw sidebar when on and when the file is longer then rows. */
      if (sidebar && file->filebot->lineno > rows) {
        mvwaddch(midwin, row, (COLS - 1), bardata[row]);
      }
      ++row;
    }
    place_the_cursor_curses_for(file);
    wnoutrefresh(midwin);
    refresh_needed = FALSE; 
  }
}

/* Refresh the screen without changing the position of lines.  Use this if we've moved and changed text. */
void edit_refresh(void) {
  CTX_CALL(edit_refresh_for);
  // linestruct *line;
  // int row = 0;
  // /* If the current line is out of view, get it back on screen. */
  // if (current_is_offscreen()) {
  //   adjust_viewport((focusing || ISSET(JUMPY_SCROLLING)) ? CENTERING : FLOWING);
  // }
  // /* Return early whwn in gui mode. */
  // if (ISSET(USING_GUI)) {
  //   return;
  // }
  // /* When needed and useful, initialize the colors for the current syntax. */
  // if (!ISSET(NO_NCURSES) && openfile->syntax && !have_palette && !ISSET(NO_SYNTAX) && has_colors()) {
  //   prepare_palette();
  // }
  // /* When the line above the viewport does not have multidata, recalculate all. */
  // recook |= (ISSET(SOFTWRAP) && openfile->edittop->prev && !openfile->edittop->prev->multidata);
  // if (recook) {
  //   precalc_multicolorinfo();
  //   perturbed = FALSE;
  //   recook    = FALSE;
  // }
  // /* Only draw sidebar when approptiet, i.e: when there is more then one ROWS worth of data. */
  // if (sidebar && openfile->filebot->lineno > editwinrows) {
  //   draw_scrollbar_curses();
  // }
  // line = openfile->edittop;
  // while (row < editwinrows && line) {
  //   row += update_line_curses(line, ((line == openfile->current) ? openfile->current_x : 0));
  //   line = line->next;
  // }
  // while (row < editwinrows) {
  //   blank_row_curses(midwin, row);
  //   /* If full linenumber bar is enabled, then draw it. */
  //   if (config->linenumber.fullverticalbar) {
  //     mvwaddchcolor(midwin, row, (margin - 1), ACS_VLINE, config->linenumber.barcolor);
  //   }
  //   /* Only draw sidebar when on and when the openfile is longer then editwin rows. */
  //   if (sidebar && openfile->filebot->lineno > editwinrows) {
  //     mvwaddch(midwin, row, (COLS - 1), bardata[row]);
  //   }
  //   ++row;
  // }
  // place_the_cursor();
  // wnoutrefresh(midwin);
  // refresh_needed = FALSE;
}

/* If path is NULL, we're in normal editing mode, so display the current
 * version of nano, the current filename, and whether the current file
 * has been modified on the title bar.  If path isn't NULL, we're either
 * in the file browser or the help viewer, so show either the current
 * directory or the title of help text, that is: whatever is in path.
 * TODO: Make another version of this that is context-less. */
void titlebar(const char *path) {
  /* The width of the diffrent title-bar elements, in columns. */
  Ulong verlen;
  Ulong prefixlen;
  Ulong pathlen;
  Ulong statelen;
  /* The width of the `Modified` would take up. */
  Ulong pluglen = 0;
  /* The position at witch the center part of the title-bar starts. */
  Ulong offset = 0;
  /* What is shown in the top left corner. */
  const char *upperleft = "";
  /* What is shown before the path -- `DIR:` or nothing. */
  const char *prefix = "";
  /* The state of the current buffer -- `Modified`, `View` or ``. */
  const char *state = "";
  /* The presentable for of the pathname. */
  char *caption;
  /* The buffer sequence number plus the total buffer count. */
  char *ranking = NULL;
  /* If the screen is to small, there is no title bar. */
  if (!IN_CURSES_CTX || !topwin) {
    return;
  }
  wattron(topwin, interface_color_pair[TITLE_BAR]);
  blank_titlebar();
  as_an_at = FALSE;
  /**
   * Do as Pico:
   *   if there is not enough width available for all items,
   *   first sacrifice the version string, then eat up the side spaces,
   *   then sacrifice the prefix, and only then start dottifying.
   */
  /* Figure out the path, prefix and state strings. */
  if (currmenu == MLINTER) {
    prefix = _("Linting --");
    path = openfile->filename;
  }
  else {
    if (!inhelp && path) {
      prefix = _("DIR:");
    }
    else {
      if (!inhelp) {
        /* If there are/were multiple buffers, show witch out of how meny. */
        if (more_than_one) {
          ranking = xmalloc(24);
          sprintf(ranking, "[%i/%i]", buffer_number(openfile), buffer_number(startfile->prev));
          upperleft = ranking;
        }
        else {
          upperleft = BRANDING;
        }
        if (!*openfile->filename) {
          path = _("New buffer");
        }
        else {
          path = openfile->filename;
        }
        if (ISSET(VIEW_MODE)) {
          state = _("View");
        }
        else if (ISSET(STATEFLAGS)) {
          state = _("+.xxxxx");
        }
        else if (ISSET(RESTRICTED)) {
          state = _("Restricted");
        }
        else {
          pluglen = (breadth(_("Modified")) + 1);
        }
      }
    }
  }
  /* Determine the width of the four elements, including their padding. */
  verlen    = (breadth(upperleft) + 3);
  prefixlen = breadth(prefix);
  if (prefixlen > 0) {
    ++prefixlen;
  }
  pathlen  = breadth(path);
  statelen = (breadth(state) + 2);
  if (statelen > 2) {
    ++pathlen;
  }
  /* Only print the version message when there is room for it. */
  if ((int)(verlen + prefixlen + pathlen + pluglen + statelen) <= COLS) {
    mvwaddstr(topwin, 0, 2, upperleft);
  }
  else {
    verlen = 2;
    /* If things don't fit yet, give up the placeholder. */
    if ((int)(verlen + prefixlen + pathlen + pluglen + statelen) > COLS) {
      pluglen = 0;
    }
    /* If things still don't fit, give up the side spaces. */
    if ((int)(verlen + prefixlen + pathlen + pluglen + statelen) > COLS) {
      verlen    = 0;
      statelen -= 2;
    }
  }
  free(ranking);
  /* If we have side spaces left, center the path name. */
  if (verlen > 0) {
    offset = (verlen + (COLS - (verlen + pluglen + statelen) - (prefixlen + pathlen)) / 2);
  }
  /* Only print the prefix when there is room for it. */
  if ((int)(verlen + prefixlen + pathlen + pluglen + statelen) <= COLS) {
    mvwaddstr(topwin, 0, offset, prefix);
    if (prefixlen > 0) {
      waddnstr(topwin, S__LEN(" "));
    }
  }
  else {
    wmove(topwin, 0, offset);
  }
  /* Print the full path if there's room, otherwise, dottify it. */
  if ((int)(pathlen + pluglen + statelen) <= COLS) {
    caption = display_string(path, 0, pathlen, FALSE, FALSE);
    waddstr(topwin, caption);
    free(caption);
  }
  else if ((int)(5 + statelen) <= COLS) {
    waddnstr(topwin, S__LEN("..."));
    caption = display_string(path, (3 + pathlen - COLS + statelen), (COLS - statelen), FALSE, FALSE);
    waddstr(topwin, caption);
    free(caption);
  }
  /* When requested, show on the title-bar, the state of three options and the state of the mark and whether a macro is beeing recorded.  */
  if (*state && ISSET(STATEFLAGS) && !ISSET(VIEW_MODE)) {
    if (openfile->modified && COLS > 1) {
      waddnstr(topwin, S__LEN(" *"));
    }
    if ((int)statelen < COLS) {
      wmove(topwin, 0, (COLS + 2 - statelen));
      show_state_at_curses(topwin);
    }
  }
  else {
    /* If there's room, right-align the state word, otherwise, clip it. */
    if (statelen > 0 && (int)statelen <= COLS) {
      mvwaddstr(topwin, 0, (COLS - statelen), state);
    }
    else {
      mvwaddnstr(topwin, 0, 0, state, actual_x(state, COLS));
    }
  }
  wattroff(topwin, interface_color_pair[TITLE_BAR]);
  wrefresh(topwin);
}

/* Draw a bar at the bottom with some minimal state information. */
void minibar(void) {
  char *thename = NULL;
  char *shortname;
  char *position;
  char *number_of_lines = NULL;
  char *ranking         = NULL;
  char *successor       = NULL;
  char *location        = xmalloc(44);
  char *hexadecimal     = xmalloc(9);
  Ulong namewidth;
  Ulong placewidth;
  Ulong count;
  Ulong tallywidth = 0;
  Ulong padding    = 2;
  wchar widecode;
  /* Make this function a `no-op` function when in gui mode. */
  if (ISSET(USING_GUI)) {
    free(location);
    free(hexadecimal);  
    return;
  }
  /* Draw the colored bar over the full width of the screen. */
  wattron(footwin, interface_color_pair[config->minibar_color]);
  mvwprintw(footwin, 0, 0, "%*s", COLS, " ");
  if (*openfile->filename) {
    as_an_at = FALSE;
    thename = display_string(openfile->filename, 0, COLS, FALSE, FALSE);
  }
  else {
    thename = copy_of(_("(nameless)"));
  }
  snprintf(location, 44, "%zi,%zi", openfile->current->lineno, (xplustabs() + 1));
  placewidth = strlen(location);
  namewidth  = breadth(thename);
  /* If the file name is relativly long drop the side spaces. */
  if ((int)(namewidth + 19) > COLS) {
    padding = 0;
  }
  /* Display the name of the current file (dottifying it if it doesn't fit), plus a star when the file has been modified. */
  if (COLS > 4) {
    if ((int)namewidth > (COLS - 2)) {
      shortname = display_string(thename, (namewidth - COLS + 5), (COLS - 5), FALSE, FALSE);
      wmove(footwin, 0, 0);
      waddnstr(footwin, S__LEN("..."));
      waddstr(footwin, shortname);
      free(shortname);
    }
    else {
      mvwaddstr(footwin, 0, padding, thename);
    }
    waddnstr(footwin, (openfile->modified ? " *": "  "), 2);
  }
  /* Right after reading or writing a file, display its number of lines.  Otherwise, when there are multiple buffers, display an [x/n] counter. */
  if (report_size && COLS > 35) {
    count           = (openfile->filebot->lineno - !*openfile->filebot->data);
    number_of_lines = xmalloc(49);
    if (openfile->fmt == NIX_FILE || openfile->fmt == UNSPECIFIED) {
      snprintf(number_of_lines, 49, P_(" (%zu line)", " (%zu lines)", count), count);
    }
    else {
      snprintf(number_of_lines, 49, P_(" (%zu line, %s)", " (%zu lines, %s)", count), count, ((openfile->fmt == DOS_FILE) ? "DOS" : "Mac"));
    }
    tallywidth = breadth(number_of_lines);
    if ((int)(namewidth + tallywidth + 11) < COLS) {
      waddstr(footwin, number_of_lines);
    }
    else {
      tallywidth = 0;
    }
    report_size = FALSE;
  }
  else if (!CLIST_SINGLE(openfile) && COLS > 35) {
    ranking = xmalloc(24);
    snprintf(ranking, 24, " [%i/%i]", buffer_number(openfile), buffer_number(startfile->prev));
    if ((int)(namewidth + placewidth + breadth(ranking) + 32) < COLS) {
      waddstr(footwin, ranking);
    }
  }
  /* Display the line/column position of the cursor. */
  if (ISSET(CONSTANT_SHOW) && (int)(namewidth + tallywidth + placewidth + 34) < COLS) {
    mvwaddstr(footwin, 0, (COLS - 29 - placewidth), location);
  }
  /* Display the hexadecimal code of the character under the cursor, plus the code of up to two succeeding zero-width characters. */
  if (ISSET(CONSTANT_SHOW) && (int)(namewidth + tallywidth + 30) < COLS) {
    position = (openfile->current->data + openfile->current_x);
    if (!*position) {
      snprintf(hexadecimal, 9, (openfile->current->next ? using_utf8() ? "U+000A" : "  0x0A" : "  ----"));
    }
    else if (*position == NL) {
      snprintf(hexadecimal, 9, "  0x00");
    }
    else if ((Uchar)*position < 0x80 && using_utf8()) {
      snprintf(hexadecimal, 9, "U+%04X", (Uchar)*position);
    }
    else if (using_utf8() && mbtowide(&widecode, position) > 0) {
      snprintf(hexadecimal, 9, "U+%04X", (int)widecode);
    }
    else {
      snprintf(hexadecimal, 9, "  0x%02X", (Uchar)*position);
    }
    mvwaddstr(footwin, 0, (COLS - 25), hexadecimal);
    successor = (position + char_length(position));
    if (*position && *successor && is_zerowidth(successor) && mbtowide(&widecode, successor) > 0) {
      snprintf(hexadecimal, 9, "|%04X", (int)widecode);
      waddstr(footwin, hexadecimal);
      successor += char_length(successor);
      if (*successor && is_zerowidth(successor) && mbtowide(&widecode, successor) > 0) {
        snprintf(hexadecimal, 9, "|%04X", (int)widecode);
        waddstr(footwin, hexadecimal);
      }
    }
    else {
      successor = NULL;
    }
  }
  /* Display the state of three flags, and the state of the macro and mark. */
  if (ISSET(STATEFLAGS) && !successor && (int)(namewidth + tallywidth + 14 + (2 * padding)) < COLS) {
    wmove(footwin, 0, (COLS - 13 - padding));
    show_state_at_curses(footwin);
  }
  /* Display how meny precent the current line is into the file. */
  if ((int)(namewidth + 6) < COLS) {
    snprintf(location, 44, "%.1f%%", ((double)(100 * openfile->current->lineno) / openfile->filebot->lineno));
    mvwaddstr(footwin, 0, (COLS - 6 - padding), location);
  }
  wattroff(footwin, interface_color_pair[config->minibar_color]);
  wrefresh(footwin);
  free(number_of_lines);
  free(hexadecimal);
  free(location);
  free(thename);
  free(ranking);
}

/* Blank all lines of the middle portion of the screen (the edit window). */
void blank_edit(void) {
  /* Always return directly when running in gui mode. */
  if (ISSET(USING_GUI)) {
    return;
  }
  /* Only perform any action when in curses mode for now. */
  else if (!ISSET(NO_NCURSES)) {
    for (int row=0; row<editwinrows; ++row) {
      blank_row_curses(midwin, row);
    }
  }
}

/* Draw all elements of the screen.  That is: the title bar plus the content of the edit window (when not in the file browser), and the bottom bars. */
void draw_all_subwindows(void) {
  if (currmenu & ~(MBROWSER | MWHEREISFILE | MGOTODIR)) {
    titlebar(title);
  }
  if (inhelp) {
    close_buffer();
    wrap_help_text_into_buffer();
  }
  else if (currmenu & ~(MBROWSER | MWHEREISFILE | MGOTODIR)) {
    edit_refresh();
  }
  bottombars_curses(currmenu);
}

/* Blank the first line of the bottom portion of the screen. */
void blank_statusbar(void) {
  /* When running in ncurses context. */
  if (IN_CURSES_CTX) {
    blank_row_curses(footwin, 0);
  }
}

/* Blank the first line of the top portion of the screen.  Using ncurses. */
void blank_titlebar(void) {
  if (IN_CURSES_CTX) {
    mvwprintw(topwin, 0, 0, "%*s", COLS, " ");
  }
}

/* Blank out the two help lines (when they are present).  Ncurses version. */
void blank_bottombars(void) {
  if (!ISSET(NO_HELP) && LINES > 5) {
    blank_row_curses(footwin, 1);
    blank_row_curses(footwin, 2);
  }
}

/* When some number of keystrokes has been reached, wipe the status bar. */
void blank_it_when_expired(void) {
  if (countdown == 0) {
    return;
  }
  else if (--countdown == 0) {
    wipe_statusbar();
  }
  /* When windows overlap, make sure to show the edit window now. */
  if (currmenu == MMAIN && (ISSET(ZERO) || LINES == 1)) {
    /* Running in curses context. */
    if (IN_CURSES_CTX) {
      wredrawln(midwin, (editwinrows - 1), 1);
      wnoutrefresh(midwin);
    }
  }
}

/* Wipe the status bar clean and include this in the next screen update. */
void wipe_statusbar(void) {
  lastmessage = VACUUM;
  if (((ISSET(ZERO) || ISSET(MINIBAR) || LINES == 1) && currmenu == MMAIN)) {
    return;
  }
  if (IN_CURSES_CTX) {
    blank_row_curses(footwin, 0);
    wnoutrefresh(footwin);
  }
}

/* Write a key's representation plus a minute description of its function to the screen.  For example,
 * the key could be "^C" and its tag "Cancel". Key plus tag may occupy at most width columns. */
void post_one_key(const char *const restrict keystroke, const char *const restrict tag, int width) {
  if (IN_CURSES_CTX) {
    post_one_key_curses(keystroke, tag, width);
  }
}

/* Display the shortcut list corresponding to the menu on the last to rows of the bottom portion of the window.  The shortcuts are shown in pairs. */
void bottombars(int menu) {
  /* Running in curses mode. */
  if (IN_CURSES_CTX) {
    bottombars_curses(menu);
  }
  /* Running in tui mode.  Note that this will be added when tui has been redesigned. */
}

/* Warn the user on the status bar and pause for a moment, so that the message can be noticed and read. */
void warn_and_briefly_pause(const char *const restrict message) {
  /* Running in curses mode. */
  if (IN_CURSES_CTX) {
    warn_and_briefly_pause_curses(message);
  }
  /* Running in tui mode.  Note that this will be added when tui has been redesigned. */
}

/* For functions that are used by the tui and gui, this prints a status message correctly. */
void statusline(message_type type, const char *const restrict format, ...) {
  ASSERT(format);
  va_list ap;
  va_start(ap, format);
  if (IN_GUI_CTX) {
    statusline_gui_va(type, format, ap);
  }
  else if (IN_CURSES_CTX) {
    statusline_curses_va(type, format, ap);
  }
  va_end(ap);
}

/* Print a normal `msg`, that conforms to the current contex, tui or gui. */
void statusbar_all(const char *const restrict msg) {
  if (IN_GUI_CTX) {
    statusbar_gui(msg);
  }
  else if (IN_CURSES_CTX) {
    statusbar_curses(msg);
  }
}

/* Display on the status bar details about the current cursor position in `file`. */
void report_cursor_position_for(openfilestruct *const file) {
  ASSERT(file);
  int linepct;
  int colpct;
  int charpct;
  Ulong fullwidth = (breadth(file->current->data) + 1);
  Ulong column = (xplustabs_for(file) + 1);
  Ulong sum;
  char saved_byte = file->current->data[file->current_x];
  file->current->data[file->current_x] = '\0';
  /* Determine the size of the file up to the cursor. */
  sum = number_of_characters_in(file->filetop, file->current);
  file->current->data[file->current_x] = saved_byte;
  /* Calculate the percentages. */
  linepct = (100 * file->current->lineno / file->filebot->lineno);
  colpct  = (100 * column / fullwidth);
  charpct = ((file->totsize == 0) ? 0 : (100 * sum / file->totsize));
  statusline(INFO, _("line %*zd/%zd (%2d%%), col %2zu/%2zu (%3d%%), char %*zu/%zu (%2d%%)"), digits(file->filebot->lineno),
    file->current->lineno, file->filebot->lineno, linepct, column, fullwidth, colpct, digits(file->totsize), sum, file->totsize, charpct);
}

/* Display on the status bar details about the current cursor position in the currently open buffer.  Note that this is context safe. */
void report_cursor_position(void) {
  report_cursor_position_for(CTX_OF);
}

/* Tell curses to unconditionally redraw whatever was on the screen. */
void full_refresh(void) {
  /* Only perform any action when using the curses context. */
  if (IN_CURSES_CTX) {
    wrefresh(curscr);
  }
}

/* ----------------------------- Curses ----------------------------- */

/* Blank a row of window. */
void blank_row_curses(WINDOW *const window, int row) {
  if (IN_CURSES_CTX) {
    ASSERT(window);
    ASSERT(row >= 0);
    wmove(window, row, 0);
    wclrtoeol(window);
  }
}

/* Display the given message on the status bar, but only if its importance is higher than that of a message that is already there.  Ncurses version. */
void statusline_curses_va(message_type type, const char *const restrict format, va_list ap) {
  static Ulong start_col = 0;
  bool showed_whitespace = ISSET(WHITESPACE_DISPLAY);
  char *compound;
  char *message;
  bool bracketed;
  int color;
  va_list copy;
  if (type >= AHEM) {
    waiting_codes = 0;
  }
  if (type < lastmessage && lastmessage > NOTICE) {
    return;
  }
  /* Construct the message from the arguments. */
  compound = xmalloc(MAXCHARLEN * COLS + 1);
  va_copy(copy, ap);
  vsnprintf(compound, (MAXCHARLEN * COLS + 1), format, copy);
  va_end(copy);
  /* When not in curses mode. */
  if (isendwin()) {
    writeferr("%s\n", compound);
    free(compound);
  }
  else {
    if (!we_are_running && type == ALERT && openfile && !openfile->fmt && !openfile->errormessage && !CLIST_SINGLE(openfile)) {
      openfile->errormessage = copy_of(compound);
    }
    /* On a one row terminal, ensure any changes in the edit window are written first, to prevent them from overwriting the message. */
    if (LINES == 1 && type < INFO) {
      wnoutrefresh(midwin);
    }
    /* If there are multiple alert messages, add trailng dot's first. */
    if (lastmessage == ALERT) {
      if (start_col > 4) {
        wmove(footwin, 0, (COLS + 2 - start_col));
        wattron(footwin, interface_color_pair[ERROR_MESSAGE]);
        waddnstr(footwin, S__LEN("..."));
        wattroff(footwin, interface_color_pair[ERROR_MESSAGE]);
        wnoutrefresh(footwin);
        start_col = 0;
        napms(100);
        beep();
      }
      free(compound);
      return;
    }
    else if (type > NOTICE) {
      if (type == ALERT) {
        beep();
      }
      color = ERROR_MESSAGE;
    }
    else if (type == NOTICE) {
      color = SELECTED_TEXT;
    }
    else {
      color = STATUS_BAR;
    }
    lastmessage = type;
    blank_statusbar();
    UNSET(WHITESPACE_DISPLAY);
    message = display_string(compound, 0, COLS, FALSE, FALSE);
    if (showed_whitespace) {
      SET(WHITESPACE_DISPLAY);
    }
    start_col = ((COLS - breadth(message)) / 2);
    bracketed = (start_col > 1);
    wmove(footwin, 0, (bracketed ? (start_col - 2) : start_col));
    wattron(footwin, interface_color_pair[color]);
    if (bracketed) {
      waddnstr(footwin, S__LEN("[ "));
      waddstr(footwin, message);
      waddnstr(footwin, S__LEN(" ]"));
    }
    else {
      waddstr(footwin, message);
    }
    wattroff(footwin, interface_color_pair[color]);
    /* Tell `footwin` to refresh. */
    wrefresh(footwin);
    free(compound);
    free(message);
    /* When requested, wipe the statusbar after just one keystroke, otherwise wipe after 20. */
    countdown = (ISSET(QUICK_BLANK) ? 1 : 20);
  }
}

/* Display the given message on the status bar, but only if its importance is higher than that of a message that is already there.  Ncurses version. */
void statusline_curses(message_type type, const char *const restrict msg, ...) {
  va_list ap;
  va_start(ap, msg);
  statusline_curses_va(type, msg, ap);
  va_end(ap);
}

/* Display a normal message on the status bar, quietly.  Ncurses version. */
void statusbar_curses(const char *const restrict msg) {
  statusline_curses(HUSH, "%s", msg);
}

/* Write a key's representation plus a minute description of its function to the screen.  For example,
 * the key could be "^C" and its tag "Cancel". Key plus tag may occupy at most width columns. */
void post_one_key_curses(const char *const restrict keystroke, const char *const restrict tag, int width) {
  ASSERT(keystroke);
  ASSERT(tag);
  wattron(footwin, interface_color_pair[KEY_COMBO]);
  waddnstr(footwin, keystroke, actual_x(keystroke, width));
  wattroff(footwin, interface_color_pair[KEY_COMBO]);
  /* If the remaining space is to small, skip the description. */
  width -= breadth(keystroke);
  if (width < 2) {
    return;
  }
  waddch(footwin, ' ');
  wattron(footwin, interface_color_pair[FUNCTION_TAG]);
  waddnstr(footwin, tag, actual_x(tag, (width - 1)));
  wattroff(footwin, interface_color_pair[FUNCTION_TAG]);
}

/* Display the shortcut list corresponding to the menu on the last to rows of the bottom portion of the window.  The shortcuts are shown in pairs. */
void bottombars_curses(int menu) {
  Ulong index     = 0;
  Ulong number    = 0;
  Ulong itemwidth = 0;
  Ulong width;
  const keystruct *s;
  funcstruct *f;
  /* Set the global variable to the given menu. */
  currmenu = menu;
  /* Set the global variable to the given menu. */
  if (ISSET(NO_HELP) || LINES < (ISSET(ZERO) ? 3 : ISSET(MINIBAR) ? 4 : 5)) {
    return;
  }
  /* Determine how meny shortcuts must be shown. */
  number = shown_entries_for(menu);
  /* Compute the width of each keyname-plus-explanation pair. */
  itemwidth = (COLS / ((number + 1) / 2));
  /* If there is no room, don't do anything. */
  if (!itemwidth) {
    return;
  }
  blank_bottombars();
  /* Display the first number of shortcuts in the given menu that have a key combination assigned to them. */
  for (f=allfuncs, index=0; index<number; f=f->next) {
    width = itemwidth;
    if (!(f->menus & menu) || !(s = first_sc_for(menu, f->func))) {
      continue;
    }
    wmove(footwin, (1 + (index % 2)), ((index / 2) * itemwidth));
    /* When the number is uneven the penultimate item can be double width. */
    if ((number % 2) == 1 && (index + 2) == number) {
      width += itemwidth;
    }
    /* For the last two items, use all the remaining space. */
    if ((index + 2) >= number) {
      width += (COLS % itemwidth);
    }
    post_one_key_curses(s->keystr, _(f->tag), width);
    ++index;
  }
  wrefresh(footwin);
}

/* Redetermine `file->cursor_row` form the current position relative to `file->edittop`, and put the cursor in the edit window at (`file->cursor_row`, `file->current_x`) */
void place_the_cursor_curses_for(openfilestruct *const file) {
  ASSERT(file);
  Ulong column;
  place_the_cursor_for_internal(file, editwincols, &column);
  if (file->cursor_row < editwinrows) {
    wmove(midwin, file->cursor_row, (margin + column));
  }
  else {
    statusline_curses(ALERT, "Misplaced cursor -- please report a bug");
  }
  /* Only needed for NetBSD curses. */
# ifdef _CURSES_H_
  wnoutrefresh(midwin);
# endif
}

/* Warn the user on the status bar and pause for a moment, so that the message can be noticed and read.  Ncurses version. */
void warn_and_briefly_pause_curses(const char *const restrict message) {
  ASSERT(message);
  blank_bottombars();
  statusline_curses(ALERT, "%s", message);
  lastmessage = VACUUM;
  napms(1500);
}

/* Draw the marked region of `row`. */
void draw_row_marked_region_for_curses(openfilestruct *const file, int row, const char *const restrict converted, linestruct *const line, Ulong from_col) {
  ASSERT(file);
  /* The lines where the marked region begins and ends. */
  linestruct *top;
  linestruct *bot;
  /* The x positions where the marked region begins and ends. */
  Ulong top_x;
  Ulong bot_x;
  /* The column where the painting starts.  Zero-based. */
  int start_col;
  /* The place in `converted` from where painting starts. */
  const char *thetext;
  /* The number of characters to paint.  Negative means all. */
  int paintlen = -1;
  Ulong endcol;
  /* If the line is at least partialy selected, paint the marked part. */
  if (line_in_marked_region_for(file, line)) {
    /* Get the full region of the mark. */
    get_region_for(file, &top, &top_x, &bot, &bot_x);
    /* If the marked region is from the start of this line. */
    if (top->lineno < line->lineno || top_x < from_x) {
      top_x = from_x;
    }
    /* If the marked region is below the end of this line. */
    if (bot->lineno > line->lineno || bot_x > till_x) {
      bot_x = till_x;
    }
    /* Only paint if the marked part of the line is on this page. */
    if (top_x < till_x && bot_x > from_x) {
      /* Compute on witch screen column to start painting. */
      start_col = (wideness(line->data, top_x) - from_col);
      CLAMP_MIN(start_col, 0);
      thetext = (converted + actual_x(converted, start_col));
      /* If the end of the mark if onscreen, compute how menu characters to paint.  Otherwise, just paint all. */
      if (bot_x < till_x) {
        endcol = (wideness(line->data, bot_x) - from_col);
        paintlen = actual_x(thetext, (endcol - start_col));
      }
      wattron(midwin, interface_color_pair[config->selectedtext_color]);
      mvwaddnstr(midwin, row, (margin + start_col), thetext, paintlen);
      wattroff(midwin, interface_color_pair[config->selectedtext_color]);
    }
  }
}

/* Draw the marked region of `row`. */
void draw_row_marked_region_curses(int row, const char *const restrict converted, linestruct *const line, Ulong from_col) {
  draw_row_marked_region_for_curses(TUI_OF, row, converted, line, from_col);
}

/* Draw the given text on the given row of the edit window.  line is the line to be drawn, and converted
 * is the actual string to be written with tabs and control characters replaced by strings of regular
 * characters.  'from_col' is the column number of the first character of this "page". */
void draw_row_curses_for(openfilestruct *const file, int row, const char *const restrict converted, linestruct *const line, Ulong from_col) {
  ASSERT(file);
  render_line_text(row, converted, line, from_col);
  if (ISSET(EXPERIMENTAL_FAST_LIVE_SYNTAX)) {
    apply_syntax_to_line(row, converted, line, from_col);
  }
  /* If there are color rules (and coloring is turned on), apply them. */
  else if (file->syntax && !ISSET(NO_SYNTAX)) {
    const colortype *varnish = file->syntax->color;
    /* If there are multiline regexes, make sure this line has a cache. */
    if (file->syntax->multiscore > 0 && !line->multidata) {
      line->multidata = xmalloc(file->syntax->multiscore * sizeof(short));
    }
    /* Iterate through all the coloring regexes. */
    for (; varnish; varnish = varnish->next) {
      /* Where in the line we currently begin looking for a match. */
      Ulong index = 0;
      /* The starting column of a piece to paint.  Zero-based. */
      int start_col = 0;
      /* The number of characters to paint. */
      int paintlen = 0;
      /* The place in converted from where painting starts. */
      const char *thetext;
      /* The match positions of a single-line regex. */
      regmatch_t match;
      /* The first line before line that matches 'start'. */
      const linestruct *start_line = line->prev;
      /* The match positions of the start and end regexes. */
      regmatch_t startmatch, endmatch;
      /* First case: varnish is a single-line expression. */
      if (!varnish->end) {
        while (index < PAINT_LIMIT && index < till_x) {
          /* If there is no match, go on to the next line. */
          if (regexec(varnish->start, &line->data[index], 1, &match, (index == 0) ? 0 : REG_NOTBOL) != 0) {
            break;
          }
          /* Translate the match to the beginning of the line. */
          match.rm_so += index;
          match.rm_eo += index;
          index = match.rm_eo;
          /* If the match is offscreen to the right, this rule is
           * done. */
          if (match.rm_so >= (int)till_x) {
            break;
          }
          /* If the match has length zero, advance over it. */
          if (match.rm_so == match.rm_eo) {
            if (line->data[index] == '\0') {
              break;
            }
            index = step_right(line->data, index);
            continue;
          }
          /* If the match is offscreen to the left, skip to next. */
          if (match.rm_eo <= (int)from_x) {
            continue;
          }
          if (match.rm_so > (int)from_x) {
            start_col = wideness(line->data, match.rm_so) - from_col;
          }
          thetext  = converted + actual_x(converted, start_col);
          paintlen = actual_x(thetext, wideness(line->data, match.rm_eo) - from_col - start_col);
          midwin_mv_add_nstr_wattr(row, (margin + start_col), thetext, paintlen, varnish->attributes);
        }
        continue;
      }
      /* Second case: varnish is a multiline expression.  Assume nothing gets painted until proven otherwise below. */
      line->multidata[varnish->id] = NOTHING;
      if (start_line && !start_line->multidata) {
        statusline(ALERT, "Missing multidata -- please report a bug");
      }
      else {
        /* If there is an unterminated start match before the current line, we need to look for an end match first. */
        if (start_line && (start_line->multidata[varnish->id] == WHOLELINE || start_line->multidata[varnish->id] == STARTSHERE)) {
          /* If there is no end on this line, paint whole line, and be done. */
          if (regexec(varnish->end, line->data, 1, &endmatch, 0) == REG_NOMATCH) {
            midwin_mv_add_nstr_wattr(row, margin, converted, -1, varnish->attributes);
            line->multidata[varnish->id] = WHOLELINE;
            continue;
          }
          /* Only if it is visible, paint the part to be coloured. */
          if (endmatch.rm_eo > (int)from_x) {
            paintlen = actual_x(converted, wideness(line->data, endmatch.rm_eo) - from_col);
            midwin_mv_add_nstr_wattr(row, margin, converted, paintlen, varnish->attributes);
          }
          line->multidata[varnish->id] = ENDSHERE;
        }
      }
      /* Second step: look for starts on this line, but begin looking only after an end match, if there is one. */
      index = (paintlen == 0) ? 0 : endmatch.rm_eo;
      while (index < PAINT_LIMIT && regexec(varnish->start, line->data + index, 1, &startmatch, (index == 0) ? 0 : REG_NOTBOL) == 0) {
        /* Make the match relative to the beginning of the line. */
        startmatch.rm_so += index;
        startmatch.rm_eo += index;
        if (startmatch.rm_so > (int)from_x) {
          start_col = wideness(line->data, startmatch.rm_so) - from_col;
        }
        thetext = converted + actual_x(converted, start_col);
        if (regexec(varnish->end, line->data + startmatch.rm_eo, 1, &endmatch, (startmatch.rm_eo == 0) ? 0 : REG_NOTBOL) == 0) {
          /* Make the match relative to the beginning of the line. */
          endmatch.rm_so += startmatch.rm_eo;
          endmatch.rm_eo += startmatch.rm_eo;
          /* Only paint the match if it is visible on screen and it is more than zero characters long. */
          if (endmatch.rm_eo > (int)from_x && endmatch.rm_eo > startmatch.rm_so) {
            paintlen = actual_x(thetext, wideness(line->data, endmatch.rm_eo) - from_col - start_col);
            midwin_mv_add_nstr_wattr(row, margin + start_col, thetext, paintlen, varnish->attributes);
            line->multidata[varnish->id] = JUSTONTHIS;
          }
          index = endmatch.rm_eo;
          /* If both start and end match are anchors, advance. */
          if (startmatch.rm_so == startmatch.rm_eo && endmatch.rm_so == endmatch.rm_eo) {
            if (!line->data[index]) {
              break;
            }
            index = step_right(line->data, index);
          }
          continue;
        }
        /* Paint the rest of the line, and we're done. */
        midwin_mv_add_nstr_wattr(row, margin + start_col, thetext, -1, varnish->attributes);
        line->multidata[varnish->id] = STARTSHERE;
        break;
      }
    }
  }
  if (stripe_column > (long)from_col && !inhelp && (!sequel_column || stripe_column <= (long)sequel_column) && stripe_column <= (long)(from_col + editwincols)) {
    long  target_column = (stripe_column - from_col - 1);
    Ulong target_x      = actual_x(converted, target_column);
    char  striped_char[MAXCHARLEN];
    Ulong charlen = 1;
    if (*(converted + target_x)) {
      charlen       = collect_char((converted + target_x), striped_char);
      target_column = wideness(converted, target_x);
#ifdef USING_OLDER_LIBVTE
    }
    else if ((target_column + 1) == editwincols) {
      /* Defeat a VTE bug -- see https://sv.gnu.org/bugs/?55896. */
# ifdef ENABLE_UTF8
      if (using_utf8()) {
        striped_char[0] = '\xC2';
        striped_char[1] = '\xA0';
        charlen         = 2;
      }
    else
# endif
      striped_char[0] = '.';
#endif
    }
    else {
      striped_char[0] = ' ';
    }
    if (ISSET(NO_NCURSES)) {
      
    }
    else {
      mv_add_nstr_color(midwin, row, (margin + target_column), striped_char, charlen, GUIDE_STRIPE);
    }
  }
  draw_row_marked_region_curses(row, converted, line, from_col);
}

/* Draw the given text on the given row of the edit window.  line is the line to be drawn, and converted
 * is the actual string to be written with tabs and control characters replaced by strings of regular
 * characters.  'from_col' is the column number of the first character of this "page". */
void draw_row_curses(int row, const char *const restrict converted, linestruct *const line, Ulong from_col) {
  draw_row_curses_for(openfile, row, converted, line, from_col);
}

/* Redraw the given line so that the character at the given index is visible -- if necessary, scroll the line
 * horizontally (when not softwrapping). Return the number of rows "consumed" (relevant when softwrapping). */
int update_line_curses_for(openfilestruct *const file, linestruct *const line, Ulong index) {
  ASSERT(file);
  ASSERT(line);
  /* The row in the edit window we will be updating. */
  int row;
  /* The data of the line with tabs and control characters expanded. */
  char *converted;
  /* From which column a horizontally scrolled line is displayed. */
  Ulong from_col;
  /* Just return early when running in gui mode. */
  if (ISSET(USING_GUI)) {
    return 0;
  }
  else if (ISSET(SOFTWRAP)) {
    return update_softwrapped_line_curses_for(file, line);
  }
  sequel_column = 0;
  row = (line->lineno - file->edittop->lineno);
  from_col = get_page_start(wideness(line->data, index), editwincols);
  /* Expand the piece to be drawn to its representable form, and draw it. */
  converted = display_string(line->data, from_col, editwincols, TRUE, FALSE);
  draw_row_curses_for(file, row, converted, line, from_col);
  free(converted);
  if (!ISSET(NO_NCURSES)) {
    if (from_col > 0) {
      mvwaddchwattr(midwin, row, margin, '<', hilite_attribute);
    }
    if (has_more) {
      mvwaddchwattr(midwin, row, (COLS - 1), '>', hilite_attribute);
    }
  }
  if (spotlighted && line == file->current) {
    spotlight_curses_for(file, light_from_col, light_to_col);
  }
  return 1;
}

/* Redraw the given line so that the character at the given index is visible -- if necessary, scroll the line
 * horizontally (when not softwrapping). Return the number of rows "consumed" (relevant when softwrapping). */
int update_line_curses(linestruct *const line, Ulong index) {
  return update_line_curses_for(CTX_OF, line, index);
}

/* Redraw all the chunks of the given line (as far as they fit onscreen), unless it's edittop,
 * which will be displayed from column firstcolumn.  Return the number of rows that were "consumed". */
int update_softwrapped_line_curses_for(openfilestruct *const file, linestruct *const line) {
  ASSERT(file);
  ASSERT(line);
  /* The first row in the edit window that gets updated. */
  int starting_row;
  /* The row in the edit window we will write to. */
  int row = 0;
  /* An iterator needed to find the relevent row. */
  linestruct *someline = file->edittop;
  /* The starting column of the current chunk. */
  Ulong from_col = 0;
  /* The end column of the current_chunk. */
  Ulong to_col = 0;
  /* The data of the chunk with tabs and controll chars expanded. */
  char *converted;
  /* This tells the softwrapping rutine to start at begining-of-line. */
  bool kickoff = TRUE;
  /* Becomes 'TRUE' when the last chunk of the line has been reached. */
  bool end_of_line = FALSE;
  if (line == file->edittop) {
    from_col = file->firstcolumn;
  }
  else {
    row -= chunk_for(editwincols, file->firstcolumn, file->edittop);
  }
  /* Find out on which screen row the target line should be shown. */
  while (someline != line && someline) {
    row += (1 + extra_chunks_in(editwincols, someline));
    someline = someline->next;
  }
  /* If the first chunk is offscreen, don't even try to display it. */
  if (row < 0 || row >= editwinrows) {
    return 0;
  }
  starting_row = row;
  while (!end_of_line && row < editwinrows) {
    to_col = get_softwrap_breakpoint(line->data, from_col, &kickoff, &end_of_line, editwincols);
    sequel_column = (end_of_line ? 0 : to_col);
    /* Convert the chunk to its displayable form and draw it. */
    converted = display_string(line->data, from_col, (to_col - from_col), TRUE, FALSE);
    draw_row_curses_for(file, row++, converted, line, from_col);
    free(converted);
    from_col = to_col;
  }
  if (spotlighted && line == file->current) {
    spotlight_softwrapped_curses(light_from_col, light_to_col);
  }
  return (row - starting_row);
}

/* Redraw all the chunks of the given line (as far as they fit onscreen), unless it's edittop,
 * which will be displayed from column firstcolumn.  Return the number of rows that were "consumed". */
int update_softwrapped_line_curses(linestruct *const line) {
  return update_softwrapped_line_curses_for(openfile, line);
}

/* ----------------------------- Spotlight curses ----------------------------- */

/* Highlight the text between the given two columns on the current line. */
void spotlight_curses_for(openfilestruct *const file, Ulong from_col, Ulong to_col) {
  ASSERT(file);
  Ulong right_edge = (get_page_start(from_col, editwincols) + editwincols);
  bool  overshoots = (to_col > right_edge);
  char *word;
  place_the_cursor_curses_for(file);
  /* Limit the end column to the edge of the screen. */
  if (overshoots) {
    to_col = right_edge;
  }
  /* If the target text is of zero length, highlight a space instead. */
  if (to_col == from_col) {
    word = COPY_OF(" ");
    ++to_col;
  }
  else {
    word = display_string(file->current->data, from_col, (to_col - from_col), FALSE, overshoots);
  }
  wattron(midwin, interface_color_pair[SPOTLIGHTED]);
  waddnstr(midwin, word, actual_x(word, to_col));
  if (overshoots) {
    mvwaddch(midwin, file->cursor_row, (COLS - 1 - sidebar), '>');
  }
  wattroff(midwin, interface_color_pair[SPOTLIGHTED]);
  free(word);
}

/* Highlight the text between the given two columns on the current line. */
void spotlight_curses(Ulong from_col, Ulong to_col) {
  spotlight_curses_for(openfile, from_col, to_col);
}

/* ----------------------------- Spotlight softwrapped curses ----------------------------- */

/* Highlight the text between the given two columns on the current line. */
void spotlight_softwrapped_curses_for(openfilestruct *const file, Ulong from_col, Ulong to_col) {
  ASSERT(file);
  long  row;
  Ulong leftedge = leftedge_for(editwincols, from_col, file->current);
  Ulong break_col;
  bool  end_of_line = FALSE;
  bool  kickoff     = TRUE;
  char *word;
  place_the_cursor_curses_for(file);
  row = file->cursor_row;
  while (row < editwinrows) {
    break_col = get_softwrap_breakpoint(file->current->data, leftedge, &kickoff, &end_of_line, editwincols);
    /* If the highlighting ends on this chunk, we can stop after it. */
    if (break_col >= to_col) {
      end_of_line = TRUE;
      break_col   = to_col;
    }
    /* If the target text is of zero length, highlight a space instead. */
    if (break_col == from_col) {
      word = COPY_OF(" ");
      break_col++;
    }
    else {
      word = display_string(file->current->data, from_col, (break_col - from_col), FALSE, FALSE);
    }
    wattron(midwin, interface_color_pair[SPOTLIGHTED]);
    waddnstr(midwin, word, actual_x(word, break_col));
    wattroff(midwin, interface_color_pair[SPOTLIGHTED]);
    free(word);
    if (end_of_line) {
      break;
    }
    wmove(midwin, ++row, margin);
    leftedge = break_col;
    from_col = break_col;
  }
}

/* Highlight the text between the given two columns on the current line. */
void spotlight_softwrapped_curses(Ulong from_col, Ulong to_col) {
  spotlight_softwrapped_curses_for(openfile, from_col, to_col);
}
