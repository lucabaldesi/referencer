/*
 * newstr.c
 *
 * Copyright (c) Chris Putnam 1999-2005
 *
 * Source code released under the GPL
 *
 *
 * newstring routines for dynamically allocated strings
 *
 * C. Putnam 3/29/02  Clean up newstr_findreplace() (x4 speed increase too)
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "newstr.h"
#include "is_ws.h"

#define newstr_initlen (64)

void 
newstr_init(newstr *string)
{
	if (string==NULL) return;
	string->dim=0;
	string->len=0;
	string->data=NULL;
}

static void 
newstr_initalloc(newstr *string, unsigned long minsize)
{
	unsigned long size = newstr_initlen;
	if (string==NULL) return;
	if (minsize > newstr_initlen) size=minsize;
	string->data = (char *) malloc (sizeof( *(string->data) ) * size);
	if (string->data==NULL) {
		fprintf(stderr,"Error.  Cannot allocate memory in newstr_initalloc.\n");
		exit(1);
	}
	string->data[0]='\0';
	string->dim=size;
	string->len=0;
}

newstr *
newstr_new( void )
{
	newstr *string;
	string = (newstr *) malloc( sizeof( *string ) );
	if (string!=NULL) 
		newstr_initalloc( string, newstr_initlen );
	return string;
}

/* newstr_empty()
 *
 * empty data in string
 */
void
newstr_empty( newstr *string )
{
	if (string && string->data) {
		string->data[0]='\0';
		string->len=0;
	}
}

#ifndef NEWSTR_PARANOIA

void 
newstr_free(newstr *string)
{
	if (string==NULL) return;
	string->dim=0;
	string->len=0;
	if (string->data!=NULL) free(string->data);
	string->data=NULL;
}

static void 
newstr_realloc(newstr *string, unsigned long minsize)
{
	char *newptr;
	unsigned long size;
	size = 2 * string->dim;
	if (size < minsize) size = minsize;
	newptr = (char *) realloc (string->data, sizeof( *(string->data) )*size);
	if (newptr==NULL) {
		fprintf(stderr,"Error.  Cannot reallocate memory (%ld bytes) in newstr_realloc.\n", sizeof(*(string->data))*size);
		exit(1);
	}
	string->data = newptr;
	string->dim = size;
}

#else

void 
newstr_free(newstr *string)
{
	unsigned long i;
	if (string==NULL) return;
	for (i=0; i<string->dim; ++i) string->data[i]='\0';
	string->dim=0;
	if (string->data!=NULL) free(string->data);
	string->data=NULL;
}

void 
newstr_realloc(newstr *string, unsigned long minsize)
{
	char *newptr;
	unsigned long size;
	unsigned long i;
	size = 2 * string->dim;
	if (size < minsize) size = minsize;
	newptr = (char *) malloc( sizeof( *(string->data) ) * size );
	if ( newptr == NULL ) {
		fprintf(stderr,"Error.  Cannot reallocate memory (%d bytes) in newstr_realloc.\n", sizeof(*(string->data))*size);
		exit(1);
	}
	for (i=0; i<string->dim; ++i) string->data[i]='\0';
	free( string->data );
	string->data = newptr;
	string->dim = size;
}

#endif

void
newstr_addchar(newstr *string, char newchar)
{
	if ( !string ) return;
	if ( !string->data || string->dim==0 ) 
		newstr_initalloc(string,newstr_initlen);
	if ( string->len + 2 > string->dim ) 
		newstr_realloc(string,string->len+2);
	string->data[string->len++] = newchar;
	string->data[string->len] = '\0';
}

void 
newstr_fprintf( FILE *fp, newstr *string )
{
    if ( string && string->data ) fprintf(fp,"%s",string->data);
}

void
newstr_prepend( newstr *string, char *addstr )
{
	unsigned long lenaddstr, i;
	if ( !string || !addstr ) return;
	lenaddstr = strlen( addstr );
	if ( string->data==NULL || string->dim==0 )
		newstr_initalloc( string, lenaddstr+1 );
	else {
		if ( string->len + lenaddstr  + 1 > string->dim )
			newstr_realloc( string, string->len + lenaddstr + 1 );
		for ( i=string->len+lenaddstr-1; i>=lenaddstr; i-- )
			string->data[i] = string->data[i-lenaddstr];
	}
	strncpy( string->data, addstr, lenaddstr );
	string->len += lenaddstr;
	string->data[ string->len ] = '\0';
}

void 
newstr_strcat( newstr *string, char *addstr )
{
	unsigned long lenaddstr;
	if ( !string || !addstr ) return;
	lenaddstr = strlen( addstr );
	if (string->data==NULL || string->dim==0) 
		newstr_initalloc(string, lenaddstr+1 );
	else {
		if ( string->len + lenaddstr + 1 > string->dim )
			newstr_realloc( string, string->len + lenaddstr + 1 );
	}
	strncpy(&(string->data[string->len]),addstr,lenaddstr);
	string->len += lenaddstr;
	string->data[string->len]='\0';
}

void
newstr_segcat( newstr *string, char *startat, char *endat )
{
	unsigned int seglength;
	char *p, *q;

	if (string==NULL || startat==NULL || endat==NULL ) return;
	if ( (unsigned int) startat > (unsigned int) endat ) return;

	seglength=(unsigned int) endat - (unsigned int) startat;
	if (string->data==NULL || string->dim==0)
		newstr_initalloc(string, seglength+1);
	else {
		if ( string->len + seglength + 1 > string->dim )
			newstr_realloc(string, string->len + seglength+1);
	}
	q = &(string->data[string->len]);
	p = startat;
	while ( *p && p!=endat ) *q++ = *p++;
	*q = '\0';
	string->len += seglength;
}

void 
newstr_strcpy (newstr *string, char *addstr)
{
	unsigned long lenaddstr;
	if ( !string || !addstr ) return;
	lenaddstr = strlen( addstr );
	if (string->data==NULL || string->dim==0)
		newstr_initalloc( string, lenaddstr+1 );
	else if ( lenaddstr+1 > string->dim) 
		newstr_realloc(string,lenaddstr+1);
	strncpy(string->data,addstr,lenaddstr);
	string->data[lenaddstr]='\0';
	string->len=lenaddstr;
}

/* newstr_segcpy( s, start, end );
 *
 * copies [start,end) into s
 */
void
newstr_segcpy (newstr *string, char *startat, char *endat )
{
	unsigned long seglength;
	char *p, *q;

	if (string==NULL || startat==NULL || endat==NULL ) return;
	if ( (unsigned long) startat > (unsigned long) endat ) return;

	seglength=(unsigned long) endat - (unsigned long) startat;
	if (string->data==NULL || string->dim==0)
		newstr_initalloc(string, seglength+1);
	else {
		if ( seglength+1 > string->dim )
			newstr_realloc(string, seglength+1);
	}
	q = string->data;
	p = startat;
	while ( *p && p!=endat ) *q++ = *p++;
	*q = '\0';
	string->len = seglength;
}

void
newstr_segdel( newstr *s, char *p, char *q )
{
	newstr tmp1, tmp2;
	char *r = &(s->data[s->len]);
	newstr_init( &tmp1 );
	newstr_init( &tmp2 );
	newstr_segcpy( &tmp1, s->data, p );
	newstr_segcpy( &tmp2, q, r );
	newstr_strcpy( s, tmp1.data );
	newstr_strcat( s, tmp2.data );
	newstr_free( &tmp2 );
	newstr_free( &tmp1 );
}

/*
 * newstr_findreplace()
 *
 *   if replace is "" or NULL, then delete find
 */

int
newstr_findreplace( newstr *string, char *find, char *replace )
{
	long diff;
	unsigned long findstart, searchstart;
	unsigned long p1, p2;
	unsigned long find_len, rep_len, curr_len;
	char empty[2] = "";
	unsigned long minsize;
	char *p;
	int n = 0;

	if ( !string || !string->data || !string->dim || !find ) return n;
	if ( !replace ) replace = empty;

	find_len = strlen( find );
	rep_len  = strlen( replace );
	diff     = rep_len - find_len;
	if ( diff < 0 ) diff = 0;

	searchstart=0;
	while ((p=strstr(string->data + searchstart,find))!=NULL) {
		curr_len = strlen(string->data);
		findstart=(unsigned long) p - (unsigned long) string->data;
		minsize = curr_len + diff + 1;
	 	if (string->dim <= minsize) newstr_realloc( string, minsize );
		if ( find_len > rep_len ) {
			p1 = findstart + rep_len;
			p2 = findstart + find_len;
			while( string->data[p2] )
				string->data[p1++]=string->data[p2++];
			string->data[p1]='\0';
			n++;
		} else if ( find_len < rep_len ) {
			for (p1=curr_len; p1>=findstart+find_len; p1--)
				string->data[p1+diff] = string->data[p1];
			n++;
		}
		for (p1=0; p1<rep_len; p1++)
			string->data[findstart+p1]=replace[p1];
		searchstart = findstart + rep_len; 
		string->len += rep_len - find_len;
	}
	return n;
}


/* newstr_fget()
 *   returns 0 if we're done, 1 if we're not done
 *   extracts line by line (regardless of end characters)
 *   and feeds from buf....
 */
int
newstr_fget( FILE *fp, char *buf, int bufsize, int *pbufpos, newstr *outs )
{
	int  bufpos = *pbufpos, done = 0;
	char *ok;
	newstr_empty( outs );
	while ( !done ) {
		while ( buf[bufpos] && buf[bufpos]!='\r' && buf[bufpos]!='\n' )
			newstr_addchar( outs, buf[bufpos++] );
		if ( buf[bufpos]=='\0' ) {
			ok = fgets( buf, bufsize, fp );
			bufpos=*pbufpos=0;
			if ( !ok && feof(fp) ) { /* end-of-file */
				buf[bufpos] = 0;
				if ( outs->len==0 ) return 0; /*nothing in out*/
				else return 1; /*one last out */
			}
		} else if ( buf[bufpos]=='\r' || buf[bufpos]=='\n' ) done=1;
	}
	if ( ( buf[bufpos]=='\n' && buf[bufpos+1]=='\r') ||
	     ( buf[bufpos]=='\r' && buf[bufpos+1]=='\n') ) bufpos+=2;
	else if ( buf[bufpos]=='\n' || buf[bufpos]=='\r' ) bufpos+=1; 
	*pbufpos = bufpos;
	return 1;
}

void
newstr_toupper( newstr *s )
{
	unsigned int i;
	if ( !s ) return;
	for ( i=0; i<s->len; ++i )
		s->data[i] = toupper( s->data[i] );
}

/* newstr_swapstrings( s1, s2 )
 * be sneaky and swap internal newstring data from one
 * string to another
 */
void
newstr_swapstrings( newstr *s1, newstr *s2 )
{
	char *tmpp;
	int tmp;

	/* swap dimensioning info */
	tmp = s1->dim;
	s1->dim = s2->dim;
	s2->dim = tmp;

	/* swap length info */
	tmp = s1->len;
	s1->len = s2->len;
	s2->len = tmp;

	/* swap data */
	tmpp = s1->data;
	s1->data = s2->data;
	s2->data = tmpp;
}

void
newstr_trimendingws( newstr *s )
{
	while ( s->len > 0 && is_ws( s->data[s->len-1] ) ) {
		s->data[s->len-1] = '\0';
		s->len--;
	}
}

