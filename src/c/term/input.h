#pragma once

#include "../../include/c_proto.h"

#define TUI_KEY_CTRL_C        CTRL('c')  /*   3 */
#define TUI_KEY_CTRL_D        0x04  /*   4 */
#define TUI_KEY_CTRL_E        0x05  /*   5 */
#define TUI_KEY_CTRL_F        0x06  /*   6 */
#define TUI_KEY_CTRL_G        0x07  /*   7 */
#define TUI_KEY_CTRL_H        0x08  /*   8 */
#define TUI_KEY_TAB           0x09  /*   9 */
#define TUI_KEY_ENTER         0x0d  /*  13 */
#define TUI_KEY_CTRL_Q        0x11  /*  17 */
#define TUI_KEY_CTRL_R        0x12  /*  18 */
#define TUI_KEY_CTRL_S        0x13  /*  19 */
#define TUI_KEY_CTRL_W        0x17  /*  23 */
#define TUI_KEY_MINUS         0x2d  /*  45 */
#define TUI_KEY_ZERO          0x30  /*  48 */
#define TUI_KEY_ONE           0x31  /*  49 */
#define TUI_KEY_TWO           0x32  /*  50 */
#define TUI_KEY_THREE         0x33  /*  51 */
#define TUI_KEY_FOUR          0x34  /*  52 */
#define TUI_KEY_FIVE          0x35  /*  53 */
#define TUI_KEY_SIX           0x36  /*  54 */
#define TUI_KEY_SEVEN         0x37  /*  55 */
#define TUI_KEY_EIGHT         0x38  /*  56 */
#define TUI_KEY_NINE          0x39  /*  57 */
#define TUI_KEY_COLON         0x3a  /*  58 */
#define TUI_KEY_SEMICOLON     0x3b  /*  59 */
#define TUI_KEY_EQUAL         0x3d  /*  61 */
#define TUI_KEY_QUESTION_MARK 0x3f  /*  63 */
#define TUI_KEY_AT            0x40  /*  64 */
#define TUI_KEY_A             0x41  /*  65 */
#define TUI_KEY_B             0x42  /*  66 */
#define TUI_KEY_C             0x43  /*  67 */
#define TUI_KEY_D             0x44  /*  68 */
#define TUI_KEY_M             0X4d  /*  77 */
#define TUI_KEY_O             0x4f  /*  79 */
#define TUI_KEY_Q             0x51  /*  81 */
#define TUI_KEY_W             0x57  /*  87 */
#define TUI_KEY_Z             0x5a  /*  90 */
#define TUI_KEY_c             0x63  /*  99 */
#define TUI_KEY_e             0x65  /* 101 */
#define TUI_KEY_g             0x67  /* 103 */
#define TUI_KEY_n             0x6e  /* 110 */
#define TUI_KEY_q             0x71  /* 113 */
#define TUI_KEY_r             0x72  /* 114 */
#define TUI_KEY_s             0x73  /* 115 */
#define TUI_KEY_w             0x77  /* 119 */
#define TUI_KEY_z             0x7a  /* 122 */

#define TUI_KEY_CSI   0x5b
#define TUI_KEY_ERR   -1

#define TUI_KEY_XTERM_ALT_KEY_STARTER  0xc2
#define TUI_KEY_XTERM_ALT_CHAR_STARTER 0xc3

#define TUI_KEY_BSP               0x1001
#define TUI_KEY_DEL               0x1002
#define TUI_KEY_UP                0x1003
#define TUI_KEY_DOWN              0x1004
#define TUI_KEY_RIGHT             0x1005
#define TUI_KEY_LEFT              0x1006

/* Alt */
#define TUI_KEY_ALT_ENTER         0x2001
#define TUI_KEY_ALT_MINUS         0x2002
#define TUI_KEY_ALT_ZERO          0x2003
#define TUI_KEY_ALT_ONE           0x2004
#define TUI_KEY_ALT_TWO           0x2005
#define TUI_KEY_ALT_THREE         0x2006
#define TUI_KEY_ALT_FOUR          0x2007
#define TUI_KEY_ALT_FIVE          0x2008
#define TUI_KEY_ALT_SIX           0x2009
#define TUI_KEY_ALT_SEVEN         0x2010
#define TUI_KEY_ALT_EIGHT         0x2011
#define TUI_KEY_ALT_NINE          0x2012
#define TUI_KEY_ALT_COLON         0x2013
#define TUI_KEY_ALT_SEMICOLON     0x2014
#define TUI_KEY_ALT_EQUAL         0x2015
#define TUI_KEY_ALT_N             0x2016
#define TUI_KEY_ALT_Q             0x2017
#define TUI_KEY_ALT_W             0x2018
#define TUI_KEY_ALT_Z             0x2019
#define TUI_KEY_ALT_BSP           0x2020
#define TUI_KEY_ALT_UP            0x2021
#define TUI_KEY_ALT_DOWN          0x2022
#define TUI_KEY_ALT_RIGHT         0x2023
#define TUI_KEY_ALT_LEFT          0x2024

/* Shift */
#define TUI_KEY_SHIFT_TAB         0x3001
#define TUI_KEY_SHIFT_ENTER       0x3002
#define TUI_KEY_SHIFT_UP          0x3003
#define TUI_KEY_SHIFT_DOWN        0x3004
#define TUI_KEY_SHIFT_RIGHT       0x3005
#define TUI_KEY_SHIFT_LEFT        0x3006
#define TUI_KEY_SHIFT_BSP         0x3007

/* Shift-alt */
#define TUI_KEY_SHIFT_ALT_C       0x3101
#define TUI_KEY_SHIFT_ALT_Q       0x3102
#define TUI_KEY_SHIFT_ALT_W       0x3103
#define TUI_KEY_SHIFT_ALT_Z       0x3104
#define TUI_KEY_SHIFT_ALT_UP      0x3105
#define TUI_KEY_SHIFT_ALT_DOWN    0x3106
#define TUI_KEY_SHIFT_ALT_RIGHT   0x3107
#define TUI_KEY_SHIFT_ALT_LEFT    0x3108

/* Shift-ctrl */
#define TUI_KEY_SHIFT_CTRL_UP     0x3201
#define TUI_KEY_SHIFT_CTRL_DOWN   0x3202
#define TUI_KEY_SHIFT_CTRL_RIGHT  0x3203
#define TUI_KEY_SHIFT_CTRL_LEFT   0x3204
#define TUI_KEY_SHIFT_CTRL_ENTER  0x3205

/* Ctrl */
#define TUI_KEY_CTRL_BSP          0x4001
#define TUI_KEY_CTRL_ENTER        0x4002
#define TUI_KEY_CTRL_UP           0x4003
#define TUI_KEY_CTRL_DOWN         0x4004
#define TUI_KEY_CTRL_RIGHT        0x4005
#define TUI_KEY_CTRL_LEFT         0x4006

/* Super */
#define TUI_KEY_SUPER_ENTER           0x5001
#define TUI_KEY_SUPER_BSP             0x5002

/* Super-alt */
#define TUI_KEY_SUPER_ALT_ENTER       0x5101

/* Super-shift */
#define TUI_KEY_SUPER_SHIFT_ENTER     0x5201
#define TUI_KEY_SUPER_SHIFT_ALT_ENTER 0x5301

_BEGIN_C_LINKAGE

int parse_input_key(const Uchar *data, Ulong len, bool *shift);

_END_C_LINKAGE
