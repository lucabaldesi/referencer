/*
 * lists.c
 *
 * Copyright (c) Chris Putnam 2004-7
 *
 * Source code released under the GPL
 *
 * Implements a simple managed array of newstrs.
 *
 */
#include "lists.h"

static int
lists_alloc( lists *a )
{
	int i, min_alloc = 20;
	a->str = ( newstr* ) malloc( sizeof( newstr ) * min_alloc );
	if ( !(a->str) ) return 0;
	a->max = min_alloc;
	a->n = 0;
	for ( i=0; i<min_alloc; ++i )
		newstr_init( &(a->str[i]) );
	return 1;
}

static int
lists_realloc( lists *a )
{
	newstr *nd;
	int i, min_alloc = a->max * 2;
	nd = ( newstr* ) realloc( a->str, sizeof( newstr ) * min_alloc );
	if ( !nd ) return 0;
	a->max = min_alloc;
	a->str = nd;
	for ( i=a->n; i<a->max; ++i )
		newstr_init( &(a->str[i]) );
	return 1;
}

int
lists_add( lists *a, char *value )
{
	int ok = 1;

	/* ensure sufficient space */
	if ( a->max==0 ) ok = lists_alloc( a );
	else if ( a->n >= a->max ) ok = lists_realloc( a );

	if ( ok ) {
		newstr_strcpy( &(a->str[a->n]), value );
		a->n++;
	}

	return ok;
}

newstr *
lists_getstr( lists *a, int n )
{
	if ( n<0 || n>a->n ) return NULL;
	else return &(a->str[n]);
}

char *
lists_getcharptr( lists *a, int n )
{
	if ( n<0 || n>a->n ) return NULL;
	else return a->str[n].data;
}

void
lists_empty( lists *a )
{
	int i;
	for ( i=0; i<a->max; ++i )
		newstr_empty( &(a->str[i]) );
	a->n = 0;
}

void
lists_free( lists *a )
{
	int i;
	for ( i=0; i<a->max; ++i )
		newstr_free( &(a->str[i]) );
	free( a->str );
	lists_init( a );
}

void
lists_init( lists *a  )
{
	a->str = NULL;
	a->max = 0;
	a->n = 0;
}

int
lists_find( lists *a, char *searchstr )
{
	int i;
	for ( i=0; i<a->n; ++i )
		if ( !strcmp(a->str[i].data,searchstr) ) return i;
	return -1;
}

int
lists_fill( lists *a, char *filename )
{
	newstr line;
	FILE *fp;
	char *p;
	char buf[512]="";
	int  bufpos = 0;

	fp = fopen( filename, "r" );
	if ( !fp ) return 0;

	lists_init( a );

	newstr_init( &line );
	while ( newstr_fget( fp, buf, sizeof(buf), &bufpos, &line ) ) {
		p = &(line.data[0]);
		if ( *p=='\0' ) continue;
		if ( !lists_add( a, line.data ) ) return 0;
	}
	newstr_free( &line );
	fclose( fp );
	return 1;
}
