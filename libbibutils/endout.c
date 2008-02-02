/*
 * endout.c
 *
 * Copyright (c) Chris Putnam 2004-7
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
#include "endout.h"

enum {
	TYPE_UNKNOWN = 0,
	TYPE_GENERIC,
	TYPE_ARTICLE,
	TYPE_MAGARTICLE,
	TYPE_BOOK,
	TYPE_INBOOK,
	TYPE_INPROCEEDINGS,
	TYPE_HEARING,
	TYPE_BILL,
	TYPE_CASE,
	TYPE_NEWSPAPER,
	TYPE_COMMUNICATION,
	TYPE_BROADCAST,
	TYPE_MANUSCRIPT,
	TYPE_REPORT,
	TYPE_THESIS,
	TYPE_MASTERSTHESIS,
	TYPE_PHDTHESIS,
	TYPE_DIPLOMATHESIS,
	TYPE_DOCTORALTHESIS,
	TYPE_HABILITATIONTHESIS,
	TYPE_PATENT,
	TYPE_PROGRAM
};

typedef struct match_type {
	char *name;
	int type;
} match_type;

static int
get_type( fields *info )
{
	match_type match_genres[] = {
		{ "academic journal",          TYPE_ARTICLE },
		{ "magazine",                  TYPE_MAGARTICLE },
		{ "conference publication",    TYPE_INPROCEEDINGS },
		{ "hearing",                   TYPE_HEARING },
		{ "Ph.D. thesis",              TYPE_PHDTHESIS },
		{ "Masters thesis",            TYPE_MASTERSTHESIS },
		{ "Diploma thesis",            TYPE_DIPLOMATHESIS },
		{ "Doctoral thesis",           TYPE_DOCTORALTHESIS },
		{ "Habilitation thesis",       TYPE_HABILITATIONTHESIS },
		{ "legislation",               TYPE_BILL },
		{ "newspaper",                 TYPE_NEWSPAPER },
		{ "communication",             TYPE_COMMUNICATION },
		{ "manuscript",                TYPE_MANUSCRIPT },
		{ "report",                    TYPE_REPORT },
		{ "legal case and case notes", TYPE_CASE },
		{ "patent",                    TYPE_PATENT },
	};
	int nmatch_genres = sizeof( match_genres ) / sizeof( match_genres[0] );

	char *tag, *data;
	int i, j, type = TYPE_UNKNOWN;

	for ( i=0; i<info->nfields; ++i ) {
		tag = info->tag[i].data;
		if ( strcasecmp( tag, "GENRE" )!=0 &&
		     strcasecmp( tag, "NGENRE" )!=0 ) continue;
		data = info->data[i].data;
		for ( j=0; j<nmatch_genres; ++j ) {
			if ( !strcasecmp( data, match_genres[j].name ) ) {
				type = match_genres[j].type;
				fields_setused( info, i );
			}
		}
		if ( type==TYPE_UNKNOWN ) {
			if ( !strcasecmp( data, "periodical" ) )
				type = TYPE_ARTICLE;
			else if ( !strcasecmp( data, "theses" ) )
				type = TYPE_THESIS;
			else if ( !strcasecmp( data, "book" ) ) {
				if ( info->level[i]==0 ) type = TYPE_BOOK;
				else type = TYPE_INBOOK;
			}
			else if ( !strcasecmp( data, "collection" ) ) {
				if ( info->level[i]==0 ) type = TYPE_BOOK;
				else type = TYPE_INBOOK;
			}
			if ( type!=TYPE_UNKNOWN ) fields_setused( info, i );
		}
	}
	if ( type==TYPE_UNKNOWN ) {
		for ( i=0; i<info->nfields; ++i ) {
			if ( strcasecmp( info->tag[i].data, "RESOURCE" ) )
				continue;
			data = info->data[i].data;
			if ( !strcasecmp( data, "moving image" ) )
				type = TYPE_BROADCAST;
			else if ( !strcasecmp( data, "software, multimedia" ) )
				type = TYPE_PROGRAM;
			if ( type!=TYPE_UNKNOWN ) fields_setused( info, i );
		}
	}

	/* default to generic */
	if ( type==TYPE_UNKNOWN ) type = TYPE_GENERIC;
	
	return type;
}

static void
output_type( FILE *fp, int type )
{
	fprintf( fp, "%%0 ");
	switch( type ) {
		case TYPE_GENERIC: fprintf( fp, "Generic" ); break;
		case TYPE_ARTICLE: fprintf( fp, "Journal Article" ); break;
		case TYPE_MAGARTICLE: fprintf( fp, "Magazine Article" ); break;
		case TYPE_INBOOK: fprintf( fp, "Book Section" ); break;
		case TYPE_BOOK: fprintf( fp, "Book" ); break;
		case TYPE_HEARING: fprintf( fp, "Hearing" ); break;
		case TYPE_BILL: fprintf( fp, "Bill" ); break;
		case TYPE_CASE: fprintf( fp, "Case" ); break;
		case TYPE_BROADCAST: fprintf( fp, "Film or Broadcast" ); break;
		case TYPE_NEWSPAPER: fprintf( fp, "Newspaper Article" ); break;
		case TYPE_MANUSCRIPT: fprintf( fp, "Manuscript" ); break;
		case TYPE_REPORT: fprintf( fp, "Report" ); break;
		case TYPE_THESIS: 
		case TYPE_PHDTHESIS: 
		case TYPE_MASTERSTHESIS: 
		case TYPE_DIPLOMATHESIS:
		case TYPE_DOCTORALTHESIS:
		case TYPE_HABILITATIONTHESIS:
			fprintf( fp, "Thesis" ); break;
		case TYPE_COMMUNICATION: fprintf( fp, "Personal Communication" ); break;
		case TYPE_INPROCEEDINGS: fprintf( fp, "Conference Proceedings" ); break;
		case TYPE_PATENT: fprintf( fp, "Patent" ); break;
		case TYPE_PROGRAM: fprintf( fp, "Computer Program" ); break;
	}
	fprintf( fp, "\n" );
}

static void
output_title( FILE *fp, fields *info, char *full, char *sub, char *endtag, 
		int level )
{
	int n1 = fields_find( info, full, level );
	int n2 = fields_find( info, sub, level );
	if ( n1!=-1 ) {
		fprintf( fp, "%s %s", endtag, info->data[n1].data );
		fields_setused( info, n1 );
		if ( n2!=-1 ) {
			if ( info->data[n1].data[info->data[n1].len]!='?' )
				fprintf( fp, ": " );
			else fprintf( fp, " " );
			fprintf( fp, "%s", info->data[n2].data );
			fields_setused( info, n2 );
		}
		fprintf( fp, "\n" );
	}
}

static void
output_person( FILE *fp, char *p )
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
output_people( FILE *fp, fields *info, char *tag, char *entag, int level )
{
	int i;
	for ( i=0; i<info->nfields; ++i ) {
		if ( level!=-1 && info->level[i]!=level ) continue;
		if ( !strcasecmp( info->tag[i].data, tag ) ) {
			fprintf( fp, "%s ", entag );
			output_person( fp, info->data[i].data );
			fprintf( fp, "\n" );
		}
	}
}

static void
output_pages( FILE *fp, fields *info )
{
	int sn = fields_find( info, "PAGESTART", -1 );
	int en = fields_find( info, "PAGEEND", -1 );
	int ar = fields_find( info, "ARTICLENUMBER", -1 );
	if ( sn!=-1 || en!=-1 ) {
		fprintf( fp, "%%P ");
		if ( sn!=-1 ) fprintf( fp, "%s", info->data[sn].data );
		if ( sn!=-1 && en!=-1 ) fprintf( fp, "-" );
		if ( en!=-1 ) fprintf( fp, "%s", info->data[en].data );
		fprintf( fp, "\n");
	} else if ( ar!=-1 ) {
		fprintf( fp, "%%P %s", info->data[ar].data );
	}
}

static void
output_year( FILE *fp, fields *info, int level )
{
	int year = fields_find( info, "YEAR", level );
	if ( year==-1 ) year = fields_find( info, "PARTYEAR", level );
	if ( year!=-1 )
		fprintf( fp, "%%D %s\n", info->data[year].data );
}

static void
output_monthday( FILE *fp, fields *info, int level )
{
	char *months[12] = { "January", "February", "March", "April",
		"May", "June", "July", "August", "September", "October",
		"November", "December" };
	int m;
	int month = fields_find( info, "MONTH", level );
	int day   = fields_find( info, "DAY", level );
	if ( month==-1 ) month = fields_find( info, "PARTMONTH", level );
	if ( day==-1 ) day = fields_find( info, "PARTDAY", level );
	if ( month!=-1 || day!=-1 ) {
		fprintf( fp, "%%8 " );
		if ( month!=-1 ) {
			m = atoi( info->data[month].data );
			if ( m>0 && m<13 )
				fprintf( fp, "%s", months[m-1] );
			else
				fprintf( fp, "%s", info->data[month].data );
		}
		if ( month!=-1 && day!=-1 ) fprintf( fp, " " );
		if ( day!=-1 ) fprintf( fp, "%s", info->data[day].data );
		fprintf( fp, "\n" );
	}
}


static void
output_thesishint( FILE *fp, int type )
{
	if ( type==TYPE_MASTERSTHESIS )
		fprintf( fp, "%%9 Masters thesis\n" );
	else if ( type==TYPE_PHDTHESIS )
		fprintf( fp, "%%9 Ph.D. thesis\n" );
	else if ( type==TYPE_DIPLOMATHESIS )
		fprintf( fp, "%%9 Diploma thesis\n" );
	else if ( type==TYPE_DOCTORALTHESIS )
		fprintf( fp, "%%9 Doctoral thesis\n" );
	else if ( type==TYPE_HABILITATIONTHESIS )
		fprintf( fp, "%%9 Habilitation thesis\n" );
}

static void
output_easyall( FILE *fp, fields *info, char *tag, char *entag, int level )
{
	int i;
	for ( i=0; i<info->nfields; ++i ) {
		if ( level!=-1 && info->level[i]!=level ) continue;
		if ( !strcmp( info->tag[i].data, tag ) )
			fprintf( fp, "%s %s\n", entag, info->data[i].data );
	}
}

static void
output_easy( FILE *fp, fields *info, char *tag, char *entag, int level )
{
	int n = fields_find( info, tag, level );
	if ( n!=-1 )
		fprintf( fp, "%s %s\n", entag, info->data[n].data );
}

void
endout_write( fields *info, FILE *fp, int format_opts, unsigned long refnum )
{
	int type;
	fields_clearused( info );
	type = get_type( info );
	output_type( fp, type );
	output_title( fp, info, "TITLE", "SUBTITLE", "%T", 0 );
	output_title( fp, info, "SHORTTITLE", "SHORTSUBTITLE", "%!", 0 );
	output_people( fp, info, "AUTHOR", "%A", 0 );
	output_people( fp, info, "EDITOR", "%E", -1 );
	if ( type==TYPE_CASE )
		output_easy( fp, info, "AUTHOR:CORP", "%I", 0 );
	else if ( type==TYPE_HEARING )
		output_easyall( fp, info, "AUTHOR:CORP", "%S", 0 );
	else if ( type==TYPE_NEWSPAPER )
		output_people( fp, info, "REPORTER", "%A", 0 );
	else if ( type==TYPE_COMMUNICATION )
		output_people( fp, info, "RECIPIENT", "%E", -1 );
	else {
		output_easy( fp, info, "AUTHOR:CORP", "%A", 0 );
		output_easy( fp, info, "AUTHOR:ASIS", "%A", 0 );
		output_easy( fp, info, "EDITOR:CORP", "%E", -1 );
		output_easy( fp, info, "EDITOR:ASIS", "%E", -1 );
	}
	if ( type==TYPE_ARTICLE || type==TYPE_MAGARTICLE )
		output_title( fp, info, "TITLE", "SUBTITLE", "%J", 1 );
	else output_title( fp, info, "TITLE", "SUBTITLE", "%B", 1 );
	output_year( fp, info, -1 );
	output_monthday( fp, info, -1 );
	output_easy( fp, info, "VOLUME", "%V", -1 );
	output_easy( fp, info, "ISSUE", "%N", -1 );
	output_easy( fp, info, "NUMBER", "%N", -1 );
	output_easy( fp, info, "EDITION", "%7", -1 );
	output_easy( fp, info, "PUBLISHER", "%I", -1 );
	output_easy( fp, info, "ADDRESS", "%C", -1 );
	output_easy( fp, info, "DEGREEGRANTOR", "%C", -1 );
	output_easy( fp, info, "DEGREEGRANTOR:CORP", "%C", -1 );
	output_easy( fp, info, "DEGREEGRANTOR:ASIS", "%C", -1 );
	output_easy( fp, info, "SERIALNUM", "%@", -1 );
	output_easy( fp, info, "ISSN", "%@", -1 );
	output_easy( fp, info, "ISBN", "%@", -1 );
	output_easy( fp, info, "REFNUM", "%F", -1 );
	output_easyall( fp, info, "NOTES", "%O", -1 );
	output_easy( fp, info, "ABSTRACT", "%X", -1 );
	output_easy( fp, info, "CLASSIFICATION", "%L", -1 );
	output_easyall( fp, info, "KEYWORD", "%K", -1 );
	output_easyall( fp, info, "NGENRE", "%9", -1 );
	output_thesishint( fp, type );
	output_easyall( fp, info, "URL", "%U", -1 ); 
	output_pages( fp, info );
	fprintf( fp, "\n" );
	fflush( fp );
}

