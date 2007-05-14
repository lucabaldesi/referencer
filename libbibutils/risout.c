/*
 * risout.c
 *
 * Copyright (c) Chris Putnam 2003-7
 *
 * Program and source code released under the GPL
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "newstr.h"
#include "strsearch.h"
#include "fields.h"
#include "risout.h"

enum { 
	TYPE_UNKNOWN = 0,
	TYPE_STD     = 1,     /* standard/generic */
	TYPE_ARTICLE = 2,
	TYPE_INBOOK  = 3,
	TYPE_BOOK    = 4,
	TYPE_CONF    = 5,     /* conference */
	TYPE_STAT    = 6,     /* statute */
	TYPE_HEAR    = 7,     /* hearing */
	TYPE_CASE    = 8,     /* case */
	TYPE_NEWS    = 9,     /* newspaper */
	TYPE_MPCT    = 10,
	TYPE_PCOMM   = 11,    /* personal communication */
	TYPE_PAMP    = 12,    /* pamphlet */
	TYPE_ELEC    = 13,    /* electronic */
	TYPE_THESIS  = 14,
	TYPE_REPORT  = 15
};

/* Try to determine type of reference from <genre></genre> */
static int
get_type_genre( fields *info )
{
	int type = TYPE_UNKNOWN, i;
	for ( i=0; i<info->nfields; ++i ) {
		if ( strcasecmp( info->tag[i].data, "GENRE" ) &&
		     strcasecmp( info->tag[i].data, "NGENRE") ) continue;
		if ( !strcasecmp( info->data[i].data, "periodical" ) ||
		     !strcasecmp( info->data[i].data, "academic journal" ) )
			type = TYPE_ARTICLE;
		else if ( !strcasecmp( info->data[i].data, "conference publication" ) )
			type = TYPE_CONF;
		else if ( !strcasecmp( info->data[i].data, "legislation" ) )
			type = TYPE_STAT;
		else if ( !strcasecmp( info->data[i].data, "newspaper" ) )
			type = TYPE_NEWS;
		else if ( !strcasecmp( info->data[i].data, "communication" ) )
			type = TYPE_PCOMM;
		else if ( !strcasecmp( info->data[i].data, "hearing" ) )
			type = TYPE_HEAR;
		else if ( !strcasecmp( info->data[i].data, "electronic" ) ) {
			type = TYPE_ELEC;
		}
		else if ( !strcasecmp( info->data[i].data, "legal case and case notes" ) )
			type = TYPE_CASE;
		else if ( !strcasecmp( info->data[i].data, "book" ) ) {
			if ( info->level[i]==0 ) type=TYPE_BOOK;
			else type=TYPE_INBOOK;
		}
		else if ( !strcasecmp( info->data[i].data, "collection" ) ) {
			if ( info->level[i]==0 ) type=TYPE_BOOK;
			else type=TYPE_INBOOK;
		}
		else if ( !strcasecmp( info->data[i].data, "thesis" ) )
			type = TYPE_THESIS;
		else if ( !strcasecmp( info->data[i].data, "report" ) )
			type = TYPE_REPORT;
	}
	return type;
}

/* Try to determine type of reference from <issuance></issuance> and */
/* <typeOfReference></typeOfReference> */
static int
get_type_issuance( fields *info )
{
	int type = TYPE_UNKNOWN;
	int i, monographic = 0, text = 0, monographic_level = 0;
	for ( i=0; i<info->nfields; ++i ) {
		if ( !strcasecmp( info->tag[i].data, "issuance" ) &&
		     !strcasecmp( info->data[i].data, "MONOGRAPHIC" ) ){
			monographic = 1;
			monographic_level = info->level[i];
		}
		if ( !strcasecmp( info->tag[i].data, "typeOfResource" ) &&
		     !strcasecmp( info->data[i].data,"text") ) {
			text = 1;
		}
	}
	if ( monographic && text ) {
		if ( monographic_level==0 ) type=TYPE_BOOK;
		else if ( monographic_level>0 ) type=TYPE_INBOOK;
	}
	return type;
}

static int
get_type( fields *info )
{
	int type;
	type = get_type_genre( info );
	if ( type==TYPE_UNKNOWN ) type = get_type_issuance( info );
	if ( type==TYPE_UNKNOWN ) type = TYPE_STD;
	return type;
}

static void
output_type( FILE *fp, int type )
{
	fprintf( fp, "TY  - " );
	switch ( type ) {
		case TYPE_STD: fprintf( fp, "STD\n" ); break;
		case TYPE_ARTICLE: fprintf( fp, "JOUR\n" ); break;
		case TYPE_BOOK: fprintf( fp, "BOOK\n" ); break;
		case TYPE_INBOOK: fprintf( fp, "CHAP\n" ); break;
		case TYPE_CONF: fprintf( fp, "CONF\n" ); break;
		case TYPE_STAT: fprintf( fp, "STAT\n" ); break;
		case TYPE_HEAR: fprintf( fp, "HEAR\n" ); break;
		case TYPE_CASE: fprintf( fp, "CASE\n" ); break;
		case TYPE_NEWS: fprintf( fp, "NEWS\n" ); break;
		case TYPE_MPCT: fprintf( fp, "MPCT\n" ); break;
		case TYPE_PCOMM: fprintf( fp, "PCOMM\n" ); break;
		case TYPE_PAMP: fprintf( fp, "PAMP\n" ); break;
		case TYPE_ELEC: fprintf( fp, "ELEC\n" ); break;
		case TYPE_THESIS: fprintf( fp, "THES\n" ); break;
		case TYPE_REPORT: fprintf( fp, "RPRT\n" ); break;
	}
}

static void
output_person ( FILE *fp, char *p )
{
	int nseps = 0, nch;
	while ( *p ) {
		nch = 0;
		if ( nseps ) fprintf( fp, " " );
		while ( *p && *p!='|' ) {
			fprintf( fp, "%c", *p++ );
			nch++;
		}
		if ( *p=='|' ) p++;
		if ( nseps==0 ) fprintf( fp, "," );
		else if ( nch==1 ) fprintf( fp, "." ); 
		nseps++;
	}
}

static void
output_people( FILE *fp, fields *info, long refnum, char *tag, 
		char *ristag, int level )
{
	int i;
	for ( i=0; i<info->nfields; ++i ) {
		if ( level!=-1 && info->level[i]!=level ) continue;
		if ( !strcasecmp( info->tag[i].data, tag ) ) {
			fprintf( fp, "%s  - ", ristag );
			output_person ( fp, info->data[i].data );
			fprintf( fp, "\n" );
		}
	}
}

static void
output_date( FILE *fp, fields *info, long refnum )
{
	int year = fields_find( info, "YEAR", -1 );
	int month = fields_find( info, "MONTH", -1 );
	int day = fields_find( info, "DAY", -1 );
	if ( year==-1 ) year = fields_find( info, "PARTYEAR", -1 );
	if ( month==-1 ) month = fields_find( info, "PARTMONTH", -1 );
	if ( day==-1 ) day = fields_find( info, "PARTDAY", -1 );
	if ( year==-1 && month==-1 && day==-1 ) return;
	fprintf( fp, "PY  - " );
	if ( year!=-1 ) fprintf( fp, "%s", info->data[year].data );
	fprintf( fp, "/" );
	if ( month!=-1 ) fprintf( fp, "%s", info->data[month].data );
	fprintf( fp, "/" );
	if ( day!=-1 ) fprintf( fp, "%s", info->data[day].data );
	fprintf( fp, "\n" );
}

static void
output_title( FILE *fp, fields *info, long refnum, char *ristag, int level )
{
	int n1 = fields_find( info, "TITLE", level );
	int n2 = fields_find( info, "SUBTITLE", level );
	if ( n1!=-1 ) {
		fprintf( fp, "%s  - %s", ristag, info->data[n1].data );
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
output_abbrtitle( FILE *fp, fields *info, long refnum, char *ristag, int level )
{
	int n1 = fields_find( info, "SHORTTITLE", level );
	int n2 = fields_find( info, "SHORTSUBTITLE", level );
	if ( n1!=-1 ) {
		fprintf( fp, "%s  - %s", ristag, info->data[n1].data );
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
output_pages( FILE *fp, fields *info, long refnum )
{
	int sn = fields_find( info, "PAGESTART", -1 );
	int en = fields_find( info, "PAGEEND", -1 );
	if ( sn!=-1 ) 
		fprintf( fp, "SP  - %s\n", info->data[sn].data );
	if ( en!=-1 ) 
		fprintf( fp, "EP  - %s\n", info->data[en].data );
}

static void
output_keywords( FILE *fp, fields *info, long refnum )
{
	int i;
	for ( i=0; i<info->nfields; ++i ) {
		if ( !strcmp( info->tag[i].data, "KEYWORD" ) )
			fprintf( fp, "KW  - %s\n", info->data[i].data );
	}
}

static void
output_pubmed( FILE *fp, fields *info, long refnum )
{
	int i;
	for ( i=0; i<info->nfields; ++i ) {
		if ( !strcmp( info->tag[i].data, "PUBMED" ) )
			fprintf( fp, "UR  - PM:%s\n", info->data[i].data );
	}
}

static void
output_easy( FILE *fp, fields *info, long refnum, char *tag, char *ristag, int level )
{
	int n = fields_find( info, tag, level );
	if ( n!=-1 ) {
		fprintf( fp, "%s  - %s\n", ristag, info->data[n].data );
	}
}

void
risout_write( fields *info, FILE *fp, int format_opts, unsigned long refnum )
{
	int type;
/*
{ int i;
fprintf(stderr,"REF----\n");
for ( i=0; i<info->nfields; ++i )
	fprintf(stderr,"\t'%s'\t'%s'\t%d\n",info->tag[i].data,info->data[i].data,info->level[i]);
}
*/
	type = get_type( info );
	output_type( fp, type );
	output_people( fp, info, refnum, "AUTHOR", "AU", 0 );
	output_people( fp, info, refnum, "CORPAUTHOR", "AU", 0 );
	output_people( fp, info, refnum, "AUTHOR", "A2", 1 );
	output_people( fp, info, refnum, "CORPAUTHOR", "A2", 1 );
	output_people( fp, info, refnum, "AUTHOR", "A3", 2 );
	output_people( fp, info, refnum, "CORPAUTHOR", "A3", 2 );
	output_people( fp, info, refnum, "EDITOR", "ED", -1 );
	output_people( fp, info, refnum, "CORPEDITOR", "ED", -1 );
	output_date( fp, info, refnum );
	output_title( fp, info, refnum, "TI", 0 );
	output_abbrtitle( fp, info, refnum, "T2", -1 );
	if ( type==TYPE_ARTICLE ) {
		output_title( fp, info, refnum, "JO", 1 );
	}
	else output_title( fp, info, refnum, "BT", 1 );
	output_title( fp, info, refnum, "T3", 2 );
	output_pages( fp, info, refnum );
	output_easy( fp, info, refnum, "VOLUME", "VL", -1 );
	output_easy( fp, info, refnum, "ISSUE", "IS", -1 );
	output_easy( fp, info, refnum, "NUMBER", "IS", -1 );
	/* output article number as pages */
	output_easy( fp, info, refnum, "ARTICLENUMBER", "SP", -1 );
	output_easy( fp, info, refnum, "PUBLISHER", "PB", -1 );
	output_easy( fp, info, refnum, "DEGREEGRANTOR", "PB", -1 );
	output_easy( fp, info, refnum, "ADDRESS", "CY", -1 );
	output_keywords( fp, info, refnum );
	output_easy( fp, info, refnum, "ABSTRACT", "AB", -1 );
	output_easy( fp, info, refnum, "ISSN", "SN", -1 );
	output_easy( fp, info, refnum, "ISBN", "SN", -1 );
	output_easy( fp, info, refnum, "URL", "UR", -1 );
	output_pubmed( fp, info, refnum );
	output_easy( fp, info, refnum, "NOTES", "N1", -1 );
	output_easy( fp, info, refnum, "REFNUM", "ID", -1 );
	fprintf( fp, "ER  - \n" );
	fflush( fp );
}

