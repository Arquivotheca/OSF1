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
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile: WmIconBox.c,v $ $Revision: 1.1.4.3 $ $Date: 1993/07/16 21:50:26 $"
#endif
#endif
/*
 * (c) Copyright 1987, 1988, 1989, 1990 HEWLETT-PACKARD COMPANY */

/*
 * Included Files:
 */

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/Xutil.h>
#include <X11/Vendor.h>

#include <X11/keysymdef.h>
#include <X11/keysym.h>


#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/DrawnB.h>
#include <Xm/ScrolledW.h>
#include <Xm/BulletinB.h>
#include <Xm/ToggleB.h>

#include "WmIBitmap.h"
#include "WmGlobal.h" 
#include "WmResNames.h"

#include <stdio.h>
#ifdef VMS
#include "WmVms.h"
#endif
#ifdef DEC_MOTIF_EXTENSION
#include "mwm_internal.h"
#include "mwm_util.h"
#endif /* DEC_MOTIF_EXTENSION */

#ifndef MAX
#define MAX(a,b) ((b)>(a)?(b):(a))
#endif

/*
 * include extern functions
 */

#include "WmIconBox.h"
#include "WmCDInfo.h"
#include "WmError.h"
#include "WmEvent.h"
#include "WmFunction.h"
#include "WmIDecor.h"
#include "WmIPlace.h"
#include "WmImage.h"
#include "WmManage.h"
#include "WmMenu.h"
#include "WmResParse.h"
#include "WmResource.h"
#include "WmWinInfo.h"




/*
 * Global Variables:
 */


Pixel select_color;
Pixmap greyedPixmap;

int frameShadowThickness;
int firstTime = 1;
#ifdef VMS
/* 
 * VMS can't differentiate between insertPosition (with a leading lower case 
 * letter i) and InsertPosition with an upper case "I". So rename the lower
 * case name to insertPosition2.
 */
Cardinal insertPosition2 = 0;	
#else
Cardinal insertPosition = 0;
#endif
#define    DEFAULT_ICON_BOX_TITLE "Icons"


/*************************************<->*************************************
 *
 *  InitIconBox (pSD)
 *
 *
 *  Description:
 *  -----------
 *  This function controls creation of icon boxes
 *
 *
 *************************************<->***********************************/
#ifdef _NO_PROTO
void InitIconBox (pSD)
    WmScreenData *pSD;
#else /* _NO_PROTO */
void InitIconBox (WmScreenData *pSD)
#endif /* _NO_PROTO */

{
    /*
     * Start the process of making the icon boxes
     */



    ManageWindow (pSD, NULL, MANAGEW_ICON_BOX);

    if (pSD->fadeNormalIcon)
    {
	MakeFadeIconGC (pSD);
    }


} /* END OF FUNCTION InitIconBox */



/*************************************<->*************************************
 *
 *  MakeIconBox (pWS, pCD);
 *
 *
 *  Description:
 *  -----------
 *  
 *
 *
 *  Inputs:
 *  ------
 *  pWS     = pointer to workspace data
 *  pCD     =  a pointer to ClientData
 *
 * 
 *  Outputs:
 *  -------
 *  
 *  Return =  (Boolean) True iff successful.
 *
 *
 *  Comments:
 *  --------
 *  If fails, frees the ClientData structure pointed to by pCD.
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
Boolean MakeIconBox (pWS, pCD)
    WmWorkspaceData *pWS;
    ClientData *pCD;

#else /* _NO_PROTO */
Boolean MakeIconBox (WmWorkspaceData *pWS, ClientData *pCD)
#endif /* _NO_PROTO */
{
    IconBoxData *pIBD;


    /* 
     * Make an icon box and return the pCD
     */

    if (pCD)
    {
        if (!(pIBD = (IconBoxData *)XtMalloc (sizeof (IconBoxData))))
	{
	    /*
	     * We need a pointer to icon box data to add to the
	     * list of icon boxes linked to pWS->pIconBox. If
	     * we can't allocate space we need to free the space
	     * allocated for the ClientData structure 
	     */

	    Warning ("Insufficient memory to create icon box data");
            XtFree ((char *)pCD);  
	    return (FALSE);  
	}

	InitializeIconBoxData (pWS, pIBD);
	InitializeClientData (pCD, pIBD);

        if (!(pIBD->IPD.placeList = 
	    (IconInfo *)XtMalloc (pIBD->IPD.totalPlaces * sizeof (IconInfo))))
	{
	    Warning ("Insufficient memory to create icon box data");
	    XtFree ((char *)pIBD);
            XtFree ((char *)pCD);  
	    return (FALSE);  
	}
	memset (pIBD->IPD.placeList, NULL, 
	    pIBD->IPD.totalPlaces * sizeof (IconInfo));

        /*
         * Make  the top level shell for this icon box
         */
        MakeShell (pWS, pIBD);


        /*
         * Make  the scrolled window for this icon box
         */

        MakeScrolledWindow (pWS, pIBD);

        /*
         * Make  the row column manager for this icon box
         */
        MakeBulletinBoard (pWS, pIBD);

        /*
         * Realize the widget tree and set client data fields
         */

        RealizeIconBox (pWS, pIBD, pCD);

	/*
	 * Link the new icon box to list of icon boxes
	 */
	AddNewBox (pWS, pIBD);
    }
    
    return (TRUE);  

} /* END OF FUNCTION MakeIconBox */



/*************************************<->*************************************
 *
 *  MakeShell (pWS, pIBD)
 *
 *
 *  Description:
 *  -----------
 *  
 *
 *
 *  Inputs:
 *  ------
 *  pWS = pointer to workspace data
 *
 *  pIBD  = pointer to IconBoxData
 *
 *  XXinput = ...
 *
 * 
 *  Outputs:
 *  -------
 *  
 *  pIBD->shellWidget 
 *
 *
 *  Comments:
 *  --------
 *  XXComments ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void MakeShell (pWS, pIBD)
    WmWorkspaceData *pWS;
    IconBoxData *pIBD;

#else /* _NO_PROTO */
void MakeShell (WmWorkspaceData *pWS, IconBoxData *pIBD)
#endif /* _NO_PROTO */
{

    Arg setArgs[20];
    int i;

    /*
     * Create top level application shell for icon box
     */

    i=0;

    XtSetArg (setArgs[i], XmNallowShellResize, (XtArgVal)True); i++; 
    
    XtSetArg (setArgs[i], XmNborderWidth, (XtArgVal)0); i++; 

    XtSetArg (setArgs[i], XmNkeyboardFocusPolicy, (XtArgVal)XmEXPLICIT); i++;

    if (!(Monochrome (XtScreen (pWS->pSD->screenTopLevelW))))
    {
	XtSetArg (setArgs[i], XmNbackground,  
		  (XtArgVal) pWS->pSD->clientAppearance.background ); i++;
	XtSetArg (setArgs[i], XmNforeground,  
		  (XtArgVal) pWS->pSD->clientAppearance.foreground ); i++;
    }
    XtSetArg (setArgs[i], XmNmappedWhenManaged, (XtArgVal)False); i++;
    XtSetArg (setArgs[i], XmNdialogStyle, (XtArgVal)XmDIALOG_MODELESS); i++;
    XtSetArg (setArgs[i], XmNdepth, 
	(XtArgVal) DefaultDepth (DISPLAY, pWS->pSD->screen)); i++;
    XtSetArg (setArgs[i], XmNscreen, 
	(XtArgVal) ScreenOfDisplay (DISPLAY, pWS->pSD->screen)); i++;

    pIBD->shellWidget = (Widget) XtCreatePopupShell (WmNiconBox, 
					topLevelShellWidgetClass,
                                        pWS->workspaceTopLevelW,
				        (ArgList)setArgs, i);

} /* END OF FUNCTION MakeShell */



/*************************************<->*************************************
 *
 *  MakeScrolledWindow (pWS, pIBD)
 *
 *
 *  Description:
 *  -----------
 *  
 *
 *
 *  Inputs:
 *  ------
 *  pWS	= pointer to workspace data
 *  pIBD  = pointer to IconBoxData
 *  XXinput = ...
 *
 * 
 *  Outputs:
 *  -------
 *  
 *  Return =  pIBD with the  pIBD->scrolledWidget set
 *
 *
 *  Comments:
 *  --------
 *  XXComments ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void MakeScrolledWindow (pWS, pIBD)
    WmWorkspaceData *pWS;
    IconBoxData *pIBD;

#else /* _NO_PROTO */
void MakeScrolledWindow (WmWorkspaceData *pWS, IconBoxData *pIBD)
#endif /* _NO_PROTO */
{

    Arg setArgs[20]; 
    int i;

    /*
     * Create frame widget to give the scrolled window 
     * an external bevel
     */

    i=0;
/*
    if (!(Monochrome (XtScreen (pWS->pSD->screenTopLevelW))))
    {
	XtSetArg (setArgs[i], XmNbackground,  
		  (XtArgVal) pWS->pSD->clientAppearance.background ); i++;
	XtSetArg (setArgs[i], XmNforeground,  
		  (XtArgVal) pWS->pSD->clientAppearance.foreground ); i++;
    }
*/
    XtSetArg (setArgs[i], XmNborderWidth,  (XtArgVal) 0 ); i++;
    XtSetArg (setArgs[i], XmNmarginWidth,  (XtArgVal) 0 ); i++;
    XtSetArg (setArgs[i], XmNmarginHeight, (XtArgVal) 0 ); i++;
    XtSetArg (setArgs[i], XmNshadowType, (XtArgVal) XmSHADOW_OUT); i++;
    XtSetArg (setArgs[i], XmNshadowThickness,
			(XtArgVal) frameShadowThickness); i++;
    pIBD->frameWidget = XtCreateManagedWidget ("IBframe", 
					xmFrameWidgetClass, 
					pIBD->shellWidget,
					(ArgList)setArgs, i);

    /*
     * Create scrolled window to hold row column manager 
     */

    i=0;

    XtSetArg (setArgs[i], XmNscrollingPolicy , (XtArgVal) XmAUTOMATIC ); i++;

    XtSetArg (setArgs[i], XmNborderWidth , (XtArgVal) 0 ); i++;
    XtSetArg (setArgs[i], XmNspacing , (XtArgVal) IB_MARGIN_WIDTH ); i++;

    if (!(Monochrome (XtScreen (pWS->pSD->screenTopLevelW))))
    {
	XtSetArg (setArgs[i], XmNbackground,  
		  (XtArgVal) pWS->pSD->clientAppearance.background ); i++;
	XtSetArg (setArgs[i], XmNforeground,  
		  (XtArgVal) pWS->pSD->clientAppearance.foreground ); i++;
    }
    /*
     * do we want to get these from a resource or set it here
     * to control the appearance of the iconBox
     */

    XtSetArg (setArgs[i], XmNscrolledWindowMarginWidth, (XtArgVal) 3); i++;
    XtSetArg (setArgs[i], XmNscrolledWindowMarginHeight, (XtArgVal) 3); i++;
    XtSetArg (setArgs[i], XmNshadowThickness,
			(XtArgVal) FRAME_EXTERNAL_SHADOW_WIDTH); i++;

    XtSetArg (setArgs[i], XmNscrollBarDisplayPolicy,(XtArgVal) XmSTATIC ); i++;
    XtSetArg (setArgs[i], XmNvisualPolicy, (XtArgVal) XmVARIABLE ); i++;

    pIBD->scrolledWidget = XtCreateManagedWidget ("IBsWindow", 
					xmScrolledWindowWidgetClass, 
					pIBD->frameWidget,
					(ArgList)setArgs, i);

#ifndef MOTIF_ONE_DOT_ONE
    XtAddCallback(pIBD->scrolledWidget, XmNtraverseObscuredCallback,
		  (XtCallbackProc) IconScrollVisibleCallback, (caddr_t)NULL);
#endif

    XtAddEventHandler(pIBD->scrolledWidget, 
			StructureNotifyMask, 
			False, 
			(XtEventHandler)UpdateIncrements, 
			(caddr_t) pIBD);



} /* END OF FUNCTION MakeScrolledWindow */



/*************************************<->*************************************
 *
 *  MakeBulletinBoard (pWS, pIBD)
 *
 *
 *  Description:
 *  -----------
 *  
 *
 *
 *  Inputs:
 *  ------
 *  pWS = pointer to workspace data
 *  pIBD  = pointer to IconBoxData
 *
 *  XXinput = ...
 *
 * 
 *  Outputs:
 *  -------
 *  
 *  Return =  pIBD with the  pIBD->bBoardWidget
 *
 *
 *  Comments:
 *  --------
 *  XXComments ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void MakeBulletinBoard (pWS, pIBD)
    WmWorkspaceData *pWS;
    IconBoxData *pIBD;

#else /* _NO_PROTO */
void MakeBulletinBoard (WmWorkspaceData *pWS, IconBoxData *pIBD)
#endif /* _NO_PROTO */
{

    int i;
    Arg setArgs[20];

    /*
     * Create bulletin board to hold icons
     */

    i=0;
    XtSetArg (setArgs[i], XmNborderWidth , 0); i++; 
    
    XtSetArg (setArgs[i], XmNshadowThickness,(XtArgVal) 0); i++;
    if (!(Monochrome (XtScreen (pWS->pSD->screenTopLevelW))))
    {
	XtSetArg (setArgs[i], XmNforeground,  
		  (XtArgVal) pWS->pSD->clientAppearance.background ); i++;
	XtSetArg (setArgs[i], XmNbottomShadowColor,  
		(XtArgVal) pWS->pSD->clientAppearance.bottomShadowColor ); i++;
	XtSetArg (setArgs[i], XmNtopShadowColor,  
		  (XtArgVal) pWS->pSD->clientAppearance.topShadowColor ); i++;
    }

    XtSetArg (setArgs[i], XmNspacing , 0); i++; 
    XtSetArg (setArgs[i], XmNmarginHeight , 0); i++;
    XtSetArg (setArgs[i], XmNmarginWidth ,  0); i++;

    XtSetArg (setArgs[i], XmNdialogStyle, (XtArgVal) XmDIALOG_WORK_AREA); i++;

    XtSetArg (setArgs[i], XmNresizePolicy, (XtArgVal) XmRESIZE_NONE); i++;
    XtSetArg (setArgs[i], XmNdefaultPosition , (XtArgVal) False); i++;

    XtSetArg (setArgs[i], XtNinsertPosition , InsertPosition); i++;

    pIBD->bBoardWidget = XtCreateManagedWidget ("IBbBoard", 
					xmBulletinBoardWidgetClass,
					pIBD->scrolledWidget,
					(ArgList)setArgs, i);

} /* END OF FUNCTION MakeBulletinBoard */



/*************************************<->*************************************
 *
 *  RealizeIconBox (pWS, pIBD, pCD)
 *
 *
 *  Description:
 *  -----------
 *  
 *
 *
 *  Inputs:
 *  ------
 *  pWS = pointer to workspace data 
 *
 *  pIBD  = pointer to IconBoxData
 *
 *  pCD   = pointer to ClientData
 *
 * 
 *  Outputs:
 *  -------
 *  

 *  Return =  pIBD with the  pIBD->shellWin set
 *  Return =  pIBD with the  pIBD->scrolledWin set
 *  Return =  pIBD with the  pIBD->bBoardWin set

 *
 *  Return =  pCD  with appropriate fields set
 *
 *
 *  Comments:
 *  --------
 *  XXComments ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void RealizeIconBox (pWS, pIBD, pCD)
    WmWorkspaceData *pWS;
    IconBoxData *pIBD;
    ClientData *pCD;

#else /* _NO_PROTO */
void RealizeIconBox (WmWorkspaceData *pWS, IconBoxData *pIBD, ClientData *pCD)
#endif /* _NO_PROTO */
{

    int i;
    Arg getArgs[10]; 
    Arg setArgs[2]; 
    Widget clipWidget;
    Pixmap  bgPixmap;
    Pixmap defaultImage;
#ifdef DEC_MOTIF_EXTENSION
char *image_bits;
int image_size;
#endif
    

    XtRealizeWidget (pIBD->shellWidget);

    pCD->client = XtWindow (pIBD->shellWidget);

    /*
     * This will set the scrolling granularity for the icon box
     */

    SetGeometry (pWS, pCD, pIBD);

    /*
     * Point to the iconBox 
     */

    pIBD->pCD_iconBox = pCD;
    pCD->thisIconBox = pIBD;    
    /*
     * get the background color of the bulletin board for
     * greyed icon work
     */

    i=0;
    XtSetArg (setArgs[i], XmNbackground, (XtArgVal) select_color ); i++;
    XtSetValues (pIBD->bBoardWidget, (ArgList) setArgs, i); 


    i=0;
    XtSetArg (getArgs[i], XmNbackgroundPixmap, (XtArgVal) &bgPixmap ); i++;
    XtGetValues (pIBD->bBoardWidget, getArgs, i);

    i=0;
    XtSetArg (getArgs[i], XmNclipWindow, (XtArgVal) &clipWidget ); i++;
    XtGetValues (pIBD->scrolledWidget, getArgs, i);

    /*
     * Set the background of the clip window for the scrolled 
     * window so the default widget background doesn't flash
     */

    i = 0;
    XtSetArg(setArgs[i], XmNbackground,	(XtArgVal) select_color); i++;
    XtSetValues (clipWidget, (ArgList) setArgs, i); 


    /*
     * Save the clipWidget id to use in constraining icon moves in box
     */

    pIBD->clipWidget = clipWidget; 

    MakeShrinkWrapIconsGC (pWS->pSD, bgPixmap);

    
    if (pWS->pSD->iconDecoration & ICON_IMAGE_PART)
    {
        /*
         * Make a pixmap to use when iconWindows are unmapped
         */
#ifdef DEC_MOTIF_EXTENSION
         /* Large icon ? */
         if (( pWS->pSD->iconImageMaximum.width >= iImage_large_width ) &&
             ( pWS->pSD->iconImageMaximum.height >= iImage_large_height ))
           {                  
             image_bits = iImage_large_bits;
             image_size = iImage_large_width;       
           }
         /* medium icon ? */
         else if (( pWS->pSD->iconImageMaximum.width >= iImage_width ) &&
                  ( pWS->pSD->iconImageMaximum.height >= iImage_height ))   
           {                  
             image_bits = iImage_bits;
             image_size = iImage_width;
           }
         /* Small icon ! */
         else 
           {                  
             image_bits = iImage_small_bits;    
             image_size = iImage_small_width;
           }
        defaultImage = XCreateBitmapFromData (DISPLAY, pWS->pSD->rootWindow,
					      image_bits, image_size, 
					      image_size );
        pWS->pSD->defaultPixmap = MakeIconPixmap (pCD,
                                                  defaultImage,
                                                  NULL, image_size,
                                                  image_size, 1); 
#else                                                                    
        defaultImage = XCreateBitmapFromData (DISPLAY, pWS->pSD->rootWindow,
					      iImage_bits, iImage_width, 
					      iImage_height);
        pWS->pSD->defaultPixmap = MakeIconPixmap (pCD,
                                                  defaultImage,
                                                  NULL, iImage_width,
                                                  iImage_height, 1);
#endif /* DEC_MOTIF_EXTENSION */
    }
    
    
} /* END OF FUNCTION RealizeIconBox */



/*************************************<->*************************************
 *
 *  AddNewBox (pWS, pIBD)
 *
 *
 *  Description:
 *  -----------
 *  
 *
 *
 *  Inputs:
 *  ------
 *  pWS = pointer to workspace data
 *
 *  pIBD  = pointer to IconBoxData
 *
 * 
 *  Outputs:
 *  -------
 *  
 *
 *  Comments:
 *  --------
 *  Finds the last iconbox on the list starting at pWS->pIconBox and
 *  adds the new icon box to the end of the list.
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void AddNewBox (pWS, pIBD)
    WmWorkspaceData *pWS;
    IconBoxData *pIBD;

#else /* _NO_PROTO */
void AddNewBox (WmWorkspaceData *pWS, IconBoxData *pIBD)
#endif /* _NO_PROTO */
{
 
    IconBoxData *pibd;

    if (pWS->pIconBox)
    {
	pibd = pWS->pIconBox;

        while (pibd->pNextIconBox)
        {
	    pibd = pibd->pNextIconBox;
        }

        pibd->pNextIconBox = pIBD;
    }
    else
    {
	pWS->pIconBox = pIBD;
    }

  
} /* END OF FUNCTION AddNewBox */



/*************************************<->*************************************
 *
 *  InitializeIconBoxData (pWS, pIBD)
 *
 *
 *  Description:
 *  -----------
 *  
 *
 *
 *  Inputs:
 *  ------
 *  pWS = pointer to Workspace Data
 *
 *  pIBD  = pointer to IconBoxData
 *
 * 
 *  Outputs:
 *  -------
 *  
 *
 *  Comments:
 *  --------
 *  Initializes all pIBD fields to NULL
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void InitializeIconBoxData (pWS, pIBD)
    WmWorkspaceData *pWS;
    IconBoxData *pIBD;

#else /* _NO_PROTO */
void InitializeIconBoxData (WmWorkspaceData *pWS, IconBoxData *pIBD)
#endif /* _NO_PROTO */
{
    int mask;
    int X;
    int Y;
    unsigned int width;
    unsigned int height;
    int sW, sH;

    frameShadowThickness = FRAME_INTERNAL_SHADOW_WIDTH;

    pIBD->numberOfIcons = 0;
    pIBD->currentRow = 0;
    pIBD->currentCol = 0;
    pIBD->lastRow = 0;
    pIBD->lastCol = 0;
    pIBD->IPD.placeList = NULL;

    pIBD->scrolledWidget = NULL;
    pIBD->bBoardWidget = NULL;
    pIBD->clipWidget = NULL; 

#ifndef VMS
    ToLower ((unsigned char *) pWS->pSD->iconBoxSBDisplayPolicy);
#else
    ToLower2 ((unsigned char *) pWS->pSD->iconBoxSBDisplayPolicy);
#endif
    
    if (!((!strcmp(pWS->pSD->iconBoxSBDisplayPolicy , "all"))      ||
	  (!strcmp(pWS->pSD->iconBoxSBDisplayPolicy , "vertical")) ||
	  (!strcmp(pWS->pSD->iconBoxSBDisplayPolicy , "horizontal"))))
    {
	strcpy(pWS->pSD->iconBoxSBDisplayPolicy, "all");
    }

	

    /*
     * this will be set by the iconPlacement resource if
     * iconBoxGeometry width and height are not specified
     */

    if (pWS->pSD->iconBoxGeometry == NULL) /* not set by user */
    {
	/*
	 * Use the iconPlacement resource 
	 */
	
	if (pWS->pSD->iconPlacement & 
	    (ICON_PLACE_TOP_PRIMARY | ICON_PLACE_BOTTOM_PRIMARY))
	{
	    pIBD->IPD.iconPlacement = ICON_PLACE_TOP_PRIMARY;
	    pIBD->IPD.placementCols = 1;  
	    pIBD->IPD.placementRows = 6;
	}
	else
	{
	    pIBD->IPD.iconPlacement = ICON_PLACE_LEFT_PRIMARY;
	    pIBD->IPD.placementCols = 6;  
	    pIBD->IPD.placementRows = 1;
	}

    }
    else
    {
	mask = XParseGeometry(pWS->pSD->iconBoxGeometry, &X, &Y, 
			      &width, &height);

	if ((mask & WidthValue) && (width > 0))
	{
	    pIBD->IPD.placementCols = (int)width; 
	}
	else
	{
	    pIBD->IPD.placementCols = 6;   
	}

	if ((mask & HeightValue) && (height > 0))
	{
	    pIBD->IPD.placementRows = (int)height; 
	}
	else
	{
	    pIBD->IPD.placementRows = 1; 
	}

	/*
	 * Set orientation 
	 */

	if (pIBD->IPD.placementRows <= pIBD->IPD.placementCols) 
	{
	    pIBD->IPD.iconPlacement = ICON_PLACE_LEFT_PRIMARY;
	}
	else
	{
	    pIBD->IPD.iconPlacement = ICON_PLACE_TOP_PRIMARY;
	}
    }


    /*
     * Override orientation if iconBoxSBDisplayPolicy is set to
     * horizontal or vertical
     */

    if (!(strcmp(pWS->pSD->iconBoxSBDisplayPolicy , "vertical")))  
    {
	pIBD->IPD.iconPlacement = ICON_PLACE_LEFT_PRIMARY;	    
    }
    else if (!(strcmp(pWS->pSD->iconBoxSBDisplayPolicy , "horizontal")))
    {
	pIBD->IPD.iconPlacement = ICON_PLACE_TOP_PRIMARY;
    }
	

    

    /* 
     * set initial size of placement space to size of screen 
     */

    sW = DisplayWidth (DISPLAY, pWS->pSD->screen) / pWS->pSD->iconWidth;
    sH = DisplayHeight (DISPLAY, pWS->pSD->screen) / pWS->pSD->iconHeight;

    pIBD->IPD.totalPlaces = sW * sH;

    pIBD->IPD.onRootWindow = False;

    /*
     * The icon box does not live in an icon box in this version
     */

    pIBD->pNextIconBox =NULL;
    

} /* END OF FUNCTION InitializeIconBoxData */   



/*************************************<->*************************************
 *
 *  SetIconBoxInfo (pWS, pCD)
 *
 *
 *  Description:
 *  -----------
 *
 *  Inputs:
 *  ------
 *  pCD
 *
 * 
 *  Comments:
 *  --------
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void SetIconBoxInfo (pWS, pCD)
    WmWorkspaceData *pWS;
    ClientData *pCD;
#else /* _NO_PROTO */
void SetIconBoxInfo (WmWorkspaceData *pWS, ClientData *pCD)
#endif /* _NO_PROTO */
{
    pCD->clientClass = WmCIconBox;
    pCD->clientName = pWS->pSD->iconBoxName;
    ProcessClientResources (pCD); 

} /* END OF FUNCTION SetIconBoxInfo */



/*************************************<->*************************************
 *
 *  InitializeClientData (pCD)
 *
 *
 *  Description:
 *  -----------
 *  
 *
 *
 *  Inputs:
 *  ------
 *  pCD
 *
 * 
 *  Outputs:
 *  -------
 *  
 *
 *  Comments:
 *  --------
 *  Initializes geometry, etc. fields 
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void InitializeClientData (pCD, pIBD)
    ClientData *pCD;
    IconBoxData *pIBD;

#else /* _NO_PROTO */
void InitializeClientData (ClientData *pCD, IconBoxData *pIBD)
#endif /* _NO_PROTO */
{
    pCD->internalBevel = FRAME_INTERNAL_SHADOW_WIDTH;

    pCD->clientX = 0;
    pCD->clientY = 0;

    pCD->clientFlags |= ICON_BOX ;

    pCD->widthInc = pIBD->IPD.iPlaceW = ICON_WIDTH(pCD)   
	+ IB_SPACING 
	+ (2 * IB_MARGIN_WIDTH); 

    pCD->heightInc = pIBD->IPD.iPlaceH = ICON_HEIGHT(pCD) 
	+ IB_SPACING  
	+ (2 * IB_MARGIN_HEIGHT);

    pCD->clientWidth = pIBD->IPD.placementCols * pCD->widthInc;
    pCD->clientHeight = pIBD->IPD.placementRows * pCD->heightInc;
    
    if (!(pCD->pSD->iconBoxTitle))
    {
#ifdef DEC_MOTIF_EXTENSION
/* For internationalization */
	pCD->pSD->iconBoxTitle =
          XmStringCreate( DEFAULT_ICON_BOX_TITLE, XmSTRING_DEFAULT_CHARSET );
#else
	pCD->pSD->iconBoxTitle = 
	    XmStringCreateLtoR(DEFAULT_ICON_BOX_TITLE,
			       XmFONTLIST_DEFAULT_TAG);
#endif
    }

    pCD->clientTitle = pCD->pSD->iconBoxTitle;
    pCD->iconTitle   = pCD->pSD->iconBoxTitle;
    
} /* END OF FUNCTION InitializeClientData */   



/*************************************<->*************************************
 *
 *  MakeShrinkWrapIconsGC (pSD, bgPixmap)
 *
 *
 *  Description:
 *  -----------
 *  Make an  graphic context to shrink the icons in the icon box
 *      box that are not in the MINIMIZED_STATE.
 *
 *
 *  Inputs:
 *  ------
 *  pSD		- pointer to screen data
 * 
 *  Outputs:
 *  -------
 *  Modifies global data
 *
 *  Comments:
 *  --------
 *  
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void MakeShrinkWrapIconsGC (pSD, bgPixmap)
    WmScreenData *pSD;
    Pixmap bgPixmap;
#else /* _NO_PROTO */
void MakeShrinkWrapIconsGC (WmScreenData *pSD, Pixmap bgPixmap)
#endif /* _NO_PROTO */
{

    XtGCMask  copyMask;
   

    if (!pSD->shrinkWrapGC)
    {
	pSD->shrinkWrapGC = XCreateGC (DISPLAY, pSD->rootWindow, 0, 
		(XGCValues *) NULL);

	copyMask = ~0L;

	XCopyGC (DISPLAY, pSD->iconAppearance.inactiveGC,
		copyMask, pSD->shrinkWrapGC);

	if (bgPixmap != XmUNSPECIFIED_PIXMAP)
	{
	    XSetTile (DISPLAY, pSD->shrinkWrapGC,  bgPixmap); 
	    XSetFillStyle (DISPLAY, pSD->shrinkWrapGC, FillTiled);
	    XSetBackground (DISPLAY, pSD->shrinkWrapGC, select_color);
	}
	else
	{
	    XSetForeground (DISPLAY, pSD->shrinkWrapGC, select_color);
	}
    }

} /* END OF FUNCTION MakeShrinkWrapIconsGC */



/*************************************<->*************************************
 *
 *  MakeFadeIconGC (pSD)
 *
 *
 *  Description:
 *  -----------
 *  Make an  graphic context for "greying" the icons in the icon
 *      box that are not in the MINIMIZED_STATE.
 *
 *
 *  Inputs:
 *  ------
 *  pSD = pointer to screen data
 * 
 *  Outputs:
 *  -------
 *  Modifies global data
 *
 *  Comments:
 *  --------
 *  
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void MakeFadeIconGC (pSD)

    WmScreenData *pSD;
#else /* _NO_PROTO */
void MakeFadeIconGC (WmScreenData *pSD)
#endif /* _NO_PROTO */
{

    XtGCMask  copyMask;
    static    Pixmap tmpFontClipMask;
   

    pSD->fadeIconGC = XCreateGC (DISPLAY, pSD->rootWindow, 0, 
				(XGCValues *) NULL);
    pSD->fadeIconTextGC = XCreateGC (DISPLAY, pSD->rootWindow, 0, 
				(XGCValues *) NULL);

    copyMask = ~0L;

    XCopyGC (DISPLAY, pSD->iconAppearance.inactiveGC,
		copyMask, pSD->fadeIconGC);

    XCopyGC (DISPLAY, pSD->iconAppearance.inactiveGC,
		copyMask, pSD->fadeIconTextGC);

    tmpFontClipMask = XCreateBitmapFromData (DISPLAY, pSD->rootWindow,
                        greyed75_bits, greyed75_width, greyed75_height);

    greyedPixmap = XCreateBitmapFromData (DISPLAY, pSD->rootWindow,
                        slant2_bits, slant2_width, slant2_height);

    XSetStipple (DISPLAY, pSD->fadeIconTextGC,  tmpFontClipMask); 
    XSetFillStyle (DISPLAY, pSD->fadeIconTextGC, FillStippled);

    XSetStipple (DISPLAY, pSD->fadeIconGC,  greyedPixmap); 
    XSetFillStyle (DISPLAY, pSD->fadeIconGC, FillStippled);
    XSetForeground (DISPLAY, pSD->fadeIconGC, select_color);

} /* END OF FUNCTION MakeFadeIconGC */



/*************************************<->*************************************
 *
 *  SetGeometry (pWS, pCD, pIBD)
 *
 *
 *  Description:
 *  -----------
 *  
 *
 *
 *  Inputs:
 *  ------
 *  pIBD  = pointer to IconBoxData
 *  pCD   = pointer to ClientData
 *  XXinput = ...
 *
 * 
 *  Outputs:
 *  -------
 *  
 *  
 *
 *
 *  Comments:
 *  --------
 *  XXComments ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void SetGeometry(pWS, pCD, pIBD)
    WmWorkspaceData *pWS;
    ClientData *pCD;
    IconBoxData *pIBD;

#else /* _NO_PROTO */
void SetGeometry (WmWorkspaceData *pWS, ClientData *pCD, IconBoxData *pIBD)
#endif /* _NO_PROTO */
{

    int i;
    Arg setArgs[10];

    int mask;
    int X;
    int Y;
    unsigned int width;
    unsigned int height;

    /*
     * Set horizontal and vertical scrolling granularity
     */

    SetGranularity (pWS, pCD, pIBD );

    /*
     * Set the initial width and height of the icon box bulletin board
     */

    i=0; 
    XtSetArg (setArgs[i], XmNwidth, (XtArgVal) pCD->clientWidth); i++;
    XtSetArg (setArgs[i], XmNheight, (XtArgVal) pCD->clientHeight); i++;
    XtSetValues (pIBD->bBoardWidget, (ArgList) setArgs, i); 

    /*
     * Adjust icon box window height for height of 
     * horizontal scroll bar etc.
     */

    pCD->clientHeight = pCD->clientHeight + pCD->baseHeight;
    pCD->maxHeight = pCD->clientHeight;

    /*
     * Adjust iconbox window width for width of 
     * vertical scroll bar etc.
     */

    pCD->clientWidth = pCD->clientWidth + pCD->baseWidth;
    pCD->maxWidth = pCD->clientWidth;


    /*
     * Set the initial width and height of the icon box scrolled Window
     */

    i=0; 
    XtSetArg (setArgs[i], XmNwidth, (XtArgVal) 
		(pCD->clientWidth - (2 * frameShadowThickness))); i++;
    XtSetArg (setArgs[i], XmNheight, (XtArgVal) 
		(pCD->clientHeight - (2 * frameShadowThickness))); i++;

    XtSetValues (pIBD->scrolledWidget, (ArgList) setArgs, i); 


	     
    /*
     * Call SetFrameInfo with fake X and Y so we can get clientOffset
     */

    pCD->xBorderWidth = 0;
    SetFrameInfo (pCD);


    /*
     * Set initial placement of icon box
     */

    mask = XParseGeometry(pCD->pSD->iconBoxGeometry, 
			  &X, &Y, &width, &height);
    
    if (mask & XValue)
    {
	if (mask & XNegative)
	{
	    pCD->clientX = X 
	    		   + DisplayWidth(DISPLAY, SCREEN_FOR_CLIENT(pCD)) 
			   - pCD->clientWidth
			   - pCD->clientOffset.x;
	}
	else 
	{
	    pCD->clientX = X + pCD->clientOffset.x;
	}
    }
    else
    {
	pCD->clientX = pCD->clientOffset.x;
    }

    if (mask & YValue)
    {
	if (mask & YNegative)
	{
	    pCD->clientY = Y 
			   + DisplayHeight(DISPLAY, SCREEN_FOR_CLIENT(pCD)) 
			   - pCD->clientHeight
			   - pCD->clientOffset.x ;
	}
	else
	{
	    pCD->clientY = Y + pCD->clientOffset.y;
	}
    }
    else
    {
	pCD->clientY =  pCD->clientOffset.x
			+ DisplayHeight(DISPLAY, SCREEN_FOR_CLIENT(pCD)) 
			- pCD->clientHeight;
    }


    PlaceFrameOnScreen (pCD, &pCD->clientX, &pCD->clientY, pCD->clientWidth,
        pCD->clientHeight);
    pCD->clientX -= (wmGD.positionIsFrame
			? pCD->clientOffset.x
			: 0);
			
    pCD->clientY -=  (wmGD.positionIsFrame
			? pCD->clientOffset.y
			: 0);


    i=0; 

    XtSetArg (setArgs[i], XmNx, (XtArgVal) pCD->clientX); i++;
    XtSetArg (setArgs[i], XmNy, (XtArgVal) pCD->clientY); i++;

    XtSetValues (pIBD->shellWidget, (ArgList) setArgs, i); 

    pCD->maxX = pCD->clientX;
    pCD->maxY = pCD->clientY;


} /* END OF FUNCTION SetGeometry */



/*************************************<->*************************************
 *
 *  SetGranularity (pWS, pCD, pIBD )
 *
 *
 *  Description:
 *  -----------
 *  
 *
 *
 *  Inputs:
 *  ------
 *  pIBD  = pointer to IconBoxData
 *  pCD   = pointer to ClientData
 *  XXinput = ...
 *
 * 
 *  Outputs:
 *  -------
 *  
 *  
 *
 *
 *  Comments:
 *  --------
 *  XXComments ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void SetGranularity(pWS, pCD, pIBD)
    WmWorkspaceData *pWS;
    ClientData *pCD;
    IconBoxData *pIBD;

#else /* _NO_PROTO */
void SetGranularity (WmWorkspaceData *pWS, ClientData *pCD, IconBoxData *pIBD)
#endif /* _NO_PROTO */
{

    int i;
    Dimension hScrollBarHeight = 0;
    Dimension hBarHeight = 0;
    Dimension vScrollBarWidth = 0;
    Dimension vBarWidth = 0;

    Dimension spacing;
    short shadowThickness;
    short marginWidth;
    short marginHeight;

    short hShighlightThickness;
    short vShighlightThickness;

    Arg setArgs[10];
    Arg getArgs[10]; 

    i=0;



    XtSetArg(getArgs[i], XmNspacing, (XtArgVal) &spacing ); i++;
    XtSetArg(getArgs[i], XmNshadowThickness, (XtArgVal) &shadowThickness); i++;
    XtSetArg(getArgs[i], XmNscrolledWindowMarginWidth, 
					(XtArgVal) &marginWidth); i++;
    XtSetArg(getArgs[i], XmNscrolledWindowMarginHeight, 
					(XtArgVal) &marginHeight); i++;
    XtSetArg (getArgs[i], XmNverticalScrollBar, 
	                                (XtArgVal) &pIBD->vScrollBar); i++;
    XtSetArg(getArgs[i], XmNhorizontalScrollBar, 
	                                (XtArgVal) &pIBD->hScrollBar); i++;
    XtGetValues (pIBD->scrolledWidget, getArgs, i);

    
    if (strcmp(pWS->pSD->iconBoxSBDisplayPolicy , "vertical"))
    {
	
	/*
	 * Set horizontal scrolling granularity
	 */
	i=0;
        XtSetArg (getArgs[i], XmNheight, (XtArgVal) &hBarHeight ); i++;
        XtSetArg (getArgs[i], XmNhighlightThickness,
                 (XtArgVal) &hShighlightThickness); i++;
        XtGetValues (pIBD->hScrollBar, getArgs, i);


	i=0; 
	XtSetArg(setArgs[i], XmNincrement, (XtArgVal) pCD->widthInc); i++;
	XtSetArg (setArgs[i], XmNhighlightThickness ,
		  IB_HIGHLIGHT_BORDER); i++;
        XtSetArg(setArgs[i], XmNheight,
                 (XtArgVal) (hBarHeight - (2 * hShighlightThickness)) +
                 (2 * IB_HIGHLIGHT_BORDER)); i++;

	XtSetValues (pIBD->hScrollBar, (ArgList) setArgs, i); 
	
	/*
	 * Get hScrollBarHeight and troughColor
	 */
	
	i=0;
	XtSetArg (getArgs[i], XmNtroughColor, (XtArgVal) &select_color ); i++;
	XtSetArg (getArgs[i], XmNheight, (XtArgVal) &hScrollBarHeight ); i++;
	XtGetValues (pIBD->hScrollBar, getArgs, i);

    }
    
    
    if (strcmp(pWS->pSD->iconBoxSBDisplayPolicy , "horizontal"))
    {
	
	/*
	 * Set vertical scrolling granularity
	 */
        i=0;
        XtSetArg (getArgs[i], XmNwidth, (XtArgVal) &vBarWidth ); i++;
        XtSetArg (getArgs[i], XmNhighlightThickness,
                 (XtArgVal) &vShighlightThickness); i++;
        XtGetValues (pIBD->vScrollBar, getArgs, i);


	i=0; 
	XtSetArg (setArgs[i], XmNincrement, (XtArgVal) pCD->heightInc); i++;
	XtSetArg (setArgs[i], XmNhighlightThickness ,
                        IB_HIGHLIGHT_BORDER); i++;
        XtSetArg(setArgs[i], XmNwidth,
                 (XtArgVal) (vBarWidth - (2 * vShighlightThickness)) +
                 (2 * IB_HIGHLIGHT_BORDER)); i++;

	XtSetValues (pIBD->vScrollBar, (ArgList) setArgs, i); 
	
	/*
	 * Get vScrollBarWidth
	 */
	
	i=0;
	XtSetArg (getArgs[i], XmNtroughColor, (XtArgVal) &select_color ); i++;
	XtSetArg (getArgs[i], XmNwidth, (XtArgVal) &vScrollBarWidth ); i++;
	XtGetValues (pIBD->vScrollBar, getArgs, i);
    }
    
    
    
    if (!strcmp(pWS->pSD->iconBoxSBDisplayPolicy , "vertical"))
    {
	XtUnmanageChild(pIBD->hScrollBar);
	hScrollBarHeight = 0;
	
	i=0;
	XtSetArg (setArgs[i], XmNscrollBarDisplayPolicy, 
		  (XtArgVal) XmAS_NEEDED ); i++;
	XtSetValues (pIBD->scrolledWidget, (ArgList) setArgs, i);
	
    }
    else if (!strcmp(pWS->pSD->iconBoxSBDisplayPolicy , "horizontal"))
    {
	XtUnmanageChild(pIBD->vScrollBar);
	vScrollBarWidth = 0;
	
	i=0;
	XtSetArg (setArgs[i], XmNscrollBarDisplayPolicy, 
		  (XtArgVal) XmAS_NEEDED ); i++;
	XtSetValues (pIBD->scrolledWidget, (ArgList) setArgs, i);
    }
    
#ifdef DEC_MOTIF_EXTENSION
/* Get the correct icon box color */

    /* Does this system have different color displays and is 
       this a monochrome or gray-scale monitor ? */
    if ( wmGD.multicolor &&
       (( pWS->pSD->monitor == k_mwm_bw_type ) ||
        ( pWS->pSD->monitor == k_mwm_gray_type )))
      /* Yes, get the icon box background color */
      {   
/* Note this routine should be placed in mwm_util.c for
   modularity. */
        /* Is the value in the user database ? */
        if ( mwm_res_get( pWS->pSD, pWS->pSD->user_database, 
                          WmNiconBoxHBackground, k_mwm_res_pixel,
                          &select_color, NULL ))
                mwm_set( pIBD->shellWidget, WmNiconBoxBackgroundPixmap, 
                         k_mwm_unspec_pixmap_str );  
            /* No, check the system database */
            else if ( mwm_res_get( pWS->pSD, pWS->pSD->sys_database, 
                                   WmNiconBoxHBackground, k_mwm_res_pixel,
                                   &select_color, NULL ))
                mwm_set( pIBD->shellWidget, WmNiconBoxBackgroundPixmap, 
                         k_mwm_unspec_pixmap_str );
      }
#endif /* DEC_MOTIF_EXTENSION */
    pCD->baseWidth =  IB_SPACING 
	               + 2 * IB_HIGHLIGHT_BORDER
		       + spacing
		       + (int) vScrollBarWidth 
		       + 2 * frameShadowThickness
		       + (int) 2 * marginWidth
		       + (marginWidth > 0 
			       ? 2 * (int) shadowThickness 
			       : shadowThickness);

			

    pCD->baseHeight =  IB_SPACING
	                + 2 * IB_HIGHLIGHT_BORDER
			+ spacing
			+ (int) hScrollBarHeight 
			+ 2 * frameShadowThickness
			+ (int) 2 * marginHeight
			+ (marginHeight > 0 
				? 2 * (int) shadowThickness 
				: shadowThickness);

    pCD->minWidth = pCD->baseWidth + pCD->widthInc;
    pCD->minHeight = pCD->baseHeight + pCD->heightInc;

    pCD->maxWidth = pCD->minWidth;

    pCD->maxHeight = pCD->minHeight;

} /* END OF FUNCTION SetGranularity */



/*************************************<->*************************************
 *
 * GetIconBoxMenuItems ()
 *
 *
 *  Description:
 *  -----------
 *  XXDescription ...
 * 
 *************************************<->***********************************/

MenuItem *GetIconBoxMenuItems (pSD)

    WmScreenData *pSD;

{

#ifdef DEC_MOTIF_EXTENSION
    /* Get from configuration file */
    return( NULL );
#else
    return(ParseMwmMenuStr (pSD, 
	(unsigned char *)"\"Pack Icons\" _P  Shift Alt<Key>F7 f.pack_icons\n"));
#endif

} /* END OF FUNCTION GetIconBoxMenuItems */



/*************************************<->*************************************
 *
 *  MapIconBoxes ()
 *
 *
 *  Description:
 *  -----------
 *  
 *
 *
 *  Inputs:
 *  ------
 *
 * 
 *  Outputs:
 *  -------
 *  
 *
 *  Comments:
 *  --------
 *  Maps all iconboxes on the list starting at pWS->pIconBox 
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void MapIconBoxes (pWS)

    WmWorkspaceData *pWS;
#else /* _NO_PROTO */
void MapIconBoxes (WmWorkspaceData *pWS)
#endif /* _NO_PROTO */
{
 
    IconBoxData *pibd;

    if (pWS->pIconBox)
    {
	pibd = pWS->pIconBox;

        while (pibd)
        {
	    XtPopup(pibd->shellWidget, XtGrabNone);
	    F_Raise (NULL, pibd->pCD_iconBox, (XEvent *)NULL);
	    XMapWindow (DISPLAY, pibd->pCD_iconBox->clientFrameWin);
	    pibd = pibd->pNextIconBox;
        }
    }

  
} /* END OF FUNCTION MapIconBoxes */






/******************************<->*************************************
 *
 *  InsertIconIntoBox
 *
 *  Inputs
 *  ------
 *  pCD		- pointer to data for client to insert
 *
 *  Description:
 *  -----------
 *  XXDescription ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
Boolean InsertIconIntoBox(pIBD, pCD)   
    IconBoxData  *pIBD;
    ClientData *pCD;

#else /* _NO_PROTO */
Boolean InsertIconIntoBox (IconBoxData *pIBD, ClientData *pCD)
#endif /* _NO_PROTO */
{

    Boolean rval = False;
    Arg setArgs[20]; 
    int i;
    int iconWidth, iconHeight;
    IconBoxData  *tmpPointerToIconBox;
    Widget iconWidget;
    IconInfo *pIconInfo;

    /*
     * If we go to multiple icon boxes, find the box this client
     * wants to live in.  For now, we only have one, so point to
     * the first one.
     */

    tmpPointerToIconBox = pIBD;
    
    if (pCD->client)
    {

        P_ICON_BOX(pCD) = tmpPointerToIconBox;

        iconWidth = ICON_WIDTH(pCD)
		+ (2 * IB_MARGIN_WIDTH); 

        iconHeight = ICON_HEIGHT(pCD) 
		+ (2 * IB_MARGIN_HEIGHT);

        pIconInfo = InsertIconInfo  (P_ICON_BOX(pCD), pCD, (Widget) NULL);

	if (pIconInfo)
	{
	    P_ICON_BOX(pCD)->numberOfIcons++;

	    i = 0;
	    XtSetArg (setArgs[i], XmNbackground,  
			    (XtArgVal) ICON_APPEARANCE(pCD).background ); i++;
	    XtSetArg (setArgs[i], XmNforeground,  
			    (XtArgVal) ICON_APPEARANCE(pCD).foreground ); i++;

	    XtSetArg (setArgs[i], XmNx ,  (XtArgVal) ICON_X(pCD)); i++;
	    XtSetArg (setArgs[i], XmNy ,  (XtArgVal) ICON_Y(pCD)); i++;

	    XtSetArg (setArgs[i], XmNwidth ,  (XtArgVal) iconWidth); i++;
	    XtSetArg (setArgs[i], XmNheight ,  (XtArgVal) iconHeight); i++;

	    XtSetArg (setArgs[i], XmNborderWidth ,  (XtArgVal) 0); i++;

	    XtSetArg (setArgs[i], XmNhighlightThickness ,  
			    IB_HIGHLIGHT_BORDER); i++;

	    XtSetArg (setArgs[i], XmNmarginHeight , (XtArgVal) 0); i++;
	    XtSetArg (setArgs[i], XmNmarginWidth , 	(XtArgVal) 0); i++;
	    /* 
	     * Use type XmString so we don't get a message from XmLabel
	     */
	    XtSetArg (setArgs[i], XmNlabelType, (XtArgVal) XmSTRING); i++;

	    XtSetArg (setArgs[i], XmNrecomputeSize, (XtArgVal) False); i++;

	    XtSetArg (setArgs[i], XmNtraversalOn, (XtArgVal) True); i++;

	    XtSetArg (setArgs[i], XmNpushButtonEnabled, (XtArgVal) False); i++;

	    XtSetArg (setArgs[i], XmNshadowThickness, (XtArgVal) 0); i++;

	    iconWidget =  XtCreateManagedWidget("iconInIconBox",
					   xmDrawnButtonWidgetClass,
					   P_ICON_BOX(pCD)->bBoardWidget,
					   (ArgList)setArgs, i);

	    pIconInfo->theWidget = iconWidget;

	    ICON_FRAME_WIN(pCD) = XtWindow (iconWidget); 

	    XtAddCallback (iconWidget, XmNactivateCallback, 
			   (XtCallbackProc) IconActivateCallback, (caddr_t)NULL);

	    XtAddEventHandler(iconWidget, 
			      SELECT_BUTTON_MOTION_MASK, 
			      False, 
			      (XtEventHandler)HandleIconBoxButtonMotion, 
			      NULL);

	    XtAddEventHandler(iconWidget, 
			      DMANIP_BUTTON_MOTION_MASK, 
			      False, 
			      (XtEventHandler)HandleIconBoxButtonMotion, 
			      NULL);

            XtAddEventHandler(iconWidget,
			      KeyPressMask,
			      False,
			      (XtEventHandler)HandleIconBoxIconKeyPress,
			      NULL);

	    
	    
	    if (ICON_DECORATION(pCD) & ICON_ACTIVE_LABEL_PART)
	    {
		XtAddEventHandler(iconWidget, 
				  FocusChangeMask, 
				  False, 
				  (XtEventHandler)ChangeActiveIconboxIconText, 
				  NULL);

		if (pCD->pSD->activeLabelParent != pCD->pSD->rootWindow)
		{
		    XRaiseWindow (DISPLAY, pCD->pSD->activeIconTextWin);
		}
	    }

	    ResetIconBoxMaxSize(P_ICON_BOX(pCD)->pCD_iconBox, 
				P_ICON_BOX(pCD)->bBoardWidget);

	    ResetArrowButtonIncrements (P_ICON_BOX(pCD)->pCD_iconBox);

	    rval = True;
	}
    } 
    return(rval);

} /* END FUNCTION InsertIconIntoBox() */   
   


/*************************************<->*************************************
 *
 *  InsertIconInfo  (pIBD, pCD, theWidget)
 *
 *
 *  Description:
 *  -----------
 *  Finds next available spot and inserts the icon 
 *
 *
 *  Inputs:
 *  ------
 *  pIBD	- pointer to icon box data
 *  pCD		- pointer to client data for this client
 *  theWidget	- widget containing the icon (may be null)
 * 
 *  Outputs:
 *  -------
 *
 *  Comments:
 *  --------
 * 
 *************************************<->***********************************/

IconInfo *InsertIconInfo (pIBD, pCD, theWidget)

    IconBoxData *pIBD;
    ClientData *pCD;
    Widget theWidget;

{
    IconInfo *pII;
    int place;
    int amt, i;
    Arg setArgs[3];
    Arg getArgs[4];
    Dimension clipWidth, clipHeight;

    place = GetNextIconPlace (&pIBD->IPD);
    if (place == NO_ICON_PLACE)
    {
	if (pIBD->IPD.iconPlacement & ICON_PLACE_LEFT_PRIMARY)
	{
	    amt = pIBD->IPD.placementCols;		/* add a new row */
	}
	else
	{
	    amt = pIBD->IPD.placementRows;		/* add a new column */
	}

	if (!ExtendIconList (pIBD, amt))
	{
	    Warning ("Insufficient memory to create icon box data");
	    return (NULL);
	}

	if (pIBD->IPD.iconPlacement & ICON_PLACE_LEFT_PRIMARY)
	{
	    pIBD->IPD.placementRows++;
	}
	else
	{
	    pIBD->IPD.placementCols++;
	}
	place = GetNextIconPlace (&pIBD->IPD);
    }
    
#ifdef VMS
    insertPosition2 = place;
#else
    insertPosition = place;
#endif
    /*
     * Update icon info values
     */

    pII = &pIBD->IPD.placeList[place];
    pII->theWidget = theWidget;

    pII->pCD = pCD;

    ICON_PLACE(pCD) = place;

    CvtIconPlaceToPosition (&pIBD->IPD, ICON_PLACE(pCD),
	    &ICON_X(pCD), &ICON_Y(pCD));


    /* update next free position */

    pIBD->currentCol = ICON_X(pCD) / pIBD->pCD_iconBox->widthInc;
    pIBD->currentRow = ICON_Y(pCD) / pIBD->pCD_iconBox->heightInc;


    /* 
     * Increase bboard size if necessary
     */
    i = 0;
    XtSetArg (getArgs[i], XmNwidth, (XtArgVal) &clipWidth ); i++;
    XtSetArg (getArgs[i], XmNheight, (XtArgVal) &clipHeight ); i++;
    XtGetValues (pIBD->clipWidget, getArgs, i);

    i = 0;
    if (pIBD->IPD.iconPlacement & ICON_PLACE_LEFT_PRIMARY)
    {
	if (pIBD->currentCol > pIBD->lastCol)
	{
	    pIBD->lastCol = pIBD->currentCol;
	}

	if (pIBD->currentRow > pIBD->lastRow)
	{
	    pIBD->lastRow = pIBD->currentRow;
	    if (clipHeight <= (pII->pCD->iconY + pIBD->pCD_iconBox->heightInc))
	    {
		/*
		 * Increase bulletin board height as needed.
		 */
		XtSetArg (setArgs[i], XmNheight, (XtArgVal) 
			  pII->pCD->iconY + pIBD->pCD_iconBox->heightInc); i++;
	    }
	}
    }
    else
    {
	if (pIBD->currentCol > pIBD->lastCol)
	{
	    pIBD->lastCol = pIBD->currentCol;
	    if (clipWidth <= (pII->pCD->iconX + pIBD->pCD_iconBox->widthInc))
	    {
		/*
		 * Increase bulletin board width as needed
		 */
		XtSetArg (setArgs[i], XmNwidth, 
		(XtArgVal) pII->pCD->iconX +
			   pIBD->pCD_iconBox->widthInc); i++;
	    }
	}

	if (pIBD->currentRow > pIBD->lastRow)
	{
	    pIBD->lastRow = pIBD->currentRow;
	}
    }

    if (i > 0)
    {
	XtSetValues (pIBD->bBoardWidget, setArgs, i);
    }

    return(pII);
    


} /* END OF FUNCTION InsertIconInfo */



/*************************************<->*************************************
 *
 *  DeleteIconFromBox
 *
 *
 *  Description:
 *  -----------
 *  XXDescription ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void DeleteIconFromBox (pIBD, pCD)
    IconBoxData *pIBD;
    ClientData *pCD;
#else /* _NO_PROTO */
void DeleteIconFromBox (IconBoxData *pIBD, ClientData *pCD)
#endif /* _NO_PROTO */
{
    Widget       theChild;
    ClientData  *pCD_tmp;
    Arg          args[4];
    Dimension    clipWidth, clipHeight;
    Dimension    oldWidth, oldHeight;
    int          newWidth, newHeight;
    int          i, newCols, newRows;

    i = 0;
    XtSetArg (args[i], XmNwidth, (XtArgVal) &oldWidth ); i++;
    XtSetArg (args[i], XmNheight, (XtArgVal) &oldHeight ); i++;
    XtGetValues (pIBD->bBoardWidget, args, i);

    i = 0;
    XtSetArg (args[i], XmNwidth, (XtArgVal) &clipWidth); i++;
    XtSetArg (args[i], XmNheight, (XtArgVal) &clipHeight ); i++;
    XtGetValues (pIBD->clipWidget, args, i);

    clipHeight /= pIBD->pCD_iconBox->heightInc;
    clipWidth  /= pIBD->pCD_iconBox->widthInc;

    /* 
     * find context of the activeIconTextWin to get pCD and then 
     * if it is the same as this client, hide it.
     */

    if (!(XFindContext (DISPLAY, pCD->pSD->activeIconTextWin,
			wmGD.windowContextType, (caddr_t *)&pCD_tmp)))
    {
	if (pCD == pCD_tmp)
	{
	    /* hide activeIconTextWin */
	    HideActiveIconText ((WmScreenData *)NULL);
	}
    }

    DeleteIconInfo (P_ICON_BOX(pCD), pCD);

    pCD->pIconBox->numberOfIcons--;

    theChild = XtWindowToWidget (DISPLAY, ICON_FRAME_WIN(pCD));
    XtUnmanageChild (theChild);

    XtDestroyWidget (theChild);

    /* update last row and col */

    SetNewBounds (pIBD);

    /* resize Bulletin board  (so scroll bars show correctly */
    i = 0;

    if (clipWidth <= pIBD->lastCol + 1)
    {
        newWidth = (pIBD->lastCol + 1) * pIBD->pCD_iconBox->widthInc;    
        XtSetArg (args[i], XmNwidth, (XtArgVal) newWidth); i++;
	newCols = newWidth / pIBD->pCD_iconBox->widthInc;
    }
    else
    {
	newWidth = clipWidth * pIBD->pCD_iconBox->widthInc;    
	XtSetArg (args[i], XmNwidth, (XtArgVal) newWidth); i++;
	newCols = newWidth / pIBD->pCD_iconBox->widthInc;
    }

    if (clipHeight <= pIBD->lastRow + 1)
    {
        /* set height of bboard */
        newHeight = (pIBD->lastRow + 1) * pIBD->pCD_iconBox->heightInc;
        XtSetArg (args[i], XmNheight, (XtArgVal) newHeight ); i++;
	newRows = newHeight / pIBD->pCD_iconBox->heightInc;
    }
    else
    {
	newHeight = clipHeight * pIBD->pCD_iconBox->heightInc;
	XtSetArg (args[i], XmNheight, (XtArgVal) newHeight ); i++;
	newRows = newHeight / pIBD->pCD_iconBox->heightInc;
    }

    if (i > 0  &&  ExpandVirtualSpace(pIBD, newWidth, newHeight))
    {
	XtSetValues (pIBD->bBoardWidget, args, i);
	RealignIconList (pIBD, newCols, newRows);
	pIBD->IPD.placementCols = newCols;
	pIBD->IPD.placementRows = newRows;
    }


    /* reset max size for icon box */
    ResetIconBoxMaxSize(pIBD->pCD_iconBox, pIBD->bBoardWidget);
    
    ResetArrowButtonIncrements (pIBD->pCD_iconBox);    

} /* END FUNCTION DeleteIconFromBox */



/*************************************<->*************************************
 *
 *  DeleteIconInfo (pIBD, pCD)
 *
 *
 *  Description:
 *  -----------
 *  Deletes an icon info record from the icon box list based on the 
 *  client data pointer.
 *
 *
 *  Inputs:
 *  ------
 *  pIBD	- pointer to icon box data
 *  pCD		- pointer to client data
 * 
 *  Outputs:
 *  -------
 *
 *
 *  Comments:
 *  --------
 *  o The deleted item is freed
 *  o Is pCD the correct key???? !!!
 * 
 *************************************<->***********************************/
#ifdef _NO_PROTO
void DeleteIconInfo (pIBD, pCD)

    IconBoxData *pIBD;
    ClientData *pCD;
#else /* _NO_PROTO */
void DeleteIconInfo (IconBoxData *pIBD, ClientData *pCD)
#endif /* _NO_PROTO */
{
    int ix, count;
    IconInfo *pII;

    /* find first matching entry in list */

    pII = &pIBD->IPD.placeList[0]; 
    count = pIBD->IPD.totalPlaces;

    for (ix = 0; ix < count && pII->pCD != pCD; ix++, pII++)
    {
    }

    if (ix < count)
    {
	/* found it, zero the entry out */
	pII->theWidget = NULL;
	pII->pCD = NULL;
    }


} /* END FUNCTION DeleteIconInfo */



/*************************************<->*************************************
 *
 *  ResetIconBoxMaxSize(pCD, bBoardWidget)
 *
 *
 *  Description:
 *  -----------
 *  XXDescription ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void ResetIconBoxMaxSize (pCD, bBoardWidget)
    ClientData *pCD;
    Widget bBoardWidget;
#else /* _NO_PROTO */
void ResetIconBoxMaxSize (ClientData *pCD, Widget bBoardWidget)
#endif /* _NO_PROTO */
{
    int i;
    Arg getArgs[3]; 
    Dimension newWidth;
    Dimension newHeight;

    i=0;
    XtSetArg (getArgs[i], XmNwidth, (XtArgVal) &newWidth ); i++;
    XtSetArg (getArgs[i], XmNheight, (XtArgVal) &newHeight ); i++;
    XtGetValues (bBoardWidget, getArgs, i);

    pCD->maxWidth = newWidth + pCD->baseWidth;

    pCD->maxHeight = newHeight + pCD->baseHeight;

    pCD->maxX = pCD->clientX;
    pCD->maxY = pCD->clientY;
    PlaceFrameOnScreen (pCD, &pCD->maxX, &pCD->maxY, 
			pCD->maxWidth, pCD->maxHeight);

} /* END OF FUNCTION 	ResetIconBoxMaxSize */



/*************************************<->*************************************
 *
 * CheckIconBoxSize(pIBD)
 *
 *
 *  Description:
 *  -----------
 *  XXDescription ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
Boolean CheckIconBoxSize (pIBD)
    IconBoxData *pIBD;
#else /* _NO_PROTO */
Boolean CheckIconBoxSize (IconBoxData *pIBD)
#endif /* _NO_PROTO */
{
    int i;
    Arg getArgs[3]; 
    Arg setArgs[3]; 
    Dimension oldWidth;
    Dimension oldHeight;
    Dimension newWidth;
    Dimension newHeight;
    int oldCol, oldRow;
    Boolean rval = True;

    i=0;
    XtSetArg (getArgs[i], XmNwidth, (XtArgVal) &oldWidth ); i++;
    XtSetArg (getArgs[i], XmNheight, (XtArgVal) &oldHeight ); i++;
    XtGetValues (pIBD->bBoardWidget, getArgs, i);

    newWidth = oldWidth;
    newHeight = oldHeight;
    oldCol = oldWidth / pIBD->pCD_iconBox->widthInc;
    oldRow = oldHeight / pIBD->pCD_iconBox->heightInc;

    /* 
     * Increase bboard size if necessary
     */

    i = 0;
    if (pIBD->IPD.iconPlacement & ICON_PLACE_LEFT_PRIMARY)
    {
	if (oldRow < pIBD->lastRow + 1)
	{
	    /*
	     * increase bulletin board height as needed
	     */
	    newHeight = (pIBD->lastRow * pIBD->pCD_iconBox->heightInc)
			 + pIBD->pCD_iconBox->heightInc;

	    XtSetArg (setArgs[i], XmNheight, (XtArgVal) newHeight); i++;
	}
    }
    else
    {
	if (oldCol  < pIBD->lastCol + 1)
	{
	    /*
	     * increase bulletin board width as needed
	     */
	    newWidth = (pIBD->lastCol * pIBD->pCD_iconBox->widthInc)
			    + pIBD->pCD_iconBox->widthInc;

	    XtSetArg (setArgs[i], XmNwidth, newWidth); i++;
	}
    }

    if (i > 0)
    {
	if (! ExpandVirtualSpace(pIBD, newWidth, newHeight))
	{
	    /*
	     * The user has resized the iconbox larger than
	     * memory will allow.  Don't honor the resize request
	     */
	    rval = False;
	    return(rval);
	}
	XtSetValues (pIBD->bBoardWidget, setArgs, i);
    }

    ResetIconBoxMaxSize(pIBD->pCD_iconBox, pIBD->bBoardWidget);
    

    return(rval);

} /* END OF FUNCTION CheckIconBoxSize */



/*************************************<->*************************************
 *
 * CheckIconBoxResize(pCD, changedValues)
 *
 *
 *  Description:
 *  -----------
 *  XXDescription ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void CheckIconBoxResize(pCD, changedValues, newWidth, newHeight)
    ClientData *pCD;
    unsigned int changedValues;
    int newWidth;
    int newHeight;
#else /* _NO_PROTO */
void CheckIconBoxResize (ClientData *pCD, unsigned int changedValues, int newWidth, int newHeight)
#endif /* _NO_PROTO */
{

    Boolean  packVert = False;
    Boolean  packHorz = False;
    WmScreenData *pSD;

    IconBoxData *pIBD;
    IconPlacementData *pIPD;
    int i, newCols, newRows;
    Arg getArgs[3]; 
    Arg setArgs[3]; 
    Dimension oldWidth;
    Dimension oldHeight;

    pIPD = &pCD->thisIconBox->IPD;
    pIBD = pCD->thisIconBox;

#ifdef DEC_MOTIF_BUG_FIX
    pSD = pCD->pSD;    
#else
    pSD = &(wmGD.Screens[SCREEN_FOR_CLIENT(pCD)]);
#endif

    
    i=0;
    XtSetArg (getArgs[i], XmNwidth, (XtArgVal) &oldWidth ); i++;
    XtSetArg (getArgs[i], XmNheight, (XtArgVal) &oldHeight ); i++;
    XtGetValues (pIBD->bBoardWidget, getArgs, i);
    
    newCols = pIPD->placementCols;
    newRows = pIPD->placementRows;
    newWidth = newWidth - pCD->baseWidth;	    	
    newHeight = newHeight - pCD->baseHeight;
    
    i = 0;
    
    if (changedValues & CWWidth) 
    {
	/*
	 * There was a change in Width, see if we need to change the
	 * bulletin board
	 */
	if (newWidth > oldWidth)
	{
	    newCols = newWidth / pCD->widthInc;
	    XtSetArg (setArgs[i], XmNwidth, (XtArgVal) newWidth ); i++;
	}
	
	if (newWidth < oldWidth)
	{

	    if ((!strcmp(pSD->iconBoxSBDisplayPolicy, "vertical")) &&
     		(newWidth / pCD->widthInc < pIBD->lastCol + 1))
	    {
		XtSetArg (setArgs[i], XmNwidth, (XtArgVal) newWidth ); i++;
		newCols = newWidth / pCD->widthInc;
		packVert = True;
	    }
	    else if (newWidth / pCD->widthInc < pIBD->lastCol + 1)
	    {
		newWidth = (pIBD->lastCol +1) * pCD->widthInc;
		XtSetArg (setArgs[i], XmNwidth, (XtArgVal) newWidth ); i++;
	    }
	    else
	    {
		newCols = newWidth / pCD->widthInc;
		XtSetArg (setArgs[i], XmNwidth, (XtArgVal) newWidth ); i++;
	    }
	}
    }
    else
    {
	newWidth = oldWidth;
    }
	
    if (changedValues & CWHeight) 
    {
	/*
	 * There was a change in Height, see if we need to change the
	 * bulletin board
	 */
	if (newHeight > oldHeight)
	{
	    newRows = newHeight / pCD->heightInc;
	    XtSetArg (setArgs[i], XmNheight, (XtArgVal) newHeight ); i++;
	}

	if (newHeight < oldHeight)
	{
	    if ((!strcmp(pSD->iconBoxSBDisplayPolicy, "horizontal")) &&
                (newHeight / pCD->heightInc < pIBD->lastRow + 1))
	    {
		XtSetArg (setArgs[i], XmNheight, (XtArgVal) newHeight ); i++;
		newRows = newHeight / pCD->heightInc;
		packHorz = True;		
	    }
	    else if (newHeight / pCD->heightInc < pIBD->lastRow + 1)
	    {
		newHeight = (pIBD->lastRow + 1) * pCD->heightInc;
		XtSetArg (setArgs[i], XmNheight, (XtArgVal) newHeight ); i++;
	    }
	    else
	    {
		newRows = newHeight / pCD->heightInc;
		XtSetArg (setArgs[i], XmNheight, (XtArgVal) newHeight ); i++;
	    }
	}
    }
    else
    {
	newHeight = oldHeight;
    }
    
    if ( i >0   &&   ExpandVirtualSpace(pIBD, newWidth, newHeight))
    {
	XtSetValues (pIBD->bBoardWidget, setArgs, i);
    }

    RealignIconList (pIBD, newCols, newRows);

    pIPD->placementCols = newCols;
    pIPD->placementRows = newRows;
    
    ResetIconBoxMaxSize(pCD, pIBD->bBoardWidget);
    
    /*
     * Pack the icon box if there are icons that can no longer
     * be scrolled to due to iconBoxSBDisplayPolicy.
     */
    if (packVert)
    {
	PackIconBox (pIBD, packVert, False , newWidth, 0);
    }
    else if (packHorz)
    {
	PackIconBox (pIBD, False, packHorz , 0, newHeight);
    }


} /* END OF FUNCTION CheckIconBoxResize */



/*************************************<->*************************************
 *
 *  ExpandVirtualSpace (pIBD, newWidth, newHeight)
 *
 *
 *  Description:
 *  -----------
 *  Add virtural space (really the icon list )
 *
 *
 *  Inputs:
 *  ------
 *  pIBD	- ptr to icon box data
 *
 * 
 *  Outputs:
 *  -------
 *  Return	- True if successful, False otherwise
 *
 *  Comments:
 *  --------
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
Boolean ExpandVirtualSpace (pIBD, newWidth, newHeight)
    IconBoxData *pIBD;
    int newWidth;
    int newHeight;
#else /* _NO_PROTO */
Boolean ExpandVirtualSpace (IconBoxData *pIBD, int newWidth, int newHeight)
#endif /* _NO_PROTO */
{
    Boolean rval = True;
    int newSize;
    int increment;

    newSize = (newWidth / pIBD->pCD_iconBox->widthInc) *
		(newHeight / pIBD->pCD_iconBox->heightInc);

    if (newSize > pIBD->IPD.totalPlaces )
    {
	increment = newSize - pIBD->IPD.totalPlaces;
	rval = ExtendIconList (pIBD, increment);
    }

    return (rval);

} /* END OF FUNCTION ExpandVirtualSpace */



/*************************************<->*************************************
 *
 *  ExtendIconList (pIBD, incr);
 *
 *
 *  Description:
 *  -----------
 *  Add space to the icon list
 *
 *
 *  Inputs:
 *  ------
 *  pIBD	- ptr to icon box data
 *  incr	- number of cells to add
 *
 * 
 *  Outputs:
 *  -------
 *  Return	- True if successful, False otherwise
 *
 *  Comments:
 *  --------
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
Boolean ExtendIconList (pIBD, incr)
    IconBoxData *pIBD;
    int incr;
#else /* _NO_PROTO */
Boolean ExtendIconList (IconBoxData *pIBD, int incr)
#endif /* _NO_PROTO */
{
    Boolean rval;
    int newSize;
    IconInfo *pTmp;

    newSize = pIBD->IPD.totalPlaces + incr;

    if ((pTmp = (IconInfo *) XtMalloc (newSize*sizeof(IconInfo))) != NULL)
    {
	/* copy data */
	memcpy (pTmp, pIBD->IPD.placeList, 
	    pIBD->IPD.totalPlaces*sizeof(IconInfo));
	memset (&pTmp[pIBD->IPD.totalPlaces], NULL, incr*sizeof(IconInfo));

	/* out with the old, in with the new */
	XtFree ((char *)pIBD->IPD.placeList);
	pIBD->IPD.placeList = pTmp;
	pIBD->IPD.totalPlaces = newSize;
	rval = True;
    }
    else
    {
	rval = False;
    }

    return (rval);
} /* END OF FUNCTION ExtendIconList */



/*************************************<->*************************************
 *
 *  PackIconBox(pIBD, packVert, packHorz, passedInWidth, passedInHeight)
 *
 *
 *  Description:
 *  -----------
 *  Packs the icons in the icon box
 *
 *
 *  Inputs:
 *  ------
 *  pIBD	- pointer to icon box data
 *
 * 
 *  Outputs:
 *  -------
 *
 *
 *  Comments:
 *  --------
 * 
 *************************************<->***********************************/
#ifdef _NO_PROTO
void PackIconBox (pIBD, packVert, packHorz, passedInWidth, passedInHeight)

    IconBoxData *pIBD;
    Boolean     packVert;
    Boolean     packHorz;
    int         passedInWidth;
    int         passedInHeight;

#else /* _NO_PROTO */
void PackIconBox (IconBoxData *pIBD, Boolean packVert, Boolean packHorz, int passedInWidth, int passedInHeight)
#endif /* _NO_PROTO */
{
    IconInfo *pII_2, *pII_1;
    int ix1, ix2;
    int count;
    int newX, newY;
    ClientData *pCD_tmp, *pMyCD;
    int hasActiveText = 1;
    Arg args[4];
    Dimension majorDimension, minorDimension;
    Dimension oldWidth, oldHeight;
    int newWidth, newHeight;
    int i;
    Boolean rippling = False;

    i = 0;
    XtSetArg (args[i], XmNwidth, (XtArgVal) &oldWidth ); i++;
    XtSetArg (args[i], XmNheight, (XtArgVal) &oldHeight ); i++;
    XtGetValues (pIBD->bBoardWidget, args, i);

    /*
     * packing to visual space, first update IconBoxData
     */

    i = 0;

    if (pIBD->IPD.iconPlacement & ICON_PLACE_LEFT_PRIMARY)
    {
	XtSetArg (args[i], XmNwidth, (XtArgVal) &majorDimension ); i++;
	XtSetArg (args[i], XmNheight, (XtArgVal) &minorDimension ); i++;
	XtGetValues (pIBD->clipWidget, args, i);
	if (packVert)
	{
	    majorDimension = passedInWidth;
	}
	
	minorDimension /= pIBD->pCD_iconBox->heightInc;
	majorDimension /= pIBD->pCD_iconBox->widthInc;
	if (majorDimension != pIBD->IPD.placementCols)
	{
	    pIBD->IPD.placementCols = majorDimension;
	}
    }
    else
    {
	XtSetArg (args[i], XmNheight, (XtArgVal) &majorDimension ); i++;
	XtSetArg (args[i], XmNwidth, (XtArgVal) &minorDimension ); i++;
	XtGetValues (pIBD->clipWidget, args, i);
	if (packHorz)
	{
	    majorDimension = passedInHeight;
	}

	minorDimension /= pIBD->pCD_iconBox->widthInc;
	majorDimension /= pIBD->pCD_iconBox->heightInc;
	if (majorDimension != pIBD->IPD.placementRows)
	{
	    pIBD->IPD.placementRows = majorDimension;
	}
    }

    /* 
     * find context of the activeIconTextWin to get pCD and then 
     * if it is the same as this client, hide it.
     */

    pMyCD = pIBD->pCD_iconBox;
    if (ICON_DECORATION(pMyCD) & ICON_ACTIVE_LABEL_PART)
    {
	if (XFindContext (DISPLAY, pMyCD->pSD->activeIconTextWin,
			wmGD.windowContextType, (caddr_t *)&pCD_tmp))
	{
	    hasActiveText = 0;
	}
    }

    pII_2 = pII_1 = pIBD->IPD.placeList;
    ix1 = ix2 = 0;
    count = pIBD->IPD.totalPlaces;

    while (ix1 < count)
    {
	if (!rippling && (pII_2->pCD != NULL))
	{
	    /* 
	     * We need to start rippling the icons into new positions if
	     * their (x,y) position changed 
	     */
	    CvtIconPlaceToPosition (&pIBD->IPD, pII_2->pCD->iconPlace,
		&newX, &newY);

	    rippling = ((newX != pII_2->pCD->iconX) ||
		        (newY != pII_2->pCD->iconY));
	}

	if ((pII_2->pCD == NULL) || rippling)
	{
	    /* find next one to move */
	    while ((ix1 < count) && (pII_1->pCD == NULL))
	    {
		ix1++;
		pII_1++;
	    }

	    if ((ix1 < count) && (pII_1->pCD != NULL))
	    {
		if (ix1 != ix2)
		{
		    MoveIconInfo (&pIBD->IPD, ix1, ix2);
		}

		CvtIconPlaceToPosition (&pIBD->IPD, ix2, &newX, &newY);

		pII_2->pCD->iconX = newX;
	 	pII_2->pCD->iconY = newY;

		if (hasActiveText && (pII_2->pCD == pCD_tmp))
		{
		    /* hide activeIconTextWin first */
		    HideActiveIconText ((WmScreenData *)NULL);
		    XtMoveWidget (pII_2->theWidget, newX, newY);
		    ShowActiveIconText (pII_2->pCD);
		}
		else
		{
		    XtMoveWidget (pII_2->theWidget, newX, newY);
		}
	    }
	}

	if (ix1 < count)
	{
	    ix2++;
	    pII_2++;
	}

	ix1++;
	pII_1++;
    }

    /* update last row and col */

    SetNewBounds (pIBD);

    /* resize Bulletin board  (so scroll bars show correctly */
    i = 0;
    if (pIBD->IPD.iconPlacement & ICON_PLACE_LEFT_PRIMARY)
    {
	if (majorDimension <= pIBD->lastCol + 1)
	{
	    newWidth = (pIBD->lastCol + 1) * pIBD->pCD_iconBox->widthInc;    
	    XtSetArg (args[i], XmNwidth, (XtArgVal) newWidth); i++;
	}
	else
	{
	    newWidth = oldWidth;
	}

	if (minorDimension <= pIBD->lastRow + 1)
	{
	    /* set height of bboard */
	    newHeight = (pIBD->lastRow + 1) * pIBD->pCD_iconBox->heightInc;
	    XtSetArg (args[i], XmNheight, (XtArgVal) newHeight ); i++;
	}
	else
	{
	    newHeight = minorDimension * pIBD->pCD_iconBox->heightInc;
	    XtSetArg (args[i], XmNheight, (XtArgVal) newHeight ); i++;
	}
    }
    else
    {
	if (majorDimension <= pIBD->lastRow + 1)
	{
	    newHeight = (pIBD->lastRow + 1) * pIBD->pCD_iconBox->heightInc;
	    XtSetArg (args[i], XmNheight, (XtArgVal) newHeight ); i++;
	}
	else
	{
	    newHeight = oldHeight;
	}

	if (minorDimension <= pIBD->lastCol + 1)
	{
	    /* set width of bboard */
	    newWidth = (pIBD->lastCol + 1) * pIBD->pCD_iconBox->widthInc;    
	    XtSetArg (args[i], XmNwidth, (XtArgVal) newWidth); i++;
	}
	else
	{
	    newWidth = minorDimension * pIBD->pCD_iconBox->widthInc;    
	    XtSetArg (args[i], XmNwidth, (XtArgVal) newWidth); i++;
	}
    }

    if (i > 0  &&  ExpandVirtualSpace(pIBD, newWidth, newHeight))
    {
	XtSetValues (pIBD->bBoardWidget, args, i);
    }


    /* reset max size for icon box */

    ResetIconBoxMaxSize (pIBD->pCD_iconBox, pIBD->bBoardWidget);
    
    ResetArrowButtonIncrements (pIBD->pCD_iconBox);    

} /* END FUNCTION PackIconBox */


/*************************************<->*************************************
 *
 *  RealignIconList (pIBD, newRows, newCols)
 *
 *
 *  Description:
 *  -----------
 *  Realigns the icon list according to the new virtual space dimensions 
 *
 *
 *  Inputs:
 *  ------
 *  pIBD	- ptr to icon box data
 *  newRows	- new number of rows
 *  newCols	- new number of columns
 *
 * 
 *  Outputs:
 *  -------
 *
 *  Comments:
 *  --------
 *  o The placement data structure contains the old values.
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void RealignIconList (pIBD, newCols, newRows)

    IconBoxData *pIBD;
    int newRows, newCols;
#else /* _NO_PROTO */
void RealignIconList (IconBoxData *pIBD, int newCols, int newRows)
#endif /* _NO_PROTO */
{
    int c1, c2, ix1, ix2;
    int oldRows, oldCols;
    IconPlacementData  ipdNew;
    IconInfo *pII;

    /* 
     * create new icon placement data for ease of calling conversion 
     * routines.
     */
    ipdNew.onRootWindow = pIBD->IPD.onRootWindow;
    ipdNew.iconPlacement = pIBD->IPD.iconPlacement;
    ipdNew.placementRows = newRows;
    ipdNew.placementCols = newCols;
    ipdNew.iPlaceW = pIBD->IPD.iPlaceW;
    ipdNew.iPlaceH = pIBD->IPD.iPlaceH;
    ipdNew.placeList = pIBD->IPD.placeList;
    ipdNew.totalPlaces = pIBD->IPD.totalPlaces;

    oldRows = pIBD->IPD.placementRows;
    oldCols = pIBD->IPD.placementCols;

    /*
     * Use the new organization and placement discipline to
     * determine how to move the icon info data around.
     */
    if (((oldRows < newRows) && 
	 (pIBD->IPD.iconPlacement & ICON_PLACE_TOP_PRIMARY)) ||
	((oldCols < newCols) && 
	 (pIBD->IPD.iconPlacement & ICON_PLACE_LEFT_PRIMARY)))
    { 
    /* 
     * work backwards 
     */
	for (ix1 = pIBD->IPD.totalPlaces - 1, 
		 pII = &pIBD->IPD.placeList[ix1]; ix1 >= 0; ix1--, pII--)
	{
	    if (pII->pCD != NULL)
	    {
		CvtIconPlaceToPosition (&pIBD->IPD, ix1, &c1, &c2);
		ix2 = CvtIconPositionToPlace (&ipdNew, c1, c2);
		if (ix1 != ix2)
		{ 
		    MoveIconInfo (&pIBD->IPD, ix1, ix2);
		} 
	    }
	}
    }
    else 
    if (((oldRows > newRows) && 
	 (pIBD->IPD.iconPlacement & ICON_PLACE_TOP_PRIMARY)) ||
	((oldCols > newCols) && 
	 (pIBD->IPD.iconPlacement & ICON_PLACE_LEFT_PRIMARY)))
    {
	/* 
	 * work forwards 
	 */
	for (ix1 = 0, pII = &pIBD->IPD.placeList[ix1]; 
		ix1 < pIBD->IPD.totalPlaces; ix1++, pII++)
	{
	    if (pII->pCD != NULL)
	    {
		CvtIconPlaceToPosition (&pIBD->IPD, ix1, &c1, &c2);
		ix2 = CvtIconPositionToPlace (&ipdNew, c1, c2);
		if (ix1 != ix2)
		{
		    MoveIconInfo (&pIBD->IPD, ix1, ix2);
		}
	    }
	}
    }

    /* 
     * update info in placement structure to reflect new reality
     */
    pIBD->IPD.placementRows = newRows;
    pIBD->IPD.placementCols = newCols;

} /* END OF FUNCTION RealignIconList */




/*************************************<->*************************************
 *
 *  SetNewBounds (pIBD)
 *
 *
 *  Description:
 *  -----------
 *
 *
 *  Inputs:
 *  ------
 *
 * 
 *  Outputs:
 *  -------
 *
 *  Comments:
 *  --------
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void SetNewBounds (pIBD)

    IconBoxData *pIBD;

#else /* _NO_PROTO */
void SetNewBounds (IconBoxData *pIBD)
#endif /* _NO_PROTO */
{

    int i;
    int X = 0;
    int Y = 0; 
    CompositeWidget cw;
    WidgetList      children;

    cw = (CompositeWidget) pIBD->bBoardWidget;
    children = cw->composite.children;

    for (i = 0; i < cw->composite.num_children; i++)
    {
        if (children[i]->core.x > X)
        {
	    X = children[i]->core.x;
        }
        if (children[i]->core.y > Y)
        {
	    Y = children[i]->core.y;
        }
    }

    pIBD->lastCol = X / pIBD->pCD_iconBox->widthInc;
    pIBD->lastRow = Y / pIBD->pCD_iconBox->heightInc;

} /* END OF FUNCTION SetNewBounds */



/*************************************<->*************************************
 *
 *  InsertPosition (w)
 *
 *
 *  Description:
 *  -----------
 *  This procedure is passed to the bulletin board at create time
 *  to be used when a child is inserted into the bulletin board
 *  
 *
 *************************************<->***********************************/
#ifdef _NO_PROTO
Cardinal InsertPosition (w)
    Widget w;

#else /* _NO_PROTO */
Cardinal InsertPosition (Widget w)
#endif /* _NO_PROTO */
{
#ifdef VMS
    return (insertPosition2);
#else
    return (insertPosition);
#endif

} /* END OF FUNCTION InsertPosition */



/*************************************<->*************************************
 *
 *  ShowClientIconState ();
 *
 *
 *  Description:
 *  -----------
 *  XXDescription ...
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO
void ShowClientIconState (pCD, newState)
   ClientData *pCD;
   int newState;
#else /* _NO_PROTO */
void ShowClientIconState (ClientData *pCD, int newState)
#endif /* _NO_PROTO */
{

    /*
     * Changing the appearance of an icon window in the box to 
     * reflect the client's state
     */

    if ((newState == MINIMIZED_STATE) && (pCD->iconWindow))
	XMapRaised (DISPLAY, pCD->iconWindow);

    if (((newState == NORMAL_STATE) || (newState == MAXIMIZED_STATE )) 
						&& (pCD->iconWindow))
    {
	XUnmapWindow (DISPLAY, pCD->iconWindow);
    }

} /* END FUNCTION ShowClientIconState */



#ifndef MOTIF_ONE_DOT_ONE
/*************************************<->*************************************
 *
 *  IconScrollVisibleCallback
 *
 *
 *  Description:
 *  -----------
 *  for each icon in the icon box
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void IconScrollVisibleCallback(w, client_data, call_data)
    Widget w; 
    caddr_t client_data; 
    XmAnyCallbackStruct *call_data;

#else /* _NO_PROTO */
void IconScrollVisibleCallback (Widget w, caddr_t client_data, XmAnyCallbackStruct *call_data)
#endif /* _NO_PROTO */
{
    XmTraverseObscuredCallbackStruct *vis_data;

    vis_data = (XmTraverseObscuredCallbackStruct *) call_data;

    XmScrollVisible(ACTIVE_WS->pIconBox->scrolledWidget,
		    vis_data->traversal_destination,
		    0,0);
/*
		    IB_MARGIN_WIDTH, IB_MARGIN_HEIGHT);
*/
} /* END OF FUNCTION IconScrollVisibleCallback */

#endif


/*************************************<->*************************************
 *
 *  IconActivateCallback
 *
 *
 *  Description:
 *  -----------
 *  for each icon in the icon box
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void IconActivateCallback(w, client_data, call_data)
    Widget w; 
    caddr_t client_data; 
    XmAnyCallbackStruct *call_data;

#else /* _NO_PROTO */
void IconActivateCallback (Widget w, caddr_t client_data, XmAnyCallbackStruct *call_data)
#endif /* _NO_PROTO */
{
    ClientData    	*pCD;
    Window		theIcon;

    theIcon = XtWindow(w);

    /* 
     * find context to get pCD and then carry out
     * default action.
     */

    if (!(XFindContext (DISPLAY, theIcon,
			wmGD.windowContextType, (caddr_t *)&pCD)))
    {
	F_Restore_And_Raise ((String)NULL, pCD, (XEvent *)NULL );
/*	F_Normalize_And_Raise ((String)NULL, pCD, (XEvent *)NULL );
*/    }

} /* END OF FUNCTION IconActivateCallback */



/*************************************<->*************************************
 *
 *  UpdateIncrements
 *
 *
 *  Description:
 *  -----------
 *  XXDescription ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void UpdateIncrements (sWidget, pIBD, event)
    Widget sWidget;
    IconBoxData *pIBD;
    XConfigureEvent *event;

#else /* _NO_PROTO */
void UpdateIncrements (Widget sWidget, IconBoxData *pIBD, XConfigureEvent *event)
#endif /* _NO_PROTO */
{
    ResetArrowButtonIncrements (pIBD->pCD_iconBox);    
  
} /* END OF FUNCTION UpdateIncrements */


/*************************************<->*************************************
 *
 *  ResetArrowButtonIncrements(pCD)
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO
void ResetArrowButtonIncrements(pCD)
    ClientData *pCD;
#else /* _NO_PROTO */
void ResetArrowButtonIncrements (ClientData *pCD)
#endif /* _NO_PROTO */
{
    int i;
    Arg setArgs[2]; 
        
    i=0;
    XtSetArg (setArgs[i], XmNincrement, (XtArgVal) pCD->heightInc); i++;    
    XtSetValues (pCD->thisIconBox->vScrollBar, (ArgList) setArgs, i); 

    i=0;
    XtSetArg (setArgs[i], XmNincrement, (XtArgVal) pCD->widthInc); i++;    
    XtSetValues (pCD->thisIconBox->hScrollBar, (ArgList) setArgs, i);     
    
} /* END OF FUNCTION ResetArrowButtonIncrements */    



/*************************************<->*************************************
 *
 *  ChangeActiveIconboxIconText
 *
 *
 *  Description:
 *  -----------
 *  XXDescription ...
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void ChangeActiveIconboxIconText (icon, dummy, event)
    Widget icon;
    caddr_t dummy;
    XFocusChangeEvent *event;

#else /* _NO_PROTO */
void ChangeActiveIconboxIconText (Widget icon, caddr_t dummy, XFocusChangeEvent *event)
#endif /* _NO_PROTO */
{

    ClientData    	*pCD;
    Window		theIcon;

    /* 
     * find context to get pCD and then hide or show active icon text. 
     * Show/hide the active icon text only if the icon box is not 
     * iconified.
     */

    theIcon = XtWindow(icon);

    if (!(XFindContext (DISPLAY, theIcon,
			wmGD.windowContextType, (caddr_t *)&pCD)) &&
	P_ICON_BOX(pCD) &&
	P_ICON_BOX(pCD)->pCD_iconBox &&
	P_ICON_BOX(pCD)->pCD_iconBox->clientState !=  MINIMIZED_STATE)
    {
	if (event->type == FocusIn) 
	{
	    if (event->send_event)
	    {
	       ShowActiveIconText (pCD);
	    }
	}
	else
	{
	    if (event->send_event)
	    {
	        HideActiveIconText (pCD->pSD);
	    }
	}
    }

} /* END OF FUNCTION ChangeActiveIconboxIconText */



/*************************************<->*************************************
 *
 *  HandleIconBoxIconKeyPress
 *
 *
 *  Description:
 *  -----------
 *  This event handler catches keyevents for icons in the icon box and
 *  passes them on to the standard key handling routine for mwm.
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO
void HandleIconBoxIconKeyPress (icon, dummy, keyEvent)
    Widget icon;
    caddr_t dummy;
    XKeyEvent *keyEvent;
    
#else /* _NO_PROTO */
void HandleIconBoxIconKeyPress (Widget icon, caddr_t dummy, XKeyEvent *keyEvent)
#endif /* _NO_PROTO */
{
    
    Context context; 
    ClientData          *pCD; 
    Window              theIcon;

    /*
     * find context to get pCD and then post menu show active icon text.
     */
    
    theIcon = XtWindow(icon);
    if (!(XFindContext (DISPLAY, theIcon,
			wmGD.windowContextType, (caddr_t *)&pCD)))
    {
	keyEvent->window = ICON_FRAME_WIN(pCD);

	if (pCD->clientState == MINIMIZED_STATE)
	{
	    context = F_SUBCONTEXT_IB_IICON;
	    pCD->grabContext = F_SUBCONTEXT_IB_IICON;
	}
	else
	{
	    context = F_SUBCONTEXT_IB_WICON;
	    pCD->grabContext = F_SUBCONTEXT_IB_WICON;
	}
	
	if(!(HandleKeyPress (keyEvent, ACTIVE_PSD->keySpecs, 
			True, context, False, pCD)))
	{
	    keyEvent->window = 0;
	    keyEvent->type = 0;
	}

    }
	
} /* END OF FUNCTION HandleIconBoxIconKeyPress */


/*************************************<->*************************************
 *
 *  HandleIconBoxButtonMotion
 *
 *
 *  Description:
 *  -----------
 *  Event handler for button motion events on icon frame window in 
 *  in icon box.
 *
 *
 *  Inputs:
 *  ------
 *  icon		- widget for icon frame 
 *  client_data		- extra client data
 *  pev			- ptr to event
 *
 * 
 *  Outputs:
 *  -------
 *
 *
 *  Comments:
 *  --------
 *  o This is added to make sure that ButtonXMotion gets added to the 
 *    event mask for icons in the icon box.
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void HandleIconBoxButtonMotion (icon, client_data, pev)
    Widget icon;
    caddr_t client_data;
    XEvent *pev;

#else /* _NO_PROTO */
void HandleIconBoxButtonMotion (Widget icon, caddr_t client_data, XEvent *pev)
#endif /* _NO_PROTO */
{

} /* END OF FUNCTION HandleIconBoxButtonMotion */



/*************************************<->*************************************
 *
 *  GetIconBoxIconRootXY (pCD, pX, pY)
 *
 *
 *  Description:
 *  -----------
 *
 *
 *  Inputs:
 *  ------
 *  pCD         - pointer to client data
 *  pX          - pointer to X return value
 *  pY          - pointer to Y return value
 *
 *  Outputs:
 *  -------
 *
 *
 *  Comments:
 *  --------
 *  o returns root-window coords
 *
 *************************************<->***********************************/
#ifdef _NO_PROTO
void GetIconBoxIconRootXY (pCD, pX, pY)

    ClientData *pCD;
    int *pX, *pY;
#else /* _NO_PROTO */
void GetIconBoxIconRootXY (ClientData *pCD, int *pX, int *pY)
#endif /* _NO_PROTO */
{

    Window child;

    if (pCD->pSD->useIconBox && P_ICON_BOX(pCD))
    {
        XTranslateCoordinates(DISPLAY,
                              XtWindow(P_ICON_BOX(pCD)->bBoardWidget),
                              ROOT_FOR_CLIENT(pCD),
                              ICON_X(pCD) + IB_MARGIN_WIDTH,
                              ICON_Y(pCD) + IB_MARGIN_HEIGHT,
                              pX, pY, &child);

    }
    else
    {
        *pX = *pY = 0;
    }
} /* END FUNCTION GetIconBoxIconRootXY */


/*************************************<->*************************************
 *
 *  IconVisible (pCD)
 *
 *
 *  Description:
 *  -----------
 *
 *  Inputs:
 *  ------
 *  pCD         - pointer to client data
 *
 *  Outputs:
 *  -------
 *
 *
 *  Comments:
 *  --------
 *
 *************************************<->***********************************/
#ifdef _NO_PROTO
Boolean IconVisible (pCD)
    ClientData *pCD;
#else /* _NO_PROTO */
Boolean IconVisible (ClientData *pCD)
#endif /*  _NO_PROTO */
{

    /*
     * May use icon->core.visible field if that gets fixed and
     * we want to accept the Intrinsics idea of what is visible.
     */

    Boolean rval = True;


    
    int i;
    Arg getArgs[5];

    Dimension tmpWidth = 0;
    Dimension tmpHeight = 0;
    Position clipX = 0;
    Position clipY = 0;
    Position tmpX = 0;
    Position tmpY = 0;
    int iconX, iconY;

    i=0;
    XtSetArg (getArgs[i], XmNwidth, (XtArgVal) &tmpWidth ); i++;
    XtSetArg (getArgs[i], XmNheight, (XtArgVal) &tmpHeight ); i++;
    XtSetArg (getArgs[i], XmNx, (XtArgVal) &tmpX ); i++;
    XtSetArg (getArgs[i], XmNy, (XtArgVal) &tmpY ); i++;
    XtGetValues (P_ICON_BOX(pCD)->clipWidget, getArgs, i);
    XtTranslateCoords(P_ICON_BOX(pCD)->scrolledWidget,
                        tmpX, tmpY,
                        &clipX, &clipY);

    GetIconBoxIconRootXY(pCD, &iconX, &iconY);


    /* 
     * demand at least 2 pixels of the 
     * real icon (not drawnButton) be showing 
     */
       
    if (iconX + 2 > ((int)clipX + (int)tmpWidth))
    {
	return(False);
    }
    if (iconY + 2 > ((int)clipY + (int)tmpHeight))
    {
	return(False);
    }

    if ((iconX + (ICON_WIDTH(pCD) -2)) < (int)clipX)
    {
	return(False);
    }
    if ((iconY + (ICON_HEIGHT(pCD) -2)) < (int)clipY)
    {
	return(False);
    }

    return(rval);

} /* END OF FUNCTION IconVisible */

/*************************************<->*************************************
 *
 *  WmXmStringToString (xmString
 *
 *
 *  Description:
 *  -----------
 *
 *
 *  Inputs:
 *  ------
 *
 *  Outputs:
 *  -------
 *
 *
 *  Comments:
 *  --------
 *  Return the ascii part of the first segment of an XmString 
 *  If xmString is NULL, then do nothing 
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO
String  WmXmStringToString (xmString) 
    XmString xmString; 
#else /* _NO_PROTO */
String WmXmStringToString (XmString xmString)
#endif /* _NO_PROTO */
{ 
    XmStringContext       xmStrContext;
    char                 *asciiString = NULL; 
    XmStringCharSet      ibTitleCharset; 
    XmStringDirection    ibTitleDirection; 
    Boolean              separator; 
    
    if (xmString)
    {
	XmStringInitContext (&xmStrContext, xmString);
    
	XmStringGetNextSegment (xmStrContext, &asciiString, 
				&ibTitleCharset, &ibTitleDirection, 
				&separator);

	if (ibTitleCharset != NULL) 
	{
	    XtFree ((char *)ibTitleCharset);
	}

	XmStringFreeContext (xmStrContext);
    }
    
    return(asciiString);
    
} /* END OF FUNCTION WmXmStringToString */
