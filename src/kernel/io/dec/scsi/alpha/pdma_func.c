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

/************************************************************************
 *
 * pdma_func.c	10/25/89
 *
 * Pseudo DMA routines for all systems.
 *
 * Modification History:
 *
 * 06/05/91	Tom Tierney
 * Merge of ULTRIX 4.2 SCSI subsystem and OSF/1 reference port work.
 * This module is a result of work done by Fred Canter and Bill Burns
 * to merge pdma_func.c version 4.1 from ULTRIX and the OSF/1
 * reference port of the SCSI subsystem.
 *
 * Removed OSF conditional ifdef.
 *
 * 09/07/90     Maria Vella
 * Submitted this file to release.
 *
 * 10/11/89	John A. Gallant
 * Created this file to support the PDMA work in the scsi drivers.
 *
 * 10/25/89	John A. Gallant
 * Added the pdma_flush() entry in the function copy area.
 *
 ************************************************************************/

/* This file contains the Pseudo DMA routines needed for all the PDMA
systems. */

#include <data/scsi_data.c>
#include <io/dec/scsi/alpha/scsi_debug.h>

/* External functions and variables. */

extern PDMA_ENTRY *pdma_entry[];		/* the entry array */

/* Global DMA independent Routines needed: */

/************************************************************************

int pdma_attach( sc, cputype )

Inputs:	sz_softc pointer
	cputype value

Function:   Using the cputype argument, scan the PDMA entry array to find
	    a match for the cputype.  When a match is found, the pdma_func()
	    pointers in the softc struct will be filled with the pointers 
	    from the PDMA entry array.  Once the softc struct is set the
	    pdma_init() routine will be called.

Return:	The return value from the pdma_init() will be returned to the caller.
	In the event that no match is made with in the PDMA entry array,
	failure will be returned.

*************************************************************************/

int pdma_attach( sc, cputype )
    struct sz_softc *sc;
    int cputype;
{
    PDMA_ENTRY *pe;		/* for accessing the entries in the array */
    int i;			/* local loop control variable */

    PRINTD( 0xFF, 0x401, ("pdma_attach entry cputype %d\n", cputype ));

    /* Within a loop scan the PDMA_ENTRY array looking for a match to the 
    cputype in the type field.  */

    for( i = 0; ; i++ )
    {
	pe = pdma_entry[ i ];		/* assign the pointer */

      /* If the entry is not defined, we have reached the end of the array.
	Return FAILURE to the caller and hope he knows what to do with it. */

	if( pe == NULL )
	{
	    PRINTD( 0xFF, 0x411, ("pdma_attach no entry match found\n"));
	    return( PDMA_ALLOC_FAIL );	/* signal no match */
	}

        /* If the cputypes match, setup the pointers and call init routine. */

	if( pe->pdma_cputype == cputype )
	{
	    PRINTD( 0xFF, 0x402, ("pdma_attach match found pe: %lx\n", pe ));
	    sc->dma_init   = pe->pdma_init;
	    sc->dma_setup  = pe->pdma_setup;
	    sc->dma_start  = pe->pdma_start;
	    sc->dma_cont   = pe->pdma_cont;
	    sc->dma_end    = pe->pdma_end;
	    sc->dma_rbcopy = pe->pdma_rbcopy;
	    sc->dma_wbcopy = pe->pdma_wbcopy;
	    sc->dma_bzero  = pe->pdma_bzero;
	    sc->dma_flush  = pe->pdma_flush;
	    sc->dma_pflags = pe->pdma_pflags;	/* this may not be used */

	    /* Call the init routine, to setup the dma channels etal. */

	    return( (*pe->pdma_init)( sc ) );	/* return what happened */
	}
    }
}

/************************************************************************/
