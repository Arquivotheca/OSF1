#!/bin/sh
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

#
# install - install a program, script, or datafile
#
# $XConsortium: install.sh,v 1.2 89/12/18 14:47:22 jim Exp $
#
# This script is compatible with the BSD install script, but was written
# from scratch.
#


# set DOITPROG to echo to test this script

doit="${DOITPROG:-}"


# put in absolute paths if you don't have them in your path; or use env. vars.

mvprog="${MVPROG:-mv}"
cpprog="${CPPROG:-cp}"
chmodprog="${CHMODPROG:-chmod}"
chownprog="${CHOWNPROG:-chown}"
chgrpprog="${CHGRPPROG:-chgrp}"
stripprog="${STRIPPROG:-strip}"
rmprog="${RMPROG:-rm}"

instcmd="$mvprog"
chmodcmd=""
chowncmd=""
chgrpcmd=""
stripcmd=""
rmcmd="$rmprog -f"
src=""
dst=""

while [ x"$1" != x ]; do
    case $1 in
	-c) instcmd="$cpprog"
	    shift
	    continue;;

	-m) chmodcmd="$chmodprog $2"
	    shift
	    shift
	    continue;;

	-o) chowncmd="$chownprog $2"
	    shift
	    shift
	    continue;;

	-g) chgrpcmd="$chgrpprog $2"
	    shift
	    shift
	    continue;;

	-s) stripcmd="$stripprog"
	    shift
	    continue;;

	*)  if [ x"$src" = x ]
	    then
		src=$1
	    else
		dst=$1
	    fi
	    shift
	    continue;;
    esac
done

if [ x"$src" = x ]
then
	echo "install:  no input file specified"
	exit 1
fi

if [ x"$dst" = x ]
then
	echo "install:  no destination specified"
	exit 1
fi


# if destination is a directory, append the input filename; if your system
# does not like double slashes in filenames, you may need to add some logic

if [ -d $dst ]
then
	dst="$dst"/`basename $src`
fi


# get rid of the old one and mode the new one in

$doit $rmcmd $dst
$doit $instcmd $src $dst


# and set any options; do chmod last to preserve setuid bits

if [ x"$chowncmd" != x ]; then $doit $chowncmd $dst; fi
if [ x"$chgrpcmd" != x ]; then $doit $chgrpcmd $dst; fi
if [ x"$stripcmd" != x ]; then $doit $stripcmd $dst; fi
if [ x"$chmodcmd" != x ]; then $doit $chmodcmd $dst; fi

exit 0
