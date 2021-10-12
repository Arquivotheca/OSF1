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
static char *rcsid = "@(#)$RCSfile: tcpslice.c,v $ $Revision: 1.1.2.3 $ (DEC) $Date: 1993/07/21 14:35:31 $";
#endif
/*
 * Copyright (c) 1987-1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * Based on:
 * char copyright[] = "Copyright (c) 1987-1990 The Regents of the University of California.\nAll rights reserved.\n";
 * static  char rcsid[] = "tcpslice.c,v 1.10 92/06/02 17:57:44 mccanne Exp $ (LBL)";
 */

/*
 * tcpslice - extract pieces of and/or glue together tcpdump files
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <netinet/in.h>
#include <varargs.h>

#include "interface.h"
#include "savefile.h"
#include "version.h"


int tflag = 0;	/* global that util routines are sensitive to */

char *program_name;

long thiszone;			/* gmt to local correction in trace file */

/* Length of saved portion of packet. */
int snaplen;

/* Length of saved portion of data past link level protocol.  */
int snapdlen;

/* Precision of clock used to generate trace file. */
int precision;

static int linkinfo;

/* Style in which to print timestamps; RAW is "secs.usecs"; READABLE is
 * ala the Unix "date" tool; and PARSEABLE is tcpslice's custom format,
 * designed to be easy to parse.  The default is RAW.
 */
enum stamp_styles { TIMESTAMP_RAW, TIMESTAMP_READABLE, TIMESTAMP_PARSEABLE };
enum stamp_styles timestamp_style = TIMESTAMP_RAW;


time_t gwtm2secs( /* struct tm *tmp */ );


long local_time_zone( /* timestamp */ );
struct timeval parse_time(/* time_string, base_time*/);
void fill_tm(/* time_string, is_delta, t, usecs_addr */);
void get_file_range( /* filename, first_time, last_time */ );
struct timeval first_packet_time(/* filename */);
void extract_slice(/* filename, start_time, stop_time */);
char *timestamp_to_string( /* timestamp */ );
void dump_times(/* filename */);
void usage();


int
main(argc, argv)
	int argc;
	char **argv;
{
	int op;
	int dump_flag = 0;
	int report_times = 0;
	char *start_time_string = 0;
	char *stop_time_string = 0;
	char *write_file_name = "-";	/* default is stdout */
	struct timeval first_time, start_time, stop_time;

	extern char *optarg;
	extern int optind, opterr;

	program_name = argv[0];

	opterr = 0;
	while ((op = getopt(argc, argv, "dRrtw:")) != EOF)
		switch (op) {

		case 'd':
			dump_flag = 1;
			break;

		case 'R':
			++report_times;
			timestamp_style = TIMESTAMP_RAW;
			break;

		case 'r':
			++report_times;
			timestamp_style = TIMESTAMP_READABLE;
			break;

		case 't':
			++report_times;
			timestamp_style = TIMESTAMP_PARSEABLE;
			break;

		case 'w':
 			write_file_name = optarg;
 			break;

		default:
			usage();
			/* NOTREACHED */
		}

	if ( report_times > 1 )
		error( "only one of -R, -r, or -t can be specified" );


	if (optind < argc)
		/* See if the next argument looks like a possible
		 * start time, and if so assume it is one.
		 */
		if (isdigit(argv[optind][0]) || argv[optind][0] == '+')
			start_time_string = argv[optind++];

	if (optind < argc)
		if (isdigit(argv[optind][0]) || argv[optind][0] == '+')
			stop_time_string = argv[optind++];


	if (optind >= argc)
		error("at least one input file must be given");


	first_time = first_packet_time(argv[optind]);
	fclose( sf_readfile );


	if (start_time_string)
		start_time = parse_time(start_time_string, first_time);
	else
		start_time = first_time;

	if (stop_time_string)
		stop_time = parse_time(stop_time_string, start_time);

	else
		{
		stop_time = start_time;
		stop_time.tv_sec += 86400*3660;	/* + 10 years; "forever" */
		}


	if (report_times) {
		for (; optind < argc; ++optind)
			dump_times(argv[optind]);
	}
	
	if (dump_flag) {
		printf( "start\t%s\nstop\t%s\n",
			timestamp_to_string( &start_time ),
			timestamp_to_string( &stop_time ) );
	}

	if (! report_times && ! dump_flag) {
		if ( ! strcmp( write_file_name, "-" ) &&
		     isatty( fileno(stdout) ) )
			error("stdout is a terminal; redirect or use -w");

		sf_write_init(write_file_name, linkinfo, thiszone, snaplen,
			precision);

		for (; optind < argc; ++optind)
			extract_slice(argv[optind], &start_time, &stop_time);

		fclose( sf_writefile );
	}

	return 0;
}


/* Returns non-zero if a string matches the format for a timestamp,
 * 0 otherwise.
 */
int is_timestamp( str )
char *str;
	{
	while ( isdigit(*str) || *str == '.' )
		++str;

	return *str == '\0';
	}


/* Return the correction in seconds for the local time zone with respect
 * to Greenwich time.
 */
long local_time_zone(timestamp)
long timestamp;
{
	struct timeval now;
	struct timezone tz;
	long localzone;

	if (gettimeofday(&now, &tz) < 0) {
		perror("tcpslice: gettimeofday");
		exit(1);
	}
	localzone = tz.tz_minuteswest * -60;

	if (localtime((time_t *) &timestamp)->tm_isdst)
		localzone += 3600;

	return localzone;
}

/* Given a string specifying a time (or a time offset) and a "base time"
 * from which to compute offsets and fill in defaults, returns a timeval
 * containing the specified time.
 */

struct timeval
parse_time(time_string, base_time)
	char *time_string;
	struct timeval base_time;
{
	struct tm *bt = localtime((time_t *) &base_time.tv_sec);
	struct tm t;
	struct timeval result;
	time_t usecs = 0;
	int is_delta = (time_string[0] == '+');

	if ( is_delta )
		++time_string;	/* skip over '+' sign */

	if ( is_timestamp( time_string ) )
		{ /* interpret as a raw timestamp or timestamp offset */
		char *time_ptr;

		result.tv_sec = atoi( time_string );
		time_ptr = strchr( time_string, '.' );

		if ( time_ptr )
			{ /* microseconds are specified, too */
			int num_digits = strlen( time_ptr + 1 );
			result.tv_usec = atoi( time_ptr + 1 );

			/* turn 123.456 into 123 seconds plus 456000 usec */
			while ( num_digits++ < 6 )
				result.tv_usec *= 10;
			}

		else
			result.tv_usec = 0;

		if ( is_delta )
			{
			result.tv_sec += base_time.tv_sec;
			result.tv_usec += base_time.tv_usec;

			if ( result.tv_usec > 1000000 )
				{
				result.tv_usec -= 1000000;
				++result.tv_sec;
				}
			}

		return result;
		}

	if (is_delta) {
		t = *bt;
		usecs = base_time.tv_usec;
	} else {
		/* Zero struct (easy way around lack of tm_gmtoff/tm_zone
		 * under older systems) */
		bzero((char *)&t, sizeof(t));

		/* Set values to "not set" flag so we can later identify
		 * and default them.
		 */
		t.tm_sec = t.tm_min = t.tm_hour = t.tm_mday = t.tm_mon =
			t.tm_year = -1;
	}

	fill_tm(time_string, is_delta, &t, &usecs);

	/* Now until we reach a field that was specified, fill in the
	 * missing fields from the base time.
	 */
#define CHECK_FIELD(field_name) 		\
	if (t.field_name < 0) 			\
		t.field_name = bt->field_name;	\
	else					\
		break

	do {	/* bogus do-while loop so "break" in CHECK_FIELD will work */
		CHECK_FIELD(tm_year);
		CHECK_FIELD(tm_mon);
		CHECK_FIELD(tm_mday);
		CHECK_FIELD(tm_hour);
		CHECK_FIELD(tm_min);
		CHECK_FIELD(tm_sec);
	} while ( 0 );

	/* Set remaining unspecified fields to 0. */
#define ZERO_FIELD_IF_NOT_SET(field_name,zero_val)	\
	if (t.field_name < 0)				\
		t.field_name = zero_val

	if (! is_delta) {
		ZERO_FIELD_IF_NOT_SET(tm_year,90);  /* should never happen */
		ZERO_FIELD_IF_NOT_SET(tm_mon,0);
		ZERO_FIELD_IF_NOT_SET(tm_mday,1);
		ZERO_FIELD_IF_NOT_SET(tm_hour,0);
		ZERO_FIELD_IF_NOT_SET(tm_min,0);
		ZERO_FIELD_IF_NOT_SET(tm_sec,0);
	}

	result.tv_sec = gwtm2secs(&t);
	result.tv_sec -= local_time_zone(result.tv_sec);
	result.tv_usec = usecs;

	return result;
}


/* Fill in (or add to, if is_delta is true) the time values in the
 * tm struct "t" as specified by the time specified in the string
 * "time_string".  "usecs_addr" is updated with the specified number
 * of microseconds, if any.
 */
void
fill_tm(time_string, is_delta, t, usecs_addr)
	char *time_string;
	int is_delta;	/* if true, add times in instead of replacing */
	struct tm *t;	/* tm struct to be filled from time_string */
	time_t *usecs_addr;
{
	char *t_start, *t_stop, format_ch;
	int val;

#define SET_VAL(lhs,rhs)	\
	if (is_delta)		\
		lhs += rhs;	\
	else			\
		lhs = rhs

	/* Loop through the time string parsing one specification at
	 * a time.  Each specification has the form <number><letter>
	 * where <number> indicates the amount of time and <letter>
	 * the units.
	 */
	for (t_stop = t_start = time_string; *t_start; t_start = ++t_stop) {
		if (! isdigit(*t_start))
			error("bad date format %s, problem starting at %s",
			      time_string, t_start);

		while (isdigit(*t_stop))
			++t_stop;
		if (! t_stop)
			error("bad date format %s, problem starting at %s",
			      time_string, t_start);

		val = atoi(t_start);

		format_ch = *t_stop;
		if ( isupper( format_ch ) )
			format_ch = tolower( format_ch );

		switch (format_ch) {
			case 'y':
				if ( val > 1900 )
					val -= 1900;
				SET_VAL(t->tm_year, val);
				break;

			case 'm':
				if (strchr(t_stop+1, 'D') ||
				    strchr(t_stop+1, 'd'))
					/* it's months */
					SET_VAL(t->tm_mon, val - 1);
				else	/* it's minutes */
					SET_VAL(t->tm_min, val);
				break;

			case 'd':
				SET_VAL(t->tm_mday, val);
				break;

			case 'h':
				SET_VAL(t->tm_hour, val);
				break;

			case 's':
				SET_VAL(t->tm_sec, val);
				break;

			case 'u':
				SET_VAL(*usecs_addr, val);
				break;

			default:
				error(
				"bad date format %s, problem starting at %s",
				      time_string, t_start);
		}
	}
}


/* Return in first_time and last_time the timestamps of the first and
 * last packets in the given file.
 */
void
get_file_range( filename, first_time, last_time )
	char filename[];
	struct timeval *first_time;
	struct timeval *last_time;
{
	*first_time = first_packet_time( filename );

	if ( ! sf_find_end( first_time, last_time ) )
		error( "couldn't find final packet in file %s", filename );
}


/* Returns the timestamp of the first packet in the given tcpdump save
 * file, which as a side-effect is initialized for further save-file
 * reading.
 */

struct timeval
first_packet_time(filename)
	char filename[];
{
	struct packet_header hdr;
	u_char *buf;

	if (sf_read_init(filename, &linkinfo, &thiszone, &snaplen, &precision))
		error( "bad tcpdump file %s", filename );

	buf = (u_char *)malloc((unsigned)snaplen);

	if (sf_next_packet(&hdr, buf, snaplen))
		error( "bad status reading first packet in %s", filename );

	free((char *)buf);

	return hdr.ts;
}


/* Extract from the given file all packets with timestamps between
 * the two time values given (inclusive).  These packets are written
 * to the save file output set up by a previous call to sf_write_init().
 * Upon return, start_time is adjusted to reflect a time just after
 * that of the last packet written to the output.
 */

void
extract_slice(filename, start_time, stop_time)
	char filename[];
	struct timeval *start_time;
	struct timeval *stop_time;
{
	long start_pos, stop_pos;
	struct timeval file_start_time, file_stop_time;
	int status;
	struct packet_header hdr;
	u_char *buf;


	if (sf_read_init(filename, &linkinfo, &thiszone, &snaplen, &precision))
		error( "bad tcpdump file %s", filename );

	buf = (u_char *)malloc((unsigned)snaplen);

	start_pos = ftell( sf_readfile );


	if ( (status = sf_next_packet( &hdr, buf, snaplen )) )
		error( "bad status %d reading packet in %s",
			status, filename );

	file_start_time = hdr.ts;


	if ( ! sf_find_end( &file_start_time, &file_stop_time ) )
		error( "problems finding end packet of file %s",
			filename );

	stop_pos = ftell( sf_readfile );


	/* sf_find_packet() requires that the time it's passed as its last
	 * argument be in the range [min_time, max_time], so we enforce
	 * that constraint here.
	 */
	if ( sf_timestamp_less_than( start_time, &file_start_time ) )
		*start_time = file_start_time;

	if ( sf_timestamp_less_than( &file_stop_time, start_time ) )
		return;	/* there aren't any packets of interest in the file */


	sf_find_packet( &file_start_time, start_pos,
			&file_stop_time, stop_pos,
			start_time );

	for ( ; ; )
		{
		struct timeval *timestamp;
		status = sf_next_packet( &hdr, buf, snaplen );

		if ( status )
			{
			if ( status != SFERR_EOF )
				error( "bad status %d reading packet in %s",
					status, filename );
			break;
			}

		timestamp = &hdr.ts;

		if ( ! sf_timestamp_less_than( timestamp, start_time ) )
			{ /* packet is recent enough */
			if ( sf_timestamp_less_than( stop_time, timestamp ) )
				/* We've gone beyond the end of the region
				 * of interest ... We're done with this file.
				 */
				break;

			sf_write( buf, timestamp, (int) hdr.len,
				  (int) hdr.caplen );
			*start_time = *timestamp;

			/* We know that each packet is guaranteed to have
			 * a unique timestamp, so we push forward the
			 * allowed minimum time to weed out duplicate
			 * packets.
			 */
			++start_time->tv_usec;
			}
		}

	fclose( sf_readfile );
	free( (char *) buf );
}


/* Translates a timestamp to the time format specified by the user.
 * Returns a pointer to the translation residing in a static buffer.
 * There are two such buffers, which are alternated on subseqeuent
 * calls, so two calls may be made to this routine without worrying
 * about the results of the first call being overwritten by the
 * results of the second.
 */

char *
timestamp_to_string(timestamp)
	struct timeval *timestamp;
{
	struct tm *t;
#define NUM_BUFFERS 2
	static char buffers[NUM_BUFFERS][128];
	static int buffer_to_use = 0;
	char *buf;

	buf = buffers[buffer_to_use];
	buffer_to_use = (buffer_to_use + 1) % NUM_BUFFERS;

	switch ( timestamp_style )
	    {
	    case TIMESTAMP_RAW:
		sprintf( buf, "%d.%d", timestamp->tv_sec, timestamp->tv_usec );
		break;

	    case TIMESTAMP_READABLE:
		t = localtime((time_t *) &timestamp->tv_sec);
		strcpy( buf, asctime( t ) );
		buf[24] = '\0';	/* nuke final newline */
		break;

	    case TIMESTAMP_PARSEABLE:
		t = localtime((time_t *) &timestamp->tv_sec);
		sprintf( buf, "%02dy%02dm%02dd%02dh%02dm%02ds%06du",
			t->tm_year, t->tm_mon + 1, t->tm_mday, t->tm_hour,
			t->tm_min, t->tm_sec, timestamp->tv_usec );
		break;

	    }

	return buf;
}


/* Given a tcpdump save filename, reports on the times of the first
 * and last packets in the file.
 */

void
dump_times(filename)
	char filename[];
{
	struct timeval first_time, last_time;

	get_file_range( filename, &first_time, &last_time );

	printf( "%s\t%s\t%s\n",
		filename,
		timestamp_to_string( &first_time ),
		timestamp_to_string( &last_time ) );
}

void
usage()
{
	(void)fprintf(stderr, "tcpslice for tcpdump version %d.%d\n",
		      VERSION_MAJOR, VERSION_MINOR);
	(void)fprintf(stderr,
"Usage: tcpslice [-dRrt] [-w file] [start-time [end-time]] file ... \n");

	exit(-1);
}
