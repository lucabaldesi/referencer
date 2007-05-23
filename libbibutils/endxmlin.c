/*
 * endxmlin.c
 *
 * Copyright (c) Chris Putnam 2006-7
 *
 * Program and source code released under the GPL
 */
#include <stdio.h>
#include <stdlib.h>
#include "newstr.h"
#include "newstr_conv.h"
#include "fields.h"
#include "name.h"
#include "xml.h"
#include "xml_encoding.h"
#include "reftypes.h"

typedef struct {
	char *attrib;
	char *internal;
} attribs;

static int
xml_readmore( FILE *fp, char *buf, int bufsize, int *bufpos )
{
	if ( !feof( fp ) && fgets( buf, bufsize, fp ) ) return 0;
	return 1;
}

int
endxmlin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, newstr *line,
	newstr *reference, int *fcharset )
{
	newstr tmp;
	char *startptr = NULL, *endptr = NULL;
	int haveref = 0, inref = 0, done = 0, file_charset = CHARSET_UNKNOWN, m;
	newstr_init( &tmp );
	while ( !haveref && !done ) {
		if ( line->data ) {
			if ( !inref ) {
				startptr = xml_findstart( line->data, "RECORD" );
				if ( startptr ) inref = 1;
			} else
				endptr = xml_findend( line->data, "RECORD" );
		}
		if ( !startptr ) newstr_empty( line );
		if ( !startptr || !endptr ) {
			done = xml_readmore( fp, buf, bufsize, bufpos );
			newstr_strcat( line, buf );
		} else {
			/* we can reallocate in the newstr_strcat, so re-find */
			startptr = xml_findstart( line->data, "RECORD" );
			endptr = xml_findend( line->data, "RECORD" );
			newstr_segcpy( reference, startptr, endptr );
			/* clear out information in line */
			newstr_strcpy( &tmp, endptr );
			newstr_strcpy( line, tmp.data );
			haveref = 1;
		}
		if ( line->data ) {
			m = xml_getencoding( line );
			if ( m!=CHARSET_UNKNOWN ) file_charset = m;
		}
	}
	newstr_free( &tmp );
	*fcharset = file_charset;
	return haveref;
}

/*
 * add data to fields
 */


/* <titles>
 *    <title>
 *       <style>ACTUAL TITLE HERE</style>
 *    </title>
 * </titles>
 */
void
endxmlin_title( xml *node, fields *info, char *int_tag, int level )
{

	if ( node->down && xml_tagexact( node->down, "style" ) ) {
		endxmlin_title( node->down, info, int_tag, level );
	} else {
		if ( node->value && node->value->len )
			fields_add( info, int_tag, node->value->data, level );
	}
	if ( node->next )
		endxmlin_title( node->next, info, int_tag, level );
}

void
endxmlin_titles( xml *node, fields *info )
{
	attribs a[] = {
		{ "title", "%T" },
		{ "secondary-title", "%B" },
		{ "tertiary-title", "%S" },
		{ "alt-title", "%!" },
		{ "short-title", "SHORTTITLE" },
	};
	int i, n = sizeof( a ) / sizeof ( a[0] );
	for ( i=0; i<n; ++i ) {
		if ( xml_tagexact( node, a[i].attrib ) && node->down )
			endxmlin_title( node->down, info, a[i].internal, 0 );
	}
	if ( node->next ) endxmlin_titles( node->next, info );
}

void
endxmlin_data( xml *node, char *inttag, fields *info, int level )
{
	if ( node->down && xml_tagexact( node->down, "style" ) ) 
		endxmlin_data( node->down, inttag, info, level );
	else if ( node && node->value && node->value->data ) {
		fields_add( info, inttag, node->value->data, level );
	}
}

/* <contributors>
 *    <secondary-authors>
 *        <author>
 *             <style>ACTUAL AUTHORS HERE</style>
 *        </author>
 *    </secondary-authors>
 * </contributors>
 */
/* <!ATTLIST author
 *      corp-name CDATA #IMPLIED
 *      first-name CDATA #IMPLIED
 *      initials CDATA #IMPLIED
 *      last-name CDATA #IMPLIED
 *      middle-initial CDATA #IMPLIED
 *      role CDATA #IMPLIED
 *      salutation CDATA #IMPLIED
 *      suffix CDATA #IMPLIED
 *      title CDATA #IMPLIED
 * >
 *
 * Don't use endxmlin_data() as we need to add these as names...
 */

void
endxmlin_contributor( xml *node, fields *info, char *int_tag )
{
	if ( node->down && xml_tagexact( node->down, "author" ) ) {
		endxmlin_contributor( node->down, info, int_tag );
	} else if ( node->down && xml_tagexact( node->down, "style" ) ) {
		endxmlin_contributor( node->down, info, int_tag );
	} else {
		if ( node->value && node->value->len )
			fields_add( info, int_tag, node->value->data, 0 );
	}
	if ( node->next )
		endxmlin_contributor( node->next, info, int_tag );
}

static void
endxmlin_contributors( xml *node, fields *info )
{
	attribs contrib[] = {
		{ "authors", "%A" },
		{ "secondary-authors", "%E" },
		{ "tertiary-authors", "%Y" },
		{ "subsidiary-authors", "%?" },
		{ "translated-authors", "%?" },
	};
	int i, n = sizeof( contrib ) / sizeof ( contrib[0] );
	for ( i=0; i<n; ++i ) {
		if ( xml_tagexact( node, contrib[i].attrib ) && node->down )
			endxmlin_contributor( node->down, info, contrib[i].internal );
	}
	if ( node->next )
		endxmlin_contributors( node->next, info );
}

static void
endxmlin_keyword( xml *node, fields *info )
{
	if ( xml_tagexact( node, "keyword" ) )
		endxmlin_data( node, "%K", info, 0 );
	if ( node->next )
		endxmlin_keyword( node->next, info );
}

static void
endxmlin_keywords( xml *node, fields *info )
{
	if ( node->down && xml_tagexact( node->down, "keyword" ) )
		endxmlin_keyword( node->down, info );
}

static void
endxmlin_urls( xml *node, fields *info )
{
	if ( xml_tagexact( node, "url" ) )
		endxmlin_data( node, "%U", info, 0 );
	else {
		if ( node->down ) {
			if ( xml_tagexact( node->down, "related-urls" ) ||
			     xml_tagexact( node->down, "url" ) )
				endxmlin_urls( node->down, info );
		}
	}
	if ( node->next )
		endxmlin_urls( node->next, info );
}

#ifdef NOCOMPILE
/*
 * There are a lot of elements in the end2xml stuff buried in element
 * attributes for which it's not clear where they should get stuck
 * -- for now put into notes
 */
static void
endxmlin_makeattribnotes( xml *node, fields *info, int level, attribs *a, int na )
{
	newstr *attrib, note;
	int i;
	newstr_init( &note );
	for ( i=0; i<na; ++i ) {
		attrib = xml_getattrib( node, a[i].attrib );
		if ( attrib ) {
			newstr_strcpy( &note, a[i].internal );
			newstr_strcat( &note, ": " );
			newstr_strcat( &note, attrib->data );
			fields_add( info, "%O", note.data, level );
			newstr_empty( &note );
		}
	}
	newstr_free( &note );
}
#endif

#ifdef NOCOMPILE
/* <!ELEMENT source-app (#PCDATA)>
 * <!ATTLIST source-app
 *        name CDATA #IMPLIED
 *        version CDATA #IMPLIED
 * >
 */
static void
endxmlin_sourceapp( xml *node, fields *info )
{
	attribs a[] = {
		{ "name", "SOURCE APPLICATION NAME" },
		{ "version", "SOURCE APPLICATION VERSION" }
	};
	int na = sizeof( a ) / sizeof( a[0] );
	endxmlin_makeattribnotes( node, info, 0, a, na );
}

/* <!ELEMENT database (#PCDATA)>
 * <!ATTLIST database
 *        name CDATA #IMPLIED
 *        path CDATA #IMPLIED
 * >
 */
static void
endxmlin_database( xml *node, fields *info )
{
	attribs a[] = {
		{ "name", "DATABASE NAME" },
		{ "path", "DATABASE PATH" }
	};
	int na = sizeof( a ) / sizeof( a[0] );
	endxmlin_makeattribnotes( node, info, 0, a, na );
}
#endif

/*
 * <ref-type name="Journal Article">17</ref-type>
 */
static void
endxmlin_reftype( xml *node, fields *info )
{
	newstr *s;
	s = xml_getattrib( node, "name" );
	if ( s && s->dim ) {
		fields_add( info, "%0", s->data, 0 );
		newstr_free( s );
	}
}

static void
endxmlin_record( xml *node, fields *info )
{
	attribs a[] = {
		{ "year", "%D" },
		{ "volume", "%V" },
		{ "num-vol", "%6" },
		{ "pages",  "%P" },
		{ "number", "%N" },
		{ "issue",  "%N" },
		{ "label",  "%F" },
		{ "auth-address", "%C" },
		{ "auth-affiliation", "%C" },
		{ "publisher", "%I" },
		{ "abstract", "%X" },
		{ "edition", "%7" },
		{ "reprint-edition", "%)" },
		{ "section", "%&" },
		{ "accession-num", "%M" },
		{ "call-num", "%L" },
		{ "isbn", "%@" },
		{ "notes", "%O" },
		{ "custom1", "%1" },
		{ "custom2", "%2" },
		{ "custom3", "%3" },
		{ "custom4", "%4" },
		{ "custom5", "%#" },
		{ "custom6", "%$" },
	};
	int i, n = sizeof ( a ) / sizeof( a[0] );
	if ( xml_tagexact( node, "DATABASE" ) ) {
/*		endxmlin_database( node, info );*/
	} else if ( xml_tagexact( node, "SOURCE-APP" ) ) {
/*		endxmlin_sourceapp( node, info );*/
	} else if ( xml_tagexact( node, "REC-NUMBER" ) ) {
	} else if ( xml_tagexact( node, "ref-type" ) ) {
		endxmlin_reftype( node, info );
	} else if ( xml_tagexact( node, "contributors" ) ) {
		if ( node->down ) endxmlin_contributors( node->down, info );
	} else if ( xml_tagexact( node, "titles" ) ) {
		if ( node->down ) endxmlin_titles( node->down, info );
	} else if ( xml_tagexact( node, "keywords" ) ) {
		endxmlin_keywords( node, info );
	} else if ( xml_tagexact( node, "urls" ) ) {
		endxmlin_urls( node, info );
	} else if ( xml_tagexact( node, "periodical" ) ) {
	} else if ( xml_tagexact( node, "secondary-volume" ) ) {
	} else if ( xml_tagexact( node, "secondary-issue" ) ) {
	} else if ( xml_tagexact( node, "reprint-status" ) ) {
	} else if ( xml_tagexact( node, "pub-location" ) ) {
	} else if ( xml_tagexact( node, "orig-pub" ) ) {
	} else if ( xml_tagexact( node, "report-id" ) ) {
	} else if ( xml_tagexact( node, "coden" ) ) {
	} else if ( xml_tagexact( node, "electronic-resource-num" ) ) {
	} else if ( xml_tagexact( node, "caption" ) ) {
	} else if ( xml_tagexact( node, "research-notes" ) ) {
	} else if ( xml_tagexact( node, "work-type" ) ) {
	} else if ( xml_tagexact( node, "reviewed-item" ) ) {
	} else if ( xml_tagexact( node, "availability" ) ) {
	} else if ( xml_tagexact( node, "remote-source" ) ) {
	} else if ( xml_tagexact( node, "meeting-place" ) ) {
	} else if ( xml_tagexact( node, "work-location" ) ) {
	} else if ( xml_tagexact( node, "work-extent" ) ) {
	} else if ( xml_tagexact( node, "pack-method" ) ) {
	} else if ( xml_tagexact( node, "size" ) ) {
	} else if ( xml_tagexact( node, "repro-ratio" ) ) {
	} else if ( xml_tagexact( node, "remote-database-name" ) ) {
	} else if ( xml_tagexact( node, "remote-database-provider" ) ) {
	} else if ( xml_tagexact( node, "language" ) ) {
	} else if ( xml_tagexact( node, "access-date" ) ) {
	} else if ( xml_tagexact( node, "modified-data" ) ) {
	} else if ( xml_tagexact( node, "misc1" ) ) {
	} else if ( xml_tagexact( node, "misc2" ) ) {
	} else if ( xml_tagexact( node, "misc3" ) ) {
	} else {
		for ( i=0; i<n; ++i ) {
			if ( xml_tagexact( node, a[i].attrib ) ) 
				endxmlin_data( node, a[i].internal, info, 0 );
		}
	}
	if ( node->next ) endxmlin_record( node->next, info );
}

static void
endxmlin_assembleref( xml *node, fields *info )
{
	if ( node->tag->len==0 ) {
		if ( node->down ) endxmlin_assembleref( node->down, info );
		return;
	} else if ( xml_tagexact( node, "RECORD" ) ) {
		if ( node->down ) endxmlin_record( node->down, info );
	}
}

/* endxmlin_processf first operates by converting to endnote input
 * the endnote->mods conversion happens in convertf.
 *
 * this is necessary as the xml format is as nasty and as overloaded
 * as the tags used in the Refer format output
 */
int
endxmlin_processf( fields *fin, char *data, char *filename, long nref )
{
	xml top;
	xml_init( &top );
	xml_tree( data, &top );
	endxmlin_assembleref( &top, fin );
	xml_free( &top );
	return 1;
}
