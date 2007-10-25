/* $Id: bgpdump.h,v 1.2 2005-05-18 13:47:33 bqu Exp $ */
/*

Copyright (c) 2002                      RIPE NCC


All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted, provided
that the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of the author not be used in advertising or
publicity pertaining to distribution of the software without specific,
written prior permission.

THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS; IN NO EVENT SHALL
AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 

*/

/* 

Parts of this code have been engineered after analiyzing GNU Zebra's
source code and therefore might contain declarations/code from GNU
Zebra, Copyright (C) 1999 Kunihiro Ishiguro. Zebra is a free routing
software, distributed under the GNU General Public License. A copy of
this license is included with libbgpdump.

*/


/*
-------------------------------------------------------------------------------
Module Header
Filename          : bgdump.h
Author            : Dan Ardelean (dan@ripe.net)
Date              : 02-SEP-2002
Revision          : 
Revised           : 
Description       : Generic header file for libbgpdump library
Language Version  : C
OSs Tested        : Linux 2.2.19
To Do             : 
-------------------------------------------------------------------------------
*/

#ifndef _BGPDUMP_H
#define _BGPDUMP_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# else
#  error "no HAVE_INTTYPES_H or HAVE_STDINT_H"
# endif 
#endif

#include <sys/types.h>
#include <netinet/in.h>

typedef uint16_t as_t;

typedef union union_BGPDUMP_IP_ADDRESS {
    struct in_addr	v4_addr;
#ifdef BGPDUMP_HAVE_IPV6
    struct in6_addr	v6_addr;
#endif
} BGPDUMP_IP_ADDRESS;

/* cygwin doesn't define INET[6]_ADDRSTRLEN */
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifdef BGPDUMP_HAVE_IPV6
#define BGPDUMP_ADDRSTRLEN INET6_ADDRSTRLEN
#else
#define BGPDUMP_ADDRSTRLEN INET_ADDRSTRLEN
#endif

#endif
