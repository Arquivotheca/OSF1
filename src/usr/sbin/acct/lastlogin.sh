#! /bin/sh
# 
# *****************************************************************
# *                                                               *
# *    Copyright (c) Digital Equipment Corporation, 1991, 1994    *
# *                                                               *
# *   All Rights Reserved.  Unpublished rights  reserved  under   *
# *   the copyright laws of the United States.                    *
# *                                                               *
# *   The software contained on this media  is  proprietary  to   *
# *   and  embodies  the  confidential  technology  of  Digital   *
# *   Equipment Corporation.  Possession, use,  duplication  or   *
# *   dissemination of the software and media is authorized only  *
# *   pursuant to a valid written license from Digital Equipment  *
# *   Corporation.                                                *
# *                                                               *
# *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
# *   by the U.S. Government is subject to restrictions  as  set  *
# *   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
# *   or  in  FAR 52.227-19, as applicable.                       *
# *                                                               *
# *****************************************************************
#
# HISTORY
# 
#	@(#)lastlogin.sh	3.1	(ULTRIX/OSF)	2/26/91
# *****************************************************************
# *                                                               *
# *    Copyright (c) Digital Equipment Corporation, 1991, 1994    *
# *                                                               *
# *   All Rights Reserved.  Unpublished rights  reserved  under   *
# *   the copyright laws of the United States.                    *
# *                                                               *
# *   The software contained on this media  is  proprietary  to   *
# *   and  embodies  the  confidential  technology  of  Digital   *
# *   Equipment Corporation.  Possession, use,  duplication  or   *
# *   dissemination of the software and media is authorized only  *
# *   pursuant to a valid written license from Digital Equipment  *
# *   Corporation.                                                *
# *                                                               *
# *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
# *   by the U.S. Government is subject to restrictions  as  set  *
# *   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
# *   or  in  FAR 52.227-19, as applicable.                       *
# *                                                               *
# *****************************************************************
#
# (c) Copyright 1990, OPEN SOFTWARE FOUNDATION, INC.
# ALL RIGHTS RESERVED
#
#
# OSF/1 Release 1.0

# 

# 
# 	 COMPONENT_NAME: (CMDACCT) Command Accounting
# 
# 	 FUNCTIONS: none
# 
# 	 ORIGINS: 3, 9, 27
# 
# 	 (C) COPYRIGHT International Business Machines Corp. 1985, 1989
# 	 All Rights Reserved
# 	 Licensed Materials - Property of IBM
# 
# 	 US Government Users Restricted Rights - Use, duplication or
# 	 disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
# 
# 	lastlogin.sh	1.2  com/cmd/acct,3.1,8943 10/24/89 10:56:23
#
# 	"lastlogin - keep record of date each person last logged in"
# 	"bug - the date shown is usually 1 more than it should be "
# 	"       because lastlogin is run at 4am and checks the last"
# 	"       24 hrs worth of process accounting info (in pacct)"
# 	"cleanup loginlog - delete entries of those no longer in"
# 	"/etc/passwd and add an entry for those recently added"
# 	"line 1 - get file of current logins in same form as loginlog"
# 	"line 2 - merge the 2 files; use uniq to delete common"
# 	"lines resulting in those lines which need to be"
# 	"deleted or added from loginlog"
# 	"line 3 - result of sort will be a file with 2 copies"
# 	"of lines to delete and 1 copy of lines that are "
# 	"valid; use uniq to remove duplicate lines"
# 

PATH=/usr/sbin/acct:/bin:/usr/bin:/sbin
export PATH
cd ${ACCTDIR:-/var/adm}/acct
if test ! -r sum/loginlog; then
	nulladm sum/loginlog
fi
printpw -u | sed "s/\([^:]*\).*/00-00-00  \1/" |\
sort +1 - sum/loginlog | uniq -u +10 |\
sort +1 - sum/loginlog | uniq -u > sum/tmploginlog
cp sum/tmploginlog sum/loginlog
#	"update loginlog"
d="`date +%y-%m-%d`"
day=`date +%m%d`
#	"lines 1 and 2 - remove everything from the total"
#	"acctng records with connect info except login"
#	"name and adds the date"
#	"line 3 - sorts in reverse order by login name; gets"
#	"1st occurrence of each login name and resorts by date"
if test ! -r nite/ctacct.$day; then
	nulladm nite/ctacct.$day
fi
acctmerg -a < nite/ctacct.$day | \
 sed -e "s/^[^ 	]*[ 	]*\([^ 	]*\)[ 	].*/$d  \1/" | \
 sort -r +1 - sum/loginlog | uniq +10 | sort >sum/tmploginlog
cp sum/tmploginlog sum/loginlog
rm -f sum/tmploginlog

