/* $Id: bgpdump_lib.h,v 1.2 2007-07-15 14:44:22 bqu Exp $ */
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
Filename          : bgdump_lib.h
Author            : Dan Ardelean (dan@ripe.net)
Date              : 02-SEP-2002
Revision          : 
Revised           : 
Description       : Library frontend declarations
Language Version  : C
OSs Tested        : Linux 2.2.19
To Do             : 
-------------------------------------------------------------------------------
*/



#ifndef _BGPDUMP_LIB_H
#define _BGPDUMP_LIB_H

#include <stdio.h>

#include "bgpdump.h"
#include "bgpdump_attr.h"
#include "bgpdump_formats.h"

#include <zlib.h>

#define BGPDUMP_MAX_FILE_LEN	1024
#define BGPDUMP_MAX_AS_PATH_LEN	2000

typedef struct struct_BGPDUMP {
#ifdef HAVE_LIBZ
  gzFile	*f;
#else
  FILE *f; 
#endif
  int		eof;
  char	filename[BGPDUMP_MAX_FILE_LEN];
  int		parsed;
  int		parsed_ok;
} BGPDUMP;

/* prototypes */

BGPDUMP *bgpdump_open_dump(const char *filename);
void	bgpdump_close_dump(BGPDUMP *dump);

BGPDUMP_ENTRY*	bgpdump_read_next(BGPDUMP *dump);
void	bgpdump_free_mem(BGPDUMP_ENTRY *entry);

#endif