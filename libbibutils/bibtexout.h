/*
 * bibtexout.h
 *
 * Copyright (c) Chris Putnam 2005-7
 */
#ifndef BIBTEXOUT_H
#define BIBTEXOUT_H

/* bibtexout output options */
#define BIBOUT_FINALCOMMA (1)
#define BIBOUT_SINGLEDASH (2)
#define BIBOUT_WHITESPACE (4)
#define BIBOUT_BRACKETS   (8)
#define BIBOUT_UPPERCASE (16)

extern void bibtexout_write( fields *info, FILE *fp, int format_opts, 
		unsigned long refnum );

#endif
