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
 * |-----------------------------------------------------------|
 * | Copyright (c) 1991 MIPS Computer Systems, Inc.            |
 * | All Rights Reserved                                       |
 * |-----------------------------------------------------------|
 * |          Restricted Rights Legend                         |
 * | Use, duplication, or disclosure by the Government is      |
 * | subject to restrictions as set forth in                   |
 * | subparagraph (c)(1)(ii) of the Rights in Technical        |
 * | Data and Computer Software Clause of DFARS 52.227-7013.   |
 * |         MIPS Computer Systems, Inc.                       |
 * |         950 DeGuigne Drive                                |
 * |         Sunnyvale, CA 94086                               |
 * |-----------------------------------------------------------|
 */
/*  $Header: /usr/sde/alpha/rcs/alpha/src/./usr/include/cmplrs/gptable.h,v 4.2.4.2 1992/04/30 15:58:06 Ken_Lesniak Exp $ */

/*
  This package helps the assembler initialize and maintain in memory a
  table which tells how big the .sdata (or .sbss) segment would be for
  each interesting value of the -G command-line argument. The table format
  is described by gp_table in <scnhdr.h>.  None of these routines
  prints any error message, but the ones which return pointers will
  return nil in case of error.

  The assembler puts into the .o file one table for .sdata (if there is
  no .sdata header, then this table goes into .data) and one for .sbss
  (if there is not .sbss header, then this table goes into .bss). The
  first element of each table is a header giving the current value of
  -G. Following that, each table has at least one entry, corresponding
  to the size the segment would have with -G set to 0.  The remaining
  entries are sorted in ascending order on the -G value, with
  inapplicable -G values omitted.

*/

#ifdef __LANGUAGE_C__

/* Given current segment address "a", compute next "b"-bytes-aligned boundary */
#define ALIGN(b, a) (((a) + (b) - 1) & ~ ((b) - 1))

/* Initialize a table, specifying the -G value actually in effect for
  this assembly.  The pointer returned is a token which you must pass
  to the other routines to indicate which table you're dealing with,
  and which you may use to access the fields of the table directly. */
union gp_table *
init_gp_table(unsigned dash_g_value);

/* Augment the table of gp-relative datum sizes to reflect an item
  which occupies "item_size" bytes. Ordinarily, "bloated_size" should
  match "item_size", but if (for example) the overlaying of a small
  data and large common symbol forces an item which is really too large
  for the gp area to go into the gp area anyway, then you should set
  "item_size" to the small size and "bloated_size" to the large size.
  "as_if_size" is the size of the item for alignment purposes. For
  example, an array of 128 doubles would have "item_size==128*8" and
  "as_if_size=8". As another example, the padding you put at the end of
  a routine has "item_size==0" but "as_if_size==16". "is_independent"
  indicates that this item's allocation is independent of the -G value.
  Returns (possibly changed) pointer to the table. */
union gp_table *
add_gp_table(
  union gp_table *the_table,
  unsigned item_size,
  unsigned bloated_size,
  unsigned as_if_size,
  int is_independent
  );

/* Return current number of entries including header */
unsigned
ask_gp_table(union gp_table *the_table);

/* Write the table to the specified file, returning 0 on error */
int
write_gp_table(FILE *fd, union gp_table *the_table);

/* Free the storage occupied by the table (you must not use "free"
  on the pointer yourself) */
void
free_gp_table(union gp_table *the_table);

#endif /* __LANGUAGE_C__ */

#ifdef __LANGUAGE_PASCAL__
#define ALIGN(b, a) (bitand((a) + (b) - 1, bitnot((b) - 1)))

function init_gp_table(dash_g_value: cardinal): gpt_ptr;
  external;
function add_gp_table(the_table: gpt_ptr;
  item_size, bloated_size, asif_size: cardinal;
  is_independent: integer): gpt_ptr;
  external;
function ask_gp_table(the_table: gpt_ptr): cardinal;
  external;
{ fd is weird, but that's the way as1 wants it }
function write_gp_table(fd: integer; the_table: gpt_ptr): integer;
  external;
procedure free_gp_table(the_table: gpt_ptr);
  external;
#endif /* __LANGUAGE_PASCAL__ */
