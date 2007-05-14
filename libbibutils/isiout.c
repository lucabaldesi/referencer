/*
 * isiout.c
 *
 * Copyright (c) Chris Putnam 2007
 *
 * Source code released under the GPL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "newstr.h"
#include "strsearch.h"
#include "fields.h"

enum {
        TYPE_UNKNOWN = 0,
        TYPE_ARTICLE = 1,
        TYPE_INBOOK  = 2,
        TYPE_BOOK    = 3,
};

static void
output_type( FILE *fp, int type )
{
	fprintf( fp, "PT " );
	if ( type==TYPE_ARTICLE ) fprintf( fp, "Journal" );
	else if ( type==TYPE_INBOOK ) fprintf( fp, "Chapter" );
	else if ( type==TYPE_BOOK ) fprintf( fp, "Book" );
	else fprintf( fp, "Unknown" );
	fprintf( fp, "\n" );
}

static int 
get_type( fields *info )
{
        int type = TYPE_UNKNOWN, i;
        for ( i=0; i<info->nfields; ++i ) {
                if ( strcasecmp( info->tag[i].data, "GENRE" ) &&
                     strcasecmp( info->tag[i].data, "NGENRE") ) continue;
                if ( !strcasecmp( info->data[i].data, "periodical" ) ||
                     !strcasecmp( info->data[i].data, "academic journal" ) )
                        type = TYPE_ARTICLE;
                else if ( !strcasecmp( info->data[i].data, "book" ) ) {
                        if ( info->level[i]==0 ) type=TYPE_BOOK;
                        else type=TYPE_INBOOK;
                }
        }
        return type;
}

static void
output_title( FILE *fp, fields *info, long refnum, char *isitag, int level )
{
        int n1 = fields_find( info, "TITLE", level );
        int n2 = fields_find( info, "SUBTITLE", level );
        if ( n1!=-1 ) {
                fprintf( fp, "%s %s", isitag, info->data[n1].data );
                if ( n2!=-1 ) {
                        if ( info->data[n1].data[info->data[n1].len]!='?' )
                                fprintf( fp, ": " );
                        else fprintf( fp, " " );
                        fprintf( fp, "%s", info->data[n2].data );
                }
                fprintf( fp, "\n" );
        }
}

static void
output_abbrtitle( FILE *fp, fields *info, long refnum, char *isitag, int level )
{
        int n1 = fields_find( info, "SHORTTITLE", level );
        int n2 = fields_find( info, "SHORTSUBTITLE", level );
        if ( n1!=-1 ) {
                fprintf( fp, "%s %s", isitag, info->data[n1].data );
                if ( n2!=-1 ){
                        if ( info->data[n1].data[info->data[n1].len]!='?' )
                                fprintf( fp, ": " );
                        else fprintf( fp, " " );
                        fprintf( fp, "%s", info->data[n2].data );
                }
                fprintf( fp, "\n" );
        }
}

static void
output_person( FILE *fp, char *name )
{
	int n = 0, nchars = 0;
	char *p = name;
	while ( *p ) {
		if ( *p=='|' )  { n++; nchars=0; }
		else {
			if ( n==1 && nchars<2 ) fprintf( fp, ", " );
			if ( n==0 || (n>0 && nchars<2) ) {
				fprintf( fp, "%c", *p );
			}
		}
		nchars++;
		p++;
	}
}

static void
output_keywords( FILE *fp, fields *info, long refnum )
{
	int n = 0, i;
	for ( i=0; i<info->nfields; ++i ) {
		if ( strcasecmp( info->tag[i].data, "KEYWORD" ) ) continue;
		if ( n==0 )
			fprintf( fp, "DE " );
		if ( n>0 )
			fprintf( fp, "; " );
		fprintf( fp, "%s", info->data[i].data );
		n++;
	}
	if ( n ) fprintf( fp, "\n" );
}

static void
output_people( FILE *fp, fields *info, long refnum, char *tag, char *isitag, int level )
{
	int n = 0, i;
        for ( i=0; i<info->nfields; ++i ) {
                if ( strcasecmp( info->tag[i].data, tag ) ) continue;
		if ( level!=-1 && info->level[i]!=level ) continue;
		if ( n==0 ) {
			fprintf( fp, "%s ", isitag );
		} else {
			fprintf( fp, "   " );
		}
		output_person( fp, info->data[i].data );
		fprintf( fp, "\n" );
		n++;
	}
}

static void
output_easy( FILE *fp, fields *info, long refnum, char *tag, char *isitag, int level )
{
        int n = fields_find( info, tag, level );
        if ( n!=-1 ) {
                fprintf( fp, "%s %s\n", isitag, info->data[n].data );
        }
}

void
isi_write( fields *info, FILE *fp, int format_opts, unsigned long refnum )
{
        int type;
/*
{ int i;
fprintf(stderr,"REF----\n");
for ( i=0; i<info->nfields; ++i )
        fprintf(stderr,"\t'%s'\t'%s'\t%d\n",info->tag[i].data,info->data[i].data
,info->level[i]);
}
*/
        type = get_type( info );
        output_type( fp, type );
	output_people( fp, info, refnum, "AUTHOR", "AU", 0 );
/*        output_people( fp, info, refnum, "AUTHOR", "AU", 0 );
        output_people( fp, info, refnum, "CORPAUTHOR", "AU", 0 );
        output_people( fp, info, refnum, "AUTHOR", "A2", 1 );
        output_people( fp, info, refnum, "CORPAUTHOR", "A2", 1 );
        output_people( fp, info, refnum, "AUTHOR", "A3", 2 );
        output_people( fp, info, refnum, "CORPAUTHOR", "A3", 2 );
        output_people( fp, info, refnum, "EDITOR", "ED", -1 );
        output_people( fp, info, refnum, "CORPEDITOR", "ED", -1 );*/
/*        output_date( fp, info, refnum );*/
        output_title( fp, info, refnum, "TI", 0 );
        if ( type==TYPE_ARTICLE ) {
                output_title( fp, info, refnum, "SO", 1 );
		output_abbrtitle( fp, info, refnum, "JI", 1 );
	}
        else output_title( fp, info, refnum, "BT", 1 );

	output_easy( fp, info, refnum, "PARTMONTH", "PD", -1 );
	output_easy( fp, info, refnum, "PARTYEAR", "PY", -1 );

	output_easy( fp, info, refnum, "PAGESTART", "BP", -1 );
	output_easy( fp, info, refnum, "PAGEEND",   "EP", -1 );
        output_easy( fp, info, refnum, "ARTICLENUMBER", "Ae", -1 );
        /* output article number as pages */
	output_easy( fp, info, refnum, "TOTALPAGES","PG", -1 );

        output_easy( fp, info, refnum, "VOLUME",    "VL", -1 );
        output_easy( fp, info, refnum, "ISSUE",     "IS", -1 );
        output_easy( fp, info, refnum, "NUMBER",    "IS", -1 );
	output_easy( fp, info, refnum, "DOI",       "DI", -1 );
	output_easy( fp, info, refnum, "ISIREFNUM", "UT", -1 );
	output_easy( fp, info, refnum, "LANGUAGE",  "LA", -1 );
	output_easy( fp, info, refnum, "ISIDELIVERNUM", "GA", -1 );
	output_keywords( fp, info, refnum );
	output_easy( fp, info, refnum, "ABSTRACT",  "AB", -1 );
	output_easy( fp, info, refnum, "TIMESCITED", "TC", -1 );
	output_easy( fp, info, refnum, "NUMBERREFS", "NR", -1 );
	output_easy( fp, info, refnum, "CITEDREFS",  "CR", -1 );
	output_easy( fp, info, refnum, "ADDRESS",    "PI", -1 );

/*        output_easy( fp, info, refnum, "PUBLISHER", "PB", -1 );
        output_easy( fp, info, refnum, "DEGREEGRANTOR", "PB", -1 );
        output_easy( fp, info, refnum, "ADDRESS", "CY", -1 );
        output_easy( fp, info, refnum, "ABSTRACT", "AB", -1 );
        output_easy( fp, info, refnum, "ISSN", "SN", -1 );
        output_easy( fp, info, refnum, "ISBN", "SN", -1 );
        output_easy( fp, info, refnum, "URL", "UR", -1 );
        output_pubmed( fp, info, refnum );
        output_easy( fp, info, refnum, "NOTES", "N1", -1 );
        output_easy( fp, info, refnum, "REFNUM", "ID", -1 );*/
        fprintf( fp, "ER\n\n" );
        fflush( fp );
}



