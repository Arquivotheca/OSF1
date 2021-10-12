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
#include <X11/Wc/COPY>

/*
* SCCS_data: @(#) Ari.c 1.2 92/03/18 10:42:25
*
* Athena Resource Interpreter - Ari.c
*
* Ari.c implements an Athena Resource Interpreter which allows prototype
* Athena widget based user interfaces to be built from resource files.  The
* Widget Creation library is used.
*
******************************************************************************
*/

#include <X11/Intrinsic.h>
#include <X11/Wc/WcCreate.h>
#include <X11/Xp/Xp.h>

/******************************************************************************
**  Private Data
******************************************************************************/

/* All Wcl applications should provide at least these Wcl options:
*/
static XrmOptionDescRec options[] = {
    WCL_XRM_OPTIONS
};


/******************************************************************************
**  Private Functions
******************************************************************************/

/*ARGSUSED*/
static void RegisterApplication ( app )
    XtAppContext app;
{
    /* -- Useful shorthand for registering things with the Wcl library */
#define RCP( name, class  ) WcRegisterClassPtr   ( app, name, class );
#define RCO( name, constr ) WcRegisterConstructor( app, name, constr );
#define RAC( name, func   ) WcRegisterAction     ( app, name, func );
#define RCB( name, func   ) WcRegisterCallback   ( app, name, func, NULL );

    /* -- register widget classes and constructors */
    /* -- Register application specific actions */
    /* -- Register application specific callbacks */
}


/******************************************************************************
*   MAIN function
******************************************************************************/

main ( argc, argv )
    int   argc;
    char* argv[];
{   
    /*  -- Intialize Toolkit creating the application shell
    */
    Widget appShell = XtInitialize (
	argv[0], WcAppClass( argc, argv ),	/* application name & class */
	options, XtNumber(options),		/* descr of cmd line options */
	&argc, argv
    );
    XtAppContext app = XtWidgetToApplicationContext(appShell);

    /*  -- Register all application specific callbacks and widget classes
    */
    RegisterApplication ( app );

    /*  -- Register all Athena and Public widget classes, CBs, ACTs
    */
    XpRegisterAll ( app );

    /*  -- Create widget tree below toplevel shell using Xrm database
    */
    WcWidgetCreation ( appShell );

    /*  -- Realize the widget tree and enter the main application loop
    */
    XtRealizeWidget ( appShell );
    XtMainLoop ( );
}
