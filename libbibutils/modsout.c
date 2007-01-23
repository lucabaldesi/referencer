/*
 * modsout.c
 *
 * Copyright (c) Chris Putnam 2003-5
 *
 * Source code released under the GPL
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "is_ws.h"
#include "newstr.h"
#include "newstr_conv.h"
#include "fields.h"
#include "utf8.h"
#include "modsout.h"

typedef struct convert {
	char oldstr[25];
	char newstr[25];
	int  code;
} convert;

extern char progname[];

static void
output_tab0( FILE *outptr, int level )
{
	int i;
	for ( i=0; i<=level; ++i ) fprintf( outptr, "    " );
}

static void
output_tab1( FILE *outptr, int level, char *tag )
{
	output_tab0( outptr, level );
	fprintf( outptr, "%s", tag );
}

static void
output_tab2( FILE *outptr, int level, char *tag, char *data, int cr )
{
	output_tab0( outptr, level );
	fprintf( outptr, "<%s>%s</%s>", tag, data, tag );
	if ( cr ) fprintf( outptr, "\n" );
}

static void
output_tab4( FILE *outptr, int level, char *tag, char *aname, char *avalue,
		char *data, int cr )
{
	output_tab0( outptr, level );
	fprintf( outptr, "<%s %s=\"%s\">%s</%s>", tag,aname,avalue,data,tag);
	if ( cr ) fprintf( outptr, "\n" );
}

static void
output_fill2( FILE *outptr, int level, char *tag, fields *info, int n, int cr )
{
	if ( n!=-1 ) {
		output_tab2( outptr, level, tag, info->data[n].data, cr );
		info->used[n] = 1;
	}
}

static void
output_fill4( FILE *outptr, int level, char *tag, char *aname, char *avalue,
		fields *info, int n, int cr )
{
	if ( n!=-1 ) {
		output_tab4( outptr, level, tag, aname, avalue,
				info->data[n].data, cr );
		info->used[n] = 1;
	}
}

static void
output_title( fields *info, FILE *outptr, int level )
{
	int ttl = fields_find( info, "TITLE", level );
	int subttl = fields_find( info, "SUBTITLE", level );
	int shrttl = fields_find( info, "SHORTTITLE", level );

	output_tab1( outptr, level, "<titleInfo>\n" );
	output_fill2( outptr, level+1, "title", info, ttl, 1);
	output_fill2( outptr, level+1, "subTitle", info, subttl, 1 );
	if ( ttl==-1 && subttl==-1 ) 
		output_tab1( outptr, level+1, "<title/>\n" );
	output_tab1( outptr, level, "</titleInfo>\n" );

	/* output shorttitle if it's different from normal title */
	if ( shrttl!=-1 ) {
		if ( ttl==-1 || subttl!=-1 ||
			strcmp(info->data[ttl].data,info->data[shrttl].data) ) {
			output_tab1( outptr, level, 
					"<titleInfo type=\"abbreviated\">\n" );
			output_fill2( outptr, level+1, "title", info, shrttl,1);
			output_tab1( outptr, level, "</titleInfo>\n" );
		}
		info->used[shrttl] = 1;
	}
}

static void
output_personalstart( FILE *outptr, int level )
{
	int j;
	for ( j=0; j<=level; ++j ) fprintf( outptr, "    " );
		fprintf( outptr, "<name type=\"personal\">\n" );
}

static void
output_name( FILE *outptr, char *p, int level )
{
	newstr family, part;
	int n=0;

	newstr_init( &family );
	newstr_init( &part );

	while ( *p && *p!='|' ) newstr_addchar( &family, *p++ );
	if ( *p=='|' ) p++;

	while ( *p ) {
		while ( *p && *p!='|' ) newstr_addchar( &part, *p++ );
		/* truncate periods from "A. B. Jones" names */
		if ( part.len ) {
			if ( part.len==2 && part.data[1]=='.' ) {
				part.len=1;
				part.data[1]='\0';
			}
			if ( n==0 ) output_personalstart( outptr, level );
			output_tab4( outptr, level+1, "namePart", "type", 
					"given", part.data, 1 );
			n++;
		}
		if ( *p=='|' ) {
			p++;
			newstr_empty( &part );
		}
	}

	if ( family.len ) {
		if ( n==0 ) output_personalstart( outptr, level );
		output_tab4( outptr, level+1, "namePart", "type", "family",
				family.data, 1 );
	}

	newstr_free( &part );
	newstr_free( &family );
}


#define NO_AUTHORITY (0)
#define MARC_AUTHORITY (1)
#define NAME_ASIS (2)
#define NAME_CORP (4)

static void
output_names( fields *info, FILE *outptr, int level )
{
	convert   names[] = {
		{ "AUTHOR",       "author",       MARC_AUTHORITY },
		{ "AUTHOR:ASIS",  "author",       MARC_AUTHORITY | NAME_ASIS },
		{ "AUTHOR:CORP",  "author",       MARC_AUTHORITY | NAME_CORP },
		{ "WRITER",       "writer",       MARC_AUTHORITY },
		{ "WRITER:ASIS",  "writer",       MARC_AUTHORITY | NAME_ASIS },
		{ "WRITER:CORP",  "writer",       MARC_AUTHORITY | NAME_CORP },
		{ "ASSIGNEE",     "patent holder",MARC_AUTHORITY },
		{ "ASSIGNEE:ASIS","patent holder",MARC_AUTHORITY | NAME_ASIS },
		{ "ASSIGNEE:CORP","patent holder",MARC_AUTHORITY | NAME_CORP },
		{ "EDITOR",       "editor",       MARC_AUTHORITY },
		{ "EDITOR:ASIS",  "editor",       MARC_AUTHORITY | NAME_ASIS },
		{ "EDITOR:CORP",  "editor",       MARC_AUTHORITY | NAME_CORP },
		{ "ARTIST",       "artist",       MARC_AUTHORITY },
		{ "ARTIST:ASIS",  "artist",       MARC_AUTHORITY | NAME_ASIS },
		{ "ARTIST:CORP",  "artist",       MARC_AUTHORITY | NAME_CORP },
		{ "CARTOGRAPHER", "cartographer", MARC_AUTHORITY },
		{ "CARTOGRAPHER:ASIS", "cartographer",MARC_AUTHORITY|NAME_ASIS},
		{ "CARTOGRAPHER:CORP", "cartographer",MARC_AUTHORITY|NAME_CORP},
		{ "DEGREEGRANTOR","degree grantor", MARC_AUTHORITY },
		{ "DEGREEGRANTOR:ASIS","degree grantor", MARC_AUTHORITY | NAME_ASIS },
		{ "DEGREEGRANTOR:CORP","degree grantor", MARC_AUTHORITY | NAME_CORP },
		{ "INVENTOR",     "inventor",     MARC_AUTHORITY },
		{ "INVENTOR:ASIS","inventor",     MARC_AUTHORITY |NAME_ASIS},
		{ "INVENTOR:CORP","inventor",     MARC_AUTHORITY |NAME_CORP},
		{ "ORGANIZER",    "organizer of meeting", MARC_AUTHORITY },
		{ "ORGANIZER:ASIS","organizer of meeting", MARC_AUTHORITY|NAME_ASIS },
		{ "ORGANIZER:CORP","organizer of meeting", MARC_AUTHORITY|NAME_CORP },
		{ "DIRECTOR",     "director",     MARC_AUTHORITY },
		{ "DIRECTOR:ASIS","director",     MARC_AUTHORITY | NAME_ASIS },
		{ "DIRECTOR:CORP","director",     MARC_AUTHORITY | NAME_CORP },
		{ "PERFORMER",    "performer",    MARC_AUTHORITY },
		{ "PERFORMER:ASIS","performer",   MARC_AUTHORITY | NAME_ASIS },
		{ "PERFORMER:CORP","performer",   MARC_AUTHORITY | NAME_CORP },
		{ "REPORTER",     "reporter",     NO_AUTHORITY   },
		{ "REPORTER:ASIS","reporter",     NO_AUTHORITY   | NAME_ASIS },
		{ "REPORTER:CORP","reporter",     NO_AUTHORITY   | NAME_CORP },
		{ "TRANSLATOR",   "translator",   MARC_AUTHORITY },
		{ "DIRECTOR",     "director",     MARC_AUTHORITY },
		{ "DIRECTOR:ASIS","director",     MARC_AUTHORITY | NAME_ASIS },
		{ "DIRECTOR:CORP","director",     MARC_AUTHORITY | NAME_CORP },
		{ "PERFORMER",    "performer",    MARC_AUTHORITY },
		{ "PERFORMER:ASIS","performer",   MARC_AUTHORITY | NAME_ASIS },
		{ "PERFORMER:CORP","performer",   MARC_AUTHORITY | NAME_CORP },
		{ "TRANSLATOR",   "translator",   MARC_AUTHORITY },
		{ "TRANSLATOR:ASIS", "translator",MARC_AUTHORITY | NAME_ASIS },
		{ "TRANSLATOR:CORP", "translator",MARC_AUTHORITY | NAME_CORP },
		{ "RECIPIENT",    "recipient",    MARC_AUTHORITY },
		{ "RECIPIENT:ASIS","recipient",   MARC_AUTHORITY | NAME_ASIS },
		{ "RECIPIENT:CORP","recipient",   MARC_AUTHORITY | NAME_CORP },
		{ "2ND_AUTHOR",   "author",       MARC_AUTHORITY },
		{ "2ND_AUTHOR:ASIS","author",     MARC_AUTHORITY | NAME_ASIS },
		{ "2ND_AUTHOR:CORP","author",     MARC_AUTHORITY | NAME_CORP },
		{ "3RD_AUTHOR",   "author",       MARC_AUTHORITY },
		{ "3RD_AUTHOR:ASIS","author",     MARC_AUTHORITY | NAME_ASIS },
		{ "3RD_AUTHOR:CORP","author",     MARC_AUTHORITY | NAME_CORP },
		{ "SUB_AUTHOR",   "author",       MARC_AUTHORITY },
		{ "SUB_AUTHOR:ASIS","author",     MARC_AUTHORITY | NAME_ASIS },
		{ "COMMITTEE",         "author",  MARC_AUTHORITY | NAME_CORP },
		{ "COURT",             "author",  MARC_AUTHORITY | NAME_CORP },
		{ "LEGISLATIVEBODY",   "author",  MARC_AUTHORITY | NAME_CORP }
	};

	int       i, n, ntypes = sizeof( names ) / sizeof( convert );

	for ( n=0; n<ntypes; ++n ) {
		for ( i=0; i<info->nfields; ++i ) {
			if ( info->level[i]!=level ) continue;
			if ( strcasecmp(info->tag[i].data,names[n].oldstr) )
				continue;
			if ( names[n].code & NAME_ASIS ) {
				output_tab0( outptr, level );
				fprintf( outptr, "<name>\n" );
				output_fill2( outptr, level+1, "namePart", info, i, 1 );
			} else if ( names[n].code & NAME_CORP ) {
				output_tab0( outptr, level );
				fprintf( outptr, "<name type=\"corporate\">\n" );
				output_fill2( outptr, level+1, "namePart", info, i, 1 );
			} else {
				output_name(outptr, info->data[i].data, level);
			}
			output_tab1( outptr, level+1, "<role>\n" );
			output_tab1( outptr, level+2, "<roleTerm" );
			if ( names[n].code & MARC_AUTHORITY )
				fprintf( outptr, " authority=\"marcrelator\"");
			fprintf( outptr, " type=\"text\">");
			fprintf( outptr, "%s", names[n].newstr );
			fprintf( outptr, "</roleTerm>\n");
			output_tab1( outptr, level+1, "</role>\n" );
			output_tab1( outptr, level, "</name>\n" );
			info->used[i] = 1;
		}
	}
}

static int
output_finddateissued( fields *info, int level, int pos[3] )
{
	char      *src_names[] = { "YEAR", "MONTH", "DAY" };
	char      *alt_names[] = { "PARTYEAR", "PARTMONTH", "PARTDAY" };
	int       i, found = -1, ntypes = 3;

	for ( i=0; i<ntypes; ++i ) {
		pos[i] = fields_find( info, src_names[i], level );
		if ( pos[i]!=-1 ) found = pos[i];
	}
	/* for LEVEL_MAIN, do what it takes to find a date */
	if ( found==-1 && level==0 ) {
		for ( i=0; i<ntypes; ++i ) {
			pos[i] = fields_find( info, src_names[i], -1 );
			if ( pos[i]!=-1 ) found = pos[i];
		}
	}
	if ( found==-1 && level==0 ) {
		for ( i=0; i<ntypes; ++i ) {
			pos[i] = fields_find( info, alt_names[i], -1 );
			if ( pos[i]!=-1 ) found = pos[i];
		}
	}
	return found;
}

static void
output_dateissued( fields *info, FILE *outptr, int level, int pos[3] )
{
	int nprinted = 0, i;
	output_tab1( outptr, level+1, "<dateIssued>" );
	for ( i=0; i<3 && pos[i]!=-1; ++i ) {
		if ( nprinted>0 ) fprintf( outptr, "-" );
		if ( i>0 && info->data[pos[i]].len==1 )
			fprintf( outptr, "0" ); /*zero pad Jan,Feb,etc*/
		fprintf( outptr,"%s",info->data[pos[i]].data );
		nprinted++;
		info->used[ pos[i] ] = 1;
	}
	fprintf( outptr, "</dateIssued>\n" );
}

static void
output_origin( fields *info, FILE *outptr, int level )
{
	convert origin[] = {
		{ "ISSUANCE",	"issuance",	0 },
		{ "PUBLISHER",	"publisher",	0 },
		{ "ADDRESS",	"place",	1 },
		{ "EDITION",	"edition",	0 }
	};
	int	  n, ntypes = sizeof( origin ) / sizeof ( convert );
	int       found, datefound, pos[5], date[3];

	/* find all information to be outputted */
	found = -1;
	for ( n=0; n<ntypes; ++n ) {
		pos[n] = fields_find( info, origin[n].oldstr, level );
		if ( pos[n]!=-1 ) found = pos[n];
	}
	datefound = output_finddateissued( info, level, date );
	if ( found==-1 && datefound==-1 ) return;

	output_tab1( outptr, level, "<originInfo>\n" );
	output_fill2( outptr, level+1, "issuance", info, pos[0], 1 );
	if ( datefound!=-1 ) output_dateissued( info, outptr, level, date );

	for ( n=1; n<ntypes; n++ ) {
		if ( pos[n]==-1 ) continue;
		output_tab0( outptr, level+1 );
		fprintf( outptr, "<%s", origin[n].newstr );
		fprintf( outptr, ">" );
		if ( origin[n].code ) {
			fprintf( outptr, "\n" );
			output_fill4( outptr, level+2, "placeTerm", "type",
					"text", info, pos[n], 1 );
			output_tab0( outptr, level+1 );
		} else {
			fprintf( outptr, "%s", info->data[pos[n]].data );
			info->used[ pos[n] ] = 1;
		}
		fprintf( outptr, "</%s>\n", origin[n].newstr );
	}
	output_tab1( outptr, level, "</originInfo>\n" );
}

static void
output_language( fields *info, FILE *outptr, int level )
{
	int n = fields_find( info, "LANGUAGE", level );
	output_fill2( outptr, level, "language", info, n, 1 );
}

static void
output_description( fields *info, FILE *outptr, int level )
{
	int n = fields_find( info, "DESCRIPTION", level );
	output_fill2( outptr, level, "physicalDescription", info, n, 1 );
}

static void
output_toc( fields *info, FILE *outptr, int level )
{
	int n = fields_find( info, "CONTENTS", level );
	output_fill2( outptr, level, "tableOfContents", info, n, 1 );
}

/* part date output
 *
 * <date>xxxx-xx-xx</date>
 *
 */
static void
mods_output_partdate( fields *info, FILE *outptr, int year, int month, int day,
		int level )
{
	if ( year==-1 && month==-1 && day==-1 ) return;

	output_tab1( outptr, level+1, "<date>" );

	if ( year!=-1 ) {
		fprintf( outptr, "%s", info->data[year].data);
		info->used[year]=1;
	}

	if ( month!=-1 ) {
		if ( year!=-1 ) fprintf( outptr, "-" );
		else fprintf( outptr, "XXXX-" );
		fprintf( outptr, "%s", info->data[month].data );
		info->used[month]=1;
	}

	if ( day!=-1 ) {
		if ( month!=-1 ) fprintf( outptr, "-" );
		else if ( year!=-1 ) fprintf( outptr, "-XX-" );
		fprintf( outptr, "%s", info->data[day].data );
		info->used[day]=1;
	}

	fprintf( outptr,"</date>\n");
}

/* detail output
 *
 * for example:
 *
 * <detail type="volume"><number>xxx</number></detail
 */
static void
mods_output_detail( fields *info, FILE *outptr, int item, char *item_name,
		int level )
{
	if ( item==-1 ) return;
	output_tab0( outptr, level+1 );
	fprintf( outptr, "<detail type=\"%s\"><number>", item_name );
	fprintf( outptr, "%s</number></detail>\n", info->data[item].data );
	info->used[item] = 1; 
}


/* extents output
 *
 * <extent unit="page">
 * 	<start>xxx</start>
 * 	<end>xxx</end>
 * </extent>
 */
static void
mods_output_extents( fields *info, FILE *outptr, int start, int end,
		int total, char *extype, int level )
{
	output_tab0( outptr, level+1 );
	fprintf( outptr, "<extent unit=\"%s\">\n", extype);
	output_fill2( outptr, level+2, "start", info, start, 1 );
	output_fill2( outptr, level+2, "end", info, end, 1 );
	output_fill2( outptr, level+2, "total", info, total, 1 );
	output_tab1 ( outptr, level+1, "</extent>\n" );
}

static void
mods_output_part( fields *info, FILE *outptr, int level )
{
	typedef struct {
		char *tag;
		int  n;
		char *ntag;
	} struct_mods_part;

	struct_mods_part parts[12] = {
		{ "PARTYEAR",        -1, "" },
		{ "PARTMONTH",       -1, "" },
		{ "PARTDAY",         -1, "" },
		{ "PAGESTART",       -1, "" },
		{ "PAGEEND",         -1, "" },
		{ "VOLUME",          -1, "volume" },
		{ "SECTION",         -1, "section" },
		{ "ISSUE",           -1, "issue" },
		{ "NUMBER",          -1, "number" },
		{ "PUBLICLAWNUMBER", -1, "publiclawnumber" },
		{ "SESSION",         -1, "session" },
		{ "TOTALPAGES",      -1, "" }
	};

	int j, found;

	found = 0;
	for ( j=0; j<12; ++j ) {
		parts[j].n = fields_find( info, parts[j].tag, level );
		found += ( parts[j].n!=-1 );
	}
	if ( !found ) return;

	output_tab1( outptr, level, "<part>\n" );

	mods_output_partdate( info, outptr, parts[0].n, parts[1].n, parts[2].n, 
			level );

	for ( j=5; j<11; ++j )
		mods_output_detail( info, outptr, parts[j].n, parts[j].ntag,
				level );

	if ( parts[3].n==-1 || parts[4].n==-1 ) {
		if ( parts[3].n!=-1 ) j=3;
		else j=4;
		mods_output_detail( info, outptr, parts[j].n, "page", level );
	}
	if ( ( parts[3].n!=-1 && parts[4].n!=-1 ) || ( parts[11].n!=-1 ) ) {
		if ( parts[3].n==-1 || parts[4].n==-1 ) {
			parts[3].n = parts[4].n = -1;
		}
		mods_output_extents( info, outptr, 
			parts[3].n, parts[4].n, parts[11].n, "page", level ); 
	}

	output_tab1( outptr, level, "</part>\n" );
}

static void
output_genre( fields *info, FILE *outptr, int level )
{
	char *marc[] = { "abstract or summary", "art original", 
		"art reproduction", "atlas", "autobiography", 
		"bibliography", "book", "catalog", "chart", "comic strip",
		"conference publication", "database", "dictionary",
		"diorama", "directory", "discography", "drama",
		"encyclopedia", "essay", "festschrift", "fiction",
       		"filmography", "filmstrip", "flash card", "folktale",
		"font", "game", "government publication", "graphic",
		"globe", "handbook", "history", "humor, satire",
		"index", "instruction", "interview", "kit",
		"language instruction", "law report or digest",
		"legal article", "legal case and case notes", "legislation",
		"letter", "loose-leaf", "map", "memoir", "microscope slide",
		"model", "motion picture", "multivolume monograph",
		"newspaper", "novel", "numeric data", 
		"online system or service", "patent", "periodical",
		"picture", "poetry", "programmed text", "realia",
		"rehersal", "remote sensing image", "reporting",
		"review", "series", "short story", "slide", "sound",
		"speech", "statistics", "survey of literature",
		"technical drawing", "technical report", "theses",
		"toy", "transparency", "treaty", "videorecording",
		"web site" 
	};
	int nmarc = sizeof( marc ) / sizeof( char* );
	int i, j, ismarc;
	for ( i=0; i<info->nfields; ++i ) {
		if ( info->level[i]!=level ) continue;
		if ( strcasecmp( info->tag[i].data, "GENRE" ) ) continue;
		ismarc = 0;
		for ( j=0; j<nmarc && ismarc==0; ++j ) {
			if ( !strcasecmp( info->data[i].data, marc[j] ) )
				ismarc = 1;
		}
		output_tab1( outptr, level, "<genre" );
		if ( ismarc ) 
			fprintf( outptr, " authority=\"marc\"" );
		fprintf( outptr, ">%s</genre>\n", info->data[i].data );
		info->used[i] = 1;
	}
}

static void
output_typeresource( fields *info, FILE *outptr, int level )
{
	char *marc[] = { "text", "cartographic", "notated music",
		"sound recording", "still image", "moving image",
		"three dimensional object", "software, multimedia",
		"mixed material"
	};
	int nmarc = sizeof( marc ) / sizeof( char* );
	int resource, j, ismarc;
	resource = fields_find( info, "RESOURCE", level );
	if ( resource!=-1 ) {
		ismarc = 0;
		for ( j=0; j<nmarc && ismarc==0; ++j ) 
			if ( !strcasecmp( info->data[resource].data, marc[j] ))
				ismarc = 1;
		if ( ismarc==0 ) {
			fprintf( stderr, "Illegal typeofResource = '%s'\n",
					info->data[resource].data );
		} else {
			for ( j=0; j<=level; ++j ) fprintf( outptr, "    " );
			fprintf( outptr, "<typeOfResource>%s</typeOfResource>\n",
			      info->data[resource].data );
		}
		info->used[resource] = 1;
	}
}

static void
output_type( fields *info, FILE *outptr, int level )
{
	int n = fields_find( info, "TYPE", 0 );
	if ( n!=-1 ) info->used[n] = 1;
	output_typeresource( info, outptr, level );
	output_genre( info, outptr, level );
}

static void
output_abs( fields *info, FILE *outptr, int level )
{
	int i;
	int nabs = fields_find( info, "ABSTRACT", level );
	output_fill2( outptr, level, "abstract", info, nabs, 1 );
	for ( i=0; i<info->nfields; ++i ) {
		if ( info->level[i]!=level ) continue;
		if ( strcasecmp( info->tag[i].data, "NOTES" ) ) continue;
		output_fill2( outptr, level, "note", info, i, 1 );
	}
}

static void
output_timescited( fields *info, FILE *outptr, int level )
{
	int n = fields_find( info, "TIMESCITED", level );
	if ( n!=-1 ) {
		output_tab0( outptr, level );
		fprintf( outptr, "<note>Times Cited: %s</note>\n",
				info->data[n].data );
		info->used[n] = 1;
	}
}

static void
output_indexkey( fields *info, FILE *outptr, int level )
{
	int n = fields_find( info, "BIBKEY", level );
	if ( n!=-1 ) {
		output_tab0( outptr, level );
		fprintf( outptr, "<note>Key: %s</note>\n",
				info->data[n].data );
		info->used[n] = 1;
	}
}

static void
output_key( fields *info, FILE *outptr, int level )
{
	int i;
	for ( i=0; i<info->nfields; ++i ) {
		if ( info->level[i]!=level ) continue;
		if ( !strcasecmp( info->tag[i].data, "KEYWORD" ) ) {
			output_tab1( outptr, level, "<subject>\n" );
			output_fill2( outptr, level+1, "topic", info, i, 1 );
			output_tab1( outptr, level, "</subject>\n" );
		}
	}
}

static void
output_sn( fields *info, FILE *outptr, int level )
{
	char      *internal_names[] = { "ISBN", "LCCN", "ISSN", "REFNUM", 
		"DOI" , "PUBMED", "MEDLINE", "PII" };
	char      *mods_types[] = { "isbn", "lccn", "issn", "citekey", "doi",
		"pubmed", "medline", "pii" };
	int       n, ntypes = sizeof( internal_names ) / sizeof( char* );
	int       found, i;

	found = fields_find ( info, "CALLNUMBER", level );
	output_fill2( outptr, level, "classification", info, found, 1 );

	for ( n=0; n<ntypes; ++n ) {
		found = fields_find( info, internal_names[n], level );
		if ( found==-1 ) continue;
		output_tab0( outptr, level );
		fprintf( outptr, "<identifier type=\"%s\">%s</identifier>\n",
				mods_types[n],
				info->data[found].data
		       );
		info->used[found] = 1;
	}
	for ( i=0; i<info->nfields;++i ) {
		if ( info->level[i]!=level ) continue;
		if ( !strcasecmp( info->tag[i].data, "SERIALNUMBER" ) ) {
			output_tab0( outptr, level );
			fprintf( outptr, "<identifier type=\"%s\">%s</identifier\n",
				"serial number", info->data[i].data );
			info->used[i] = 1;
		}
	}
}

static void
output_url( fields *info, FILE *outptr, int level )
{
	int location = fields_find( info, "LOCATION", level );
	int url = fields_find( info, "URL", level );
	int i;
	if ( url==-1 && location==-1 ) return;
	output_tab1( outptr, level, "<location>\n" );
	for ( i=0; i<info->nfields; ++i ) {
		if ( info->level[i]!=level ) continue;
		if ( !strcasecmp( info->tag[i].data, "URL" ) ) {
			output_fill2( outptr, level+1, "url", info, i, 1 );
		}
	}
/*	if ( url!=-1 )
		output_fill2( outptr, level+1, "url", info, url, 1 );*/
	if ( location!=-1 )
		output_fill2( outptr, level+1, "physicalLocation", info, location, 1 );
	output_tab1( outptr, level, "</location>\n" );
}

/* refnum should start with a non-number and not include spaces */
static void
output_refnum( fields *info, int n, FILE *outptr )
{
	char *p = info->data[n].data;
	if ( p && ((*p>='0' && *p<='9') || *p=='-' || *p=='_' ))
		fprintf( outptr, "ref" );
	while ( p && *p ) {
		if ( !is_ws(*p) ) fprintf( outptr, "%c", *p );
/*		if ( (*p>='A' && *p<='Z') ||
		     (*p>='a' && *p<='z') ||
		     (*p>='0' && *p<='9') ||
		     (*p=='-') || (*p=='
		     (*p=='_') ) fprintf( outptr, "%c", *p );*/
		p++;
	}
}

static void
output_head( fields *info, FILE *outptr, int dropkey, unsigned long numrefs )
{
	int n;
	fprintf( outptr, "<mods");
	if ( !dropkey ) {
		n = fields_find( info, "REFNUM", 0 );
		if ( n!=-1 ) {
			fprintf( outptr, " ID=\"");
			output_refnum( info, n, outptr );
			fprintf( outptr, "\"");
		}
	}
	fprintf( outptr, ">\n" );
}

static void
output_citeparts( fields *info, FILE *outptr, int level, int max )
{
	int i;
	output_title( info, outptr, level );
	output_names( info, outptr, level);
	output_origin( info, outptr, level );
	output_type( info, outptr, level );
	output_language( info, outptr, level );
	output_description( info, outptr, level );
	if ( level < max ) {
		for ( i=0; i<=level; ++i ) fprintf( outptr, "    " );
		fprintf( outptr, "<relatedItem type=\"host\">\n");
		output_citeparts(info, outptr, level+1, max);
		for ( i=0; i<=level; ++i ) fprintf( outptr, "    " );
		fprintf( outptr, "</relatedItem>\n");
	}
	output_abs( info, outptr, level );
	output_timescited( info, outptr, level );
	output_indexkey( info, outptr, level );
	output_toc( info, outptr, level );
	output_key( info, outptr, level );
	output_sn( info, outptr, level );
	output_url( info, outptr, level );
	/* as of MODS 3.1, <part> tags can be in the main items */
	/*if ( level>0 ) */
	mods_output_part( info, outptr, level );
}

void
modsout_write( fields *info, FILE *outptr, /*int charset, int latexin,*/
	/*int unicode,*/ int format_opts, /*int dropkey,*/ unsigned long numrefs )
{
	int i, max = fields_maxlevel( info );
	int dropkey = ( format_opts & MODSOUT_DROPKEY );

	output_head( info, outptr, dropkey, numrefs );
	output_citeparts( info, outptr, 0, max );

	for ( i=0; i<info->nfields; ++i ) {
		if ( !info->used[i] ) {
			fprintf( stderr, "%s warning: unused tag: '%s' "
				"value: '%s' level: %d\n", progname,
				info->tag[i].data, 
				info->data[i].data, info->level[i] );
		}
	}

	fprintf( outptr, "</mods>\n" );
	fflush( outptr );
}

static void
modsout_writebom( FILE *outptr )
{
	int i, nc;
	unsigned char code[6];
	nc = utf8_encode( 0xFEFF, code );
	for ( i=0; i<nc; ++i )
		fprintf(outptr,"%c",code[i]);
}

void
modsout_writeheader( FILE *outptr, int format )
{
	if ( format & MODSOUT_BOM ) modsout_writebom( outptr );
	fprintf(outptr,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(outptr,"<modsCollection xmlns=\"http://www.loc.gov/mods/v3\">\n");
}

void
modsout_writefooter( FILE *outptr )
{
	fprintf(outptr,"</modsCollection>\n");
	fflush( outptr );
}
