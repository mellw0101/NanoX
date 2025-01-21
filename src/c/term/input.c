#include "input.h"

#include "../ascii_defs.h"
#include "terminfo.h"

#include <sys/ioctl.h>
#include <sys/ttydefaults.h>

int parse_input_key(const Uchar *data, Ulong len, bool *shift) {
  Uchar modifiers = 6;
  if (!(terminfo->on_a_vt && (ioctl(STDIN_FILENO, TIOCLINUX, &modifiers) >= 0))) {
    modifiers = 0;
  }
  switch (len) {
    /* If there is only one byte of data and its a alpha-numeric ascii char, just return it. */
    case 1: {
      if (ASCII_ISALNUM(*data)) {
        return *data;
      }
      else if (*data == terminfo->backspace_key) {
        if (modifiers) {
          /* Shift */
          if (modifiers & 0x01) {
            *shift = TRUE;
            return TUI_KEY_SHIFT_BSP;
          }
          else if (modifiers & 0x04) {
            return TUI_KEY_CTRL_BSP;
          }
        }
        return TUI_KEY_BSP;
      }
      else if (*data == TUI_KEY_ENTER) {
        /* Only works for a true linux tty. */
        if (modifiers) {
          /* Shift-ctrl */
          if ((modifiers & 0x01) && (modifiers & 0x04)) {
            *shift = TRUE;
            return TUI_KEY_SHIFT_CTRL_ENTER;
          }
          /* Shift */
          if (modifiers & 0x01) {
            *shift = TRUE;
            return TUI_KEY_SHIFT_ENTER;
          }
          /* Ctrl */
          if (modifiers & 0x04) {
            return TUI_KEY_CTRL_ENTER;
          }
        }
        return TUI_KEY_ENTER;
      }
      else if ((terminfo->backspace_key == TUI_KEY_CTRL_H && *data == DEL) || (terminfo->backspace_key == DEL && *data == TUI_KEY_CTRL_H)) {
        return TUI_KEY_CTRL_BSP;
      }
      else {
        switch (data[0]) {
          case TUI_KEY_CTRL_C:
          case TUI_KEY_CTRL_D:
          case TUI_KEY_CTRL_E:
          case TUI_KEY_CTRL_F:
          case TUI_KEY_TAB:
          case TUI_KEY_ENTER:
          case TUI_KEY_CTRL_Q:
          case TUI_KEY_CTRL_R:
          case TUI_KEY_CTRL_S:
          case TUI_KEY_CTRL_W: {
            return data[0];
          }
        }
      }
      break;
    }
    case 2: {
      switch (data[0]) {
        case ESC: {
          switch (data[1]) {
            case TUI_KEY_TAB: {
              *shift = TRUE;
              return TUI_KEY_SHIFT_TAB;
            }
            case TUI_KEY_ENTER: {
              return TUI_KEY_ALT_ENTER;
            }
            case TUI_KEY_MINUS: {
              return TUI_KEY_ALT_MINUS;
            }
            case TUI_KEY_ZERO: {
              return TUI_KEY_ALT_ZERO;
            }
            case TUI_KEY_ONE: {
              return TUI_KEY_ALT_ONE;
            }
            case TUI_KEY_TWO: {
              return TUI_KEY_ALT_TWO;
            }
            case TUI_KEY_THREE: {
              return TUI_KEY_ALT_THREE;
            }
            case TUI_KEY_FOUR: {
              return TUI_KEY_ALT_FOUR;
            }
            case TUI_KEY_FIVE: {
              return TUI_KEY_ALT_FIVE;
            }
            case TUI_KEY_SIX: {
              return TUI_KEY_ALT_SIX;
            }
            case TUI_KEY_SEVEN: {
              return TUI_KEY_ALT_SEVEN;
            }
            case TUI_KEY_EIGHT: {
              return TUI_KEY_ALT_EIGHT;
            }
            case TUI_KEY_NINE: {
              return TUI_KEY_ALT_NINE;
            }
            case TUI_KEY_COLON: {
              return TUI_KEY_ALT_COLON;
            }
            case TUI_KEY_SEMICOLON: {
              return TUI_KEY_ALT_SEMICOLON;
            }
            case TUI_KEY_EQUAL: {
              return TUI_KEY_ALT_EQUAL;
            }
            case TUI_KEY_C: {
              *shift = TRUE;
              return TUI_KEY_SHIFT_ALT_C;
            }
            case TUI_KEY_Q: {
              *shift = TRUE;
              return TUI_KEY_SHIFT_ALT_Q;
            }
            case TUI_KEY_W: {
              *shift = TRUE;
              return TUI_KEY_SHIFT_ALT_W;
            }
            case TUI_KEY_Z: {
              *shift = TRUE;
              return TUI_KEY_SHIFT_ALT_Z;
            }
            case TUI_KEY_n: {
              return TUI_KEY_ALT_N;
            }
            case TUI_KEY_q: {
              return TUI_KEY_ALT_Q;
            }
            case TUI_KEY_w: {
              return TUI_KEY_ALT_W;
            }
            case TUI_KEY_z: {
              return TUI_KEY_ALT_Z;
            }
            case DEL: {
              return TUI_KEY_ALT_BSP;
            }
          }
          break;
        }
        case TUI_KEY_XTERM_ALT_KEY_STARTER: {
          switch (data[1]) {
            case 0xad: /* Xterm alt-minus. */ {
              return TUI_KEY_ALT_MINUS;
            }
            case 0xb0: /* Xterm alt-zero. */ {
              return TUI_KEY_ALT_ZERO;
            }
            case 0xb1: /* Xterm alt-one. */ {
              return TUI_KEY_ALT_ONE;
            }
            case 0xb2: /* Xterm alt-two. */ {
              return TUI_KEY_ALT_TWO;
            }
            case 0xb3: /* Xterm alt-three. */ {
              return TUI_KEY_ALT_THREE;
            }
            case 0xb4: /* Xterm alt-four. */ {
              return TUI_KEY_ALT_FOUR;
            }
            case 0xb5: /* Xterm alt-five. */ {
              return TUI_KEY_ALT_FIVE;
            }
            case 0xb6: /* Xterm alt-six. */ {
              return TUI_KEY_ALT_SIX;
            }
            case 0xb7: /* Xterm alt-seven. */ {
              return TUI_KEY_ALT_SEVEN;
            }
            case 0xb8: /* Xterm alt-eight. */ {
              return TUI_KEY_ALT_EIGHT;
            }
            case 0xb9: /* Xterm alt-nine. */ {
              return TUI_KEY_ALT_NINE;
            }
            case 0xba: /* Xterm atl-colon. */ {
              return TUI_KEY_ALT_COLON;
            }
            case 0xbb: /* Xterm alt-semicolon. */ {
              return TUI_KEY_ALT_SEMICOLON;
            }
            case 0xbd: /* Xterm alt-equal. */ {
              return TUI_KEY_ALT_EQUAL;
            }
            case 0x88: /* Xterm alt-bsp. */ {
              return TUI_KEY_ALT_BSP;
            }
          }
          break;
        }
        case TUI_KEY_XTERM_ALT_CHAR_STARTER: {
          switch (data[1]) {
            case 0x91: /* Xterm shift-alt-q. */ {
              *shift = TRUE;
              return TUI_KEY_SHIFT_ALT_Q;
            }
            case 0x97: /* Xterm shift-alt-w. */ {
              *shift = TRUE;
              return TUI_KEY_SHIFT_ALT_W;
            }
            case 0x9a: /* Xterm shift-alt-z. */ {
              *shift = TRUE;
              return TUI_KEY_SHIFT_ALT_Z;
            }
            case 0xb7: /* Xterm alt-w. */ {
              return TUI_KEY_ALT_W;
            }
            case 0xae: /* Xterm alt-n. */ {
              return TUI_KEY_ALT_N;
            }
            case 0xb1: /* Xterm alt-q. */ {
              return TUI_KEY_ALT_Q;
            }
            case 0xba: /* Xterm alt-z. */ {
              return TUI_KEY_ALT_Z;
            }
          }
          break;
        }
      }
      break;
    }
    case 3: {
      switch (data[0]) {
        case ESC: {
          switch (data[1]) {
            case TUI_KEY_CSI: {
              switch (data[2]) {
                case TUI_KEY_A: {
                  /* Only works for a true linux tty. */
                  if (modifiers) {
                    /* Shift-alt */
                    if ((modifiers & 0x01) && (modifiers & 0x08)) {
                      *shift = TRUE;
                      return TUI_KEY_SHIFT_ALT_UP;
                    }
                    /* Shift-ctrl */
                    if ((modifiers & 0x01) && (modifiers & 0x04)) {
                      *shift = TRUE;
                      return TUI_KEY_SHIFT_CTRL_UP;
                    }
                    /* Shift */
                    if (modifiers & 0x01) {
                      *shift = TRUE;
                      return TUI_KEY_SHIFT_UP;
                    }
                    /* Ctrl */
                    if (modifiers & 0x04) {
                      return TUI_KEY_CTRL_UP;
                    }
                  }
                  return TUI_KEY_UP;
                }
                case TUI_KEY_B: {
                  /* Only works for a true linux tty. */
                  if (modifiers) {
                    /* Shift-alt */
                    if ((modifiers & 0x01) && (modifiers & 0x08)) {
                      *shift = TRUE;
                      return TUI_KEY_SHIFT_ALT_DOWN;
                    }
                    /* Shift-ctrl */
                    if ((modifiers & 0x01) && (modifiers & 0x04)) {
                      *shift = TRUE;
                      return TUI_KEY_SHIFT_CTRL_DOWN;
                    }
                    /* Shift */
                    if (modifiers & 0x01) {
                      *shift = TRUE;
                      return TUI_KEY_SHIFT_DOWN;
                    }
                    /* Ctrl */
                    if (modifiers & 0x04) {
                      return TUI_KEY_CTRL_DOWN;
                    }
                  }
                  return TUI_KEY_DOWN;
                }
                case TUI_KEY_C: {
                  /* Only works for a true linux tty. */
                  if (modifiers) {
                    /* Shift-alt */
                    if ((modifiers & 0x01) && (modifiers & 0x08)) {
                      *shift = TRUE;
                      return TUI_KEY_SHIFT_ALT_RIGHT;
                    }
                    /* Shift-ctrl */
                    if ((modifiers & 0x01) && (modifiers & 0x04)) {
                      *shift = TRUE;
                      return TUI_KEY_SHIFT_CTRL_RIGHT;
                    }
                    /* Shift */
                    if (modifiers & 0x01) {
                      *shift = TRUE;
                      return TUI_KEY_SHIFT_RIGHT;
                    }
                    /* Ctrl */
                    if (modifiers & 0x04) {
                      return TUI_KEY_CTRL_RIGHT;
                    }
                  }
                  return TUI_KEY_RIGHT;
                }
                case TUI_KEY_D: {
                  /* Only works for a true linux tty. */
                  if (modifiers) {
                    /* Shift-alt */
                    if ((modifiers & 0x01) && (modifiers & 0x08)) {
                      *shift = TRUE;
                      return TUI_KEY_SHIFT_ALT_LEFT;
                    }
                    /* Shift-ctrl */
                    if ((modifiers & 0x01) && (modifiers & 0x04)) {
                      *shift = TRUE;
                      return TUI_KEY_SHIFT_CTRL_LEFT;
                    }
                    /* Shift */
                    if (modifiers & 0x01) {
                      *shift = TRUE;
                      return TUI_KEY_SHIFT_LEFT;
                    }
                    /* Ctrl */
                    if (modifiers & 0x04) {
                      return TUI_KEY_CTRL_LEFT;
                    }
                  }
                  return TUI_KEY_LEFT;
                }
                case TUI_KEY_Z: {
                  *shift = TRUE;
                  return TUI_KEY_SHIFT_TAB;
                }
              }
              break;
            }
            case TUI_KEY_O: {
              switch (data[2]) {
                case TUI_KEY_M: {
                  *shift = TRUE;
                  return TUI_KEY_SHIFT_ENTER;
                }
              }
              break;
            }
          }
          break;
        }
      }
      break;
    }
    case 4: {
      switch (data[0]) {
        case CAN: {
          switch (data[1]) {
            case TUI_KEY_AT: {
              switch (data[2]) {
                case TUI_KEY_s: {
                  switch (data[3]) /* Nether of these work for a true linux tty, or in xterm. */ {
                    case TUI_KEY_ENTER: {
                      return TUI_KEY_SUPER_ENTER;
                    }
                    case DEL: {
                      return TUI_KEY_SUPER_BSP;
                    }
                  }
                  break;
                }
              }
              break;
            }
          }
          break;
        }
      }
      break;
    }
    case 5: {
      switch (data[0]) {
        case CAN: {
          switch (data[1]) {
            case TUI_KEY_AT: {
              switch (data[2]) {
                case TUI_KEY_s: {
                  switch (data[3]) {
                    case ESC: {
                      switch (data[4]) {
                        case TUI_KEY_ENTER: {
                          return TUI_KEY_SUPER_ALT_ENTER;
                        }
                      }
                      break;
                    }
                  }
                  break;
                }
              }
              break;
            }
          }
          break;
        }
      }
      break;
    }
    case 6: {
      switch (data[0]) {
        case CAN: {
          switch (data[1]) {
            case TUI_KEY_AT: {
              switch (data[2]) {
                case TUI_KEY_s: {
                  switch (data[3]) {
                    case ESC: {
                      switch (data[4]) {
                        case TUI_KEY_O: {
                          switch (data[5]) {
                            case TUI_KEY_M: {
                              *shift = TRUE;
                              return TUI_KEY_SUPER_SHIFT_ENTER;
                            }
                          }
                          break;
                        }
                      }
                      break;
                    }
                  }
                  break;
                }
              }
              break;
            }
          }
          break;
        }
        case ESC: {
          switch (data[1]) {
            case TUI_KEY_CSI: {
              switch (data[2]) {
                case TUI_KEY_ONE: {
                  switch (data[3]) {
                    case TUI_KEY_SEMICOLON: {
                      switch (data[4]) {
                        case TUI_KEY_TWO: /* Shift */ {
                          switch (data[5]) {
                            case TUI_KEY_A: {
                              *shift = TRUE;
                              return TUI_KEY_SHIFT_UP;
                            }
                            case TUI_KEY_B: {
                              *shift = TRUE;
                              return TUI_KEY_SHIFT_DOWN;
                            }
                            case TUI_KEY_C: {
                              *shift = TRUE;
                              return TUI_KEY_SHIFT_RIGHT;
                            }
                            case TUI_KEY_D: {
                              *shift = TRUE;
                              return TUI_KEY_SHIFT_LEFT;
                            }
                          }
                          break;
                        }
                        case TUI_KEY_THREE: /* Alt */ {
                          switch (data[5]) {
                            case TUI_KEY_A: {
                              return TUI_KEY_ALT_UP;
                            }
                            case TUI_KEY_B: {
                              return TUI_KEY_ALT_DOWN;
                            }
                            case TUI_KEY_C: {
                              return TUI_KEY_ALT_RIGHT;
                            }
                            case TUI_KEY_D: {
                              return TUI_KEY_ALT_LEFT;
                            }
                          }
                          break;
                        }
                        case TUI_KEY_FOUR: /* Shift-alt */ {
                          switch (data[5]) {
                            case TUI_KEY_A: {
                              *shift = TRUE;
                              return TUI_KEY_SHIFT_ALT_UP;
                            }
                            case TUI_KEY_B: {
                              *shift = TRUE;
                              return TUI_KEY_SHIFT_ALT_DOWN;
                            }
                            case TUI_KEY_C: {
                              *shift = TRUE;
                              return TUI_KEY_SHIFT_ALT_RIGHT;
                            }
                            case TUI_KEY_D: {
                              *shift = TRUE;
                              return TUI_KEY_SHIFT_ALT_LEFT;
                            }
                          }
                          break;
                        }
                        case TUI_KEY_FIVE: /* Ctrl */ {
                          switch (data[5]) {
                            case TUI_KEY_A: {
                              return TUI_KEY_CTRL_UP;
                            }
                            case TUI_KEY_B: {
                              return TUI_KEY_CTRL_DOWN;
                            }
                            case TUI_KEY_C: {
                              return TUI_KEY_CTRL_RIGHT;
                            }
                            case TUI_KEY_D: {
                              return TUI_KEY_CTRL_LEFT;
                            }
                          }
                          break;
                        }
                        case TUI_KEY_SIX: /* Shift-ctrl */ {
                          switch (data[5]) {
                            case TUI_KEY_A: {
                              *shift = TRUE;
                              return TUI_KEY_SHIFT_CTRL_UP;
                            }
                            case TUI_KEY_B: {
                              *shift = TRUE;
                              return TUI_KEY_SHIFT_CTRL_DOWN;
                            }
                            case TUI_KEY_C: {
                              *shift = TRUE;
                              return TUI_KEY_SHIFT_CTRL_RIGHT;
                            }
                            case TUI_KEY_D: {
                              *shift = TRUE;
                              return TUI_KEY_SHIFT_CTRL_LEFT;
                            }
                          }
                          break;
                        }
                      }
                      break;
                    }
                  }
                  break;
                }
              }
              break;
            }
          }
          break;
        }
      }
      break;
    }
    case 7: {
      switch (data[0]) {
        case CAN: {
          switch (data[1]) {
            case TUI_KEY_AT: {
              switch (data[2]) {
                case TUI_KEY_s: {
                  switch (data[3]) {
                    case ESC: {
                      switch (data[4]) {
                        case ESC: {
                          switch (data[5]) {
                            case TUI_KEY_O: {
                              switch (data[6]) {
                                case TUI_KEY_M: {
                                  *shift = TRUE;
                                  return TUI_KEY_SUPER_SHIFT_ALT_ENTER;
                                }
                              }
                              break;
                            }
                          }
                          break;
                        }
                      }
                      break;
                    }
                  }
                  break;
                }
              }
              break;
            }
          }
          break;
        }
      }
      break;
    }
    /* case 10: {
      switch (data[0]) {
        case ESC: {
          switch (data[1]) {
            case TUI_KEY_CSI: {
              switch (data[2]) {
                case TUI_KEY_QUESTION_MARK: {
                  switch (data[3]) {
                    case TUI_KEY_SIX: {
                      switch (data[4]) {
                        case TUI_KEY_TWO: {
                          switch (data[5]) {
                            case TUI_KEY_SEMICOLON: {
                              switch (data[6]) {
                                case TUI_KEY_ONE: {
                                  switch (data[7]) {
                                    case TUI_KEY_SEMICOLON: {
                                      switch (data[8]) {
                                        case TUI_KEY_FOUR: {
                                          switch (data[9]) {
                                            case TUI_KEY_c: {
                                              return TUI_KEY_SHIFT_ALT_Z;
                                            }
                                          }
                                          break;
                                        }
                                      }
                                      break;
                                    }
                                  }
                                  break;
                                }
                              }
                              break;
                            }
                          }
                          break;
                        }
                      }
                      break;
                    }
                  }
                  break;
                }
              }
              break;
            }
          }
          break;
        } 
      }
      break;
    } */
  }
  return TUI_KEY_ERR;
}
