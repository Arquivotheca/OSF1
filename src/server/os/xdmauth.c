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
 * XDM-AUTHENTICATION-1 (XDMCP authentication) and
 * XDM-AUTHORIZATION-1 (client authorization) protocols
 *
 * $XConsortium: xdmauth.c,v 1.6 92/05/19 17:24:20 keith Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include "X.h"
#include "os.h"

#ifdef HASXDMAUTH

static Bool authFromXDMCP;

#ifdef XDMCP
#include "Xmd.h"
#include "Xdmcp.h"

/* XDM-AUTHENTICATION-1 */

static XdmAuthKeyRec	privateKey;
static char XdmAuthenticationName[] = "XDM-AUTHENTICATION-1";
#define XdmAuthenticationNameLen (sizeof XdmAuthenticationName - 1)
static XdmAuthKeyRec	rho;

static Bool XdmAuthenticationValidator (privateData, incomingData, packet_type)
    ARRAY8Ptr	privateData, incomingData;
    xdmOpCode	packet_type;
{
    XdmAuthKeyPtr	incoming;

    XdmcpUnwrap (incomingData->data, &privateKey,
			      incomingData->data,incomingData->length);
    switch (packet_type)
    {
    case ACCEPT:
    	if (incomingData->length != 8)
	    return FALSE;
    	incoming = (XdmAuthKeyPtr) incomingData->data;
    	XdmcpDecrementKey (incoming);
    	return XdmcpCompareKeys (incoming, &rho);
    }
    return FALSE;
}

static Bool
XdmAuthenticationGenerator (privateData, outgoingData, packet_type)
    ARRAY8Ptr	privateData, outgoingData;
    xdmOpCode	packet_type;
{
    outgoingData->length = 0;
    outgoingData->data = 0;
    switch (packet_type)
    {
    case REQUEST:
	if (XdmcpAllocARRAY8 (outgoingData, 8))
	    XdmcpWrap (&rho, &privateKey, outgoingData->data, 8);
    }
    return TRUE;
}

static Bool
XdmAuthenticationAddAuth (name_len, name, data_len, data)
    int	    name_len, data_len;
    char    *name, *data;
{
    Bool    ret;
    XdmcpUnwrap (data, &privateKey, data, data_len);
    authFromXDMCP = TRUE;
    ret = AddAuthorization (name_len, name, data_len, data);
    authFromXDMCP = FALSE;
    return ret;
}


#define atox(c)	('0' <= c && c <= '9' ? c - '0' : \
		 'a' <= c && c <= 'f' ? c - 'a' + 10 : \
		 'A' <= c && c <= 'F' ? c - 'A' + 10 : -1)

static
HexToBinary (in, out, len)
    char    *out, *in;
{
    int	    top, bottom;

    while (len > 0)
    {
	top = atox(in[0]);
	if (top == -1)
	    return 0;
	bottom = atox(in[1]);
	if (bottom == -1)
	    return 0;
	*out++ = (top << 4) | bottom;
	in += 2;
	len -= 2;
    }
    if (len)
	return 0;
    *out++ = '\0';
    return 1;
}

XdmAuthenticationInit (cookie, cookie_len)
    char    *cookie;
    int	    cookie_len;
{
    bzero (privateKey.data, 8);
    if (!strncmp (cookie, "0x", 2) || !strncmp (cookie, "0X", 2))
    {
	if (cookie_len > 2 + 2 * 8)
	    cookie_len = 2 + 2 * 8;
	HexToBinary (cookie + 2, privateKey.data, cookie_len - 2);
    }
    else
    {
    	if (cookie_len > 7)
	    cookie_len = 7;
    	bcopy (cookie, privateKey.data + 1, cookie_len);
    }
    XdmcpGenerateKey (&rho);
    XdmcpRegisterAuthentication (XdmAuthenticationName, XdmAuthenticationNameLen,
				 &rho,
				 sizeof (rho),
				 XdmAuthenticationValidator,
				 XdmAuthenticationGenerator,
				 XdmAuthenticationAddAuth);
}

#endif /* XDMCP */

/* XDM-AUTHORIZATION-1 */
typedef struct _XdmAuthorization {
    struct _XdmAuthorization	*next;
    XdmAuthKeyRec		rho;
    XdmAuthKeyRec		key;
    XID				id;
} XdmAuthorizationRec, *XdmAuthorizationPtr;

XdmAuthorizationPtr xdmAuth;

typedef struct _XdmClientAuth {
    struct _XdmClientAuth   *next;
    XdmAuthKeyRec	    rho;
    char		    client[6];
    long		    time;
} XdmClientAuthRec, *XdmClientAuthPtr;

XdmClientAuthPtr    xdmClients;
static long	    clockOffset;
static Bool	    gotClock;

#define TwentyMinutes	(20 * 60)

static Bool
XdmClientAuthCompare (a, b)
    XdmClientAuthPtr	a, b;
{
    int	i;

    if (!XdmcpCompareKeys (&a->rho, &b->rho))
	return FALSE;
    for (i = 0; i < 6; i++)
	if (a->client[i] != b->client[i])
	    return FALSE;
    return a->time == b->time;
}

static
XdmClientAuthDecode (plain, auth)
    unsigned char	*plain;
    XdmClientAuthPtr	auth;
{
    int	    i, j;

    j = 0;
    for (i = 0; i < 8; i++)
    {
	auth->rho.data[i] = plain[j];
	++j;
    }
    for (i = 0; i < 6; i++)
    {
	auth->client[i] = plain[j];
	++j;
    }
    auth->time = 0;
    for (i = 0; i < 4; i++)
    {
	auth->time |= plain[j] << ((3 - i) << 3);
	j++;
    }
}

XdmClientAuthTimeout (now)
    long	now;
{
    XdmClientAuthPtr	client, next, prev;

    prev = 0;
    for (client = xdmClients; client; client=next)
    {
	next = client->next;
	if (abs (now - client->time) > TwentyMinutes)
	{
	    if (prev)
		prev->next = next;
	    else
		xdmClients = next;
	    xfree (client);
	}
	else
	    prev = client;
    }
}

static XdmClientAuthPtr
XdmAuthorizationValidate (plain, length, rho)
    char		*plain;
    int			length;
    XdmAuthKeyPtr	rho;
{
    XdmClientAuthPtr	client, existing;
    long		now;

    if (length != (192 / 8))
	return NULL;
    client = (XdmClientAuthPtr) xalloc (sizeof (XdmClientAuthRec));
    if (!client)
	return NULL;
    XdmClientAuthDecode (plain, client);
    if (!XdmcpCompareKeys (&client->rho, rho))
    {
	xfree (client);
	return NULL;
    }
    now = time(0);
    if (!gotClock)
    {
	clockOffset = client->time - now;
	gotClock = TRUE;
    }
    now += clockOffset;
    XdmClientAuthTimeout (now);
    if (abs (client->time - now) > TwentyMinutes)
    {
	xfree (client);
	return NULL;
    }
    for (existing = xdmClients; existing; existing=existing->next)
    {
	if (XdmClientAuthCompare (existing, client))
	{
	    xfree (client);
	    return NULL;
	}
    }
    return client;
}

int
XdmAddCookie (data_length, data, id)
unsigned short	data_length;
char	*data;
XID	id;
{
    XdmAuthorizationPtr	new;
    unsigned char	*rho_bits, *key_bits;

    switch (data_length)
    {
    case 16:		    /* auth from files is 16 bytes long */
	if (authFromXDMCP)
	{
	    /* R5 xdm sent bogus authorization data in the accept packet,
	     * but we can recover */
	    rho_bits = rho.data;
	    key_bits = (unsigned char *) data;
	    key_bits[0] = '\0';
	}
	else
	{
	    rho_bits = (unsigned char *) data;
	    key_bits = (unsigned char *) (data + 8);
	}
	break;
    case 8:		    /* auth from XDMCP is 8 bytes long */
	rho_bits = rho.data;
	key_bits = (unsigned char *) data;
	break;
    default:
	return 0;
    }
    /* the first octet of the key must be zero */
    if (key_bits[0] != '\0')
	return 0;
    new = (XdmAuthorizationPtr) xalloc (sizeof (XdmAuthorizationRec));
    if (!new)
	return 0;
    new->next = xdmAuth;
    xdmAuth = new;
    bcopy (key_bits, new->key.data, (int) 8);
    bcopy (rho_bits, new->rho.data, (int) 8);
    new->id = id;
    return 1;
}

XID
XdmCheckCookie (cookie_length, cookie)
unsigned short	cookie_length;
char	*cookie;
{
    XdmAuthorizationPtr	auth;
    XdmClientAuthPtr	client;
    char		*plain;

    /* Auth packets must be a multiple of 8 bytes long */
    if (cookie_length & 7)
	return (XID) -1;
    plain = (char *) xalloc (cookie_length);
    if (!plain)
	return (XID) -1;
    for (auth = xdmAuth; auth; auth=auth->next) {
	XdmcpUnwrap (cookie, &auth->key, plain, cookie_length);
	if (client = XdmAuthorizationValidate (plain, cookie_length, &auth->rho))
	{
	    client->next = xdmClients;
	    xdmClients = client;
	    xfree (plain);
	    return auth->id;
	}
    }
    xfree (plain);
    return (XID) -1;
}

int
XdmResetCookie ()
{
    XdmAuthorizationPtr	auth, next_auth;
    XdmClientAuthPtr	client, next_client;

    for (auth = xdmAuth; auth; auth=next_auth)
    {
	next_auth = auth->next;
	xfree (auth);
    }
    xdmAuth = 0;
    for (client = xdmClients; client; client=next_client)
    {
	next_client = client->next;
	xfree (client);
    }
    xdmClients = (XdmClientAuthPtr) 0;
}

XID
XdmToID (cookie_length, cookie)
unsigned short	cookie_length;
char	*cookie;
{
    XdmAuthorizationPtr	auth;
    XdmClientAuthPtr	client;
    char		*plain;

    plain = (char *) xalloc (cookie_length);
    if (!plain)
	return (XID) -1;
    for (auth = xdmAuth; auth; auth=auth->next) {
	XdmcpUnwrap (cookie, &auth->key, plain, cookie_length);
	if (client = XdmAuthorizationValidate (plain, cookie_length, &auth->rho))
	{
	    xfree (client);
	    xfree (cookie);
	    return auth->id;
	}
    }
    xfree (cookie);
    return (XID) -1;
}

XdmFromID (id, data_lenp, datap)
XID id;
unsigned short	*data_lenp;
char	**datap;
{
    XdmAuthorizationPtr	auth;

    for (auth = xdmAuth; auth; auth=auth->next) {
	if (id == auth->id) {
	    *data_lenp = 16;
	    *datap = (char *) &auth->rho;
	    return 1;
	}
    }
    return 0;
}

XdmRemoveCookie (data_length, data)
unsigned short	data_length;
char	*data;
{
    XdmAuthorizationPtr	auth, prev;
    XdmAuthKeyPtr	key_bits, rho_bits;

    prev = 0;
    switch (data_length)
    {
    case 16:
	rho_bits = (XdmAuthKeyPtr) data;
	key_bits = (XdmAuthKeyPtr) (data + 8);
	break;
    case 8:
	rho_bits = &rho;
	key_bits = (XdmAuthKeyPtr) data;
	break;
    default:
	return;
    }
    for (auth = xdmAuth; auth; auth=auth->next) {
	if (XdmcpCompareKeys (rho_bits, &auth->rho) &&
	    XdmcpCompareKeys (key_bits, &auth->key))
 	{
	    if (prev)
		prev->next = auth->next;
	    else
		xdmAuth = auth->next;
	    xfree (auth);
	    return 1;
	}
    }
    return 0;
}

#endif
