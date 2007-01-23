/*
 * copactypes.c
 *
 * Copyright (c) Chris Putnam 2004-5
 *
 * Program and source code released under the GPL
 *
 */

#include <stdio.h>
#include <string.h>
#include "is_ws.h"
#include "fields.h"
#include "reftypes.h"

/* if no specific type can be identified */
static lookups generic[] = {
	{ "TI-", "TITLE" ,    TITLE,  LEVEL_MAIN },
	{ "AU-", "AUTHOR",    PERSON, LEVEL_MAIN },
	{ "MV-", "VOLUME",    SIMPLE, LEVEL_MAIN },
	{ "SE-", "TITLE",     TITLE,  LEVEL_HOST },
	{ "ED-", "EDITION",   SIMPLE, LEVEL_MAIN },
	{ "SC-", "SCALE",     SIMPLE, LEVEL_MAIN }, /* for maps */
	{ "PU-", "PUBLISHER", SIMPLE, LEVEL_MAIN },
	{ "PY-", "YEAR",      SIMPLE, LEVEL_MAIN },
	{ "PD-", "DESCRIPTION",SIMPLE, LEVEL_MAIN }, /* physical description */
	{ "DT-", "TYPE",      SIMPLE, LEVEL_MAIN },
	{ "LA-", "LANGUAGE",  SIMPLE, LEVEL_MAIN },
	{ "IS-", "SERIALNUM", SERIALNO, LEVEL_MAIN },
	{ "NT-", "NOTES",     SIMPLE, LEVEL_MAIN },
	{ "KW-", "KEYWORD",   SIMPLE, LEVEL_MAIN },
	{ "UL-", "URL",       SIMPLE, LEVEL_MAIN },
	{ "HL-", "LOCATION",  SIMPLE, LEVEL_MAIN }
};

/* order is important....."Book" matches "Book" and "Book Section", hence
 * "Book Section must come first */

variants copac_all[] = {
	{"Generic", &(generic[0]), sizeof(generic)/sizeof(lookups)},
};


int copac_nall = sizeof( copac_all ) / sizeof( variants );

#ifdef NOCOMPILE
int
get_reftype( char *p, long refnum )
{
	int i;
	while ( is_ws( *p ) ) p++;
	for ( i=0; i<nall; ++i ) {
		if ( !strncasecmp( all[i].type, p, strlen(all[i].type)) )
			return i;
	}
	fprintf( stderr, "Warning: Did not recognize type '%s' of refnum %ld, defaulting to generic.\n",
			p, refnum );
	return 0;
}
#endif

