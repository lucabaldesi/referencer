/*
 * risin.h
 *
 * Copyright (c) Chris Putnam 2003-7
 *
 * Program and source code released under the GPL
 *
 */
#ifndef RISIN_H
#define RISIN_H

#include "newstr.h"
#include "fields.h"
#include "reftypes.h"

extern int risin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, newstr *line, newstr *reference, int *fcharset );
extern int risin_processf( fields *risin, char *p, char *filename, long nref );
extern int risin_typef( fields *risin, char *filename, int nref, variants *all, int nall );
extern void risin_convertf( fields *risin, fields *info, int reftype, int verbose, variants *all, int nall ); 

extern variants ris_all[];
extern int ris_nall;

#endif

