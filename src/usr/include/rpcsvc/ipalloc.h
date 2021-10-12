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
 *	@(#)$RCSfile: ipalloc.h,v $ $Revision: 4.2 $ (DEC) $Date: 1991/09/20 05:20:32 $
 */ 
/*
 */
/*
 * OSF/1 Release 1.0
 */

/*
 * Portions of this software have been licensed to
 * Digital Equipment Company, Maynard, MA.
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <rpc/types.h>

#define IPALLOC_TIMEOUT 86400
#define MAX_MACHINELEN 256

enum ip_status {
	ip_success = 0,
	ip_failure = 0 + 1,
	ip_rpc = 0 + 2,
	ip_no_addresses = 0 + 3,
	ip_no_system = 0 + 4,
	ip_no_priv = 0 + 5,
};
typedef enum ip_status ip_status;
bool_t xdr_ip_status();

struct ip_alloc_arg {
	char etheraddr[6];
	u_long netnum;
	u_long subnetmask;
};
typedef struct ip_alloc_arg ip_alloc_arg;
bool_t xdr_ip_alloc_arg();

struct ip_alloc_res {
	ip_status status;
	union {
		u_long ipaddr;
	} ip_alloc_res_u;
};
typedef struct ip_alloc_res ip_alloc_res;
bool_t xdr_ip_alloc_res();

struct ip_addr_arg {
	u_long ipaddr;
};
typedef struct ip_addr_arg ip_addr_arg;
bool_t xdr_ip_addr_arg();

struct ip_toname_res {
	ip_status status;
	union {
		char *name;
	} ip_toname_res_u;
};
typedef struct ip_toname_res ip_toname_res;
bool_t xdr_ip_toname_res();

#define IPALLOC_PROG ((u_long)100042)
#define IPALLOC_VERS ((u_long)2)
#define IP_ALLOC ((u_long)1)
extern ip_alloc_res *ip_alloc_2();
#define IP_TONAME ((u_long)2)
extern ip_toname_res *ip_toname_2();
#define IP_FREE ((u_long)3)
extern ip_status *ip_free_2();
