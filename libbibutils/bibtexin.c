/*
 * bibtexin.c
 *
 * Copyright (c) Chris Putnam 2003-5
 *
 * Program and source code released under the GPL
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "is_ws.h"
#include "strsearch.h"
#include "newstr.h"
#include "newstr_conv.h"
#include "fields.h"
#include "lists.h"
#include "name.h"
#include "title.h"
#include "reftypes.h"
#include "bibtexin.h"

extern lists asis;
extern lists corps;

lists find    = { 0, 0, NULL };
lists replace = { 0, 0, NULL };

/*
 * readf can "read too far", so we store this information in line, thus
 * the next new text is in line, either from having read too far or
 * from the next chunk obtained via newstr_fget()
 *
 * return 1 on success, 0 on error/end-of-file
 *
 */
static int
readmore( FILE *fp, char *buf, int bufsize, int *bufpos, newstr *line )
{
	if ( line->len ) return 1;
	else return newstr_fget( fp, buf, bufsize, bufpos, line );
}

/*
 * readf()
 *
 * returns zero if cannot get reference and hit end of-file
 * returns 1 if last reference in file, 2 if reference within file
 */
int
bibtexin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, newstr *line, newstr *reference, int *fcharset )
{
	int haveref = 0;
	char *p;
	while ( haveref!=2 && readmore( fp, buf, bufsize, bufpos, line ) ) {
		if ( line->len == 0 ) continue; /* blank line */
		p = &(line->data[0]);
		while ( is_ws( *p ) ) p++;
		if ( *p == '%' ) { /* commented out line */
			newstr_empty( line );
			continue;
		}
		if ( *p == '@' ) haveref++;
		if ( haveref<2 ) {
			newstr_strcat( reference, p );
			newstr_addchar( reference, '\n' );
			newstr_empty( line );
		}
	
	}
	*fcharset = CHARSET_UNKNOWN;
	return haveref;
}
#ifdef NOCOMPILE
void
bibtex_strfree( void )
{
	lists_free( &find );
	lists_free( &replace );
}
#endif

static char *
bibtex_item( char *p, newstr *s )
{
	int nquotes = 0;
	int nbrackets = 0;
	while ( *p ) {
		if ( !nquotes && !nbrackets ) {
			if ( /*is_ws( *p ) ||*/ *p==',' || *p=='=' || *p=='}' || *p==')' )
				goto out;
		}
		if ( *p=='\"' && *(p-1)!='\\') {
			nquotes = ( nquotes==0 );
			newstr_addchar( s, *p );
		} else if ( *p=='{' ) {
			nbrackets++;
			/*if ( s->len!=0 )*/ newstr_addchar( s, *p );
		} else if ( *p=='}' ) {
			nbrackets--;
			/*if ( nbrackets>0 )*/ newstr_addchar( s, *p );
		} else {
			if ( s->len!=0 || ( s->len==0 && !is_ws( *p ) ) )
				newstr_addchar( s, *p );
		}
		p++;
	}
out:
	newstr_trimendingws( s );
	return p;
}

static char *
process_bibtexline( char *p, newstr *tag, newstr *data )
{
	while ( is_ws( *p ) ) p++;
	p = bibtex_item( p, tag );
	while ( is_ws( *p ) ) p++;
	if ( *p=='=' ) {
		p++;
		while ( is_ws ( *p ) ) p++;
		p = bibtex_item( p, data );
		while ( is_ws ( *p ) ) p++;
	}
	if ( *p==',' || *p=='}' || *p==')' ) p++;
	while ( is_ws ( *p ) ) p++;
	return p;
}

static void
bibtex_cleantoken( newstr *s )
{
	newstr_findreplace( s, "\\it ", "" );
	newstr_findreplace( s, "\\em ", "" );
	newstr_findreplace( s, "\\%", "%" );
	newstr_findreplace( s, "\\$", "$" );
	newstr_findreplace( s, "{", "" );
	newstr_findreplace( s, "}", "" );
	while ( newstr_findreplace( s, "  ", " " ) ) {}
}

static void
bibtex_addstring( char *p )
{
	newstr s1, s2;
	newstr_init( &s1 );
	newstr_init( &s2 );
	while ( is_ws( *p ) ) p++;
	if ( *p=='(' || *p=='{' ) p++;
	p = process_bibtexline( p, &s1, &s2 );
	lists_add( &find, s1.data );
	newstr_findreplace( &s2, "\\ ", " " );
/*	newstr_findreplace( &s2, "\&", "&" );*/
	bibtex_cleantoken( &s2 );
	lists_add( &replace, s2.data );
/*	if ( verbose ) {
		fprintf( stderr, "String replacement: '%s' = '%s'\n",
				s1.data, s2.data );
	} */
	newstr_free( &s1 );
	newstr_free( &s2 );
}

static int
bibtex_protected( newstr *data )
{
	if ( data->data[0]=='{' && data->data[data->len-1]=='}' ) return 1;
	if ( data->data[0]=='\"' && data->data[data->len-1]=='\"' ) return 1;
	return 0;
}

static void
bibtex_removeprotection( newstr *data )
{
	newstr s;
	if ( data->len<3 ) newstr_empty( data ); /* "", {} to nothing */
	else {
		newstr_init( &s );
		newstr_segcpy( &s, &(data->data[1]), 
				&(data->data[data->len-1]) );
		newstr_strcpy( data, s.data );
		newstr_free( &s );
	}
}

static void
bibtex_split( lists *tokens, newstr *s )
{
	newstr currtok;
	int nquotes = 0, nbrackets = 0;
	int i, n = s->len;
	newstr_init( &currtok );
	for ( i=0; i<n; ++i ) {
		if ( s->data[i]=='\"' ) {
			if ( nquotes ) nquotes = 0;
			else nquotes = 1;
			newstr_addchar( &currtok, '\"' );
		} else if ( s->data[i]=='{' ) {
			nbrackets++;
			newstr_addchar( &currtok, '{' );
		} else if ( s->data[i]=='}' ) {
			nbrackets--;
			newstr_addchar( &currtok, '}' );
		} else if ( s->data[i]=='#' && !nquotes && !nbrackets ) {
			lists_add( tokens, currtok.data );
			newstr_empty( &currtok );
		} else if ( !is_ws( s->data[i] ) || nquotes || nbrackets ) {
			newstr_addchar( &currtok, s->data[i] );
		}
	}
	if ( currtok.len ) lists_add( tokens, currtok.data );
	for ( i=0; i<tokens->nstr; ++i ) {
		newstr_trimendingws( &(tokens->str[i]) );
	}
	newstr_free( &currtok );
}

static int
bibtex_usestrings( newstr *s )
{
	int i;
	for ( i=0; i<find.nstr; ++i ) {
		if ( !strcasecmp( s->data, (find.str[i]).data ) ) {
			newstr_findreplace( s, (find.str[i]).data, 
					(replace.str[i]).data );
			return 1;
		}
	}
	return 0;
}

/* get reference type */
static char*
process_bibtextype( char *p, newstr *data )
{
	if ( *p ) p++; /* skip '@' character */
	newstr_addchar( data, '{' ); /*protect type from string expansion */
	while ( *p && *p!='{' && *p!='(' )
		newstr_addchar( data, *p++ );
	if ( *p ) p++; /* skip ending bracket */
	if ( is_ws( *p ) ) p++;
	newstr_addchar( data, '}' ); /* protect type from string expansion */
	return p;
}
/* get reference name */
static char*
process_bibtexid( char *p, newstr *data )
{
	newstr_addchar( data, '{' ); /*protect type from string expansion */
	while ( *p && *p!=',' )
		newstr_addchar( data, *p++ );
	if ( *p ) p++; /* skip ending comma */
	if ( is_ws( *p ) ) p++; /* skip ending newline/carriage return */
	newstr_addchar( data, '}' ); /*protect type from string expansion */
	return p;
}

static void
process_cite( fields *bibin, char *p, char *filename, long nref )
{
	newstr tag, data;
	newstr_init( &tag );
	newstr_init( &data );
	p = process_bibtextype( p, &data );
	if ( data.len ) fields_add( bibin, "TYPE", data.data, 0 );
	newstr_empty( &data );
	if ( *p ) p = process_bibtexid ( p, &data );
	if ( data.len ) fields_add( bibin, "REFNUM", data.data, 0 );
	newstr_empty( &data );
	while ( *p ) {
		p = process_bibtexline( p, &tag, &data );
		/* no anonymous or empty fields allowed */
		if ( tag.len && data.len )
			fields_add( bibin, tag.data, data.data, 0 );
		newstr_empty( &tag );
		newstr_empty( &data );
	}
	newstr_free( &tag );
	newstr_free( &data );
}

static void
process_string( char *p )
{
	while ( *p && *p!='{' && *p!='(' ) p++;
	bibtex_addstring( p );
}

int
bibtexin_processf( fields *bibin, char *data, char *filename, long nref )
{
	if ( !strncasecmp( data, "@STRING", 7 ) ) {
		process_string( data );
		return 0;
	} else {
		process_cite( bibin, data, filename, nref );
		return 1;
	}
}

static void
bibtex_addtitleurl( fields *info, newstr *in )
{
	newstr s;
	char *p,*q;
	newstr_init( &s );
	q = p = in->data + 6; /*skip past \href{ */
	while ( *q && *q!='}' ) q++;
	newstr_segcpy( &s, p, q );
	fields_add( info, "URL", s.data, 0 );
	newstr_empty( &s );
	if ( *q=='}' ) q++;
	p = q;
	while ( *q ) q++;
	newstr_segcpy( &s, p, q );
	newstr_swapstrings( &s, in );
	newstr_free( &s );
}

static void
bibtex_cleandata( newstr *s, fields *info )
{
	lists tokens = { 0, 0, NULL };
	int i;
	if ( !s->len ) return;
	bibtex_split( &tokens, s );
	for ( i=0; i<tokens.nstr; ++i ) {
		if ( !bibtex_protected( &(tokens.str[i] ) ) ) {
			bibtex_usestrings( &(tokens.str[i]) );
		} else {
			if (!strncasecmp(tokens.str[i].data,"\\href{", 6)) {
				bibtex_addtitleurl( info, &(tokens.str[i]) );
			}
			bibtex_cleantoken( &(tokens.str[i]) );
		}
	}
	newstr_empty( s );
	for ( i=0; i<tokens.nstr; ++i ) {
		if ( bibtex_protected( &(tokens.str[i]) ) )
			bibtex_removeprotection( &(tokens.str[i]));
		newstr_strcat( s, tokens.str[i].data ); 
	}
	lists_free( &tokens );
}

static long
bibtexin_findref( bibl *bin, char *citekey )
{
	int n;
	long i;
	for ( i=0; i<bin->nrefs; ++i ) {
		n = fields_find( bin->ref[i], "refnum", -1 );
		if ( n==-1 ) continue;
		if ( !strcmp( bin->ref[i]->data[n].data, citekey ) ) return i;
	}
	return -1;
}

static void
bibtexin_crossref( bibl *bin )
{
	char booktitle[] = "booktitle";
	long i, j, ncross;
	char *nt, *nd, *type;
	int n, ntype, nl;
        for ( i=0; i<bin->nrefs; ++i ) {
		n = fields_find( bin->ref[i], "CROSSREF", -1 );
		if ( n==-1 ) continue;
		ncross = bibtexin_findref( bin, bin->ref[i]->data[n].data );
		if ( ncross==-1 ) {
			int n1 = fields_find( bin->ref[i], "REFNUM", -1 );
			fprintf( stderr, "Cannot find cross-reference '%s'",
				bin->ref[i]->data[n].data);
			if ( n1!=-1 )
				fprintf( stderr, " for reference '%s'\n",
					bin->ref[i]->data[n1].data );
			fprintf( stderr, "\n" );
			
			continue;
		}
		ntype = fields_find( bin->ref[i], "TYPE", -1 );
		type = bin->ref[i]->data[ntype].data;
		bin->ref[i]->used[n] = 1;
		for ( j=0; j<bin->ref[ncross]->nfields; ++j ) {
			nt = bin->ref[ncross]->tag[j].data;
			if ( !strcasecmp( nt, "TYPE" ) ) continue;
			if ( !strcasecmp( nt, "REFNUM" ) ) continue;
			if ( !strcasecmp( nt, "TITLE" ) ) {
				if ( !strcasecmp( type, "Inproceedings" ) ||
				     !strcasecmp( type, "Incollection" ) )
					nt = booktitle;
			}
			nd = bin->ref[ncross]->data[j].data;
			nl = bin->ref[ncross]->level[j] + 1;
			fields_add( bin->ref[i], nt, nd, nl );

		}
	}
}

static void
bibtexin_cleanref( fields *bibin )
{
	newstr *t, *d;
	int i;
	for ( i=0; i<bibin->nfields; ++i ) {
		t = &( bibin->tag[i] );
		d = &( bibin->data[i] );
		bibtex_cleandata( d, bibin );
		if ( !strsearch( t->data, "AUTHORS" ) ) {
			newstr_findreplace( d, "\n", " " );
			newstr_findreplace( d, "\r", " " );
		}
		else if ( !strsearch( t->data, "ABSTRACT" ) &&
		     !strsearch( t->data, "SUMMARY" ) ) {
			newstr_findreplace( d, "\n", "" );
			newstr_findreplace( d, "\r", "" );
		}
	}
}

void
bibtexin_cleanf( bibl *bin )
{
	long i;
        for ( i=0; i<bin->nrefs; ++i )
		bibtexin_cleanref( bin->ref[i] );
	bibtexin_crossref( bin );
}

/*
 * process_names( info, newtag, field, level);
 *
 * split names in author list separated by and's (use '|' character)
 * and add names
 */
static void
process_names( fields *info, char *tag, newstr *data, int level )
{
	newstr_findreplace( data, " and ", "|" );
	name_add( info, tag, data->data, level );
}

static void
process_pages( fields *info, newstr *s, int level )
{
	char *p, *q;
	newstr sp, ep;

	newstr_init( &sp );
	newstr_init( &ep );

	newstr_findreplace( s, " ", "" );

	p = q = s->data;
	while ( isdigit( *q ) ) q++;
	newstr_segcpy( &sp, p, q );
	if ( sp.len>0 )
		fields_add( info, "PAGESTART", sp.data, level );

	p = q;
	while ( *p && !isdigit(*p) ) p++;
	q = p;
	while ( isdigit( *q ) ) q++;
	newstr_segcpy( &ep, p, q );
	if ( ep.len>0 )
		fields_add( info, "PAGEEND", ep.data, level );

	newstr_free(&sp);
	newstr_free(&ep);
}

static void
process_url( fields *info, char *p, int level )
{
	if ( !strncasecmp( p, "\\urllink", 8 ) )
		fields_add( info, "URL", p+8, level );
	else if ( !strncasecmp( p, "\\url", 4 ) )
		fields_add( info, "URL", p+4, level );
	else fields_add( info, "URL", p, level );
}

int
bibtexin_typef( fields *bibin, char *filename, int nrefs, 
	variants *all, int nall )
{
	int reftype, n;
	n = fields_find( bibin, "TYPE", 0 );
	if ( n!=-1 )
		reftype = get_reftype( (bibin->data[n]).data, nrefs,
			all, nall );
	else
		reftype = get_reftype( "", nrefs, all, nall ); /*default */
	return reftype;
}

void
bibtexin_convertf( fields *bibin, fields *info, int reftype, int verbose,
		variants *all, int nall )
{
	newstr *t, *d;
	int process, level, i, n;
	char *newtag;
	for ( i=0; i<bibin->nfields; ++i ) {
		d = &( bibin->data[i] );
		if ( d->len == 0 ) continue; /* skip ones with no data */
		/* skip ones already "used" such as successful crossref */
		if ( bibin->used[i] ) continue;
		t = &( bibin->tag[i] );
		n = process_findoldtag( t->data, reftype, all, nall );
		if ( n==-1 ) {
			if ( verbose && strcmp(t->data,"TYPE") ) {
				fprintf( stderr, "Cannot find tag '%s'\n", 
					t->data );
			}
			continue;
		}
		process = ((all[reftype]).tags[n]).processingtype;
		if ( process == ALWAYS ) continue; /* add these later */
		level   = ((all[reftype]).tags[n]).level;
		newtag  = ((all[reftype]).tags[n]).newstr;
		if ( process==SIMPLE )
			fields_add( info, newtag, d->data, level );
		else if ( process==TITLE )
			title_process( info, "TITLE", d->data, level);
		else if ( process==PERSON )
			process_names( info, newtag, d, level);
		else if ( process==PAGES )
			process_pages( info, d, level);
		else if ( process==BIBTEX_URL )
			process_url( info, d->data, level );
	}
{
if ( verbose ) {
	for ( i=0; i<info->nfields; ++i )
		fprintf(stderr, "'%s' %d = '%s'\n",info->tag[i].data,info->level[i],
			info->data[i].data);
}
}
}

