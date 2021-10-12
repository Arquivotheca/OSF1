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
#ifndef lint
static char *rcsid = "@(#)$RCSfile: np_lpmaint.c,v $ $Revision: 1.1.6.2 $ (DEC) $Date: 1993/07/13 17:46:46 $";
#endif
/*
 * derived from np_lpmaint.c	5.1	(ULTRIX)	8/5/91";
 */
/************************************************************************
 *
 *   Facility:	Systems Communication Architecture
 *		Computer Interconnect N_PORT Port Driver
 *
 *   Abstract:	This module contains Computer Interconnect N_PORT Port Driver
 *		local port maintenance service functions.
 *
 *   Creator:	Peter Keilty	Creation Date:	July 1, 1991
 *	        This file derived from Todd Katz CI port driver.
 *
 *   Function/Routines:
 *
 *   np_crash_lport		Crash Local CI Port
 *   np_remote_reset		Reset Remote Port and System
 *   np_remote_start		Start Remote Port and System
 *   np_shutdown		Inform Known Systems of Local Shutdown
 *
 *   Modification History:
 *
 *   31-Oct-1991	Peter Keilty
 *	Ported to OFS/1
 *
 *   23-Jul-1991	Brian Nadeau
 *	New NPORT module.
 */

/* Libraries and Include Files.
 */
#include		<sys/types.h>
#include		<sys/param.h>
#include		<sys/systm.h>
#include		<dec/binlog/errlog.h>
#include                <machine/pmap.h>
#include                <machine/cpu.h>
#include		<io/dec/scs/scs.h>
#include		<io/dec/sysap/sysap.h>
#include		<io/dec/ci/cippd.h>
#include		<io/dec/np/npport.h>
#include		<io/dec/np/npadapter.h>

#ifdef __alpha
#include		<mach/error.h>
#include		<io/common/devdriver.h>
#include		<io/dec/mbox/mbox.h>
#endif

/* External Variables and Routines.
 */
extern	u_short		np_lpc_panic, np_max_reinits;
extern	NPBH		*np_alloc_pkt();
extern	PB		*cippd_get_pb();
extern	void		np_cleanup_port(), np_dealloc_pkt(), np_log_dev_attn(),
			np_log_packet();

/*   Name:	np_crash_lport	- Crash CI Local Port
 *
 *   Abstract:	This routine crashes a specific local CI port and initiates the
 *		process by which it is disabled, cleaned up, and
 *		re-initialized.  It may be sequentially invoked any number of
 *		times to crash a specific port; however, only its first
 *		invocation actually results in crashing of the port.  All
 *		subsequent invocations only log crash requests and dispose of
 *		optional buffers.  This situation persists until after the next
 *		incarnation of the port begins following completion of port
 *		clean up and re-initialization.
 *
 *		Power failure recovery of the CI7B family of CI ports( CI750,
 *		CI780, and CIBCI ) deserves special mentioning because of the
 *		potential of the ports themselves to power fail independently
 *		of the rest of the system.  An independent loss of power is
 *		detected by the port, triggers an interrupt, and results in
 *		crashing of the port( reason code == SE_POWER ) by the
 *		appropriate interrupt service routine.  The local port is
 *		disabled and cleaned up as it would be for any other event.
 *		Unfortunately, the local port can not be re-initialized while
 *		there is no power.
 *
 *		Restoration of power to the port may occur at either of the
 *		following two times:
 *
 *		1. Before port disablement and clean up completes.
 *		2. Sometime after port disablement and clean up completes.
 *
 *		At both times it is a special routine( np_unmapped_isr()) which
 *		services the interrupt triggered by detection of power
 *		restoration by the local port.  This special routine is in
 *		position to handle the interrupt instead of the routine
 *		normally employed by the local port because at the time the
 *		local port was disabled it was also unmapped as protection
 *		against extraneous machine checks.  This unmapping included
 *		switching the interrupt service handler for the local port to
 *		this special routine.
 *
 *		When the special routine is invoked it re-maps the local port.
 *		This action switches the interrupt service hander for the local
 *		port back to the routine appropriate for the local port's
 *		hardware port type.  The special routine also marks the local
 *		port as having power.  What it does next is dependent upon when
 *		power was restored to the local port.  It does nothing more if
 *		port disablement and clean up are still in progress.  Port
 *		re-initialization occurs immediately following port clean up
 *		as it would for any other event.  However, if port clean up has
 *		completed then the special routine explicitly schedules port
 *		re-initialization before dismissing the interrupt.  The end
 *		result is the same regardless of when local port power is
 *		restored, successful recovery from independent failure through
 *		re-initialization of the local port.
 *
 *		The CIBX family of CI ports( CIBCA, CIXCD ) is not subject to
 *		such Byzantine recovery scenarios.  This is because such ports
 *		are unable to detect independent losses of power; and thus, may
 *		not power fail independently of the rest of the system.
 *
 *		An additional explanation is also warranted for those instances
 *		when this routine is invoked to crash a local port and take it
 *		permanently offline.  Such instances are indicated by the local
 *		port marked broken.  The specific fatal event maybe any one of
 *		a number of codes.  The actions taken are the same as for any
 *		other event, the local port is disabled, cleaned up, and the
 *		initialization routine is scheduled.  However, the local port
 *		initialization routine detects that the local port is marked
 *		broken, aborts port re-initialization, and takes the local port
 *		permanently offline.  Furthermore, when the local port was
 *		disabled it was also unmapped because it was marked broken at
 *		the time.  This act of unmapping included switching the
 *		interrupt service handler for the broken port to a special
 *		routine, np_unmapped_isr().  Note that this is the same routine
 *		used as the interrupt service handler during power failure
 *		recovery of the CI7B family of CI ports.
 *
 *		NOTE: All error recovery is bypassed in favor of immediately
 *		      panicing the system whenever the CI configuration
 *		      variable flag np_lpc_panic is set.  Normally, this flag
 *		      is 0 and error recovery is the default action taken.  It
 *		      is set only when special debugging activity is required.
 *
 *		NOTE: The CI7B family of CI ports may report restoration of
 *		      port power without first reporting loss of it.  This
 *		      occurs when power loss is detected and signaled by the
 *		      port, but its restoration is noticed before the initial
 *		      interrupt is even received and processed.  In such cases
 *		      the appropriate interrupt service routine crashes the
 *		      local port with a reason code of SE_POWERUP, and
 *		      processing proceeds as it does for any other event.
 *		      Furthermore, the local port may always be immediately
 *		      re-initialized when the time comes to do so, because the
 *		      local port always has power unlike many instances when
 *		      the local port is crashed for power loss(
 *		      reason == SE_POWER ).
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   np_lpc_panic		- CI local port crash panic flag
 *   np_max_reinits		- CI max number consecutive reinitializations
 *   pccb			- Port Command and Control Block pointer
 *   reason			- Reason for crashing local CI port
 *   scsbp			- Address of SCS header in optional buffer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	forkb			-  PCCB fork block
 *	lpinfo.reason		-  Reason for port failure
 *	pd.gvp.type.ci		-  CI specific PCCB fields
 *	    reinit_tries	-   Number consecutive re-initializations left
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.cleanup	-   1
 *	    fsmstatus.fkip	-   1
 *	    fsmstatus.online	-   0
 *
 *   SMP:	The PCCB is locked INTERNALLY whenever it was not locked
 *		EXTERNALLY prior to routine invocation.  Locking the PCCB
 *		allows exclusive access to PCCB contents and prevents
 *		premature PB deletion.  PCCB addresses are always valid because
 *		these data structures are never deleted once their
 *		corresponding ports have been initialized.
 */
void
np_crash_lport( pccb, reason, scsbp )
    PCCB		*pccb;
    u_long		reason;
    SCSH		*scsbp;
{
    PB			*pb;
    NPBH		*cibp;
    u_long		port_state;
    u_long		log_regs = LOG_NOREGS, unlock = 0;

    /* Crashing a local CI port consists of a number of distinct steps:
     *
     * 1. The crash request is pre-processed.
     * 2. The local port is disabled.
     * 3. The local port is cleaned up.
     * 4. The local port is re-initialized.
     *
     * The first step is executed each time this routine is invoked.  It is the
     * latter three steps which constitute crashing the port and are executed
     * only once per port incarnation.  The major benefits of this single 
     * threading of port crashing are the assumptions made during port clean up
     * and re-initialization which follow directly from it.  These assumptions
     * greatly reduce code complexity and allow extension of the CI port driver
     * to an SMP environment with a minimum of pain.  They are pointed out as
     * they occur.
     *
     * Pre-processing of crash requests includes:
     *
     * 1. Locking the PCCB whenever it was not locked EXTERNALLY.
     * 2. Retrieving the appropriate PB whenever an optional buffer was
     *	  provided.
     * 3. Applying the local port crash severity modifier( ESM_LPC ) to the
     *    local port crash reason code.
     * 4. Logging the crash request according to the provided crash reason.
     * 5. Optionally panicing the system.
     * 6. Deallocating the optional buffer.
     *
     * The local port crash severity modifier is applied( Step 3 ) ONLY if the
     * local port is not already in the process of being crashed.  
     *
     * Panicing of the system( Step 5 ) immediately terminates all further
     * processing of the crash request.  Such panicing is OPTIONAL.  It is
     * requested only when the CI configuration variable flag np_lpc_panic is
     * set.  This flag is set only when special debugging activity is required.
     *
     * Following pre-processing this routine ascertains whether or not the port
     * has already been crashed but not yet re-initialized.  If this is indeed
     * the case, as indicated by a PCCB clean up in progress status flag, then
     * the current request is dismissed after the PCCB is unlocked( provided it
     * was unlocked within this routine ).  Otherwise, actual crashing of the
     * port commences with disablement of the local port including:
     *
     *  1. Setting the port clean up in progress flag to prohibit additional
     *     crashings of the port until it has been re-initialized.
     *  2. Taking the port offline.
     *  3. Ascertaining the port state from the crash reason.  Note, it is only
     *	   necessary to ascertain whether the port is currently enabled or not.
     *  4. Disabling interrupts on the port( enabled ports only ).
     *  5. Gracefully shutting down the port to abort all commands currently
     *	   undergoing processing( enabled ports only ).
     *  6. Disabling the port adapter in a hardware port type specific fashion.
     *  7. Resetting bus specific register contents( CIBCI/CIBCA/CIXCD only ).
     *  8. Disabling the port maintenance timer.
     *
     * Two additional actions are taken whenever the local port has lost power(
     * CI750/CI780/CIBCI only ) or is marked broken and is to be permanently
     * shutdown.  These two actions constitute unmapping of the local port.
     *
     *  9. Unmapping the adapter I/O space system PTEs to a black hole page to
     *	   prevent stray machine checks.
     * 10. Changing the interrupt service handler for the local port to a
     *	   special routine( np_unmapped_isr()) for fielding of all future
     *	   interrupts while the adapter I/O space is unmapped.
     *
     * The first few steps of local port disablement( Steps 1-3 ) are executed
     * by this routine.  The remaining steps( Steps 4-10 ) are accomplished
     * through indirect invocation of the appropriate CI family specific
     * local CI port disablement routine.
     *
     * At the time the decision is made to crash the port, the processor from
     * which the port is crashed exists in one of two states distinguished
     * mainly by whether the processor is at kernel mode or interrupt level.
     * The existence of these two possible environments does not interfere with
     * port disablement.  Unfortunately, the same is not necessary true for
     * port clean up, a section of the port driver which is quite complicated
     * in its own right.  Therefore, to avoid potential problems and to allow
     * certain code simplifying assumptions to be made, port clean up has been
     * structured into two stages, separated by both time and environment.
     *
     * The first stage of port clean up consists of those actions which should
     * be performed immediately following port disablement and are insensitive
     * to processor state.  The second stage consists of those activities which
     * need not be performed immediately following port disablement and should
     * be executed within a constant well-defined processor state.  The first
     * stage of port clean up is directed by this routine which then schedules
     * the second stage, directed by np_cleanup_port(), through forking.  It is
     * this act of forking which generates the constant environment necessary
     * for the second stage of port clean up.
     *
     * The PCCB fork block is used to schedule the second stage of clean up.
     * It should always be available because it is used only for port clean up
     * and re-initialization, these activities are single threaded, and
     * re-initialization always follows clean up.  Guaranteed availability
     * of the PCCB fork block is one of the benefits of single threading of
     * port clean up and re-initialization.
     *
     * The first stage of port clean up includes:
     *
     * 1. Scheduling clean up of the CI port through forking.
     * 2. Unlocking the PCCB provided it was locked within this routine.
     *
     * Note that the first stage of port clean up currently does not contain
     * any processor state insensitive actions which must be immediately
     * performed following port disablement.  However, the structure exists to
     * add them as it becomes necessary to do so.
     *
     * The second stage of port clean up includes:
     *
     * 1. Locking the PCCB.
     * 2. Removing and deallocating all packets logged out to the datagram and
     *    message buffer logout areas.  CI ports logout all internalized queue
     *	  entries to the appropriate area only when power failure is detected.
     * 3. Unlocking the PCCB.
     * 4. Notifying the CI PPD of failure of the local port.
     *
     * The CI PPD completes the clean up of its portion of the local port
     * including the failure and clean up all paths, both formative and fully
     * established, originating at the port.  Clean up of the last path
     * triggers scheduling of port re-initialization by the CI PPD.  Successful
     * completion of port re-initialization marks the beginning of the next
     * incarnation of the port and releases all restriction on crashing the
     * port if further requests to do so are made.
     */
    if( !Test_pccb_lock( pccb )) {
	Lock_pccb( pccb )
	unlock = 1;
    }
    if( scsbp ) {
	cibp =  Scs_to_pd( scsbp, pccb );
	pb = cippd_get_pb( pccb, scsbp, BUF );
    } else {
	pb = NULL;
	cibp = NULL;
    }
    if( !pccb->Fsmstatus.cleanup ) {
	Set_lpc_event( reason );
    }
    switch( Mask_esevmod( reason )) {

	case SE_MSE: 		case SE_DSE:		case SE_PARITY: 
	case SE_PORTERROR: 	case SE_MFQE:		case SE_DFQE:
	case SE_SANITYTIMER: 	case FE_NOCI: 		case FE_BADMAXPORT:
	case FE_PORTERROR:
	    log_regs = LOG_REGS;

	case FE_BADUCODE:
	    ( void )np_log_dev_attn( pccb, reason, log_regs );
	    break;

	case SE_PPDSANITY:	case SE_NOPATH:		case SE_INVLPKTSIZE:
	case SE_INVBNAME:	case SE_INVBSIZE:	case SE_INVDPORT:
	case SE_UNKCMD:		case SE_UNKSTATUS:	case SE_UNKOPCODE:
	case SE_INVOPCODE:	case SE_INVCGN: 	case SE_INVPA:
	case SE_INVSN:		case SE_INVDDL: 	case SE_IRESVCD:
	case SE_IRESEQ:		case SE_DISCVCPKT:
	    ( void )np_log_packet( pccb, pb, cibp, reason, LPORT_EVENT );
	    break;

	default:
	    ( void )panic( PANIC_UNKLPC );
    }
    if( np_lpc_panic ) {
	( void )panic( PANIC_REQLPC );
    } else if( cibp ) {
	( void )np_dealloc_pkt( cibp, pccb );
    }
    if( !pccb->Fsmstatus.cleanup ) {
	pccb->Fsmstatus.cleanup = 1;
	pccb->Fsmstatus.online = 0;
	pccb->Reinit_tries = np_max_reinits;
	switch( Mask_esevmod( reason )) {

	    case SE_MSE: 	 case SE_DSE:		case SE_PARITY:
	    case SE_PORTERROR:   case SE_SANITYTIMER:   case FE_NOCI:
	    case FE_PORTERROR:
		port_state = PS_UNINIT;
		break;

	    default:
		port_state = PS_ENAB;
	}
	( void )( *pccb->Disable_port )( pccb, port_state );
	pccb->lpinfo.reason = reason;
	Pccb_fork( pccb, np_cleanup_port, PANIC_PCCBFB )
    }
    if( unlock ) {
	Unlock_pccb( pccb )
    }
}

/*   Name:	np_remote_reset	- Reset Remote Port and System
 *
 *   Abstract:	This function initiates resetting of a remote CI port and
 *		system.  The remote system need NOT be known to be reset;
 *		however, the remote port must be in the appropriate maintenance
 *		state for it and the remote system to be reset.
 *
 *		Resetting is initiated by placing a SNDRST command packet onto
 *		the lowest priority port command queue and notifying the port
 *		when the queue was previously empty.  Execution of the SNDRST
 *		port command transmits a maintenance reset CI packet to the
 *		target port.
 *
 *		A CI command packet is used to contain the command.  It is
 *		allocated by this function and deallocated following command
 *		execution.
 *
 *		The port is crashed if the queue interlock can not be obtained.
 *
 *		Resetting of the remote port and system can optionally be
 *		forced; however, the port must in an appropriate maintenance
 *		state for this to occur.
 *
 *              NOTE: SCA port numbers are 6 bytes in size; however, maximum CI
 *                    port numbers only occupy 1 byte.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   force			- If set, force maintenance reset
 *   pccb			- Port Command and Control Block pointer
 *   rport_addr			- Station address of target remote port
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Transmission successfully initiated
 *   RET_ALLOCFAIL		- Failed to allocate command packet
 *   RET_INVLPSTATE		- Local port in invalid state
 *   RET_NOPATH			- Invalid remote port station address
 *
 *   SMP:       The PCCB is locked allowing exclusive access to PCCB contents.
 *              PCCB addresses are always valid because these data structures
 *              are never deleted once their corresponding ports have been
 *		initialized.
 *
 *              Access to port command queues is by means of memory
 *              interlocking queuing instructions.
 */
u_long
np_remote_reset( pccb, rport_addr, force )
    PCCB		*pccb;
    scaaddr		*rport_addr;
    u_long		force;
{
    NPBH		*cibp;
    SNDRSTH		*bp;
    u_long		status = RET_SUCCESS;

    /* The steps involved in resetting a remote port and system are:
     *
     * 1. Lock the PCCB.
     * 2. Verify operational status of the local port and validity of the
     *    target remote port address.
     * 3. Allocate a CI port command packet to contain the SNDRST command.
     * 4. Format the SNDRST specific portion of the command packet.
     * 5. Format the generic Vaxport header.
     * 6. Initiate resetting of the target remote port and system.
     * 7. Unlock the PCCB.
     *
     * The port is crashed if command execution can not be successfully
     * initiated.
     */
    Lock_pccb( pccb )
    if( !pccb->Fsmstatus.online ) {
	status = RET_INVLPSTATE;
    } else if(  rport_addr->low > pccb->lpinfo.Max_port ) {
	status = RET_NOPATH;
    } else if(( cibp = np_alloc_pkt( pccb ))) {
	bp = ( SNDRSTH * )Pd_to_ppd( cibp, pccb );
	bp->xctid.blockid = 0;
	U_int( bp->xctid.lconnid ) = 0;
	Format_nph( pccb, cibp, SNDPM, SNDRST, rport_addr->low, DEALLOC_BUF )
	if( force ) {
	    Freset( cibp ) = 1;
	}
	Insqt_dccq0( cibp, pccb )
    } else {
	status = RET_ALLOCFAIL;
    }
    Unlock_pccb( pccb )
    return( status );
}

/*   Name:	np_remote_start	- Start Remote Port and System
 *
 *   Abstract:	This function initiates the starting of a remote port and
 *		system.  The remote system need NOT be known to be started;
 *		however, the local port must have previously reset the remote
 *		port for it to start it.
 *
 *		Starting is initiated by placing a SNDSTRT command packet onto
 *		the lowest priority port command queue and notifying the port
 *		when the queue was previously empty.  Execution of the SNDSTRT
 *		port command transmits a maintenance start CI packet to the
 *		target port.
 *
 *		A CI command packet is used to contain the command.  It is
 *		allocated by this function and deallocated following command
 *		execution.
 *
 *		The port is crashed if the queue interlock can not be obtained.
 *
 *		A start address can be optionally supplied; otherwise, the
 *		default start address is used.
 *
 *              NOTE: SCA port numbers are 6 bytes in size; however, maximum CI
 *                    port numbers only occupy 1 byte.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *   rport_addr			- Station address of target remote port
 *   start_addr			- Start address( OPTIONAL )
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Transmission successfully initiated
 *   RET_ALLOCFAIL		- Failed to allocate command packet
 *   RET_INVLPSTATE		- Local port in invalid state
 *   RET_NOPATH			- Invalid remote port station address
 *
 *   SMP:       The PCCB is locked allowing exclusive access to PCCB contents.
 *              PCCB addresses are always valid because these data structures
 *              are never deleted once their corresponding ports have been
 *		initialized.
 *
 *              Access to port command queues is by means of memory
 *              interlocking queuing instructions.
 */
u_long
np_remote_start( pccb, rport_addr, start_addr )
    PCCB		*pccb;
    scaaddr		*rport_addr;
    u_long		start_addr;
{
    NPBH		*cibp;
    SNDSTRTH		*bp;
    u_long		status = RET_SUCCESS;

    /* The steps involved in starting a remote port and system are:
     *
     * 1. Lock the PCCB.
     * 2. Verify operational status of the local port and validity of the
     *    target remote port address.
     * 3. Allocate a CI port command packet to contain the SNDSTRT command.
     * 4. Format the SNDSTRT specific portion of the command packet.
     * 5. Format the generic Vaxport header.
     * 6. Initiate starting of the target remote port and system.
     * 7. Unlock the PCCB.
     *
     * The port is crashed if command execution can not be successfully
     * initiated.
     */
    Lock_pccb( pccb )
    if( !pccb->Fsmstatus.online ) {
	status = RET_INVLPSTATE;
    } else if(  rport_addr->low > pccb->lpinfo.Max_port ) {
	status = RET_NOPATH;
    } else if(( cibp = np_alloc_pkt( pccb ))) {
	bp = ( SNDSTRTH * )Pd_to_ppd( cibp, pccb );
	bp->xctid.blockid = 0;
	U_int( bp->xctid.lconnid ) = 0;
	bp->start_addr = start_addr;
	Format_nph( pccb, cibp, SNDPM, SNDSTRT, rport_addr->low, DEALLOC_BUF )
	if( start_addr == 0 ) {
	    Dstart( cibp ) = 1;
	}
	Insqt_dccq0( cibp, pccb )
    } else {
	status = RET_ALLOCFAIL;
    }
    Unlock_pccb( pccb )
    return( status );
}

/*   Name:	np_shutdown	- Inform Known Systems of Local Shutdown
 *
 *   Abstract:	This routine informs systems known through paths originating
 *		at a specific local CI port of local system shutdown.  This is
 *		accomplished through disabling of the specified port.  The
 *		remote system discovers the unavailability of the port either
 *		during CI PPD polling or during utilization of the virtual
 *		circuit.  At that time it terminates its path to the port and
 *		and thus to the local system.
 *
 *		NOTE: STOP CI PPD datagrams are not transmitted across all
 *		      formative and established paths.  This is because the
 *		      state of the port is unknown and may in fact preclude
 *		      datagram transmission.
 *
 *   Inputs:
 *
 *   IPL_POWER			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *
 *   Outputs:
 *
 *   IPL_POWER			- Interrupt processor level
 *   pccb			- Port Command and Control Block pointer
 *	ppd.cippd		-  CI PPD specific PCCB fields
 *	    fsmstatus.broken	-   1
 *	    fsmstatus.online	-   0
 *
 *   SMP:       No locks are required.  This routine is only called(
 *		indirectly ) from panic() and it is invoked at maximum IPL.
 *		Only the processor executing this code is operational and this
 *              routine can not be interrupted on this processor.  This
 *              guarantees uncompromised access to the PCCB without locking it.
 */
void
np_shutdown( pccb )
    PCCB		*pccb;
{
    /* Local port disablement is accomplished through indirect invocation of
     * the appropriate CI family specific local CI port disablement routine.
     * The local port is marked broken prior to disablement to force unmapping
     * of the local port adapter I/O space.
     *
     * Prior to disablement the local port is also taken explicitly offline.
     * This aborts all CI PPD port polling on the local port.
     *
     * Only mapped local ports are disabled.  Unmapped local ports have already
     * been completely disabled including unmapping of the local port adapter
     * I/O space.
     */
    if( pccb->Lpstatus.mapped ) {
	pccb->Fsmstatus.broken = 1;
	pccb->Fsmstatus.online = 0;
	( void )( *pccb->Disable_port )( pccb, PS_UNINIT );
    }
}
