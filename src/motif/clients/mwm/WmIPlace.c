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
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.1
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile: WmIPlace.c,v $ $Revision: 1.1.4.3 $ $Date: 1993/07/16 21:37:40 $"
#endif
#endif
/*
 * (c) Copyright 1987, 1988, 1989, 1990 HEWLETT-PACKARD COMPANY */

/*
 * Included Files:
 */

#include "WmGlobal.h"

/*
 * include extern functions
 */

#include "WmError.h"
#include "WmIDecor.h"
#include "WmIconBox.h"
#include "WmWinConf.h"


/*
 * Function Declarations:
 */

#ifdef _NO_PROTO
void 	CvtIconPlaceToPosition ();
int 	CvtIconPositionToPlace ();
int 	FindIconPlace ();
int 	GetNextIconPlace ();
void 	InitIconPlacement ();
void 	MoveIconInfo ();
void	PackRootIcons ();
#else /* _NO_PROTO */
void InitIconPlacement (WmWorkspaceData *pWS);
int GetNextIconPlace (IconPlacementData *pIPD);
void CvtIconPlaceToPosition (IconPlacementData *pIPD, int place, int *pX, int *pY);
int FindIconPlace (ClientData *pCD, IconPlacementData *pIPD, int x, int y);
int CvtIconPositionToPlace (IconPlacementData *pIPD, int x, int y);
void PackRootIcons (void);
void MoveIconInfo (IconPlacementData *pIPD, int p1, int p2);
#endif /* _NO_PROTO */



/*
 * Global Variables:
 */
extern Dimension clipWidth;
extern Dimension clipHeight;
extern Position clipX;
extern Position clipY;

/*************************************<->*************************************
 *
 *  InitIconPlacement ()
 *
 *
 *  Description:
 *  -----------
 *  This function intializes icon placement information.
 *
 *
 *  Inputs:
 *  ------
 *  pWS = pointer to workspace data
 *
 * 
 *  Outputs:
 *  -------
 *  IconPlacmementData
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO
void InitIconPlacement (pWS)

    WmWorkspaceData *pWS;

#else /* _NO_PROTO */
void InitIconPlacement (WmWorkspaceData *pWS)
#endif /* _NO_PROTO */
{
    Boolean useMargin;
    int sW;
    int sH;
    int iSpaceX;
    int iSpaceY;
    int placementW;
    int placementH;
    int extraXSpace;
    int extraYSpace;
    int xMargin;
    int yMargin;
    int extraPX;
    int extraPY;
    int i;


    xMargin = yMargin = extraPX = extraPY = 0;

    sW = DisplayWidth (DISPLAY, pWS->pSD->screen);
    sH = DisplayHeight (DISPLAY, pWS->pSD->screen);
    useMargin = (pWS->pSD->iconPlacementMargin >= 0);
    pWS->IPData.iconPlacement = pWS->pSD->iconPlacement;

    if (useMargin)
    {
	pWS->IPData.placementCols =
	    (sW - (2 * pWS->pSD->iconPlacementMargin)) / pWS->pSD->iconWidth;
	pWS->IPData.placementRows =
	    (sH - (2 * pWS->pSD->iconPlacementMargin)) / pWS->pSD->iconHeight;
    }
    else
    {
	pWS->IPData.placementCols = sW / pWS->pSD->iconWidth;
	pWS->IPData.placementRows = sH / pWS->pSD->iconHeight;
    }

    if (pWS->IPData.iconPlacement & ICON_PLACE_TIGHT)
    {
	iSpaceX = 0;
	iSpaceY = 0;
	xMargin = 2;
	yMargin = 2;
    }
    else
    {
	do
	{
	    if (useMargin)
	    {
	        iSpaceX = 
		    (sW - (2 * pWS->pSD->iconPlacementMargin) -
			  (pWS->IPData.placementCols * pWS->pSD->iconWidth)) /
			      (pWS->IPData.placementCols - 1);
	    }
	    else
	    {
	        iSpaceX = 
		    (sW - (pWS->IPData.placementCols * pWS->pSD->iconWidth)) /
			       pWS->IPData.placementCols;
	    }
	    if (iSpaceX < MINIMUM_ICON_SPACING)
	    {
	        pWS->IPData.placementCols--;
	    }
	}
	while (iSpaceX < MINIMUM_ICON_SPACING);

	do
	{
	    if (useMargin)
	    {
	        iSpaceY = (sH - (2 * pWS->pSD->iconPlacementMargin) -
		       (pWS->IPData.placementRows * pWS->pSD->iconHeight)) /
				  (pWS->IPData.placementRows - 1);
	    }
	    else
	    {
	        iSpaceY = 
		    (sH - (pWS->IPData.placementRows * pWS->pSD->iconHeight)) /
					    pWS->IPData.placementRows;
	    }
	    if (iSpaceY < MINIMUM_ICON_SPACING)
	    {
	        pWS->IPData.placementRows--;
	    }
	}
	while (iSpaceY < MINIMUM_ICON_SPACING);
    }

    pWS->IPData.iPlaceW = pWS->pSD->iconWidth + iSpaceX;
    pWS->IPData.iPlaceH = pWS->pSD->iconHeight + iSpaceY;

    placementW = pWS->IPData.placementCols * pWS->IPData.iPlaceW;
    placementH = pWS->IPData.placementRows * pWS->IPData.iPlaceH;

    pWS->IPData.placeIconX = 
	((pWS->IPData.iPlaceW - pWS->pSD->iconWidth) + 1) / 2;
    pWS->IPData.placeIconY = 
        ((pWS->IPData.iPlaceH - pWS->pSD->iconHeight) + 1) / 2;

    /*
     * Special case margin handling for TIGHT icon placement
     */
    if (pWS->IPData.iconPlacement & ICON_PLACE_TIGHT)
    {
	if (useMargin)
	{
	    xMargin = pWS->pSD->iconPlacementMargin;
	    yMargin = pWS->pSD->iconPlacementMargin;
	}

	extraXSpace = 0;
	extraYSpace = 0;

	if ((pWS->IPData.iconPlacement & ICON_PLACE_RIGHT_PRIMARY) ||
	   (pWS->IPData.iconPlacement & ICON_PLACE_RIGHT_SECONDARY))
	    xMargin = sW - placementW - xMargin;

	if ((pWS->IPData.iconPlacement & ICON_PLACE_BOTTOM_PRIMARY) ||
	   (pWS->IPData.iconPlacement & ICON_PLACE_BOTTOM_SECONDARY))
	    yMargin = sH - placementH - yMargin;
    }
    else
    {
	if (useMargin)
	{
	    xMargin = pWS->pSD->iconPlacementMargin - pWS->IPData.placeIconX;
	    extraXSpace = sW - (2 * pWS->pSD->iconPlacementMargin) -
			  (placementW - iSpaceX);
	    extraPX = (pWS->IPData.iconPlacement & ICON_PLACE_RIGHT_PRIMARY) ?
				1 : (pWS->IPData.placementCols - extraXSpace);

	    yMargin = pWS->pSD->iconPlacementMargin - pWS->IPData.placeIconY;
	    extraYSpace = sH - (2 * pWS->pSD->iconPlacementMargin) -
			  (placementH - iSpaceY);
	    extraPY = (pWS->IPData.iconPlacement & ICON_PLACE_BOTTOM_PRIMARY) ?
				1 : (pWS->IPData.placementRows - extraYSpace);
	}
	else
	{
	    xMargin = (sW - placementW + 
		((pWS->IPData.iPlaceW - pWS->pSD->iconWidth) & 1)) / 2;
	    extraXSpace = 0;
	    yMargin = (sH - placementH + 
		((pWS->IPData.iPlaceH - pWS->pSD->iconHeight) & 1))/ 2;
	    extraYSpace = 0;

	    if (pWS->IPData.iconPlacement & ICON_PLACE_RIGHT_PRIMARY)
	    {
		xMargin = sW - placementW - xMargin;
		pWS->IPData.placeIconX = pWS->IPData.iPlaceW - 
					 pWS->pSD->iconWidth - 
					 pWS->IPData.placeIconX;
	    }
	    if (pWS->IPData.iconPlacement & ICON_PLACE_BOTTOM_PRIMARY)
	    {
		yMargin = sH - placementH - yMargin;
		pWS->IPData.placeIconY = pWS->IPData.iPlaceH - 
					 pWS->pSD->iconHeight - 
					 pWS->IPData.placeIconY;
	    }
	}
    }

    /*
     * Setup array of grid row positions and grid column positions:
     */

    if ((pWS->IPData.placementRowY =
	    (int *)XtMalloc ((pWS->IPData.placementRows+2) * sizeof (int)))
	== NULL)
    {
	Warning ("Insufficient memory for icon placement");
	wmGD.iconAutoPlace = False;
	return;
    }
    else if ((pWS->IPData.placementColX =
		(int *)XtMalloc ((pWS->IPData.placementCols+2) * sizeof (int)))
	     == NULL)
    {
	XtFree ((char *)pWS->IPData.placementRowY);
	Warning ("Insufficient memory for icon placement");
	wmGD.iconAutoPlace = False;
	return;
    }

    pWS->IPData.placementRowY[0] = yMargin;
    for (i = 1; i <= pWS->IPData.placementRows; i++)
    {
	pWS->IPData.placementRowY[i] = pWS->IPData.placementRowY[i - 1] + 
	    pWS->IPData.iPlaceH;
	if ((extraYSpace > 0) && (i >= extraPY))
	{
	    (pWS->IPData.placementRowY[i])++;
	    extraYSpace--;
	}
    }

    pWS->IPData.placementColX[0] = xMargin;
    for (i = 1; i <= pWS->IPData.placementCols; i++)
    {
	pWS->IPData.placementColX[i] = pWS->IPData.placementColX[i - 1] + 
	    pWS->IPData.iPlaceW;
	if ((extraXSpace > 0) && (i >= extraPX))
	{
	    (pWS->IPData.placementColX[i])++;
	    extraXSpace--;
	}
    }


    /*
     * Setup an array of icon places.
     */

    pWS->IPData.totalPlaces = 
	pWS->IPData.placementRows * pWS->IPData.placementCols;

    if ((pWS->IPData.placeList =
	  (IconInfo *)XtMalloc (pWS->IPData.totalPlaces * sizeof (IconInfo)))
	== NULL)
    {
	Warning ("Insufficient memory for icon placement");
	XtFree ((char *)pWS->IPData.placementRowY);
	XtFree ((char *)pWS->IPData.placementColX);
	wmGD.iconAutoPlace = False;
	return;
    }

    memset ((char *)pWS->IPData.placeList, 0, 
	pWS->IPData.totalPlaces * sizeof (IconInfo));

    pWS->IPData.onRootWindow = True;


} /* END OF FUNCTION InitIconPlacement */



/*************************************<->*************************************
 *
 *  GetNextIconPlace (pIPD)
 *
 *
 *  Description:
 *  -----------
 *  This function identifies and returns the next free icon grid place.
 *
 * 
 *  Outputs:
 *  -------
 *  Return = next free place (index)
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO
int GetNextIconPlace (pIPD)

    IconPlacementData *pIPD;
#else /* _NO_PROTO */
int GetNextIconPlace (IconPlacementData *pIPD)
#endif /* _NO_PROTO */
{
    int i;


    for (i = 0; i < pIPD->totalPlaces; i++)
    {
	if (pIPD->placeList[i].pCD == (ClientData *)NULL)
	{
	    return (i);
	}
    }

    /*
     * All places are filled!  Find an alternative place.
     */

    return (NO_ICON_PLACE);

} /* END OF FUNCTION GetNextIconPlace */



/*************************************<->*************************************
 *
 *  CvtIconPlaceToPosition (pIPD, place, pX, pY)
 *
 *
 *  Description:
 *  -----------
 *  This function converts an icon place (index) into an icon position.
 *
 *
 *  Inputs:
 *  ------
 *  pIPD = ptr to icon placement data
 *
 *  place = place to be converted
 *
 *  wmGD = (iconPlacement ...)
 *
 * 
 *  Outputs:
 *  -------
 *  pX = pointer to icon place X location
 *
 *  pY = pointer to icon place Y location
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void CvtIconPlaceToPosition (pIPD, place, pX, pY)
    IconPlacementData *pIPD;
    int place;
    int *pX;
    int *pY;

#else /* _NO_PROTO */
void CvtIconPlaceToPosition (IconPlacementData *pIPD, int place, int *pX, int *pY)
#endif /* _NO_PROTO */
{
    int row;
    int col;


    if (pIPD->iconPlacement &
	(ICON_PLACE_LEFT_PRIMARY | ICON_PLACE_RIGHT_PRIMARY))
    {
	col = place % pIPD->placementCols;
	row = place / pIPD->placementCols;
    }
    else
    {
	col = place / pIPD->placementRows;
	row = place % pIPD->placementRows;
    }

    if (pIPD->iconPlacement &
	(ICON_PLACE_RIGHT_PRIMARY | ICON_PLACE_RIGHT_SECONDARY))
    {
	col = pIPD->placementCols - col - 1;
    }
    if (pIPD->iconPlacement &
	     (ICON_PLACE_BOTTOM_PRIMARY | ICON_PLACE_BOTTOM_SECONDARY))
    {
	row = pIPD->placementRows - row - 1;
    }

    if (pIPD->onRootWindow)
    {
	*pX = pIPD->placementColX[col] + pIPD->placeIconX;
	*pY = pIPD->placementRowY[row] + pIPD->placeIconY;
    }
    else 
    {
	*pX = col * pIPD->iPlaceW;
	*pY = row * pIPD->iPlaceH;
    }

} /* END OF FUNCTION CvtIconPlaceToPosition */



/*************************************<->*************************************
 *
 *  FindIconPlace (pCD, pIPD, x, y)
 *
 *
 *  Description:
 *  -----------
 *  This function is used to find a free icon place in the proximity of the
 *  specified position.
 *
 *
 *  Inputs:
 *  ------
 *  pIPD = ptr to icon placement data
 *
 *  x = desired x location of icon place
 *
 *  y = desired y location of icon place
 *
 * 
 *  Outputs:
 *  -------
 *  Return = icon place (index)
 *
 *
 *  Comments:
 *  --------
 *  Look first for a free icon place at the position passed in.  If that place
 *  is taken then look at positions that are +- one half the icon width/height
 *  from the postion passed in.  If those positions are taken look at
 *  positions that are +- one half icon placement width/height from the
 *  position passed in.
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
int FindIconPlace (pCD, pIPD, x, y)
    ClientData *pCD;
    IconPlacementData *pIPD;
    int x;
    int y;

#else /* _NO_PROTO */
int FindIconPlace (ClientData *pCD, IconPlacementData *pIPD, int x, int y)
#endif /* _NO_PROTO */
{
    int place;
    int i;
    int j;
    int diffX;
    int diffY;
    int altX;
    int altY;
    int amt;


    place = CvtIconPositionToPlace (pIPD, x, y);

    if (place < pIPD->totalPlaces)
    {
	if (pIPD->placeList[place].pCD == (ClientData *)NULL)
	{
	return (place);
	}
    }
    else
    {
        if (pIPD->iconPlacement & ICON_PLACE_LEFT_PRIMARY)
        {
            amt = pIPD->placementCols;              /* add a new row */
        }
        else
        {
            amt = pIPD->placementRows;              /* add a new column */
        }

	if (!ExtendIconList (P_ICON_BOX(pCD), amt))
	{
	    Warning ("Insufficient memory to create icon box data");
	    return (NO_ICON_PLACE);            
	}
    }
    /*
     * The place for the passed in position is in use, look at places for
     * alternative positions.
     */

    for (i = 0; i < 2; i++)
    {
	switch (i)
	{
	    case 0:
	    {
		diffX = ICON_WIDTH(pCD) / 2;
		diffY = ICON_HEIGHT(pCD) / 2;
		break;
	    }

	    case 1:
	    {
		diffX = pIPD->iPlaceW / 2;
		diffY = pIPD->iPlaceH / 2;
		break;
	    }
	}


	for (j = 0; j < 4; j++)
	{
	    switch (j)
	    {
		case 0:
		{
		    if (pIPD->iconPlacement & ICON_PLACE_LEFT_PRIMARY)
		    {
			altX = x - diffX;
			altY = y;
		    }
		    else if (pIPD->iconPlacement & ICON_PLACE_RIGHT_PRIMARY)
		    {
			altX = x + diffX;
			altY = y;
		    }
		    else if (pIPD->iconPlacement & ICON_PLACE_TOP_PRIMARY)
		    {
			altX = x;
			altY = y - diffY;
		    }
		    else
		    {
			altX = x;
			altY = y + diffY;
		    }
		    break;
		}

		case 1:
		{
		    if (pIPD->iconPlacement & ICON_PLACE_LEFT_PRIMARY)
		    {
			altX = x + diffX;
			altY = y;
		    }
		    else if (pIPD->iconPlacement & ICON_PLACE_RIGHT_PRIMARY)
		    {
			altX = x - diffX;
			altY = y;
		    }
		    else if (pIPD->iconPlacement & ICON_PLACE_TOP_PRIMARY)
		    {
			altX = x;
			altY = y + diffY;
		    }
		    else
		    {
			altX = x;
			altY = y - diffY;
		    }
		    break;
		}

		case 2:
		{
		    if (pIPD->iconPlacement & ICON_PLACE_LEFT_SECONDARY)
		    {
			altX = x - diffX;
			altY = y;
		    }
		    else if (pIPD->iconPlacement & ICON_PLACE_RIGHT_SECONDARY)
		    {
			altX = x + diffX;
			altY = y;
		    }
		    else if (pIPD->iconPlacement & ICON_PLACE_TOP_SECONDARY)
		    {
			altX = x;
			altY = y + diffY;
		    }
		    else
		    {
			altX = x;
			altY = y - diffY;
		    }
		    break;
		}

		case 3:
		{
		    if (pIPD->iconPlacement & ICON_PLACE_LEFT_SECONDARY)
		    {
			altX = x + diffX;
			altY = y;
		    }
		    else if (pIPD->iconPlacement & ICON_PLACE_RIGHT_SECONDARY)
		    {
			altX = x - diffX;
			altY = y;
		    }
		    else if (pIPD->iconPlacement & ICON_PLACE_TOP_SECONDARY)
		    {
			altX = x;
			altY = y - diffY;
		    }
		    else
		    {
			altX = x;
			altY = y + diffY;
		    }
		    break;
		}
	    }

	    if (P_ICON_BOX(pCD))
	    {
		GetClipDimensions(pCD, False);
		if (altX < clipX) 
		{
		    return (NO_ICON_PLACE);
		}
		if (altY < clipY) 
		{
		    return (NO_ICON_PLACE);
		}
		if (((int)altX) > ((int)clipX + 
			(int)clipWidth - ((int)ICON_WIDTH(pCD)))) 
		{
		    return (NO_ICON_PLACE);
		}
		if (((int)altY) > ((int)clipY + 
			(int)clipHeight - ((int)ICON_HEIGHT(pCD))))
		{
		    return (NO_ICON_PLACE);
		}
	    }

	    place = CvtIconPositionToPlace (pIPD, altX, altY);
	    if ((pIPD->placeList[place].pCD) == NULL)
	    {
		return (place);
	    }

	}
    }

    /*
     * Couldn't find an unoccupied place in the proximity of the passed-in
     * position.
     */

    return (NO_ICON_PLACE);


} /* END OF FUNCTION FindIconPlace */



/*************************************<->*************************************
 *
 *  CvtIconPostionToPlace (pIPD, x, y)
 *
 *
 *  Description:
 *  -----------
 *  This function converts an icon position to an icon place.
 *
 *
 *  Inputs:
 *  ------
 *  pIPD = ptr to icon placement data
 *
 *  x,y = location to be converted into an icon place
 *
 * 
 *  Outputs:
 *  -------
 *  Return = icon place (index)
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO
int CvtIconPositionToPlace (pIPD, x, y)
    IconPlacementData *pIPD;
    int x;
    int y;

#else /* _NO_PROTO */
int CvtIconPositionToPlace (IconPlacementData *pIPD, int x, int y)
#endif /* _NO_PROTO */
{
    int row;
    int col;


    if (pIPD->onRootWindow)
    {
	/*
	 * Scan through the root window row/column arrays and find the 
	 * placement position.
	 */

	for (row = 1; row < pIPD->placementRows; row++)
	{
	    if (y < pIPD->placementRowY[row])
	    {
		break;
	    }
	}
	row--;

	for (col = 1; col < pIPD->placementCols; col++)
	{
	    if (x < pIPD->placementColX[col])
	    {
		break;
	    }
	}
	col--;


	if (pIPD->iconPlacement &
	    (ICON_PLACE_RIGHT_PRIMARY | ICON_PLACE_RIGHT_SECONDARY))
	{
	    col = pIPD->placementCols - col - 1;
	}
	if (pIPD->iconPlacement &
	    (ICON_PLACE_BOTTOM_PRIMARY | ICON_PLACE_BOTTOM_SECONDARY))
	{
	    row = pIPD->placementRows - row - 1;
	}
    }
    else
    {
	/* 
	 * convert icon box coords
	 */
	col = x / pIPD->iPlaceW;
	row = y / pIPD->iPlaceH;
    }


    if (pIPD->iconPlacement &
	(ICON_PLACE_LEFT_PRIMARY | ICON_PLACE_RIGHT_PRIMARY))
    {
	return ((row * pIPD->placementCols) + col);
    }
    else
    {
	return ((col * pIPD->placementRows) + row);
    }
    

} /* END OF FUNCTION CvtIconPositionToPlace */





/*************************************<->*************************************
 *
 *  PackRootIcons ()
 *
 *
 *  Description:
 *  -----------
 *  This function packs the icons on the root window
 *
 *
 *  Inputs:
 *  ------
 * 
 *  Outputs:
 *  -------
 *
 *  Comments:
 *  ---------
 *
 *************************************<->***********************************/

#ifdef _NO_PROTO
void PackRootIcons ()

#else /* _NO_PROTO */
void PackRootIcons (void)
#endif /* _NO_PROTO */
{
    int iOld, iNew;
    ClientData *pCD;
    ClientData *pCD_active;
    int hasActiveText = 1;

    /* 
     * find context of the activeIconTextWin to get pCD and then 
     * if it is the same as this client, hide it.
     */

    if (XFindContext (DISPLAY, ACTIVE_PSD->activeIconTextWin,
			wmGD.windowContextType, (caddr_t *)&pCD_active))
    {
	hasActiveText = 0;
    }

    /* 
     *  Traverse the list and pack them together
     */

    if (wmGD.iconAutoPlace)
    {
	for (iOld = iNew = 0; iOld < ACTIVE_WS->IPData.totalPlaces; 
	    iOld++, iNew++)
	{
	    if (ACTIVE_WS->IPData.placeList[iOld].pCD == NULL)
	    {
		/* advance to next non-null entry */
		while (++iOld < ACTIVE_WS->IPData.totalPlaces && 
		       !ACTIVE_WS->IPData.placeList[iOld].pCD)
		    ;
	    }

	    if (iOld < ACTIVE_WS->IPData.totalPlaces && iOld != iNew)
	    {
		/* move the icon from its old place to the new place */

		MoveIconInfo (&ACTIVE_WS->IPData, iOld, iNew);

		pCD = ACTIVE_WS->IPData.placeList[iNew].pCD;
		pCD->iconPlace = iNew;
		CvtIconPlaceToPosition (&ACTIVE_WS->IPData, 
		    pCD->iconPlace, &pCD->iconX, &pCD->iconY);

		if (hasActiveText && (pCD == pCD_active))
		{
		    /* hide activeIconTextWin first */
		    HideActiveIconText ((WmScreenData *)NULL);
		    XMoveWindow (DISPLAY, ICON_FRAME_WIN(pCD), pCD->iconX, 
			     pCD->iconY);
		    ShowActiveIconText (pCD);
		}
		else
		{
		    XMoveWindow (DISPLAY, ICON_FRAME_WIN(pCD), pCD->iconX, 
			     pCD->iconY);
		}
	    }
	}
    }
} /* END OF FUNCTION PackRootIcons */



/*************************************<->*************************************
 *
 *  MoveIconInfo (pIPD, p1, p2)
 *
 *
 *  Description:
 *  -----------
 *  Move icon info from place 1 to place 2 in the placement list.
 *
 *
 *  Inputs:
 *  ------
 *  pIPD	- ptr to icon placement data
 *  p1		- placement index 1 (source)
 *  p2		- placement index 2 (destination)
 * 
 *  Outputs:
 *  -------
 *
 *  Comments:
 *  --------
 * 
 *************************************<->***********************************/

#ifdef _NO_PROTO
void MoveIconInfo (pIPD, p1, p2)

    IconPlacementData *pIPD;
    int p1, p2;
#else /* _NO_PROTO */
void MoveIconInfo (IconPlacementData *pIPD, int p1, int p2)
#endif /* _NO_PROTO */
{

    /* only move if destination is empty */
    if (pIPD->placeList[p2].pCD == NULL)
    {
	pIPD->placeList[p2].pCD = pIPD->placeList[p1].pCD;
	pIPD->placeList[p2].theWidget = pIPD->placeList[p1].theWidget;
	pIPD->placeList[p2].pCD->iconPlace = p2;

	pIPD->placeList[p1].pCD =  NULL;
	pIPD->placeList[p1].theWidget = NULL;
    }
}
