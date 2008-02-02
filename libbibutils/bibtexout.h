/*
 * bibtexout.h
 *
 * Copyright (c) Chris Putnam 2005-7
 */
#ifndef BIBTEXOUT_H
#define BIBTEXOUT_H

/* bibtexout output options */
#define BIBOUT_FINALCOMMA (2)
#define BIBOUT_SINGLEDASH (4)
#define BIBOUT_WHITESPACE (8)
#define BIBOUT_BRACKETS  (16)
#define BIBOUT_UPPERCASE (32)

extern void bibtexout_write( fields *info, FILE *fp, int format_opts, 
		unsigned long refnum );

#endif
