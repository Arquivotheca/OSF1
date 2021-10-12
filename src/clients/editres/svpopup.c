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
 * $XConsortium: svpopup.c,v 1.13 91/07/09 09:46:48 rws Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Chris D. Peterson, MIT X Consortium
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>	/* Get standard string definations. */
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>

#include "editresP.h"

#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Cardinals.h>	
#include <X11/Xaw/Command.h>	
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>	

#include <stdio.h>

extern void SetMessage(), SetCommand(), InsertWidgetFromNode();
extern void GetAllStrings(), PopupCentered();

static void _SetField(), CreateSetValuesPopup();
static void DoSetValues(), CancelSetValues();

/*	Function Name: PopupSetValues
 *	Description: This function pops up the setvalues dialog
 *	Arguments: parent - the parent of the setvalues popup.
 *                 event - the event that caused this popup, or NULL.
 *	Returns: none
 */

/* ARGSUSED */
void
PopupSetValues(parent, event)
Widget parent;
XEvent * event;
{
    Arg args[1];

    if (global_tree_info == NULL) {
	SetMessage(global_screen_data.info_label,
		   "No widget Tree is avaliable.");
	return;
    }

/* 
 * Check and possibly create the popup.
 */

    if (global_screen_data.set_values_popup == NULL)
	CreateSetValuesPopup(parent, &global_screen_data);

/*
 * Clear out the old strings, and set the active widget to the name widget.
 */

    XtSetArg(args[0], XtNstring, "");
    XtSetValues(global_screen_data.res_text, args, ONE);
    XtSetValues(global_screen_data.val_text, args, ONE);

    _SetField(global_screen_data.res_text, global_screen_data.val_text);

/*
 * Pop it up.
 */

    PopupCentered(event, global_screen_data.set_values_popup, XtGrabNone);
}

/*	Function Name: ModifySVEntry
 *	Description: Action routine that can be bound to the set values 
 *                   dialog box's Text Widget that will send input to the 
 *                   field specified.
 *	Arguments:   (Standard Action Routine args) 
 *	Returns:     none.
 */

/* ARGSUSED */
void 
ModifySVEntry(w, event, params, num_params)
Widget w;
XEvent *event;
String * params;
Cardinal * num_params;
{
    Widget new, old;
    char msg[BUFSIZ];
    
    if (*num_params != 1) {
	strcpy(msg, 
	       "Error: SVActiveEntry Action must have exactly one argument.");
	SetMessage(global_screen_data.info_label, msg);
	return;
    }
    
    switch (params[0][0]) {
    case 'r':
    case 'R':
	new = global_screen_data.res_text;
	old = global_screen_data.val_text;
	break;
    case 'v':
    case 'V':
	new = global_screen_data.val_text;
	old = global_screen_data.res_text;
	break;
    default:
	sprintf(msg, "%s %s", "Error: SVActiveEntry Action's first Argument",
		"must be either 'Resource' or 'Value'.");
	SetMessage(global_screen_data.info_label, msg);
	return;
    }
    
    _SetField(new, old);
}

/************************************************************
 *
 * Private Functions
 *
 ************************************************************/

/*	Function Name: _SetField
 *	Description: Sets the current text entry field.
 *	Arguments: new, old - new and old text fields.
 *	Returns: none
 */

static void
_SetField(new, old)
Widget new, old;
{
    Arg args[2];
    Pixel new_border, old_border, old_bg;
    
    if (!XtIsSensitive(new)) {
	XBell(XtDisplay(old), 0); /* Don't set field to an inactive Widget. */
	return;
    }
    
    XtSetKeyboardFocus(XtParent(new), new); 
    
    XtSetArg(args[0], XtNborderColor, &old_border);
    XtSetArg(args[1], XtNbackground, &old_bg);
    XtGetValues(new, args, TWO);
    
    XtSetArg(args[0], XtNborderColor, &new_border);
    XtGetValues(old, args, ONE);
    
    if (old_border != old_bg)	/* Colors are already correct, return. */
	return;

    XtSetArg(args[0], XtNborderColor, old_border);
    XtSetValues(old, args, ONE);

    XtSetArg(args[0], XtNborderColor, new_border);
    XtSetValues(new, args, ONE);
}

/*	Function Name: CreateSetValuesPopup
 *	Description: Creates the setvalues popup.
 *	Arguments: parent - the parent of the popup.
 *                 scr_data - the data about this screen.
 *	Returns: the set values popup.
 */

static void
CreateSetValuesPopup(parent, scr_data)
Widget parent;
ScreenData * scr_data;
{
    Widget form, cancel, do_it, label;
    Widget res_label;
    Arg args[10];
    Cardinal num_args;
    
    scr_data->set_values_popup = XtCreatePopupShell("setValuesPopup", 
						    transientShellWidgetClass, 
						    parent, NULL, ZERO);

    form = XtCreateManagedWidget("form", formWidgetClass, 
				 scr_data->set_values_popup, NULL, ZERO);

    num_args = 0;
    label = XtCreateManagedWidget("label", labelWidgetClass,
				  form, args, num_args);


    num_args = 0;
    XtSetArg(args[num_args], XtNfromVert, label); num_args++;
    res_label = XtCreateManagedWidget("resourceLabel", labelWidgetClass,
				  form, args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], XtNfromVert, label); num_args++;
    XtSetArg(args[num_args], XtNfromHoriz, res_label); num_args++;
    scr_data->res_text = XtCreateManagedWidget("resourceText", 
						  asciiTextWidgetClass,
						  form, args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], XtNfromVert, scr_data->res_text); num_args++;
    (void)  XtCreateManagedWidget("valueLabel", labelWidgetClass,
				  form, args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], XtNfromHoriz, res_label); num_args++;
    XtSetArg(args[num_args], XtNfromVert, scr_data->res_text); num_args++;
    scr_data->val_text = XtCreateManagedWidget("valueText", 
						  asciiTextWidgetClass,
						  form, args, num_args);
  
    num_args = 0;
    XtSetArg(args[num_args], XtNfromVert, scr_data->val_text); num_args++;
    do_it = XtCreateManagedWidget("setValues", commandWidgetClass, 
					  form, args, num_args);

    num_args = 0;
    XtSetArg(args[num_args], XtNfromVert, scr_data->val_text); num_args++;
    XtSetArg(args[num_args], XtNfromHoriz, do_it); num_args++;
    cancel = XtCreateManagedWidget("cancel", commandWidgetClass,
				   form, args, num_args);

    XtAddCallback(do_it, XtNcallback, DoSetValues, NULL);
    XtAddCallback(cancel, XtNcallback, CancelSetValues, NULL);

/*
 * Initialize the text entry fields.
 */

    {
	Pixel color;

	num_args = 0;
	XtSetArg(args[num_args], XtNbackground, &color); num_args++;
	XtGetValues(scr_data->val_text, args, num_args);

	num_args = 0;
	XtSetArg(args[num_args], XtNborderColor, color); num_args++;
	XtSetValues(scr_data->val_text, args, num_args);

	XtSetKeyboardFocus(form, scr_data->res_text);
    }
}

/*	Function Name: DoSetValues
 *	Description: Performs a SetValues.
 *	Arguments: w - the widget that called this.
 *                 junk, garbage - ** UNUSED **.
 *	Returns: none.
 */

/* ARGSUSED */
static void
DoSetValues(w, junk, garbage)
Widget w;
caddr_t junk, garbage;
{
    ProtocolStream * stream = &(global_client.stream);
    char *res_name, *res_value;
    Arg args[1];
    Cardinal i;

    if (global_tree_info->num_nodes == 0) {
	SetMessage(global_screen_data.info_label,
		   "There are no currently active widgets.");
	return;
    }
		
    XtSetArg(args[0], XtNstring, &res_name);
    XtGetValues(global_screen_data.res_text, args, ONE);

    XtSetArg(args[0], XtNstring, &res_value);
    XtGetValues(global_screen_data.val_text, args, ONE);

    if ((global_tree_info->res_value != NULL)
	&& (strlen(global_tree_info->res_value) < strlen(res_value)))
	{
        XtFree(global_tree_info->res_value);
	global_tree_info->res_value = XtMalloc(strlen(res_value) + 1);
	}
    strcpy(global_tree_info->res_value, res_value);
    global_tree_info->res_name = res_name;

  
    _XEditResResetStream(stream);
    _XEditResPutString8(stream, res_name);
    _XEditResPutString8(stream, XtRString);
    _XEditResPutString8(stream, res_value);
    _XEditResPut16(stream, global_tree_info->num_nodes);

    for (i = 0; i < global_tree_info->num_nodes; i++) 
	InsertWidgetFromNode(stream, global_tree_info->active_nodes[i]);

    SetCommand(w, LocalSetValues, NULL);
}

/*	Function Name: CancelSetValues
 *	Description: Pops down the setvalues popup.
 *	Arguments: w - any grandchild of the popup.
 *                 junk, garbage - ** UNUSED **.
 *	Returns: none.
 */

/* ARGSUSED */
static void
CancelSetValues(w, junk, garbage)
Widget w;
caddr_t junk, garbage;
{
    XtPopdown(XtParent(XtParent(w))); 
}
