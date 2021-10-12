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
 * derived from cippd_data.c	4.1	(ULTRIX)	7/2/90";
 */
/*
 *
 *   Facility:	Systems Communication Architecture
 *		Computer Interconnect Port-to-Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect Port-to-Port
 *		Driver( CI PPD ) configurable variables.
 *
 *   Creator:	Todd M. Katz	Creation Date:	September 13, 1987
 *
 *   Modification History:
 *
 *   18-Sep-1989	Pete Keilty
 *	Add errlogging variable cippd_errlog.
 *
 *   30-Aug-1988	Todd M. Katz		TMK0003
 *	1. Remove the configuration variables cippd_burst and cippd_contact.
 *	   It is no longer desirable to specify port polling burst size and
 *	   contact frequency for the CI PPD as a whole because the optimum
 *	   values for these parameters now differ on an individual port driver
 *	   basis.  These parameters are now specified by each port driver which
 *	   interfaces to the CI PPD( CI and MSI ) and their values passed to
 *	   the CI PPD through CI PPD specific PCCB fields.
 *	2. Add the configuration variable cippd_init_dgs.  This variable
 *	   controls the number of initial datagrams allocated for path
 *	   establishment by the CI PPD on a per-port basis.
 *
 *   13-Apr-1988	Todd M. Katz		TMK0002
 *	Add the configuration variable cippd_pc_panic.  This configuration
 *	variable controls whether CI PPD path failures are recovered from or
 *	immediately panic the system.  Error recovery is the default but may be
 *	overruled by setting the configuration file option SCA_PANIC to an
 *	appropriate value.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, and made CI PPD completely
 *	independent from underlying port drivers.
 */

/* Libraries and Include Files.
 */
#include		<sys/types.h>
#include		<io/dec/scs/scaparam.h>

/* CI PPD Configuration Variables.
 */
u_short		cippd_init_dgs		/* CI PPD num path establishment dgs */
		    = 2;		/*  MAX:   223; DEF:      2; MIN:  2 */
u_short		cippd_itime		/* CI PPD port interval timer( secs )*/
		    = 2;		/*  MAX: 32767; DEF:      2; MIN:  2 */
u_short		cippd_max_port		/* CI PPD max port num to recognize  */
		    = 31;		/*  MAX:   223; DEF:	 31; MIN:  0 */
u_short		cippd_pc_panic		/* CI PPD path crash panic flag      */
		    = SCA_PANIC;	/*  MAX:     3; DEF:      0; MIN:  0 */
u_short		cippd_penable		/* CI PPD port polling enable flag   */
		    = 1;		/*  MAX:     1; DEF:      1; MIN:  0 */
u_short		cippd_severity		/* CI PPD console logging severity   */
		    = SCA_SEVERITY;	/*  MAX:     5; DEF:	  0; MIN:  0 */
u_short		cippd_errlog		/* CI PPD errlog logging severity    */
		    = SCA_ERRLOG0;	/*  MAX:     3; DEF:	  0; MIN:  0 */
