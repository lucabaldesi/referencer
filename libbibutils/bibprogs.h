/*
 * bibprogs.h
 *
 * Copyright (c) Chris Putnam 2005
 *
 */

#ifndef BIBPROGS_H
#define BIBPROGS_H

extern void tomods_processargs( int *argc, char *argv[], param *p, 
	char *progname, char *help1, char *help2 );
extern void args_encoding( int argc, char *argv[], int i, int *charset, 
	int *utf8, char *progname );
extern int args_match( char *check, char *shortarg, char *longarg );

#endif

