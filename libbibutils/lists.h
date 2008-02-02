/*
 * lists.h
 *
 * Copyright (c) Chris Putnam 2004-7
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
	int n, max;
	newstr *str;
} lists;

extern void    lists_init( lists *a );
extern int     lists_add( lists *a, char *value );
extern int     lists_find( lists *a, char *searchstr );
extern void    lists_empty( lists *a );
extern void    lists_free( lists *a );
extern int     lists_fill( lists *a, char *filename );
extern newstr* lists_getstr( lists *a, int n );
extern char*   lists_getcharptr( lists *a, int n );

#endif
