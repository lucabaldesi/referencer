/*
 * endin.h
 *
 * Copyright (c) Chris Putnam 2003-7
 *
 * Program and source code released under the GPL
 *
 */
#ifndef ENDIN_H
#define ENDIN_H

#include "newstr.h"
#include "fields.h"
#include "reftypes.h"

extern int endin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, newstr *line, newstr *reference, int *fcharset );
extern int endin_processf( fields *endin, char *p, char *filename, long nref );
extern int endin_typef( fields *endin, char *filename, int nrefs, variants *all, int nall );
extern void endin_convertf( fields *endin, fields *info, int reftype, int verbose, variants *all, int nall );

extern variants end_all[];
extern int end_nall;


#endif

