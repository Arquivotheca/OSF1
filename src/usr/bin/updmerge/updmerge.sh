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
# @(#)$RCSfile: updmerge.sh,v $ $Revision: 4.2.3.2 $ (DEC) $Date: 1992/01/29 17:55:50 $ 
#
#	updmerge - Merge updates
#		 This is used to facilitate merge of customizations.
#
#       @(#)updmerge.sh	3.2     (DEC OSF/1)    11/7/91
# *********************************************************************
# *                                                                   *
# *      Copyright (c) Digital Equipment Corporation, 1991, 1994      *
# *                                                                   *
# *                       All Rights Reserved.                        *
# *                                                                   *
# *********************************************************************
#		
Isdiff()
{(
	NEW=$1
	OLD=$2
	LINK=0

	Islink $OLD && LINK=`expr $LINK + 1`
	Islink $NEW && LINK=`expr $LINK + 1`

	case $LINK in
	0)
		set xx `sum $OLD`; shift
		OLDSUM=$1
		OLDSIZE=$2
		set xx `sum $NEW`; shift
		NEWSUM=$1
		NEWSIZE=$2

		[ $OLDSUM -eq $NEWSUM  ] &&
		{
			[ $OLDSIZE -eq $NEWSIZE ] &&
			{
				return 1
			}
		}
		return 0
		;;
	1)
		return 0
		;;
	2)
		E1=`ls -l $NEW | awk '{print $10}'`
		E2=`ls -l $OLD | awk '{print $10}'`
		[ $E1 = $E2 ] &&
		{
			return 1
		}
		return 0
		;;
	esac
)}

Isbinary()
{(
	file $1 > /usr/tmp/Isbinary
	grep -s "executable"  /usr/tmp/Isbinary
	STATUS=$?
	rm -r  /usr/tmp/Isbinary
	return $STATUS
)}
	 
Showfile()
{(
	F=$1

	if Islink $F ; then
	{
		echo "\tsymbolic link:  $F -> `ls -l $F | awk '{print $10}'`" 
	}
	else
	{
		more $F
	};fi
)}

Handlenew()
{(
	FILE=$1
	TDIR=$2

	[ -f $TDIR/$FILE ] ||
	{
		while :
		do
			if [ $TDIR = "/" ] ; then
			{
				FILENAME=/${FILE}
			}
			else
			{
				FILENAME=${TDIR}/${FILE}
			}; fi
			echo "
\t${FILENAME} :\t does not exist on newly installed system\nShow file/Restore file/Ignore (s/r/i)? [i]: \c"
			read ANS
			ANS=`echo $ANS`
			case "$ANS" in
			R*|r*)
				mv $FILE $TDIR/$FILE
				return 0
				;;
			S*|s*)
				Showfile $FILE
				;;
			I*|i*|"")
				return 0
				;;
			esac
		done	
 
		return 0
	}
	return 1
)}

Showdiffs()
{(
	NEW=$1
	OLD=$2
	TEMP=$3
	LINK=0

	Islink $OLD && LINK=`expr $LINK + 1`
	Islink $NEW && LINK=`expr $LINK + 1`

	case $LINK in
	0)
		if Isbinary $NEW ; then
		{
			echo "\t$NEW is a binary file.  Cannot show differences."
		}
		else
		{
			[ -s $TEMP ] ||
			{
				diff $NEW $OLD > $TEMP
			}
			echo "\
\t< restored file 
\t> existing file
"
			more $TEMP
		};fi
		;;
	1)
		echo "Cannot show differences:\n"
		for F in $NEW $OLD
		do
			echo "\t$F\t\c"
			if Islink $F ; then
			{
				echo "is a link to `ls -l $F | awk '{print $10}'`"
			}
			else
			{
				echo "is a file"
			};fi
		done
		;;
	2)
		for F in $NEW $OLD
		do
			echo "\t$F\t is a link to `ls -l $F | awk '{print $10}'`"
		done
		;;
	esac	
)}


Handlecust()
{(
	FILE=$1
	TDIR=$2

	NEW=$FILE
	if [ $TDIR = "/" ] ; then
	{
		OLD=/${NEW}
	}
	else
	{
		OLD=${TDIR}/${NEW}
	}; fi

	Isdiff $OLD $NEW ||
	{
		echo "\t${OLD} :\t no changes"
		return
	}

	rm -f $TEMP 2>/dev/null
	while :
	do

		echo "
\t$NEW \t differs from ${OLD}\nShow differences/Restore file/Ignore (s/r/i) ? [i]: \c"
	       read ANS
		ANS=`echo $ANS`
		case "$ANS" in
		R*|r*)
			mv $OLD ${OLD}.orig
			mv $NEW $OLD
			return
			;;
		I*|i*|"")	return
			;;
		S*|s*)
			Showdiffs $NEW $OLD $TEMP
			;;
		esac
	
	done	
)}

Islink()
{
	if ls -ld $1 | grep -s \^l ; then
	{
		return 0
	}
	else
	{
		return 1
	};fi
}

Handledir()
{(
	DIR=$1
	TDIR=$2

	if Islink $DIR ; then
	{
		return;
	};fi

	[ -d $TDIR/$DIR ] || 
	{

		while :
		do

			echo "
\tDirectory ${TDIR}/${DIR} does not exist on newly installed system\nRestore directory/Ignore (r/i) ? [i]: \c"

			read ANS
			ANS=`echo $ANS`
			case "$ANS" in
			R*|r*)
				mv $DIR $TDIR/$DIR 2>>/dev/null ||
				{
					tar cf - $DIR | (cd /; tar xpf -)&&
					{
						rm -r $DIR
					}
				}
				return
				;;
			I*|i*|"")	return
				;;
			esac
		done	
	}

	cd $DIR
	for FILE in .?* *
	do
	if [ $DIR = "." ] ; then
	{
		Cmp $FILE ${TDIR}
	}
	else
	{	
		if [ $TDIR = "/" ] ; then
		{
			Cmp $FILE /${DIR}
		}
		else
		{
			Cmp $FILE ${TDIR}/${DIR}
		}; fi
	}; fi
	done
)}

Cmp()
{(

	FILE=$1
	TDIR=$2
	
	[ $FILE = "*" ] &&
	{
		return
	}
	if [ -d $FILE ] ;then
	{
		case $RECURSE in
		0)
			;;
		*)
			RECURSE=`expr $RECURSE - 1`
			DIR=$FILE
			[ $DIR != ".." ] &&
			{
				Handledir $DIR $TDIR
			}
			;;
		esac
	}
	else
	{
		Handlenew $FILE $TDIR || Handlecust $FILE $TDIR
  
	}; fi
)}

Main()
{
	TEMP=/usr/tmp/updmerge.$$
	RECURSE=1

	case "$#" in
	1)
		Cmp . $1
		;;
	2)	
		case "$1" in
		-r)
			RECURSE=999
			Cmp . $2
			;;
		*)
			Cmp $1 $2
			;;
		esac
		;;
	*)	echo "
Usage:	updmerge file directory
	updmerge directory
	updmerge -r directory
"
		exit 1
	esac
	rm -f $TEMP 2> /dev/null
}

[ "$UPDMERGE_DEBUG" ] || Main $*









