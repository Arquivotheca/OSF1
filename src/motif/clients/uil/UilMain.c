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
static char rcsid[] = "$RCSfile: UilMain.c,v $ $Revision: 1.1.4.2 $ $Date: 1993/05/07 00:35:45 $"
#endif
#endif

/*
*  (c) Copyright 1989, 1990, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */

/*
**++
**  FACILITY:
**
**      User Interface Language Compiler (UIL)
**
**  ABSTRACT:
**
**      This module is the main procedure of the UIL Compiler.
**
**--
**/

/*
**
**  INCLUDE FILES
**
*/

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif
#include "UilDefI.h"
#include <setjmp.h>

/*
**
**  TABLE OF CONTENTS
**
*/

/*
** FORWARD DECLARATIONS
*/


#ifdef _NO_PROTO
extern int main  _ARGUMENTS(( void ));
#else
extern int main  _ARGUMENTS(( int l_argc , char *rac_argv []));
#endif

static void common_main  _ARGUMENTS(( void ));
static void common_cleanup  _ARGUMENTS(( void ));

#ifdef CALLABLE
static void UilWrapup _ARGUMENTS((Uil_compile_desc_type *compile_desc));
#endif	/* CALLABLE */


/*
**
**  EXTERNAL Definitions
**
*/


/*
**
**  MODULE Definitions
**
*/

static status      return_status = 0;
static jmp_buf     environment;
static unsigned    module_flags = 0;
static unsigned	   doing_exit = 0;

/*  Bit definitions for module_flags */

#define     compiler_called (1 << 0)


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      This is the common part of the main routine.
**
**  FORMAL PARAMETERS:
**
**      none
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      void
**
**  SIDE EFFECTS:
**
**      compilation occurs
**
**--
**/
 
static void	common_main()
{
  /* Initialize the X toolkit. */
  XtToolkitInitialize(); 
  
    /* use the user supplied data base instead */
    if (Uil_cmd_z_command.v_database)
	db_incorporate();

    /* initialize the diagnostic system */
    diag_initialize_diagnostics();

    /* initialize the symbol table */
    sym_initialize_storage();

    /* initialize the source */
    src_initialize_source();

    /* open listing file if requested */
    if (Uil_cmd_z_command.v_listing_file)
	lst_open_listing();

    /* initialize the lexical analyzer */
    lex_initialize_analyzer();

    /* initialize the keyword table */
    key_initialize();

    /* initialize the sar data structures */
    sar_initialize();

    /* call YACC to parse the source file */
    /* return 0 for success, 1 for failure */
    /* Make sure the root entry sections pointer is filled in */
    if (yyparse() != 0)
    	diag_issue_diagnostic
	    (d_inv_module, diag_k_no_source, diag_k_no_column);
    sym_az_root_entry->sections = sym_az_current_section_entry;

    /* call forward reference resolver */
    sem_resolve_forward_refs();

    /* call semantic validation */
    sem_validation ();

    /* call the output phase if requested */
    if (Uil_cmd_z_command.v_resource_file)
	sem_output_uid_file();

    /* call symbol table dumper - if requested */
#if debug_version
    if (uil_v_dump_symbols == TRUE)
	sym_dump_symbols();
#endif


    /* write compilation summary */
    diag_issue_summary();


    /* write listing file if requested */
    if (Uil_cmd_z_command.v_listing_file)
	lst_output_listing();

    /* Storage is not cleaned up, since we will exit */
    uil_exit( uil_l_compile_status );

}


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      This is the image termination procedure for the UIL compiler.
**
**  FORMAL PARAMETERS:
**
**      severity	status of compilation on exit
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      void
**
**  SIDE EFFECTS:
**
**      never return from this procedure
**
**--
**/

void	uil_exit(severity)

int	severity;

{


#ifdef VMS

#define inhibit_printing 0x10000000

    globalvalue ss$_normal;
    globalvalue ss$_abort;

    /* Prevent multiple looping through this routine */
    if ( doing_exit ) return;
    doing_exit = 1;

    if (module_flags & compiler_called)
        return_status = severity;
    else
        switch (severity)
        {
        case uil_k_success_status:
            return_status = (ss$_normal & 0xFFFFFFF8) + inhibit_printing + 1;
            break;
        case uil_k_info_status:
            return_status = (ss$_normal & 0xFFFFFFF8) + inhibit_printing + 3;
            break;
        case uil_k_warning_status:
            return_status = (ss$_normal & 0xFFFFFFF8) + inhibit_printing + 0;
            break;
        case uil_k_error_status:
            return_status = (ss$_abort & 0xFFFFFFF8) + inhibit_printing + 2;
            break;
        case uil_k_severe_status:
            return_status = (ss$_abort & 0xFFFFFFF8) + inhibit_printing + 4;
            break;
        }


#else

    /* Prevent multiple looping through this routine */
    if ( doing_exit ) return;
    doing_exit = 1;

    return_status = (severity >= uil_k_error_status);

#endif

    /* Close and remove the uid file if it's still open. */
    /* It will only be open if a severe error occurred during the output */
    /* of the uid file. */

    if (out_az_idbfile_id != NULL)
	UrmIdbCloseFile (out_az_idbfile_id, FALSE);

    /* If compiler called, return to calling program rather than */
    /* exiting process */

    if (module_flags & compiler_called)
        longjmp (environment, 1);

    /* RAP FIX for listing files */
    common_cleanup();


    exit(return_status);
}






#ifndef CALLABLE
/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      This is the main routine
**
**  FORMAL PARAMETERS:
**
**      l_argc:	    the number of command line arguments
**	rac_argv:   an array of pointers to character array arguments
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  COMPLETION CODES:
**
**      1 if errors; 0 otherwise
**
**  SIDE EFFECTS:
**
**	produce possibly a resource file and a listing file
**--
**/

#ifdef VMS
int	main(  )
#else
int	main( l_argc, rac_argv )
int	l_argc;
char	*rac_argv[ ];
#endif
{
    
    /* call routine to parse the command line */

#ifdef VMS
    cmd_decode_command_line(  );
#else
    cmd_decode_command_line( l_argc, rac_argv );
#endif

    /* call common main routine */

    common_main();

}
#endif /* !CALLABLE */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      This is the dynamic memory cleanup routine.  It calls all facility
**	provided cleanup routines so that dynamic structures can be released.
**
**  FORMAL PARAMETERS:
**
**      none
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      void
**
**  SIDE EFFECTS:
**
**      all dynamic memory is freed 
**
**--
**/

static void	common_cleanup()
{

    /* cleanup the source file information */

    Uil_src_cleanup_source();


    /* cleanup listing facility */

    Uil_lst_cleanup_listing();


    /* cleanup the lexical analyzer */

    Uil_lex_cleanup_analyzer();


    return;
}


#ifdef CALLABLE
/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      This is the callable entry point for the UIL Compiler. 
**
**  FORMAL PARAMETERS:
**
**      command_desc    pointer to data structure defining UIL command line
**
**	compile_desc	pointer to data structure describing the results of
**			the compilation.
**
**	message_cb	callback routine to process messages
**
**	message_data	user-data passed to the message_cb
**
**	status_cb	callback routine to process messages
**
**	status_data	user-data passed to the status_cb
**
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  COMPLETION CODES:
**
**      uil$_normal if no errors; uil$_no_output otherwise
**
**  SIDE EFFECTS:
**
**	    none
**--
**/


Uil_status_type Uil 

#ifndef _NO_PROTO
(Uil_command_type        *command_desc,
Uil_compile_desc_type   *compile_desc,
Uil_continue_type       (*message_cb)(),
char            *message_data,
Uil_continue_type       (*status_cb)(),
char            *status_data)

#else 

(command_desc, compile_desc, message_cb, message_data,
			status_cb, status_data)

Uil_command_type	*command_desc;
Uil_compile_desc_type	*compile_desc;
Uil_continue_type	(*message_cb)();
char			*message_data;
Uil_continue_type	(*status_cb)();
char			*status_data;
#endif

{

    /* Indicate compiler called rather than being started via command line */

    module_flags = module_flags | compiler_called;
    doing_exit = 0;


    /* Initialize command line data structure */

    Uil_cmd_z_command.ac_database = command_desc -> database;
    Uil_cmd_z_command.v_database = command_desc -> database_flag;
    Uil_cmd_z_command.ac_source_file = command_desc -> source_file;
    Uil_cmd_z_command.ac_resource_file = command_desc -> resource_file;
    Uil_cmd_z_command.ac_listing_file = command_desc -> listing_file;
    Uil_cmd_z_command.include_dir_count = command_desc -> include_dir_count;
    Uil_cmd_z_command.ac_include_dir = command_desc -> include_dir;
    Uil_cmd_z_command.v_listing_file = command_desc -> listing_file_flag;
    Uil_cmd_z_command.v_resource_file = command_desc -> resource_file_flag;
    Uil_cmd_z_command.v_show_machine_code = command_desc -> machine_code_flag;
    Uil_cmd_z_command.v_report_info_msg = command_desc -> report_info_msg_flag;
    Uil_cmd_z_command.v_report_warn_msg = command_desc -> report_warn_msg_flag;
    Uil_cmd_z_command.v_parse_tree = command_desc -> parse_tree_flag;
    Uil_cmd_z_command.v_use_setlocale = command_desc -> use_setlocale_flag;
    Uil_cmd_z_command.v_issue_summary = command_desc -> issue_summary;
    Uil_cmd_z_command.status_update_delay = command_desc -> status_update_delay;
    Uil_cmd_z_command.message_cb = message_cb;
    Uil_cmd_z_command.message_data = message_data;
    Uil_cmd_z_command.status_cb = status_cb;
    Uil_cmd_z_command.status_data = status_data;


    /* The setjmp function allows us to unwind from a fatal error setjmp is */
    /* nonzero if we are returning from a fatal error			    */

        if (setjmp(environment) == 0) {
    
	    /* use the user supplied data base instead. If no source file
	       is given (this call is only to change the database), return at
	       this point with a success. */
	    if ( Uil_cmd_z_command.v_database )
		{
		db_incorporate ();
		if ( Uil_cmd_z_command.ac_source_file == NULL )
		    uil_exit (uil_k_success_status);
		}

            /* initialize the diagnostic system */
	    uil_l_compile_status = uil_k_success_status;
            diag_initialize_diagnostics();
    
            /* initialize the symbol table */
            sym_initialize_storage();
    
            /* initialize the source */
            src_initialize_source();
    
            /* open listing file if requested */
            if (Uil_cmd_z_command.v_listing_file)
            	lst_open_listing();

            /* initialize the lexical analyzer */
            lex_initialize_analyzer();
    
            /* initialize the keyword table */
            key_initialize();
    
	    /* initialize the sar data structures */
	    sar_initialize();

	    /* call YACC to parse the source file */
            /* return 0 for success, 1 for failure */
	    /* Make sure the root entry sections pointer is filled in */
            if (yyparse() != 0)
            	diag_issue_diagnostic
        	    (d_inv_module, diag_k_no_source, diag_k_no_column);
	    sym_az_root_entry->sections = sym_az_current_section_entry;
    
            /* call forward reference resolver */
            sem_resolve_forward_refs();
        
	    /* call semantic validation */
	    sem_validation ();

            /* call the output phase if requested */
            if (Uil_cmd_z_command.v_resource_file)
            	sem_output_uid_file();

            /* call symbol table dumper - if requested */
#if debug_version
		if (uil_v_dump_symbols == TRUE)
		    sym_dump_symbols();
#endif

	    /* Perform standard wrapup processing */
	    UilWrapup (compile_desc);
    
	    /* exit with the compile status */
	    return uil_l_compile_status;

            }

	/*
	**  longjmp return from setjmp.  This is the case of a return via 
	**  uil_exit the value return_status is set by uil_exit.
	*/
        else
	    {
	    /* Do standard compiler wrapup */
	    UilWrapup (compile_desc);

	    /* return the status set by uil_exit */
	    return uil_l_compile_status;

	    }

}

/*
 * Local function to provide compiler wrapup processing. It is called both
 * from the longjmp and sequential paths in the callable compiler above.
 */
static void UilWrapup (compile_desc)
    Uil_compile_desc_type	*compile_desc;

{

    int i;  /* loop index for copying message counts		    */

    /* write compilation summary if requested */
    if ( Uil_cmd_z_command.v_issue_summary )
	diag_issue_summary();
    
    /* write listing file if requested */
    if (Uil_cmd_z_command.v_listing_file)
	lst_output_listing();

    /*
     ** fill in the "parse tree root" in the compile descriptor,
     ** and set the version for the compiler and the symbol table
     ** structure.
     */
    compile_desc->parse_tree_root = (char *)sym_az_root_entry;
    compile_desc->data_version = _data_version;
    compile_desc->compiler_version = _compiler_version_int;

    /* Fill in the message_summary array in the compile_desc */
    for (i = uil_k_min_status; i <= uil_k_max_status; i++)
	compile_desc->message_count[i] = Uil_message_count[i];

    /* If there are any error/severe messages, then don't return */
    /* a symbol table for the callable compiler - clean up here */
    if ( Uil_message_count[Uil_k_error_status]>0 ||
	 Uil_message_count[Uil_k_severe_status]>0 )
	{
	Uil_cmd_z_command.v_parse_tree = FALSE;
	compile_desc->parse_tree_root = NULL;
	}

    /* Call the cleanup routine to free dynamic memory */
    common_cleanup();
    
    /* Cleanup storage; what is cleaned up depends on whether compiler	*/
    /* was called or not						*/
    Uil_sym_cleanup_storage (Uil_cmd_z_command.v_parse_tree!=1);    

    }

#endif	/* CALLABLE */





