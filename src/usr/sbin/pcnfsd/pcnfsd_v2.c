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
static char *rcsid = "@(#)$RCSfile: pcnfsd_v2.c,v $ $Revision: 1.1.2.4 $ (DEC) $Date: 1992/11/13 17:21:05 $";
#endif
/* RE_SID: @(%)/tmp_mnt/vol/dosnfs/shades_SCCS/unix/pcnfsd/v2/src/SCCS/s.pcnfsd_v2.c 1.6 92/08/18 12:54:11 SMI */
/*
**=====================================================================
** Copyright (c) 1986,1987,1988,1989,1990,1991 by Sun Microsystems, Inc.
**	@(#)pcnfsd_v2.c	1.6	8/18/92
**=====================================================================
*/
#include "common.h"
/*
**=====================================================================
**             I N C L U D E   F I L E   S E C T I O N                *
**                                                                    *
** If your port requires different include files, add a suitable      *
** #define in the customization section, and make the inclusion or    *
** exclusion of the files conditional on this.                        *
**=====================================================================
*/
#include "pcnfsd.h"

#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <string.h>

#ifdef USE_YP
#include <rpcsvc/ypclnt.h>
#endif

#ifndef SYSV
#include <sys/wait.h>
#endif

#ifdef ISC_2_0
#include <sys/fcntl.h>
#endif

#ifdef SHADOW_SUPPORT
#include <shadow.h>
#endif

#ifdef SIA
#include <sia.h>
#endif

/*
**---------------------------------------------------------------------
** Other #define's 
**---------------------------------------------------------------------
*/

void            fillin_extra_groups();
extern char    *my_strdup();
extern void     scramble();
extern void    *grab();
extern char    *crypt();
extern int      build_pr_list();
extern void	mon_printers();
extern pirstat  build_pr_queue();
extern psrstat  pr_start();
extern psrstat  pr_start2();
extern pcrstat  pr_cancel();
extern pirstat  get_pr_status();

extern struct passwd  *get_password();

#ifdef WTMP
extern void wlogin();
#endif

#ifdef USE_YP
char *find_entry();
#endif

/*
**---------------------------------------------------------------------
**                       Misc. variable definitions
**---------------------------------------------------------------------
*/

extern pr_list         printers;
extern pr_queue        queue;

/*
**=====================================================================
**                      C O D E   S E C T I O N                       *
**=====================================================================
*/

#ifdef SIA
static SIAENTITY *entity=NULL;
static int oargc = 1;
static char *oargv[1] = { "rpc.pcnfsd" };
static int (*sia_collect)()=sia_collect_trm;
#endif

static char no_comment[] = "No comment";
static char not_supported[] = "Not supported";
static char pcnfsd_version[] = "@(#)pcnfsd_v2.c	1.6 - rpc.pcnfsd V2.0 (c) 1991 Sun Technology Enterprises, Inc.";

/*ARGSUSED*/
void *pcnfsd2_null_2(arg)
void*arg;
{
static char dummy;
return((void *)&dummy);
}

v2_auth_results *pcnfsd2_auth_2(arg)
v2_auth_args *arg;
{
static v2_auth_results  r;

char            uname[32];
char            pw[64];
int             c1, c2;
#ifdef SIA
struct passwd	p_instance;
struct passwd	*p = (struct passwd *) &p_instance;
#else
struct passwd  *p;
#endif /* SIA */
static u_int           extra_gids[EXTRAGIDLEN];
static char     home[256];
char	       *msgp = NULL;
#ifdef USE_YP
char           *yphome;
char           *cp;
#endif /*USE_YP*/


	r.stat = AUTH_RES_FAIL;	/* assume failure */
	r.uid = (int)-2;
	r.gid = (int)-2;
	r.cm = &no_comment[0];
	r.gids.gids_len = 0;
	r.gids.gids_val = &extra_gids[0];
	home[0] = '\0';
	r.home = &home[0];
	r.def_umask = umask(0);
	(void)umask(r.def_umask);	/* or use 022 */

	scramble(arg->id, uname);
	scramble(arg->pw, pw);

#ifdef USER_CACHE
	if(check_cache(uname, pw, &r.uid, &r.gid)) {
		 r.stat = AUTH_RES_OK;
#ifdef WTMP
		wlogin(uname);
#endif
                 fillin_extra_groups
			(uname, r.gid, &r.gids.gids_len, extra_gids);
#ifdef USE_YP
		yphome = find_entry(uname, "auto.home");
		if(yphome) {
			strcpy(home, yphome);
			free(yphome);
			cp = strchr(home, ':');
			cp++;
			cp = strchr(cp, ':');
			if(cp)
				*cp = '/';
		}
#endif
		 return (&r);
   }
#endif

#ifdef SIA
        if (sia_ses_init(&entity,oargc,oargv,NULL,uname,NULL,FALSE,NULL) == SIASUCCESS) {
		if ((sia_ses_authent(sia_collect,pw,entity)) == SIASUCCESS) {
			if ((sia_ses_estab(sia_collect,entity)) == SIASUCCESS) {
				r.stat = AUTH_RES_OK;
				p->pw_uid = entity->pwd->pw_uid;
				p->pw_gid = entity->pwd->pw_gid;
				p->pw_passwd = entity->pwd->pw_passwd;
				p->pw_name = entity->pwd->pw_name;
				r.uid = p->pw_uid;
				r.gid = p->pw_gid;
				sia_ses_release(&entity);
			}
			else {
			    return(&r);
		    }
		}
		else {
		    return(&r);
	    }
	}
	else {
		return(&r);
	}
#else /* not SIA */
	p = get_password(uname, &msgp);
	if(msgp)
		r.cm = msgp;
	if (p == (struct passwd *)NULL)
	   return (&r);

	c1 = strlen(pw);
	c2 = strlen(p->pw_passwd);
	if ((c1 && !c2) || (c2 && !c1) ||
	   (strcmp(p->pw_passwd, crypt(pw, p->pw_passwd)))) 
           {
	   return (&r);
	   }
	r.stat = AUTH_RES_OK;
	r.uid = p->pw_uid;
	r.gid = p->pw_gid;
#endif /* SIA */

#ifdef WTMP
	wlogin(uname);
#endif
        fillin_extra_groups(uname, r.gid, &r.gids.gids_len, extra_gids);

#ifdef USE_YP
	yphome = find_entry(uname, "auto.home");
	if(yphome) {
		strcpy(home, yphome);
		free(yphome);
		cp = strchr(home, ':');
		cp++;
		cp = strchr(cp, ':');
		if(cp)
			*cp = '/';
	}
#endif

#ifdef USER_CACHE
	add_cache_entry(p);
#endif

return(&r);

}

v2_pr_init_results *pcnfsd2_pr_init_2(arg)
v2_pr_init_args *arg;
{
static v2_pr_init_results res;

	res.stat = 
	 (pirstat) pr_init(arg->system, arg->pn, &res.dir);
	res.cm = &no_comment[0];


return(&res);
}

v2_pr_start_results *pcnfsd2_pr_start_2(arg)
v2_pr_start_args *arg;
{
static v2_pr_start_results res;

	res.stat =
	  (psrstat) pr_start2(arg->system, arg->pn, arg->user,
	  arg ->file, arg->opts, &res.id);
	res.cm = &no_comment[0];

return(&res);
}

/*ARGSUSED*/
v2_pr_list_results *pcnfsd2_pr_list_2(arg)
void *arg;
{
static v2_pr_list_results res;

	mon_printers();
	if(printers == NULL)
		(void)build_pr_list();
	res.cm = &no_comment[0];
	res.printers = printers;

return(&res);
}

v2_pr_queue_results *pcnfsd2_pr_queue_2(arg)
v2_pr_queue_args *arg;
{
static v2_pr_queue_results res;

	res.stat = build_pr_queue(arg->pn, arg->user,
		arg->just_mine, &res.qlen, &res.qshown);
	res.cm = &no_comment[0];
	res.just_yours = arg->just_mine;
	res.jobs = queue;
	

return(&res);
}

v2_pr_status_results *pcnfsd2_pr_status_2(arg)
v2_pr_status_args *arg;
{
static v2_pr_status_results res;
static char status[128];

	res.stat = get_pr_status(arg->pn, &res.avail, &res.printing,
		&res.qlen, &res.needs_operator, &status[0]);
	res.status = &status[0];	
	res.cm = &no_comment[0];

return(&res);
}

v2_pr_cancel_results *pcnfsd2_pr_cancel_2(arg)
v2_pr_cancel_args *arg;
{
static v2_pr_cancel_results res;

	res.stat = pr_cancel(arg->pn, arg->user, arg->id);
	res.cm = &no_comment[0];

return(&res);
}

/*ARGSUSED*/
v2_pr_requeue_results *pcnfsd2_pr_requeue_2(arg)
v2_pr_requeue_args *arg;
{
static v2_pr_requeue_results res;
	res.stat = PC_RES_FAIL;
	res.cm = &not_supported[0];

return(&res);
}

/*ARGSUSED*/
v2_pr_hold_results *pcnfsd2_pr_hold_2(arg)
v2_pr_hold_args *arg;
{
static v2_pr_hold_results res;

	res.stat = PC_RES_FAIL;
	res.cm = &not_supported[0];

return(&res);
}

/*ARGSUSED*/
v2_pr_release_results *pcnfsd2_pr_release_2(arg)
v2_pr_release_args *arg;
{
static v2_pr_release_results res;

	res.stat = PC_RES_FAIL;
	res.cm = &not_supported[0];

return(&res);
}

/*ARGSUSED*/
v2_pr_admin_results *pcnfsd2_pr_admin_2(arg)
v2_pr_admin_args *arg;
{
static v2_pr_admin_results res;
/*
** The default action for admin is to fail.
** If someone wishes to implement an administration
** mechanism, and isn't worried about the security
** holes, go right ahead.
*/

	res.cm = &not_supported[0];
	res.stat = PI_RES_FAIL;

return(&res);
}

void
free_mapreq_results(p)
mapreq_res p;
{
	if(p->mapreq_next)
		free_mapreq_results(p->mapreq_next); /* recurse */
	if(p->name)
		(void)free(p->name);
	(void)free(p);
	return;
}


v2_mapid_results *pcnfsd2_mapid_2(arg)
v2_mapid_args *arg;
{
static v2_mapid_results res;
struct passwd *p_passwd;
struct group  *p_group;

mapreq_arg a;
mapreq_res next_r;
mapreq_res last_r = NULL;


	if(res.res_list) {
		free_mapreq_results(res.res_list);
		res.res_list = NULL;
	}

	a = arg->req_list;
	while(a) {
		next_r = (struct mapreq_res_item *)
			grab(sizeof(struct mapreq_res_item));
		next_r->stat = MAP_RES_UNKNOWN;
		next_r->req = a->req;
		next_r->id = a->id;
		next_r->name = NULL;
		next_r->mapreq_next = NULL;

		if(last_r == NULL)
			res.res_list = next_r;
		else
			last_r->mapreq_next = next_r;
		last_r = next_r;
		switch(a->req) {
		case MAP_REQ_UID:
			p_passwd = getpwuid((uid_t)a->id);
			if(p_passwd) {
				next_r->name = my_strdup(p_passwd->pw_name);
				next_r->stat = MAP_RES_OK;
			}
			break;
		case MAP_REQ_GID:
			p_group = getgrgid((gid_t)a->id);
			if(p_group) {
				next_r->name = my_strdup(p_group->gr_name);
				next_r->stat = MAP_RES_OK;
			}
			break;
		case MAP_REQ_UNAME:
			next_r->name = my_strdup(a->name);
			p_passwd = getpwnam(a->name);
			if(p_passwd) {
				next_r->id = p_passwd->pw_uid;
				next_r->stat = MAP_RES_OK;
			}
			break;
		case MAP_REQ_GNAME:
			next_r->name = my_strdup(a->name);
			p_group = getgrnam(a->name);
			if(p_group) {
				next_r->id = p_group->gr_gid;
				next_r->stat = MAP_RES_OK;
			}
			break;
		}
		if(next_r->name == NULL)
			next_r->name = my_strdup("");
		a = a->mapreq_next;
	}

	res.cm = &no_comment[0];

return(&res);
}

	
/*ARGSUSED*/
v2_alert_results *pcnfsd2_alert_2(arg)
v2_alert_args *arg;
{
static v2_alert_results res;

	res.stat = ALERT_RES_FAIL;
	res.cm = &not_supported[0];

return(&res);
}

/*ARGSUSED*/
v2_info_results *pcnfsd2_info_2(arg)
v2_info_args *arg;
{
static v2_info_results res;
static int facilities[FACILITIESMAX];
static int onetime = 1;

#define UNSUPPORTED -1
#define QUICK 100
#define SLOW 2000

	if(onetime) {
		onetime = 0;
		facilities[PCNFSD2_NULL] = QUICK;
		facilities[PCNFSD2_INFO] = QUICK;
		facilities[PCNFSD2_PR_INIT] = QUICK;
		facilities[PCNFSD2_PR_START] = SLOW;
		facilities[PCNFSD2_PR_LIST] = QUICK; /* except first time */
		facilities[PCNFSD2_PR_QUEUE] = SLOW;
		facilities[PCNFSD2_PR_STATUS] = SLOW;
		facilities[PCNFSD2_PR_CANCEL] = SLOW;
		facilities[PCNFSD2_PR_ADMIN] = UNSUPPORTED;
		facilities[PCNFSD2_PR_REQUEUE] = UNSUPPORTED;
		facilities[PCNFSD2_PR_HOLD] = UNSUPPORTED;
		facilities[PCNFSD2_PR_RELEASE] = UNSUPPORTED;
		facilities[PCNFSD2_MAPID] = QUICK;
		facilities[PCNFSD2_AUTH] = QUICK;
		facilities[PCNFSD2_ALERT] = QUICK;
	}
	res.facilities.facilities_len = PCNFSD2_ALERT+1;
	res.facilities.facilities_val = facilities;
	
	res.vers = &pcnfsd_version[0];
	res.cm = &no_comment[0];

return(&res);
}



void
fillin_extra_groups(uname, main_gid, len, extra_gids)
char *uname;
u_int main_gid;
int *len;
u_int extra_gids[EXTRAGIDLEN];
{
struct group *grp;
char **members;
int n = 0;

	setgrent();

	while(n < EXTRAGIDLEN) {
		grp = getgrent();
		if(grp == NULL)
			break;
		if(grp->gr_gid == main_gid)
			continue;
		for(members = grp->gr_mem; members && *members; members++) {
			if(!strcmp(*members, uname)) {
				extra_gids[n++] = grp->gr_gid;
				break;
			}
		}
	}
	endgrent();
	*len = n;
}

#ifdef USE_YP
/* the following is from rpcsvc/yp_prot.h */
#define YPMAXDOMAIN 64
/*
 * find_entry returns NULL on any error (printing a message) and
 * otherwise returns a pointer to the malloc'd result. The caller
 * is responsible for free()ing the result string.
 */
char *
find_entry(key, map)
char *key;
char *map;
{
	int err;
	char *val = NULL;
	char *cp;
	int len = 0;
	static char domain[YPMAXDOMAIN+1];

	if(getdomainname(domain, YPMAXDOMAIN) ) {
#ifdef DEBUG
		msg_out("rpc.pcnfsd: getdomainname failed");
#endif
		return(NULL);
	}

	if (err = yp_bind(domain)) {
#ifdef DEBUG
		msg_out("rpc.pcnfsd: yp_bind failed");
#endif
		return(NULL);
	}

	err = yp_match(domain, map, key, strlen(key), &val, &len);

	if (err) {
#ifdef DEBUG
		msg_out("rpc.pcnfsd: yp_match failed");
#endif
		return(NULL);
	}

	if(cp = strchr(val, '\n'))
			*cp = '\0';		/* in case we get an extra NL at the end */
	return(val);
}

#endif
