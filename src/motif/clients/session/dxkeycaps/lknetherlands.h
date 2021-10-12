/*
 * *****************************************************************
 * *                                                               *
 * *    Copyright (c) Digital Equipment Corporation, 1991, 1994    *
 * *                                                               *
 * *   All Rights Reserved.  Unpublished rights  reserved  under   *
 * *   the copyright laws of the United States.                    *
 * *                                                               *
 * *   The software contained on this media  is  proprietary  to   *
 * *   and  embodies  the  confidential  technology  of  Digital   *
 * *   Equipment Corporation.  Possession, use,  duplication  or   *
 * *   dissemination of the software and media is authorized only  *
 * *   pursuant to a valid written license from Digital Equipment  *
 * *   Corporation.                                                *
 * *                                                               *
 * *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
 * *   by the U.S. Government is subject to restrictions  as  set  *
 * *   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
 * *   or  in  FAR 52.227-19, as applicable.                       *
 * *                                                               *
 * *****************************************************************
 */
/*
 * HISTORY
 */
/*
 * @(#)$RCSfile: lknetherlands.h,v $ $Revision: 1.1.4.3 $ (DEC) $Date: 1994/01/24 19:20:01 $
 */
/* xkeycaps, Copyright (c) 1991 Jamie Zawinski <jwz@lucid.com>
 *
 * This file describes the LK444 (102-key DEC PC) keyboard.
 * By Steven W. Orr <steveo@world.std.com>.
 */

static struct key lknetherlands_row1 [] = {
 {0x55, 0, 0, 8, 8,	0,	XK_Escape},
 {0,	0,	0,	8, 8},
 {0x56, 0, 0, 8, 8,	0,	XK_F1},
 {0x57, 0, 0, 8, 8,	0,	XK_F2},
 {0x58, 0, 0, 8, 8,	0,	XK_F3},
 {0x59, 0, 0, 8, 8,	0,	XK_F4},
 {0,	0,	0,	4, 8},
 {0x5a, 0, 0, 8, 8,	0,	XK_F5},
 {0x64, 0, 0, 8, 8,	0,	XK_F6},
 {0x65, 0, 0, 8, 8,	0,	XK_F7},
 {0x66, 0, 0, 8, 8,	0,	XK_F8},
 {0,	0,	0,	4, 8},
 {0x67, 0, 0, 8, 8,	0,	XK_F9},
 {0x68, 0, 0, 8, 8,	0,	XK_F10},
 {0x71, 0, 0, 8, 8,	0,	XK_F11},
 {0x72, 0, 0, 8, 8,	0,	XK_F12},
 {0,	0,	0,	4, 8},
 {0x73, 0, 0, 8, 8,	0,	XK_Print, XK_Sys_Req},
 {0x74, 0, 0, 8, 8,   0,  XK_Scroll_Lock},
 {0x7c, 0, 0, 8, 8,   0,  XK_Pause},
};

static struct key lknetherlands_row2 [] = {
 {0xbf, StrXK_section, "@", 8, 8,	0,	XK_at, XK_section, XK_notsign, XK_at, XK_section, XK_notsign},
 {0xc0, "!", "1", 8, 8,	0,	XK_1, XK_exclam, XK_onesuperior},
 {0xc5, "\"", "2",8, 8,	0,	XK_2, XK_quotedbl, XK_twosuperior},
 {0xcb, "#", "3", 8, 8,	0,	XK_3, XK_numbersign, XK_threesuperior},
 {0xd0, "$", "4", 8, 8,	0,	XK_4, XK_dollar, XK_onequarter},
 {0xd6, "", "5", 8, 8,	0,	XK_5, XK_percent, XK_onehalf},
 {0xdb, "&", "6", 8, 8,	0,	XK_6, XK_ampersand, XK_threequarters},
 {0xe0, "_", "7", 8, 8,	0,	XK_7, XK_underscore, XK_sterling},
 {0xe5, "(", "8", 8, 8,	0,	XK_8, XK_parenleft, XK_braceleft},
 {0xea, ")", "9", 8, 8,	0,	XK_9, XK_parenright, XK_braceright},
 {0xef, "'", "0", 8, 8,	0,	XK_0, XK_apostrophe},
 {0xf9, "?", "/", 8, 8,	0,	XK_slash, XK_question, XK_backslash},
 {0xf5, "~", StrXK_degree, 8, 8,	0,	XK_degree, XK_asciitilde, XK_cedilla},
/*DEL*/
 {0xbc, 0, 0, 16, 8,	0,	XK_Delete},
 {0,	0,	0,	4, 8},
 {0x8b, 0, 0, 8, 8,   0,  XK_Home},
 {0x8a, 0, 0, 8, 8,   0,  XK_Insert},
 {0x8c, 0, 0, 8, 8,   0,  XK_Prior},
 {0,    0,  0,  4, 8},
 {0xa1, 0, 0, 8, 8,   0,  XK_Num_Lock, XK_KP_F1},
 {0xa2, 0, 0, 8, 8,   0,  XK_KP_Divide, XK_KP_F2},
 {0xa3, 0, 0, 8, 8,   0,  XK_KP_Multiply, XK_KP_F3},
 {0xa4, 0, 0, 8, 8,   0,  XK_KP_Subtract, XK_KP_F4},
};

static struct key lknetherlands_row3 [] = {
 {0xbe, 0, 0, 12, 8,	0,	XK_Tab},
 {0xc1, 0, "Q", 8, 8,	0,	XK_Q},
 {0xc6, 0, "W", 8, 8,	0,	XK_W},
 {0xcc, 0, "E", 8, 8,	0,	XK_E},
 {0xd1, "R", "r", 8, 8,	0,	XK_r, XK_R, XK_paragraph},
 {0xd7, 0, "T", 8, 8,	0,	XK_T},
 {0xdc, 0, "Y", 8, 8,	0,	XK_Y},
 {0xe1, 0, "U", 8, 8,	0,	XK_U},
 {0xe6, 0, "I", 8, 8,	0,	XK_I},
 {0xeb, 0, "O", 8, 8,	0,	XK_O},
 {0xf0, 0, "P", 8, 8,	0,	XK_P},
 {0xfa, "^", StrXK_diaeresis, 8, 8,	0,	XK_diaeresis, XK_asciicircum},
 {0xf6, StrXK_brokenbar, "*", 8, 8,	0,	XK_asterisk, XK_brokenbar},
 {0,    0,  0, 	2, 7},
 {0xbd, 0, 0, 10, 16,   0,  XK_Return},
 {0,    0,  0,	4, 7},
/*KPDEL*/
 {0x8d, 0, 0, 8, 8,   0,  XK_Delete},
 {0x8e, 0, 0, 8, 8,   0,  XK_End},
 {0x8f, 0, 0, 8, 8,   0,  XK_Next},
 {0,    0,  0,  4, 8},
 {0x9d, 0, 0, 8, 8,   0, XK_KP_7, XK_Home},
 {0x9e, 0, 0, 8, 8,   0, XK_KP_8, XK_Up},
 {0x9f, 0, 0, 8, 8,   0, XK_KP_9, XK_Prior},
 {0x9c, 0, 0, 8, 16,  0, XK_KP_Add},
};

static struct key lknetherlands_row4 [] = {
 {0xb0, 0, 0, 14, 8,	LockMask,	XK_Caps_Lock},
 {0xc2, 0, "A", 8, 8,	0,	XK_A},
 {0xc7, "S", "s", 8, 8,	0,	XK_s, XK_S, XK_ssharp},
 {0xcd, 0, "D", 8, 8,	0,	XK_D},
 {0xd2, 0, "F", 8, 8,	0,	XK_F},
 {0xd8, 0, "G", 8, 8,	0,	XK_G},
 {0xdd, 0, "H", 8, 8,	0,	XK_H},
 {0xe2, 0, "J", 8, 8,	0,	XK_J},
 {0xe7, 0, "K", 8, 8,	0,	XK_K},
 {0xec, 0, "L", 8, 8,	0,	XK_L},
 {0xf2, StrXK_plusminus, "+", 8, 8,	0,	XK_plus, XK_plusminus},
 {0xfb, "`", StrXK_acute, 8, 8,	0,	XK_acute, XK_grave},
 {0xf7, ">", "<", 8, 8,	0,	XK_less, XK_greater},
 {0,    0,  0, 42, 7},
/*LEFT*/
 {0x99, 0, 0, 8, 8,	0, XK_Left,	XK_KP_4, XK_Left},
 {0x9a, 0, 0, 8, 8,	0, XK_KP_5},
 {0x9b, 0, 0, 8, 8,	0, XK_Right, XK_KP_6, XK_Right},
};

static struct key lknetherlands_row5 [] = {
 {0xae, 0, 0, 10, 8,	ShiftMask,	XK_Shift_L},
 {0xc9, "[", "]", 8, 	8,   0,      XK_bracketright, XK_bracketleft, XK_brokenbar},
 {0xc3, "Z", "z", 8, 	8,	0,		XK_z, XK_Z, XK_guillemotleft},
 {0xc8, "X", "x", 8, 	8,	0,		XK_x, XK_X, XK_guillemotright},
 {0xce, "C", "c", 8, 	8,	0,		XK_c, XK_C, XK_cent},
 {0xd3, 0, "V", 8, 	8,	0,		XK_V},
 {0xd9, 0, "B", 8, 	8,	0,		XK_B},
 {0xde, 0, "N", 8, 	8,	0,		XK_N},
 {0xe3, "M", "m", 8, 	8,	0,		XK_m, XK_M, XK_mu},
 {0xe8, ";", ",", 8, 	8,	0,		XK_comma, XK_semicolon},
 {0xed, ":", ".", 8, 	8,	0,		XK_period, XK_colon, XK_abovedot},
 {0xf3, "=", "-", 8, 	8,	0,		XK_minus, XK_equal},
/*SHIFT*/
 {0xab, 0, 0, 22, 8,	ShiftMask,	XK_Shift_R},
 {0,	0,	0,	12, 8},
 {0xaa, 0, 0, 8, 8,	0,		XK_Up},
 {0,	0,	0,	12, 8},
 {0x96, 0, 0, 8, 8,   0, XK_KP_1, XK_End},
 {0x97, 0, 0, 8, 8,   0, XK_KP_2, XK_Down},
 {0x98, 0, 0, 8, 8,   0, XK_KP_3, XK_Next},
 {0x95, 0, 0, 8, 16,	0, XK_KP_Enter},
};

static struct key lknetherlands_row6 [] = {
 {0xaf, 0, 0, 12, 8,	ControlMask,	XK_Control_L},
 {0,	0,	0,	8, 8},
 {0xac, 0, 0, 12, 8,	Mod1Mask,	XK_Alt_L, XK_Meta_L},
 {0xd4, 0, " ", 57, 8,	0,		XK_space},
 {0xb2, 0, 0, 12, 8,	Mod3Mask,	XK_Mode_switch},
 {0,	0,	0,	7, 8},
 {0xad, 0, 0, 12, 8,	ControlMask,	XK_Control_R},
 {0,	0,	0,	4, 8},
 {0xa7, 0, 0, 8, 8,	0,		XK_Left},
 {0xa9, 0, 0, 8, 8,	0,		XK_Down},
 {0xa8, 0, 0, 8, 8,	0,		XK_Right},
 {0,	0,	0,	4, 8},
 {0x92, 0, 0, 16, 8,  0,      XK_KP_0, XK_Insert},
 {0x94, 0, 0, 8, 8,   0,      XK_KP_Decimal, XK_Delete},
};

static struct row lknetherlands_rows [] = {
  { sizeof (lknetherlands_row1) / sizeof (struct key), 8, lknetherlands_row1 },
  { 0, 8, 0 },
  { sizeof (lknetherlands_row2) / sizeof (struct key), 8, lknetherlands_row2 },
  { sizeof (lknetherlands_row3) / sizeof (struct key), 8, lknetherlands_row3 },
  { sizeof (lknetherlands_row4) / sizeof (struct key), 8, lknetherlands_row4 },
  { sizeof (lknetherlands_row5) / sizeof (struct key), 8, lknetherlands_row5 },
  { sizeof (lknetherlands_row6) / sizeof (struct key), 8, lknetherlands_row6 },
};

static struct keyboard lknetherlands = {
  "LK444 (Nederlands)",
  sizeof (lknetherlands_rows) / sizeof (struct row),
  lknetherlands_rows,
  6, 3, 3
};
