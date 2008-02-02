/*
 * wordout.h
 *
 * Copyright (c) Chris Putnam 2007
 *
 * Source code released under the GPL
 *
 */
#ifndef WORDOUT_H
#define WORDOUT_H

/* format-specific options */
#define WORDOUT_DROPKEY (2)
#define WORDOUT_BOM     (4)

#include <stdio.h>
#include <stdlib.h>
#include "bibl.h"

extern void wordout_writeheader( FILE *outptr, int unicode );
extern void wordout_writefooter( FILE *outptr );
extern void wordout_write( fields *info, FILE *outptr,
	int format_opts, unsigned long numrefs );

#endif

