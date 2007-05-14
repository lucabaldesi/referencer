/*
 * bibtexiin.h
 *
 * Copyright (c) Chris Putnam 2003-7
 *
 * Program and source code released under the GPL
 *
 */
#ifndef BIBTEXIN_H
#define BIBTEXIN_H

#include "newstr.h"
#include "fields.h"
#include "bibl.h"
#include "reftypes.h"

extern void bibtexin_convertf( fields *bibin, fields *info, int reftype, int verbose, variants *all, int nall );
extern int  bibtexin_processf( fields *bibin, char *data, char *filename, long nref );
extern void bibtexin_cleanf( bibl *bin );
extern int  bibtexin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, newstr *line, newstr *reference, int *fcharset );
extern int  bibtexin_typef( fields *bibin, char *filename, int nrefs,
        variants *all, int nall );


extern variants bibtex_all[];
extern int bibtex_nall;


#endif

