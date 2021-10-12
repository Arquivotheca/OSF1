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
/* $XConsortium: spfile.c,v 1.7 92/09/17 11:57:01 gildea Exp $ */
/*
 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation and the
 * Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Network Computing Devices, Digital or
 * MIT not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.
 *
 * NETWORK COMPUTING DEVICES, DIGITAL AND MIT DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL NETWORK COMPUTING DEVICES, DIGITAL OR MIT BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Dave Lemke, Network Computing Devices Inc
 */

#include	<stdio.h>
#include	"fontfilest.h"

#include	"spint.h"

SpeedoFontPtr sp_fp_cur = (SpeedoFontPtr) 0;

#ifdef EXTRAFONTS
#include	"ncdkeys.h"
#endif

#include	"keys.h"

#ifdef EXTRAFONTS
static ufix8 skey[] =
{
    SKEY0,
    SKEY1,
    SKEY2,
    SKEY3,
    SKEY4,
    SKEY5,
    SKEY6,
    SKEY7,
    SKEY8
};				/* Sample Font decryption key */

static ufix8 rkey[] =
{
    RKEY0,
    RKEY1,
    RKEY2,
    RKEY3,
    RKEY4,
    RKEY5,
    RKEY6,
    RKEY7,
    RKEY8
};				/* Retail Font decryption key */

#endif				/* EXTRAFONTS */

static ufix8 mkey[] =
{
    KEY0,
    KEY1,
    KEY2,
    KEY3,
    KEY4,
    KEY5,
    KEY6,
    KEY7,
    KEY8
};				/* Font decryption key */


static      fix15
read_2b(ptr)
    ufix8      *ptr;
{
    fix15       tmp;

    tmp = *ptr++;
    tmp = (tmp << 8) + *ptr;
    return tmp;
}

static      fix31
read_4b(ptr)
    ufix8      *ptr;
{
    fix31       tmp;

    tmp = *ptr++;
    tmp = (tmp << 8) + *ptr++;
    tmp = (tmp << 8) + *ptr++;
    tmp = (tmp << 8) + *ptr;
    return tmp;
}

/*
 * loads the specified char's data
 */
buff_t     *
sp_load_char_data(file_offset, num, cb_offset)
    fix31       file_offset;
    fix15       num;
    fix15       cb_offset;
{
    SpeedoMasterFontPtr master = sp_fp_cur->master;

    if (fseek(master->fp, (long) file_offset, (int) 0)) {
	SpeedoErr("can't seek to char\n");
    }
    if ((num + cb_offset) > master->mincharsize) {
	SpeedoErr("char buf overflow\n");
    }
    if (fread((master->c_buffer + cb_offset), sizeof(ufix8), num,
	      master->fp) != num) {
	SpeedoErr("can't get char data\n");
    }
    master->char_data.org = (ufix8 *) master->c_buffer + cb_offset;
    master->char_data.no_bytes = num;

    return &master->char_data;
}

int
sp_open_master(filename, master)
    char       *filename;
    SpeedoMasterFontPtr *master;
{
    SpeedoMasterFontPtr spmf;
    ufix8       tmp[16];
    ufix16      cust_no;
    FILE       *fp;
    ufix32      minbufsize;
    ufix16      mincharsize;
    ufix8      *f_buffer;
    ufix8      *c_buffer;
    int         ret;
    ufix8      *key;

    spmf = (SpeedoMasterFontPtr) xalloc(sizeof(SpeedoMasterFontRec));
    if (!spmf)
	return AllocError;
    bzero(spmf, sizeof(SpeedoMasterFontRec));
    spmf->entry = NULL;
    spmf->f_buffer = NULL;
    spmf->c_buffer = NULL;

    /* open font */
    spmf->fname = (char *) xalloc(strlen(filename) + 1);
    fp = fopen(filename, "r");
    if (!fp) {
	ret = BadFontName;
	goto cleanup;
    }
    strcpy(spmf->fname, filename);
    spmf->fp = fp;
    spmf->state |= MasterFileOpen;

    if (fread(tmp, sizeof(ufix8), 16, fp) != 16) {
	ret = BadFontName;
	goto cleanup;
    }
    minbufsize = (ufix32) read_4b(tmp + FH_FBFSZ);
    f_buffer = (ufix8 *) xalloc(minbufsize);
    if (!f_buffer) {
	ret = AllocError;
	goto cleanup;
    }
    spmf->f_buffer = f_buffer;

    fseek(fp, (ufix32) 0, 0);

    /* read in the font */
    if (fread(f_buffer, sizeof(ufix8), (ufix16) minbufsize, fp) != minbufsize) {
	ret = BadFontName;
	goto cleanup;
    }
    spmf->copyright = (char *) (f_buffer + FH_CPYRT);
    spmf->mincharsize = mincharsize = read_2b(f_buffer + FH_CBFSZ);

    c_buffer = (ufix8 *) xalloc(mincharsize);
    if (!c_buffer) {
	ret = AllocError;
	goto cleanup;
    }
    spmf->c_buffer = c_buffer;

    spmf->font.org = spmf->f_buffer;
    spmf->font.no_bytes = minbufsize;

    cust_no = sp_get_cust_no(spmf->font);

    /* XXX add custom encryption stuff here */

#ifdef EXTRAFONTS
    if (cust_no == SCUS0) {
	key = skey;
    } else if (cust_no == RCUS0) {
	key = rkey;
    } else
#endif

    if (cust_no == CUS0) {
	key = mkey;
    } else {
	SpeedoErr("Non - standard encryption for \"%s\"\n", filename);
	ret = BadFontName;
	goto cleanup;
    }
    spmf->key = key;
    sp_set_key(key);

    spmf->first_char_id = read_2b(f_buffer + FH_FCHRF);
    spmf->num_chars = read_2b(f_buffer + FH_NCHRL);


    spmf->enc = sp_bics_map;
    spmf->enc_size = sp_bics_map_size;

#ifdef EXTRAFONTS
    {				/* choose the proper encoding */
	char       *f;

	f = rindex(filename, '/');
	if (f) {
	    f++;
	    if (strncmp(f, "bx113", 5) == 0) {
		spmf->enc = adobe_map;
		spmf->enc_size = adobe_map_size;
	    }
	}
    }
#endif

    /* XXX slam back to ISO Latin1 */
    spmf->first_char_id = spmf->enc[0];
    /* size of extents array */
    spmf->max_id = spmf->enc[(spmf->enc_size - 1) * 2];
    spmf->num_chars = spmf->enc_size;

    *master = spmf;

    return Successful;

cleanup:
    *master = (SpeedoMasterFontPtr) 0;
    sp_close_master_font(spmf);
    return ret;
}

void
sp_close_master_font(spmf)
    SpeedoMasterFontPtr spmf;
{
    if (!spmf)
	return;
    if (spmf->state & MasterFileOpen)
	fclose(spmf->fp);
    if (spmf->entry)
	spmf->entry->u.scalable.extra->private = NULL;
    xfree(spmf->fname);
    xfree(spmf->f_buffer);
    xfree(spmf->c_buffer);
    xfree(spmf);
}

void
sp_close_master_file(spmf)
    SpeedoMasterFontPtr spmf;
{
    (void) fclose(spmf->fp);
    spmf->state &= ~MasterFileOpen;
}


/*
 * reset the encryption key, and make sure the file is opened
 */
void
sp_reset_master(spmf)
    SpeedoMasterFontPtr spmf;
{
    sp_set_key(spmf->key);
    if (!(spmf->state & MasterFileOpen)) {
	spmf->fp = fopen(spmf->fname, "r");
	/* XXX -- what to do if we can't open the file? */
	spmf->state |= MasterFileOpen;
    }
}
