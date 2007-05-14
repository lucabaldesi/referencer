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

int
lists_add( lists *info, char *value )
{
	newstr *newdata;
	int min_alloc = 20, i;
	if ( info->maxstr==0 ) {
		info->str = (newstr*) malloc( sizeof(newstr)*min_alloc );
		if ( !(info->str) ) return 0;
		info->maxstr = min_alloc;
		info->nstr = 0;
		for ( i=0; i<min_alloc; ++i )
			newstr_init( &(info->str[i]) );
	} else if ( info->nstr >= info->maxstr ) {
		min_alloc = info->maxstr * 2;
		newdata = (newstr*) realloc( info->str,
				sizeof( newstr ) * min_alloc );
		if ( !newdata ) return 0;
		info->maxstr = min_alloc;
		info->str = newdata;
		for ( i=info->nstr; i<info->maxstr; ++i )
			newstr_init( &(info->str[i]) );
	}
	newstr_strcpy( &(info->str[info->nstr]), value );
	info->nstr++;
	return 1;
}

void
lists_free( lists *info )
{
	int i;
	for ( i=0; i<info->maxstr; ++i )
		newstr_free( &(info->str[i]) );
	free( info->str );
	lists_init( info );
}

void
lists_init( lists *info  )
{
	info->str = NULL;
	info->maxstr = 0;
	info->nstr = 0;
}

int
lists_find( lists *info, char *searchstr )
{
	int i;
	for ( i=0; i<info->nstr; ++i )
		if ( !strcmp(info->str[i].data,searchstr) ) return i;
	return -1;
}

int
lists_fill( lists *info, char *filename )
{
	newstr line;
	FILE *fp;
	char *p;
	char buf[512]="";
	int  bufpos = 0;

	fp = fopen( filename, "r" );
	if ( !fp ) return 0;

	info->str = NULL;
	info->nstr = info->maxstr = 0;

	newstr_init( &line );
	while ( newstr_fget( fp, buf, sizeof(buf), &bufpos, &line ) ) {
		p = &(line.data[0]);
		if ( *p=='\0' ) continue;
		if ( !lists_add( info, line.data ) ) return 0;
	}
	newstr_free( &line );
	fclose( fp );
	return 1;
}
