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
# $XConsortium: mkdirhier.sh,v 1.6 91/08/13 18:13:04 rws Exp $
# Courtesy of Paul Eggert

newline='
'
IFS=$newline

case ${1--} in
-*) echo >&2 "mkdirhier: usage: mkdirhier directory ..."; exit 1
esac

status=

for directory
do
	case $directory in
	'')
		echo >&2 "mkdirhier: empty directory name"
		status=1
		continue;;
	*"$newline"*)
		echo >&2 "mkdirhier: directory name contains a newline: \`\`$directory''"
		status=1
		continue;;
	///*) prefix=/;; # See Posix 2.3 "path".
	//*) prefix=//;;
	/*) prefix=/;;
	-*) prefix=./;;
	*) prefix=
	esac

	IFS=/
	set x $directory
	IFS=$newline
	shift

	for filename
	do
		path=$prefix$filename
		prefix=$path/
		shift

		test -d "$path" || {
			paths=$path
			for filename
			do
				if [ "$filename" != "." ]; then
					path=$path/$filename
					paths=$paths$newline$path
				fi
			done

			mkdir $paths || status=$?

			break
		}
	done
  done

exit $status
