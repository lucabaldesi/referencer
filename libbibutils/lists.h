/*
 * lists.h
 *
 * Copyright (c) Chris Putnam 2004-5
 *
 * Source code released under the GPL
 *
 */

#ifndef LISTS_H
#define LISTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "newstr.h"

typedef struct lists {
	int nstr, maxstr;
	newstr *str;
} lists;

extern void lists_init( lists *info );
extern int  lists_add( lists *info, char *value );
extern int  lists_find( lists *info, char *searchstr );
extern void lists_free( lists *info );
extern int  lists_fill( lists *info, char *filename );

#endif
