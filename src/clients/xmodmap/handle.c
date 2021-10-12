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
 * xmodmap - program for loading keymap definitions into server
 *
 * $XConsortium: handle.c,v 1.25 91/07/18 10:26:00 rws Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <ctype.h>
#include "xmodmap.h"
#include "wq.h"

static XModifierKeymap *map = NULL;


/*
 * The routines in this file manipulate a queue of intructions.  Instead of
 * executing each line as it is entered, we build up a list of actions to 
 * take and execute them all at the end.  This allows us to find all errors
 * at once, and to preserve the context in which we are looking up keysyms.
 */

struct wq work_queue = {NULL, NULL};


/*
 * common utility routines
 */

KeyCode *KeysymToKeycodes(dpy, keysym, pnum_kcs)
    Display *dpy;
    KeySym keysym;
    int *pnum_kcs;
{
    KeyCode *kcs = NULL;
    int i, j;

    *pnum_kcs = 0;
    for (i = min_keycode; i <= max_keycode; i++) {
	for (j = 0; j < 8; j++) {
	    if (XKeycodeToKeysym(dpy, (KeyCode) i, j) == keysym) {
		if (!kcs)
		    kcs = (KeyCode *)malloc(sizeof(KeyCode));
		else
		    kcs = (KeyCode *)realloc((char *)kcs, *pnum_kcs + 1);
		kcs[*pnum_kcs] = i;
		*pnum_kcs += 1;
		break;
	    }
	}
    }
    return kcs;
}

char *copy_to_scratch (s, len)
    char *s;
    int len;
{
    static char *buf = NULL;
    static int buflen = 0;

    if (len > buflen) {
	if (buf) free (buf);
	buflen = (len < 40) ? 80 : (len * 2);
	buf = (char *) malloc (buflen+1);
    }
    if (len > 0)
      strncpy (buf, s, len);
    else 
      len = 0;

    buf[len] = '\0';
    return (buf);
}

static badheader ()
{
    fprintf (stderr, "%s:  %s:%d:  bad ", ProgramName, inputFilename, lineno);
    parse_errors++;
}

#define badmsg(what,arg) { badheader(); fprintf (stderr, what, arg); \
			   putc ('\n', stderr); }

#define badmsgn(what,s,len) badmsg (what, copy_to_scratch (s, len))

void initialize_map ()
{
    map = XGetModifierMapping (dpy);
    return;
}

static void do_keycode(), do_keysym(), finish_keycodes();
static void do_add(), do_remove(), do_clear(), do_pointer();
static int get_keysym_list();

int skip_word(), skip_space(), skip_chars();

static struct dt {
    char *command;			/* name of input command */
    int length;				/* length of command */
    void (*proc)();			/* handler */
} dispatch_table[] = {
    { "keycode", 7, do_keycode },
    { "keysym", 6, do_keysym },
    { "add", 3, do_add },
    { "remove", 6, do_remove },
    { "clear", 5, do_clear },
    { "pointer", 7, do_pointer },
    { NULL, 0, NULL }};

/*
 * handle_line - this routine parses the input line (which has had all leading
 * and trailing whitespace removed) and builds up the work queue.
 */

void handle_line (line, len)
    char *line;				/* string to parse */
    int len;				/* length of line */
{
    int n;
    struct dt *dtp;

    n = skip_chars (line, len);
    if (n < 0) {
	badmsg ("input line '%s'", line);
	return;
    }

    for (dtp = dispatch_table; dtp->command != NULL; dtp++) {
	if (n == dtp->length &&
	    strncmp (line, dtp->command, dtp->length) == 0) {

	    n += skip_space (line+n, len-n);
	    line += n, len -= n;

	    (*(dtp->proc)) (line, len);
	    return;
	}
    }

    fprintf (stderr, "%s:  unknown command on line %s:%d\n",
	     ProgramName, inputFilename, lineno);
    parse_errors++;
}

/*
 * the following routines are useful for parsing
 */ 

int skip_word (s, len)
    register char *s;
    register int len;
{
    register int n;

    n = skip_chars (s, len);
    return (n + skip_space (s+n, len-n));
}

int skip_chars (s, len)
    register char *s;
    register int len;
{
    register int i;

    if (len <= 0 || !s || *s == '\0') return (0);

    for (i = 0; i < len; i++) {
	if (isspace(s[i])) break;
    }
    return (i);
}

int skip_space (s, len)
    register char *s;
    register int len;
{
    register int i;

    if (len <= 0 || !s || *s == '\0') return (0);

    for (i = 0; i < len; i++) {
	if (!s[i] || !isspace(s[i])) break;
    }
    return (i);
}


int skip_until_char (s, len, c)
    register char *s;
    register int len;
    register char c;
{
    register int i;

    for (i = 0; i < len; i++) {
	if (s[i] == c) break;
    }
    return (i);
}

int skip_until_chars (s, len, cs, cslen)
    char *s;
    int len;
    register char *cs;
    register int cslen;
{
    int i;

    for (i = 0; i < len; i++) {
	register int j;
	register char c = s[i];

	for (j = 0; j < cslen; j++) {
	    if (c == cs[j]) goto done;
	}
    }
  done:
    return (i);
}

/*
 * The action routines.
 *
 * This is where the real work gets done.  Each routine is responsible for
 * parsing its input (note that the command keyword has been stripped off)
 * and adding to the work queue.  They are also in charge of outputting
 * error messages and returning non-zero if there is a problem.
 *
 * The following global variables are available:
 *     dpy                the display descriptor
 *     work_queue         linked list of opcodes
 *     inputFilename      name of the file being processed
 *     lineno             line number of current line in input file
 */

add_to_work_queue (p)			/* this can become a macro someday */
    union op *p;
{
    if (work_queue.head == NULL) {	/* nothing on the list */
	work_queue.head = work_queue.tail = p;	/* head, tail, no prev */
    } else {
	work_queue.tail->generic.next = p;  /* head okay, prev */
	work_queue.tail = p;		/* tail */
    }
    p->generic.next = NULL;

    if (verbose) {
	print_opcode (p);
    }
    return;
}

char *copystring (s, len)
    char *s;
    int len;
{
    char *retval;

    retval = (char *) malloc (len+1);
    if (retval) {
	strncpy (retval, s, len);
	retval[len] = '\0';
    }
    return (retval);
}

static Bool parse_number (str, val)
    char *str;
    unsigned int *val;
{
    char *fmt = "%d";

    if (*str == '0') {
	str++;
	fmt = "%o";
	if (*str == '\0') {
	    *val = 0;
	    return 1;
	}
	if (*str == 'x' || *str == 'X') {
	    str++;
	    fmt = "%x";
	}
    }
    return (sscanf (str, fmt, val) == 1);
}

static Bool parse_keysym (line, n, name, keysym)
    char *line;
    int n;
    char **name;
    KeySym *keysym;
{
    *name = copy_to_scratch (line, n);
    if (!strcmp(*name, "NoSymbol")) {
	*keysym = NoSymbol;
	return (True);
    }
    *keysym = XStringToKeysym (*name);
    if (*keysym == NoSymbol && '0' <= **name && **name <= '9')
	return parse_number(*name, keysym);
    return (*keysym != NoSymbol);
}

/*
 * do_keycode - parse off lines of the form
 *
 *                 "keycode" number "=" [keysym ...]
 *                           ^
 *
 * where number is in decimal, hex, or octal.  Any number of keysyms may be
 * listed.
 */

static void do_keycode (line, len)
    char *line;
    int len;
{
    int dummy;
    char *fmt = "%d";
    KeyCode keycode;

    if (len < 3 || !line || *line == '\0') {  /* 5=a minimum */
	badmsg ("keycode input line", NULL);
	return;
    }

    if (*line == '0') line++, len--, fmt = "%o";
    if (*line == 'x' || *line == 'X') line++, len--, fmt = "%x";

    dummy = 0;
    if (sscanf (line, fmt, &dummy) != 1 || dummy == 0) {
	badmsg ("keycode value", NULL);
	return;
    }
    keycode = (KeyCode) dummy;
    if ((int)keycode < min_keycode || (int)keycode > max_keycode) {
	badmsg ("keycode value (out of range)", NULL);
	return;
    }

    finish_keycodes (line, len, &keycode, 1);
}


/*
 * do_keysym - parse off lines of the form
 *
 *                 "keysym" keysym "=" [keysym ...]
 *                          ^
 *
 * The left keysyms has to be checked for validity and evaluated.
 */

static void do_keysym (line, len)
    char *line;
    int len;
{
    int n;
    KeyCode *keycodes;
    KeySym keysym;
    char *tmpname;

    if (len < 3 || !line || *line == '\0') {  /* a=b minimum */
	badmsg ("keysym input line", NULL);
	return;
    }

    n = skip_chars (line, len);
    if (n < 1) {
	badmsg ("target keysym name", NULL);
	return;
    }
    if (!parse_keysym(line, n, &tmpname, &keysym)) {
	badmsg ("keysym target key symbol '%s'", tmpname);
	return;
    }

    keycodes = KeysymToKeycodes (dpy, keysym, &n);
    if (n == 0) {
	badmsg ("keysym target keysym '%s', no corresponding keycodes",
		tmpname);
	return;
    }
    if (verbose) {
	int i;
	printf ("! Keysym %s (0x%lx) corresponds to keycode(s)",
		tmpname, (long) keysym);
	for (i = 0; i < n; i++)
	    printf (" 0x%x", keycodes[i]);
	printf("\n");
    }

    finish_keycodes (line, len, keycodes, n);
}

static void finish_keycodes (line, len, keycodes, count)
    char *line;
    int len;
    KeyCode *keycodes;
    int count;
{
    int n;
    KeySym *kslist;
    union op *uop;
    struct op_keycode *opk;
   
    n = skip_until_char (line, len, '=');
    line += n, len -= n;
    
    if (len < 1 || *line != '=') {	/* = minimum */
	badmsg ("keycode command (missing keysym list),", NULL);
	return;
    }
    line++, len--;			/* skip past the = */

    n = skip_space (line, len);
    line += n, len -= n;

    /* allow empty list */
    if (get_keysym_list (line, len, &n, &kslist) < 0)
	return;

    while (--count >= 0) {
	uop = AllocStruct (union op);
	if (!uop) {
	    badmsg ("attempt to allocate a %ld byte keycode opcode",
		    (long) sizeof (struct op_keycode));
	    return;
	}
	opk = &uop->keycode;

	opk->type = doKeycode;
	opk->target_keycode = keycodes[count];
	opk->count = n;
	opk->keysyms = kslist;

	add_to_work_queue (uop);
    }

#ifdef later
    /* make sure we handle any special keys */
    check_special_keys (keycode, n, kslist);
#endif
}


/*
 * parse_modifier - convert a modifier string name to its index
 */

struct modtab modifier_table[] = {	/* keep in order so it can be index */
    { "shift", 5, 0 },
    { "lock", 4, 1 },
    { "control", 7, 2 },
    { "mod1", 4, 3 },
    { "mod2", 4, 4 },
    { "mod3", 4, 5 },
    { "mod4", 4, 6 },
    { "mod5", 4, 7 },
    { "ctrl", 4, 2 },
    { NULL, 0, 0 }};

static int parse_modifier (line, n)
    register char *line;
    register int n;
{
    register int i;
    struct modtab *mt;

    /* lowercase for comparison against table */
    for (i = 0; i < n; i++) {
	if (isupper (line[i])) line[i] = tolower (line[i]);
    }

    for (mt = modifier_table; mt->name; mt++) {
	if (n == mt->length && strncmp (line, mt->name, n) == 0)
	  return (mt->value);
    }
    return (-1);
}


/*
 * do_add - parse off lines of the form
 *
 *                 add MODIFIER = keysym ...
 *                     ^
 * where the MODIFIER is one of Shift, Lock, Control, Mod[1-5] where case
 * is not important.  There should also be an alias Ctrl for control.
 */

static void do_add (line, len)
    char *line;
    int len;
{
    int n;
    int modifier;
    KeySym *kslist;
    union op *uop;
    struct op_addmodifier *opam;

    if (len < 6 || !line || *line == '\0') {  /* Lock=a minimum */
	badmsg ("add modifier input line", NULL);
	return;
    }

    n = skip_chars (line, len);
    if (n < 1) {
	badmsg ("add modifier name %s", line);
	return;
    }

    modifier = parse_modifier (line, n);
    if (modifier < 0) {
	badmsgn ("add modifier name '%s', not allowed", line, n);
	return;
    }

    line += n, len -= n;
    n = skip_until_char (line, len, '=');
    if (n < 0) {
	badmsg ("add modifier = keysym", NULL);
	return;
    }

    n++;				/* skip = */
    n += skip_space (line+n, len-n);
    line += n, len -= n;

    if (get_keysym_list (line, len, &n, &kslist) < 0)
	return;
    if (n == 0) {
	badmsg ("add modifier keysym list (empty)", NULL);
	return;
    }

    uop = AllocStruct (union op);
    if (!uop) {
	badmsg ("attempt to allocate %ld byte addmodifier opcode",
		(long) sizeof (struct op_addmodifier));
	return;
    }
    opam = &uop->addmodifier;

    opam->type = doAddModifier;
    opam->modifier = modifier;
    opam->count = n;
    opam->keysyms = kslist;

    add_to_work_queue (uop);
}

#ifdef AUTO_ADD_REMOVE
/*
 * make_add - stick a single add onto the queue
 */
static void make_add (modifier, keysym)
    int modifier;
    KeySym keysym;
{
    union op *uop;
    struct op_addmodifier *opam;

    uop = AllocStruct (union op);
    if (!uop) {
	badmsg ("attempt to allocate %ld byte addmodifier opcode",
		(long) sizeof (struct op_addmodifier));
	return;
    }
    opam = &uop->addmodifier;

    opam->type = doAddModifier;
    opam->modifier = modifier;
    opam->count = 1;
    opam->keysyms = (KeySym *) malloc (sizeof (KeySym));
    if (!opam->keysyms) {
	badmsg ("attempt to allocate %ld byte KeySym", (long) sizeof (KeySym));
	free ((char *) opam);
	return;
    }
    opam->keysyms[0] = keysym;

    add_to_work_queue (uop);
    return;
}
#endif /* AUTO_ADD_REMOVE */


/*
 * do_remove - parse off lines of the form
 *
 *                 remove MODIFIER = oldkeysym ...
 *                        ^
 * where the MODIFIER is one of Shift, Lock, Control, Mod[1-5] where case
 * is not important.  There should also be an alias Ctrl for control.
 */

static void do_remove (line, len)
    char *line;
    int len;
{
    int n;
    int nc;
    int i;
    int tot;
    int modifier;
    KeySym *kslist;
    KeyCode *kclist;
    union op *uop;
    struct op_removemodifier *oprm;

    if (len < 6 || !line || *line == '\0') {  /* Lock=a minimum */
	badmsg ("remove modifier input line", NULL);
	return;
    }

    n = skip_chars (line, len);
    if (n < 1) {
	badmsg ("remove modifier name %s", line);
	return;
    }

    modifier = parse_modifier (line, n);
    if (modifier < 0) {
	badmsgn ("remove modifier name '%s', not allowed", line, n);
	return;
    }

    line += n, len -= n;
    n = skip_until_char (line, len, '=');
    if (n < 0) {
	badmsg ("remove modifier = keysym", NULL);
	return;
    }

    n++;
    n += skip_space (line+n, len-n);
    line += n, len -= n;

    if (get_keysym_list (line, len, &n, &kslist) < 0)
	return;
    if (n == 0) {
	badmsg ("remove modifier keysym list (empty)", NULL);
	return;
    }

    /*
     * unlike the add command, we have to now evaluate the keysyms
     */

    kclist = (KeyCode *) malloc (n * sizeof (KeyCode));
    if (!kclist) {
	badmsg ("attempt to allocate %ld byte keycode list",
		(long) (n * sizeof (KeyCode)));
	free ((char *) kslist);
	return;
    }

    tot = n;
    nc = 0;
    for (i = 0; i < n; i++) {
        int num_kcs;
	KeyCode *kcs;
	kcs = KeysymToKeycodes (dpy, kslist[i], &num_kcs);
	if (num_kcs == 0) {
	    char *tmpname = XKeysymToString (kslist[i]);
	    badmsg ("keysym in remove modifier list '%s', no corresponding keycodes",
		    tmpname ? tmpname : "?");
	    continue;
	}
	if (verbose) {
	    int j;
	    char *tmpname = XKeysymToString (kslist[i]);
	    printf ("! Keysym %s (0x%lx) corresponds to keycode(s)", 
		    tmpname ? tmpname : "?", (long) kslist[i]);
	    for (j = 0; j < num_kcs; j++)
		printf(" 0x%x", kcs[j]);
	    printf("\n");
	}
	if (nc + num_kcs > tot) {
	    int j;
	    tot = nc + num_kcs;
	    kclist = (KeyCode *)realloc((char *)kclist, tot * sizeof(KeyCode));
	    if (!kclist) {
		badmsg ("attempt to allocate %ld byte keycode list",
			(long) (tot * sizeof (KeyCode)));
		free ((char *) kslist);
		return;
	    }
	}
	while (--num_kcs >= 0)
	    kclist[nc++] = *kcs++;		/* okay, add it to list */
    }

    free ((char *) kslist);		/* all done with it */

    uop = AllocStruct (union op);
    if (!uop) {
	badmsg ("attempt to allocate %ld byte removemodifier opcode",
		(long) sizeof (struct op_removemodifier));
	return;
    }
    oprm = &uop->removemodifier;

    oprm->type = doRemoveModifier;
    oprm->modifier = modifier;
    oprm->count = nc;
    oprm->keycodes = kclist;

    add_to_work_queue (uop);
}

#ifdef AUTO_ADD_REMOVE
/*
 * make_remove - stick a single remove onto the queue
 */
static void make_remove (modifier, keycode)
    int modifier;
    KeyCode keycode;
{
    union op *uop;
    struct op_removemodifier *oprm;

    uop = AllocStruct (union op);
    if (!uop) {
	badmsg ("attempt to allocate %ld byte removemodifier opcode",
		(long) sizeof (struct op_removemodifier));
	return;
    }
    oprm = &uop->removemodifier;

    oprm->type = doRemoveModifier;
    oprm->modifier = modifier;
    oprm->count = 1;
    oprm->keycodes = (KeyCode *) malloc (sizeof (KeyCode));
    if (!oprm->keycodes) {
	badmsg ("attempt to allocate %ld byte KeyCode",
		(long) sizeof (KeyCode));
	free ((char *) oprm);
	return;
    }
    oprm->keycodes[0] = keycode;

    add_to_work_queue (uop);
    return;
}
#endif /* AUTO_ADD_REMOVE */


/*
 * do_clear - parse off lines of the form
 *
 *                 clear MODIFIER
 *                       ^
 */

static void do_clear (line, len)
    char *line;
    int len;
{
    int n;
    int modifier;
    union op *uop;
    struct op_clearmodifier *opcm;

    if (len < 4 || !line || *line == '\0') {  /* Lock minimum */
	badmsg ("clear modifier input line", NULL);
	return;
    }

    n = skip_chars (line, len);

    modifier = parse_modifier (line, n);
    if (modifier < 0) {
	badmsgn ("clear modifier name '%s'", line, n);
	return;
    }
    n += skip_space (line+n, len-n);
    if (n != len) {
	badmsgn ("extra argument '%s' to clear modifier", line+n, len-n);
	/* okay to continue */
    }

    uop = AllocStruct (union op);
    if (!uop) {
	badmsg ("attempt to allocate %d byte clearmodifier opcode",
		(long) sizeof (struct op_clearmodifier));
	return;
    }
    opcm = &uop->clearmodifier;

    opcm->type = doClearModifier;
    opcm->modifier = modifier;

    add_to_work_queue (uop);
}

static int strncmp_nocase (a, b, n)
    char *a, *b;
    int n;
{
    int i;
    int a1, b1;

    for (i = 0; i < n; i++, a++, b++) {
	if (!*a) return -1;
	if (!*b) return 1;

	if (*a != *b) {
	    a1 = (isascii(*a) && isupper(*a)) ? tolower(*a) : *a;
	    b1 = (isascii(*b) && isupper(*b)) ? tolower(*b) : *b;
	    if (a1 != b1) return b1 - a1;
	}
    }
    return 0;
}


/*
 * do_pointer = get list of numbers of the form
 *
 *                 buttons = NUMBER ...
 *                         ^
 */

static void do_pointer (line, len)
    char *line;
    int len;
{
    int n;
    int i;
    unsigned int val;
    union op *uop;
    struct op_pointer *opp;
    unsigned char buttons[MAXBUTTONCODES];
    int nbuttons;
    char *strval;
    Bool ok;

    if (len < 2 || !line || *line == '\0') {  /* =1 minimum */
	badmsg ("buttons input line", NULL);
	return;
    }

    nbuttons = XGetPointerMapping (dpy, buttons, MAXBUTTONCODES);

    n = skip_space (line, len);
    line += n, len -= n;

    if (line[0] != '=') {
	badmsg ("buttons pointer code list, missing equal sign", NULL);
	return;
    }

    line++, len--;			/* skip = */
    n = skip_space (line, len);
    line += n, len -= n;

    i = 0;
    if (len < 7 || strncmp_nocase (line, "default", 7) != 0) {
	while (len > 0) {
	    n = skip_space (line, len);
	    line += n, len -= n;
	    if (line[0] == '\0') break;
	    n = skip_word (line, len);
	    if (n < 1) {
		badmsg ("skip of word in buttons line:  %s", line);
		return;
	    }
	    strval = copy_to_scratch(line, n);
	    ok = parse_number (strval, &val);
	    if (!ok || val >= MAXBUTTONCODES) {
		badmsg ("value %s given for buttons list", strval);
		return;
	    }
	    buttons[i++] = (unsigned char) val;
	    line += n, len -= n;
	}
    }
    
    if (i > 0 && i != nbuttons) {
	badheader ();
	fprintf (stderr, "number of buttons, must have %d instead of %d\n",
		 nbuttons, i);
	return;
    }

    uop = AllocStruct (union op);
    if (!uop) {
	badmsg ("attempt to allocate a %ld byte pointer opcode",
		(long) sizeof (struct op_pointer));
	return;
    }
    opp = &uop->pointer;

    opp->type = doPointer;
    opp->count = i;
    for (i = 0; i < opp->count; i++) {
	opp->button_codes[i] = buttons[i];
    }

    add_to_work_queue (uop);
}


/*
 * get_keysym_list - parses the rest of the line into a keysyms assumes
 * that the = sign has been parsed off but there may be leading whitespace
 *
 *                 keysym ...
 *                 ^
 *
 * this involves getting the word containing the keysym, checking its range,
 * and adding it to the list.
 */

static int get_keysym_list (line, len, np, kslistp)
    char *line;
    int len;
    int *np;
    KeySym **kslistp;
{
    int havesofar, maxcanhave;
    KeySym *keysymlist;

    *np = 0;
    *kslistp = NULL;

    if (len == 0) return (0);		/* empty list */

    havesofar = 0;
    maxcanhave = 4;			/* most lists are small */
    keysymlist = (KeySym *) malloc (maxcanhave * sizeof (KeySym));
    if (!keysymlist) {
	badmsg ("attempt to allocate %ld byte initial keysymlist",
		(long) (maxcanhave * sizeof (KeySym)));
	return (-1);
    }

    while (len > 0) {
	KeySym keysym;
	int n;
	char *tmpname;
	Bool ok;

	n = skip_space (line, len);
	line += n, len -= n;

	n = skip_chars (line, len);
	if (n < 0) {
	    badmsg ("keysym name list", NULL);
	    return (-1);
	}

	ok = parse_keysym (line, n, &tmpname, &keysym);
	line += n, len -= n;
	if (!ok) {
	    badmsg ("keysym name '%s' in keysym list", tmpname);
	    /* do NOT return here, look for others */
	    continue;
	}

	/*
	 * Do NOT test to see if the keysym translates to a keycode or you
	 * won't be able to assign new ones....
	 */

	/* grow the list bigger if necessary */
	if (havesofar >= maxcanhave) {
	    maxcanhave *= 2;
	    keysymlist = (KeySym *) realloc (keysymlist,
					     maxcanhave * sizeof (KeySym));
	    if (!keysymlist) {
		badmsg ("attempt to grow keysym list to %ld bytes",
			(long) (maxcanhave * sizeof (KeySym)));
		return (-1);
	    }
	}

	/* and add it to the list */
	keysymlist[havesofar++] = keysym;
    }

    *kslistp = keysymlist;
    *np = havesofar;
    return (0);
}


#ifdef later
/*
 * check_special_keys - run through list of keysyms and generate "add" or
 * "remove" commands for for any of the key syms that appear in the modifier
 * list.  this involves running down the modifier map which is an array of
 * 8 by map->max_keypermod keycodes.
 */

static void check_special_keys (keycode, n, kslist)
    KeyCode keycode;
    int n;
    KeySym *kslist;
{
    int i;				/* iterator variable */
    KeyCode *kcp;			/* keycode pointer */

    /*
     * walk the modifiermap array.  since its dimensions are not known at
     * compile time, we have to walk it by hand instead of indexing.  this
     * is why it is initialized outside the loop, but incremented inside the
     * second loop.
     */

    kcp = map->modifiermap;		/* start at beginning and iterate */
    for (i = 0; i < 8; i++) {		/* there are 8 modifier keys */
	int j;

	for (j = 0; j < map->max_keypermod; j++, kcp++) {
	    KeySym keysym;
	    int k;

	    if (!*kcp) continue;	/* only non-zero entries significant */

	    /*
	     * check to see if the target keycode is already a modifier; if so,
	     * then we have to remove it
	     */
	    if (keycode == *kcp) {
		make_remove (i, keycode);
	    }

	    /*
	     * now, check to see if any of the keysyms map to keycodes
	     * that are in the modifier list
	     */
	    for (k = 0; k < n; k++) {
		KeyCodes kc;

		kc = XKeysymToKeycode (dpy, kslist[k]);
		if (kc == *kcp) {	/* yup, found one */
		    /*
		     * have to generate a remove of the CURRENT keycode
		     * and then an add of the new KEYCODE
		     */
		    make_remove (i, kc);  /* modifier, keycode */
		    make_add (i, kslist[k]);  /* modifier, keysym */
		}
	    }
	}
    }
    return;
}
#endif

/*
 * print_work_queue - disassemble the work queue and print it on stdout
 */

void print_work_queue ()
{
    union op *op;

    printf ("! dump of work queue\n");
    for (op = work_queue.head; op; op = op->generic.next) {
	print_opcode (op);
    }
    return;
}

void print_opcode (op)
    union op *op;
{
    int i;

    printf ("        ");
    switch (op->generic.type) {
      case doKeycode:
	printf ("keycode 0x%lx =", (long) op->keycode.target_keycode);
	for (i = 0; i < op->keycode.count; i++) {
	    char *name = XKeysymToString (op->keycode.keysyms[i]);

	    printf (" %s", name ? name : "BADKEYSYM");
	}
	printf ("\n");
	break;
      case doAddModifier:
	printf ("add %s =", modifier_table[op->addmodifier.modifier].name);
	for (i = 0; i < op->addmodifier.count; i++) {
	    char *name = XKeysymToString (op->addmodifier.keysyms[i]);
	    printf (" %s", name ? name : "BADKEYSYM");
	}
	printf ("\n");
	break;
      case doRemoveModifier:
	printf ("remove %s = ",
		modifier_table[op->removemodifier.modifier].name);
	for (i = 0; i < op->removemodifier.count; i++) {
	    printf (" 0x%lx", (long) op->removemodifier.keycodes[i]);
	}
	printf ("\n");
	break;
      case doClearModifier:
	printf ("clear %s\n", modifier_table[op->clearmodifier.modifier].name);
	break;
      default:
	printf ("! unknown opcode %d\n", op->generic.type);
	break;
    }				/* end switch */
    return;
}

/*
 * execute_work_queue - do the real meat and potatoes now that we know what
 * we need to do and that all of the input is correct.
 */

static int exec_keycode(), exec_add(), exec_remove(), exec_clear();
static int exec_pointer();

int execute_work_queue ()
{
    union op *op;
    int errors;
    Bool update_map = False;
    enum opcode lastop;

    if (verbose) {
	printf ("!\n");
	printf ("! executing work queue\n");
	printf ("!\n");
    }

    errors = 0;
    lastop = doClearModifier;				/* fake it for op test */

    for (op = work_queue.head; op; op = op->generic.next) {
	if (verbose) print_opcode (op);

	/* check to see if we have to update the keyboard mapping */
	if (lastop == doKeycode && op->generic.type != doKeycode) {
	    XSync (dpy, 0);
	    while (XEventsQueued (dpy, QueuedAlready) > 0) {
		XEvent event;
		XNextEvent (dpy, &event);
		if (event.type == MappingNotify) {
		    /* read all MappingNotify events */
		    while (XCheckTypedEvent (dpy, MappingNotify, &event)) ;
		    XRefreshKeyboardMapping (&event.xmapping);
		} else {
		    fprintf (stderr, "%s:  unknown event %ld\n", 
		    	     ProgramName, (long) event.type);
		}
	    }
	}

	switch (op->generic.type) {
	  case doKeycode:
	    if (exec_keycode (op) < 0) errors++;
	    break;
	  case doAddModifier:
	    if (exec_add (op) < 0) errors++;
	    else update_map = True;
	    break;
	  case doRemoveModifier:
	    if (exec_remove (op) < 0) errors++;
	    else update_map = True;
	    break;
	  case doClearModifier:
	    if (exec_clear (op) < 0) errors++;
	    else update_map = True;
	    break;
	  case doPointer:
	    if (exec_pointer (op) < 0) errors++;
	    break;
	  default:
	    fprintf (stderr, "%s:  unknown opcode %d\n", 
		     ProgramName, op->generic.type);
	    break;
	}
	lastop = op->generic.type;

    }

    if (update_map) {
	if (UpdateModifierMapping (map) < 0) errors++;
    }

    return (errors > 0 ? -1 : 0);
}

static int exec_keycode (opk)
    struct op_keycode *opk;
{
    if (opk->count == 0) {
	KeySym dummy = NoSymbol;
	XChangeKeyboardMapping (dpy, opk->target_keycode, 1,
				&dummy, 1);
    } else {
	XChangeKeyboardMapping (dpy, opk->target_keycode, opk->count, 
				opk->keysyms, 1);
    }
    return (0);
}

static int exec_add (opam)
    struct op_addmodifier *opam;
{
    int i;
    int status;

    status = 0;
    for (i = 0; i < opam->count; i++) {
	int num_kcs;
	KeyCode *kcs;

	kcs = KeysymToKeycodes (dpy, opam->keysyms[i], &num_kcs);
	if (num_kcs == 0)
	    status = -1;
	while (--num_kcs >= 0) {
	    if (AddModifier (&map, *kcs++, opam->modifier) < 0)
		status = -1;
	}
    }
    return (status);
}

static int exec_remove (oprm)
    struct op_removemodifier *oprm;
{
    int i;
    int status;

    status = 0;
    for (i = 0; i < oprm->count; i++) {
	if (RemoveModifier (&map, oprm->keycodes[i], oprm->modifier) < 0)
	  status = -1;
    }
    return (status);
}

static int exec_clear (opcm)
    struct op_clearmodifier *opcm;
{
    return (ClearModifier (&map, opcm->modifier));
}


static int exec_pointer (opp)
    struct op_pointer *opp;
{
    return (SetPointerMap (opp->button_codes, opp->count));
}

void print_modifier_map ()
{
    PrintModifierMapping (map, stdout);
    return;
}

void print_key_table (exprs)
    Bool exprs;
{
    PrintKeyTable (exprs, stdout);
    return;
}

void print_pointer_map ()
{
    PrintPointerMap (stdout);
    return;
}


