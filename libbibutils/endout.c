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
	TYPE_THESIS
};

static int
get_type( fields *info )
{
	int i, type = TYPE_UNKNOWN;

	for ( i=0; i<info->nfields; ++i ) {
		if ( strcasecmp( info->tag[i].data, "GENRE" )!=0 &&
		     strcasecmp( info->tag[i].data, "NGENRE" )!=0 ) continue;
		if ( !strcasecmp( info->data[i].data, "periodical" ) ||
		     !strcasecmp( info->data[i].data, "academic journal" ) )
			type = TYPE_ARTICLE;
		else if ( !strcasecmp( info->data[i].data, "conference publication" ) )
			type = TYPE_INPROCEEDINGS;
		else if ( !strcasecmp( info->data[i].data, "hearing" ) )
			type = TYPE_HEARING;
		else if ( !strcasecmp( info->data[i].data, "theses" ) )
			type = TYPE_THESIS;
		else if ( !strcasecmp( info->data[i].data, "legislation" ) )
			type = TYPE_BILL;
		else if ( !strcasecmp( info->data[i].data, "newspaper" ) )
			type = TYPE_NEWSPAPER;
		else if ( !strcasecmp( info->data[i].data, "communication" ) )
			type = TYPE_COMMUNICATION;
		else if ( !strcasecmp( info->data[i].data, "manuscript" ) )
			type = TYPE_MANUSCRIPT;
		else if ( !strcasecmp( info->data[i].data, "report" ) )
			type = TYPE_REPORT;
		else if ( !strcasecmp( info->data[i].data, "legal case and case notes" ) )
			type = TYPE_CASE;
		else if ( !strcasecmp( info->data[i].data, "book" ) ) {
			if ( info->level[i]==0 ) type = TYPE_BOOK;
			else type = TYPE_INBOOK;
		}
		else if ( !strcasecmp( info->data[i].data, "collection" ) ) {
			if ( info->level[i]==0 ) type = TYPE_BOOK;
			else type = TYPE_INBOOK;
		}
	}
	if ( type==TYPE_UNKNOWN ) {
		for ( i=0; i<info->nfields; ++i ) {
			if ( strcasecmp( info->tag[i].data, "RESOURCE" ) ) continue;
			if ( !strcasecmp( info->data[i].data, "moving image" ) )
				type = TYPE_BROADCAST;
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
		case TYPE_INBOOK: fprintf( fp, "Book Section" ); break;
		case TYPE_BOOK: fprintf( fp, "Book" ); break;
		case TYPE_HEARING: fprintf( fp, "Hearing" ); break;
		case TYPE_BILL: fprintf( fp, "Bill" ); break;
		case TYPE_CASE: fprintf( fp, "Case" ); break;
		case TYPE_BROADCAST: fprintf( fp, "Film or Broadcast" ); break;
		case TYPE_NEWSPAPER: fprintf( fp, "Newspaper Article" ); break;
		case TYPE_MANUSCRIPT: fprintf( fp, "Manuscript" ); break;
		case TYPE_REPORT: fprintf( fp, "Report" ); break;
		case TYPE_THESIS: fprintf( fp, "Thesis" ); break;
		case TYPE_COMMUNICATION: fprintf( fp, "Personal Communication" ); break;
		case TYPE_INPROCEEDINGS: fprintf( fp, "Conference Proceedings" ); break;
	}
	fprintf( fp, "\n" );
}

static void
output_title( FILE *fp, fields *info, long refnum, char *endtag, int level )
{
	int n1 = fields_find( info, "TITLE", level );
	int n2 = fields_find( info, "SUBTITLE", level );
	if ( n1!=-1 ) {
		fprintf( fp, "%s %s", endtag, info->data[n1].data );
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
output_abbrtitle( FILE *fp, fields *info, long refnum, char *endtag, int level )
{
	int n1 = fields_find( info, "SHORTTITLE", level );
	int n2 = fields_find( info, "SHORTSUBTITLE", level );
	if ( n1!=-1 ) {
		fprintf( fp, "%s %s", endtag, info->data[n1].data );
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
output_people( FILE *fp, fields *info, long refnum, char *tag,
		char *entag, int level )
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
output_pages( FILE *fp, fields *info, long refnum )
{
	int sn = fields_find( info, "PAGESTART", -1 );
	int en = fields_find( info, "PAGEEND", -1 );
	if ( sn!=-1 || en!=-1 ) {
		fprintf( fp, "%%P ");
		if ( sn!=-1 ) fprintf( fp, "%s", info->data[sn].data );
		if ( sn!=-1 && en!=-1 ) fprintf( fp, "-" );
		if ( en!=-1 ) fprintf( fp, "%s", info->data[en].data );
		fprintf( fp, "\n");
	}
}

static void
output_year( FILE *fp, fields *info, long refnum, int level )
{
	int year = fields_find( info, "YEAR", level );
	if ( year==-1 ) year = fields_find( info, "PARTYEAR", level );
	if ( year!=-1 )
		fprintf( fp, "%%D %s\n", info->data[year].data );
}

static void
output_monthday( FILE *fp, fields *info, long refnum, int level )
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
output_easyall( FILE *fp, fields *info, long refnum, char *tag, char *entag, int level )
{
	int i;
	for ( i=0; i<info->nfields; ++i ) {
		if ( level!=-1 && info->level[i]!=level ) continue;
		if ( !strcmp( info->tag[i].data, tag ) )
			fprintf( fp, "%s %s\n", entag, info->data[i].data );
	}
}

static void
output_easy( FILE *fp, fields *info, long refnum, char *tag, char *entag, int level )
{
	int n = fields_find( info, tag, level );
	if ( n!=-1 )
		fprintf( fp, "%s %s\n", entag, info->data[n].data );
}

void
endout_write( fields *info, FILE *fp, int format_opts, unsigned long refnum )
{
	int type;
	type = get_type( info );
	output_type( fp, type );
	output_title( fp, info, refnum, "%T", 0 );
	output_abbrtitle( fp, info, refnum, "%!", 0 );
	output_people( fp, info, refnum, "AUTHOR", "%A", 0 );
	output_people( fp, info, refnum, "EDITOR", "%E", -1 );
	if ( type==TYPE_CASE )
		output_easy( fp, info, refnum, "CORPAUTHOR", "%I", 0 );
	else if ( type==TYPE_HEARING )
		output_easyall( fp, info, refnum, "CORPAUTHOR", "%S", 0 );
	else if ( type==TYPE_NEWSPAPER )
		output_people( fp, info, refnum, "REPORTER", "%A", 0 );
	else if ( type==TYPE_COMMUNICATION )
		output_people( fp, info, refnum, "RECIPIENT", "%E", -1 );
	else {
		output_easy( fp, info, refnum, "CORPAUTHOR", "%A", 0 );
		output_easy( fp, info, refnum, "CORPEDITOR", "%E", -1 );
	}
	if ( type==TYPE_ARTICLE )
		output_title( fp, info, refnum, "%J", 1 );
	else output_title( fp, info, refnum, "%B", 1 );
/*	output_easy( fp, info, refnum, "YEAR", "%D", -1 );
	output_easy( fp, info, refnum, "PARTYEAR", "%D", -1 );*/
	output_year( fp, info, refnum, -1 );
	output_monthday( fp, info, refnum, -1 );
	output_easy( fp, info, refnum, "VOLUME", "%V", -1 );
	output_easy( fp, info, refnum, "ISSUE", "%N", -1 );
	output_easy( fp, info, refnum, "NUMBER", "%N", -1 );
	/* output article number as pages */
	output_easy( fp, info, refnum, "ARTICLENUMBER", "%P", -1 );
	output_easy( fp, info, refnum, "EDITION", "%7", -1 );
	output_easy( fp, info, refnum, "PUBLISHER", "%I", -1 );
	output_easy( fp, info, refnum, "ADDRESS", "%C", -1 );
	output_easy( fp, info, refnum, "DEGREEGRANTOR", "%C", -1 );
	output_easy( fp, info, refnum, "SERIALNUM", "%@", -1 );
	output_easy( fp, info, refnum, "ISSN", "%@", -1 );
	output_easy( fp, info, refnum, "ISBN", "%@", -1 );
	output_easy( fp, info, refnum, "REFNUM", "%F", -1 );
	output_easyall( fp, info, refnum, "NOTES", "%O", -1 );
	output_easy( fp, info, refnum, "ABSTRACT", "%X", -1 );
	output_easy( fp, info, refnum, "CLASSIFICATION", "%L", -1 );
	output_easyall( fp, info, refnum, "KEYWORD", "%K", -1 );
	output_easyall( fp, info, refnum, "NGENRE", "%9", -1 );
	output_easy( fp, info, refnum, "URL", "%U", -1 ); 
	output_pages( fp, info, refnum );
	fprintf( fp, "\n" );
	fflush( fp );
}



