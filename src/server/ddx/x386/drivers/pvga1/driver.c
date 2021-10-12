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
 * $XConsortium: driver.c,v 1.2 91/08/20 15:13:33 gildea Exp $
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 */

#include "X.h"
#include "input.h"
#include "screenint.h"

#include "compiler.h"

#include "x386.h"
#include "x386Priv.h"
#include "x386OSD.h"
#include "vga.h"


typedef struct {
  vgaHWRec      std;          /* std IBM VGA register */
  unsigned char PR0A;           /* PVGA1A, WD90Cxx */
  unsigned char PR0B;
  unsigned char MemorySize;
  unsigned char VideoSelect;
  unsigned char CRTCCtrl;
  unsigned char VideoCtrl;
  unsigned char InterlaceStart; /* WD90Cxx */
  unsigned char InterlaceEnd;
  unsigned char MiscCtrl2;
  unsigned char InterfaceCtrl;  /* WD90C1x */
} vgaPVGA1Rec, *vgaPVGA1Ptr;

static Bool  PVGA1Probe();
static void  PVGA1ClockSelect();
static void  PVGA1EnterLeave();
static void  PVGA1Init();
static void *PVGA1Save();
static void  PVGA1Restore();
static void  PVGA1Adjust();
extern void  PVGA1SetRead();
extern void  PVGA1SetWrite();
extern void  PVGA1SetReadWrite();


vgaVideoChipRec PVGA1 = {
  PVGA1Probe,
  PVGA1EnterLeave,
  PVGA1Init,
  PVGA1Save,
  PVGA1Restore,
  PVGA1Adjust,
  PVGA1SetRead,
  PVGA1SetWrite,
  PVGA1SetReadWrite,
  0x10000,
  0x08000,
  15,
  0x7FFF,
  0x08000, 0x10000,
  0x00000, 0x08000,
};

#define new ((vgaPVGA1Ptr)vgaNewVideoState)



/*
 * PVGA1ClockSelect --
 *      select one of the possible clocks ...
 */

static void
PVGA1ClockSelect(no)
     int no;
{
  unsigned char temp;

  temp = inb(0x3CC);
  outb(0x3C2, ( temp & 0xf3) | ((no << 2) & 0x0C));
  outw(0x3CE, 0x0C | ((no & 0x04) << 7));
}



/*
 * PVGA1Probe --
 *      check up whether a PVGA1 based board is installed
 */

static Bool
PVGA1Probe()
{
  if (vga256InfoRec.chipset)
    {
      if (strcmp(vga256InfoRec.chipset, "pvga1"))
	return (FALSE);
    }
  else
    {
      char ident[4];
      int  fd;

      if ((fd = open("/dev/mem", O_RDONLY)) == -1 ||
	  lseek(fd, 0xC007D, SEEK_SET) == -1 ||
	  read(fd, ident, 4) != 4)
	{
	  close(fd);
	  ErrorF("Failed to read VGA Bios.  (X386 must be installed as suid root.)\n");
	  return(FALSE);
	}
      
      close(fd);
      if (strncmp(ident, "VGA=",4)) return(FALSE);
    }

  PVGA1EnterLeave(ENTER);

  /*
   * Detect how much memory is installed
   */
  if (!vga256InfoRec.videoRam)
    {
      unsigned char config;

      outb(0x3CE, 0x0B); config = inb(0x3CF);
      
      switch(config & 0xC0) {
      case 0x00:
      case 0x40:
	vga256InfoRec.videoRam = 256;
	break;
      case 0x80:
	vga256InfoRec.videoRam = 512;
	break;
      case 0xC0:
	vga256InfoRec.videoRam = 1024;
	break;
      }
    }

  if (!vga256InfoRec.clocks) vgaGetClocks(8, PVGA1ClockSelect);

  vga256InfoRec.chipset = "pvga1";
  return(TRUE);
}



/*
 * PVGA1EnterLeave --
 *      enable/disable io-mapping
 */

static void 
PVGA1EnterLeave(enter)
     Bool enter;
{
  unsigned char temp;

  if (enter)
    {
      ioctl(x386Info.consoleFd, KDENABIO, 0);

      vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
      outw(0x3CE, 0x050F);           /* unlock PVGA1 Register Bank 1 */
      outw(vgaIOBase + 4, 0x8529);   /* unlock PVGA1 Register Bank 2 */
      outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
      outb(vgaIOBase + 5, temp & 0x7F);
    }
  else
    {
      ioctl(x386Info.consoleFd, KDDISABIO, 0);
    }
}



/*
 * PVGA1Restore --
 *      restore a video mode
 */

static void
PVGA1Restore(restore)
     vgaPVGA1Ptr restore;
{
  unsigned char temp;

  /*
   * First unlock all these special registers ...
   * NOTE: Locking will not be fully renabled !!!
   */
  outb(0x3CE, 0x0D); temp = inb(0x3CF); outb(0x3CF, temp & 0x1C);
  outb(0x3CE, 0x0E); temp = inb(0x3CF); outb(0x3CF, temp & 0xFB);

  outb(vgaIOBase + 4, 0x2A); temp = inb(vgaIOBase + 5);
  outb(vgaIOBase + 5, temp & 0xF8);

  outw(0x3CE, 0x0009);   /* segment select A */
  outw(0x3CE, 0x000A);   /* segment select B */

  vgaHWRestore(restore);

  outw(0x3CE, (restore->PR0A << 8) | 0x09);
  outw(0x3CE, (restore->PR0B << 8) | 0x0A);

  outb(0x3CE, 0x0B); temp = inb(0x3CF);          /* banking mode ... */
  outb(0x3CF, (temp & 0xF7) | (restore->MemorySize & 0x08));
       
  outw(0x3CE, (restore->VideoSelect << 8) | 0x0C);
  outw(0x3CE, (restore->CRTCCtrl << 8)    | 0x0D);
  outw(0x3CE, (restore->VideoCtrl << 8)   | 0x0E);
  
  /*
   * Now the WD90Cxx specials (Register Bank 2)
   */
  outw(vgaIOBase + 4, (restore->InterlaceStart << 8) | 0x2C);
  outw(vgaIOBase + 4, (restore->InterlaceEnd << 8)   | 0x2D);
  outw(vgaIOBase + 4, (restore->MiscCtrl2 << 8)      | 0x2F);

  /*
   * For the WD90C10 & WD90C11 we have to select segment mapping.
   * NOTE: Only bit7 is save/restored !!!!
   */
  outb(0x3C4, 0x11); temp = inb(0x3C5);
  outb(0x3C5, (temp & 0x7F) | (restore->InterfaceCtrl & 0x80));

  outw(0x3C4, 0x0300); /* now reenable the timing sequencer */
}



/*
 * PVGA1Save --
 *      save the current video mode
 */

static void *
PVGA1Save(save)
     vgaPVGA1Ptr save;
{
  unsigned char PR0A, PR0B, temp;

  vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
  outb(0x3CE, 0x0D); temp = inb(0x3CF); outb(0x3CF, temp & 0x1C);
  outb(0x3CE, 0x0E); temp = inb(0x3CF); outb(0x3CF, temp & 0xFD);

  outb(vgaIOBase + 4, 0x2A); temp = inb(vgaIOBase + 5);
  outb(vgaIOBase + 5, temp & 0xF8);

  outb(0X3CE, 0x09); PR0A = inb(0x3CF); outb(0x3CF, 0x00);
  outb(0X3CE, 0x0A); PR0B = inb(0x3CF); outb(0x3CF, 0x00);

  save = (vgaPVGA1Ptr)vgaHWSave(save, sizeof(vgaPVGA1Rec));

  save->PR0A = PR0A;
  save->PR0B = PR0B;
  outb(0x3CE, 0x0B); save->MemorySize  = inb(0x3CF);
  outb(0x3CE, 0x0C); save->VideoSelect = inb(0x3CF);
  outb(0x3CE, 0x0D); save->CRTCCtrl    = inb(0x3CF);
  outb(0x3CE, 0x0E); save->VideoCtrl   = inb(0x3CF);

  /* WD90Cxx */
  outb(vgaIOBase + 4, 0x2C); save->InterlaceStart = inb(vgaIOBase+5);
  outb(vgaIOBase + 4, 0x2D); save->InterlaceEnd   = inb(vgaIOBase+5);
  outb(vgaIOBase + 4, 0x2F); save->MiscCtrl2      = inb(vgaIOBase+5);

  /* WD90C1x */
  outb(0x3C4, 0x11); save->InterfaceCtrl = inb(0x3C5);

  return ((void *) save);
}



/*
 * PVGA1Init --
 *      Handle the initialization, etc. of a screen.
 */

static void
PVGA1Init(mode)
     DisplayModePtr mode;
{
  vgaHWInit(mode,sizeof(vgaPVGA1Rec));

  new->std.CRTC[19] = vga256InfoRec.virtualX >> 3; /* we are in byte-mode */
  new->std.CRTC[20] = 0x40;
  new->std.CRTC[23] = 0xE3; /* thats what the man says */

  new->PR0A = 0x00;
  new->PR0B = 0x00;
  new->MemorySize = 0x08;
  new->VideoSelect = (new->std.NoClock & 0x4) ? 0x02 : 0x00;
  new->CRTCCtrl = 0x00;
  new->VideoCtrl = 0x01;

  /* WD90Cxx */
  if (mode->Flags & V_INTERLACE) 
    {
      new->InterlaceStart = (mode->HSyncStart >> 3) - (mode->HTotal >> 4);
      new->InterlaceEnd = 0x20 | 
	((mode->HSyncEnd >> 3) - (mode->HTotal >> 4)) & 0x1F;
    }
  else
    {
      new->InterlaceStart = 0x00;
      new->InterlaceStart = 0x00;
    }
  new->MiscCtrl2 = 0x00;

  /* WD90C1x */
  new->InterfaceCtrl = 0x00;
}
	


/*
 * PVGA1Adjust --
 *      adjust the current video frame to display the mousecursor
 */

static void 
PVGA1Adjust(x, y)
     int x, y;
{
  int           Base = (y * vga256InfoRec.virtualX + x) >> 2;
  unsigned char temp;

  outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
  outw(vgaIOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);
  outb(0x3CE, 0x0D); temp=inb(0x3CF); 
  outb(0x3CF, ((Base & 0x030000) >> 13) | (temp & 0xE7));
}



