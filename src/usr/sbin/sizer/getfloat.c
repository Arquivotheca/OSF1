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
static char     *sccsid = "@(#)$RCSfile: getfloat.c,v $ $Revision: 4.2.2.2 $ (DEC) $Date: 1992/04/28 15:54:27 $";
#endif lint
/************************************************************************
 *									*
 *			Copyright(c) 1987 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/


/************************************************************************
 *
 * Name: getfloat.c
 *
 * Modification History
 * 
 * Jun 5, 1991 - jaa
 *	ported to OSF/1.  modified to use new bus/controller/device
 *	kernel structures.  in porting, we don't use /dev/kmem anymore.
 *	getsysinfo(3) was modified to return the needed information.
 *	NOTE: ifdef TODO's in this code will have to be done as the 
 *	pieces are added (i.e. floating csr space, workstation specifics)
 *
 * Oct 11, 1989 - Alan Frechette
 *	Fixed sizer to assume no drives attached to a disk controller
 *	in floating address space. Previously sizer would assume that
 *	there are 4 drives attached which is wrong in most cases.
 *
 * July 6, 1989 - Alan Frechette
 * 	Added check for a VAXSTAR cputype.
 *
 * May 02, 1989 - Alan Frechette
 *	Changes to deal with new unique "cpu" handling for both
 *	vax and mips architectures.
 *
 * Feb 12, 1989 - Alan Frechette
 *	New sizer code which supports multiple architectures.
 *	Restructured this code and cleaned it up considerably.
 *	Based on the original V3.0 sizer by Tungning Cherng.
 *
 ***********************************************************************/

#include "sizer.h"

#define QB_IO_OFFSET		0x400000

/****************************************************************
*  Name:	getfloat					*
*    								*
*  Abstract:	Search all the UNIBUS ADAPTERS for devices that	*
*		exist in floating address space. Add all found	*
*		devices to the alive device table "adltbl[]".	*
*								*
*  Inputs:							*
*  ubas		The number of UNIBUS ADAPTERS for the system.	*
*								*
*  Outputs:	None.						*
*								*
*  Return Values: None.						*
*								*
*  Side Effects:This routine uses an algorithm which guesses	*
*		where certain devices can exist in floating	*
*		address space. The algorithm goes through the	*
*		floating devices table "floattbl[]" and gets 	*
*		the offset of where this device may exist in 	*
*		floating space. It then pokes at that address	*
*		to see if that device exists there. If it does	*
*		exist there then it will add the device to the	*
*		alive device table "adltbl[]". The following	*
*		routines are called:				*	
*								*
* match_device_name()	Search device name in device table.	*
* foundit()		Find if device is in alive device table.*
* float_ctlr()		Handle a floating controller device.	*
* getunit()		Get unit number for the device.		*
****************************************************************/
getfloat(ubas)
int ubas;
{

	char tconname[10];
	int unit, addr, retval, i, x, index;
	int tcsr, kumem, umem_start, tconnum, tdevtype;
	short shortint;
	struct stat stbuf;
	struct float_devices *fadptr;

	/* No UNIBUS/QBUS virtual memory interface */
	if(stat("/dev/kUmem", &stbuf) != 0)
		return;

	/* Open up UNIBUS/QBUS virtual memory interface */
	if((kumem = open("/dev/kUmem", 0)) < 0)
		quitonerror(-9);

	/* Get start of UNIBUS/QBUS memory based on CPU type */
	switch(Cpu) {
	case DS_5400:
	case DS_5500:
	case DS_5800:
		umem_start = nl[NL_qmem].n_value;/* Get start of qmem */
		break;
	default:
		umem_start = nl[NL_umem].n_value;/* Get start of umem */
		break;
	}
	
	/* Search all UBA's for floating address devices */
	for(i=0;i<ubas;i++) {
		/* Get start of floating address space */
		switch(Cpu) {
		case DS_5400:
		case DS_5500:
		case DS_5800:
			addr = umem_start + QB_IO_OFFSET;
			break;
		default:
			addr = umem_start + (i * 01000000) + 0760000;
			break;
		}

		/* Point to floating address devices table */
		fadptr = floattbl;
		addr += fadptr->gap * 2;
		while(strcmp(fadptr->name,"\0") != 0) {

			/* Poke to see if device exists at this address */
		    	lseek(kumem,addr,0);
		    	retval = read(kumem,&shortint,sizeof(shortint));
		    	unit = 0;
		    	while(retval > 0) {
				/* Find device in configuration device table */
				index = match_device_name(fadptr->name);
				strcpy(tconname,"uba");
				tconnum = i;
				tdevtype = (index == -1) ? 
					UNKNOWN : devtbl[index].devtype;

				/* Figure out this device's CSR address */
				switch(Cpu) {
				case DS_5400:
				case DS_5500:
				case DS_5800:
				    tcsr = (u_short)
					(addr - umem_start - QB_IO_OFFSET);
				    break;
				default:
				    tcsr = (u_short)
				        (addr - umem_start - 0600000 -
							(i * 01000000));
				    break;
				}
				/*
				 * Check if this device is already in
				 * the alive device table "adltbl[]".
				 */
				if(!foundit(tcsr,tconname,tconnum,tdevtype)) {
				    init_adl_entry(Adlindex);
				    adltbl[Adlindex].device_index = index;
				    strcpy(adltbl[Adlindex].conn_name,tconname);
				    adltbl[Adlindex].conn_number = i;

				    if(index == -1) {
					strcpy(adltbl[Adlindex].unsupp_devname,
							fadptr->name);
				  	adltbl[Adlindex].csr = tcsr;
				        Adlindex++;
				    }
				    else {
					if(devtbl[index].devtype != CONTROLLER)
				  	    adltbl[Adlindex].csr = tcsr;

					if(devtbl[index].supportedflag) {
					    unit = getunit(fadptr->name);
					    adltbl[Adlindex].device_unit = unit;
				        }

				        Adlindex++;
					if(devtbl[index].devtype == CONTROLLER)
					    float_ctlr(addr,umem_start,
						unit,devtbl[index].devname);
				    }
				    unit++;
				}
				addr += fadptr->gap * 2;
				x = (addr - umem_start) % (fadptr->gap * 2);
				if(x > 0)
					addr += fadptr->gap * 2 - x;
				lseek(kumem,addr,0);
				retval = read(kumem,&shortint,sizeof(shortint));
			}
			fadptr++;
			if(fadptr->gap == 0)
				continue;
			x = (addr - umem_start) % (fadptr->gap * 2);
			if(x > 0)
				addr += (fadptr->gap * 2) - x;
			else
				addr += fadptr->gap * 2;
		}
	}
}

/****************************************************************
*  Name:	foundit						*
*    								*
*  Abstract:	Check alive device table to see if this device 	*
*		has already been added. Devices are straight 	*
*		forward, but controllers are tricky. For the	*
*		controllers we must first find a device with 	*
*		the same CSR as ours and then check the device 	*
*		before it (i - 1) to see if it is on the same 	*
*		bus as we are. This depends on the fact that 	*
*		the order of things in the alive device table 	*
*		are as follows:					*
*								*
*	(1) uda on uba   (2) uq on uda at csr   (3) ra on uq	*
*								*
*  Inputs:							*
*  csr		The csr of the device to look for.		*
*  connectname	The device connection point name to look for.	*
*  connectnum	The device connection point number to look for.	*
*  devtype	The device type from device table "devtbl[]".	*
*								*
*  Outputs:	None.						*
*								*
*  Return Values: 						*
*  1		Device is in the alive device table.  		*
*  0		Device is not in the alive device table.  	*
****************************************************************/
foundit(csr,connectname,connectnum,devtype)
int csr;
char *connectname;
int connectnum;
int devtype;
{
	int i, found;

	found = 0;
	for(i=0;i<Adlindex;i++) {
	    if(adltbl[i].csr == csr) {
		if(strcmp(adltbl[i].conn_name,connectname) == 0 &&
			adltbl[i].conn_number == connectnum) {
		    found = 1;
		    break;
		}
		else if(devtype == CONTROLLER) {
		    if(strcmp(adltbl[i - 1].conn_name, connectname) == 0 &&
			adltbl[i - 1].conn_number == connectnum) {
			found = 1;
			break;
		    }
		}
	    }
	}
	return(found);
}

/****************************************************************
*  Name:	float_ctlr					*
*    								*
*  Abstract:	We found a controller in floating address space.*
*		Figure out the type of controller and how many 	*
*		devices are attached to	this controller. Add 	*
*		the devices to the alive device table. This 	*
*		code assumes that a default number of devices	*
*		are attached to the controller.			*
*								*
*  Inputs:							*
*  addr		The address of the device in floating space.	*
*  umem_start	The start of UNIBUS/QBUS floating address space.*
*  unit		The controller device unit number.		*
*  name		The controller device name.			*
*								*
*  Outputs:	None.						*
*								*
*  Return Values: None.						*
****************************************************************/
float_ctlr(addr,umem_start,unit,name)
int addr,umem_start,unit;
char name[20];
{
	int ctlrunit, j, devunit, index;

	init_adl_entry(Adlindex);
	if(strncmp(name,"uda",3) == 0) {
		ctlrunit = getunit("uq");
		index = match_device_name("uq");
		adltbl[Adlindex].device_index = index;
		adltbl[Adlindex].device_unit = ctlrunit;
		adltbl[Adlindex].csr =(u_short)(addr - 0600000 - umem_start);
		strcpy(adltbl[Adlindex].conn_name, "uda");
		adltbl[Adlindex].conn_number = unit;
		Adlindex++;
#ifdef notdef
		devunit = getunit("ra");		
		for(j=0;j<4;j++) {
			init_adl_entry(Adlindex);
			index = match_device_name("ra");
			adltbl[Adlindex].device_index = index;
			adltbl[Adlindex].device_unit = devunit;
			adltbl[Adlindex].device_drive = j;
			strcpy(adltbl[Adlindex].conn_name, "uq");
			adltbl[Adlindex].conn_number = ctlrunit;
			devunit++;
			Adlindex++;
		}
#endif notdef
	}
	if(strncmp(name,"klesiu",6) == 0) {
		ctlrunit = getunit("uq");
		index = match_device_name("uq");
		adltbl[Adlindex].device_index = index;
		adltbl[Adlindex].device_unit = ctlrunit;
		adltbl[Adlindex].csr =(u_short)(addr - 0600000 - umem_start);
		strcpy(adltbl[Adlindex].conn_name, "klesiu");
		adltbl[Adlindex].conn_number = unit;
		Adlindex++;
		init_adl_entry(Adlindex);
		devunit = getunit("tms");
		index = match_device_name("tms");
		adltbl[Adlindex].device_index = index;
		adltbl[Adlindex].device_unit = devunit;
		adltbl[Adlindex].device_drive = 0;
		strcpy(adltbl[Adlindex].conn_name, "uq");
		adltbl[Adlindex].conn_number = ctlrunit;
		Adlindex++;
	}		
	if(strncmp(name,"hl",2) == 0) {
		devunit = getunit("rl");
		for(j=0;j<4;j++) {
			init_adl_entry(Adlindex);
			index = match_device_name("rl");
			adltbl[Adlindex].device_index = index;
			adltbl[Adlindex].device_unit = devunit;
			adltbl[Adlindex].device_drive = j;
			strcpy(adltbl[Adlindex].conn_name, "hl");
			adltbl[Adlindex].conn_number = unit;
			Adlindex++;
			devunit++;
		}
	}
	if(strncmp(name,"fx",2) == 0) {
		devunit = getunit("rx");
		for(j=0;j<2;j++) {
			init_adl_entry(Adlindex);
			index = match_device_name("rx");
			adltbl[Adlindex].device_index = index;
			adltbl[Adlindex].device_unit = devunit;
			adltbl[Adlindex].device_drive = j;
			strcpy(adltbl[Adlindex].conn_name, "fx");
			adltbl[Adlindex].conn_number = unit;
			Adlindex++;
			devunit++;
		}
	}
}

/****************************************************************
*  Name:	getunit						*
*    								*
*  Abstract:	Find the next available unit number for the 	*
*		given device name. We seach through the alive 	*
*		device table to find out if any devices match 	*
*		the device name. If so we keep track of the 	*
*		highest unit number for that device name and 	*
*		use the next available one for our floating 	*
*		device.						*
*								*
*  Inputs:							*
*  name		The name of the device to search for.		*
*								*
*  Outputs:	None.						*
*								*
*  Return Values: 						*
*  unit		The next available unit number for the device.	*
****************************************************************/
getunit(name)
char *name;
{

	int i, index, newunit, unit, len, found;

	newunit = unit = found = 0;
	len = strlen(name);
	for(i=0;i<Adlindex;i++) {
		index = adltbl[i].device_index;
		if((strncmp(name,devtbl[index].devname,len) == 0) &&
		    			adltbl[i].device_unit != -1) {
			found = 1;
			newunit = adltbl[i].device_unit;
			if(newunit > unit)
				unit = newunit;
		}
	}
	if(found > 0)
		unit++;
	return(unit);
}
