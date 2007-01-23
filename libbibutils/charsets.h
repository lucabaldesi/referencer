/*
 * charsets.h
 *
 * Copyright (c) Chris Putnam 2003-5
 *
 * Source code released under the GPL
 *
 */

/*typedef struct charconvert {
	char old[10];
	char utf8[20];
} charconvert; */
typedef unsigned int charconvert;

typedef struct allcharconvert_t {
	char name[15];
	char name2[25];
	charconvert *table;
	int ntable;
} allcharconvert_t;
extern allcharconvert_t allcharconvert[];
extern int nallcharconvert;


