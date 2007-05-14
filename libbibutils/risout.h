/*
 * risout.h
 *
 * Copyright (c) Chris Putnam 2005-7
 *
 */
#ifndef RISOUT_H
#define RISOUT_H

extern void risout_write( fields *info, FILE *fp, int format_opts, 
		unsigned long refnum );

#endif
