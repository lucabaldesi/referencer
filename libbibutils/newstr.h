/*
 * newstring.h
 *
 * Copyright (c) Chris Putnam 1999-2007
 *
 * Source code released under the GPL
 *
 */

#ifndef NEWSTR_H
#define NEWSTR_H

#include <stdio.h>

typedef struct newstr {
	char *data;
	unsigned long dim;
	unsigned long len;
}  newstr;

newstr *newstr_new   ( void ); 
void newstr_init        ( newstr *string );
void newstr_free        ( newstr *string );
void newstr_addchar     ( newstr *string, char newchar );
void newstr_strcat      ( newstr *string, char *addstr );
void newstr_segcat      ( newstr *string, char *startat, char *endat );
void newstr_prepend     ( newstr *string, char *addstr );
void newstr_strcpy      ( newstr *string, char *addstr );
void newstr_newstrcpy   ( newstr *s, newstr *old );
void newstr_segcpy      ( newstr *string, char *startat, char *endat );
void newstr_segdel      ( newstr *string, char *startat, char *endat );
void newstr_fprintf     ( FILE *fp, newstr *string );
int  newstr_fget        ( FILE *fp, char *buf, int bufsize, int *pbufpos,
                          newstr *outs );
int  newstr_findreplace ( newstr *string, char *find, char *replace );
void newstr_empty       ( newstr *string );
void newstr_toupper     ( newstr *s );
void newstr_trimendingws( newstr *s );
void newstr_swapstrings ( newstr *s1, newstr *s2 );

/* NEWSTR_PARANOIA
 *
 * set to clear memory before it is freed or reallocated
 * note that this is slower...may be important if string
 * contains sensitive information
 */

#undef NEWSTR_PARANOIA

#endif

