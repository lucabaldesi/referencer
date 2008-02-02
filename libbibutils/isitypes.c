/*
 * isitypes.c
 *
 * Copyright (c) Chris Putnam 2004-7
 *
 * Program and source code released under the GPL
 *
 */

#include <stdio.h>
#include <string.h>
#include "is_ws.h"
#include "fields.h"
#include "reftypes.h"

static lookups article[] = {
	{ "AU",     "AUTHOR",    PERSON, LEVEL_MAIN },
	{ "AF",     "AUTHOR",    PERSON, LEVEL_MAIN },
	{ "TI",     "TITLE",     TITLE,  LEVEL_MAIN },
	{ "SO",     "TITLE",     TITLE,  LEVEL_HOST }, /* full journal name */
	{ "JI",     "SHORTTITLE",TITLE,  LEVEL_HOST }, /* abbr journal name */
	{ "J9",     "SHORTTITLE",TITLE,  LEVEL_HOST }, /* 29char journal name */
	{ "PU",     "PUBLISHER", SIMPLE, LEVEL_HOST },
	{ "PI",     "ADDRESS",   SIMPLE, LEVEL_HOST }, /* publisher city */
	{ "C1",     "ADDRESS",   SIMPLE, LEVEL_MAIN }, /* author address */
	{ "PA",     "ADDRESS",   SIMPLE, LEVEL_HOST }, /* publisher address */
	{ "RP",     "REPRINTADDRESS", SIMPLE, LEVEL_MAIN },
	{ "PY",     "PARTYEAR",  SIMPLE, LEVEL_MAIN },
	{ "PD",     "PARTMONTH", SIMPLE, LEVEL_MAIN },
	{ "VL",     "VOLUME",    SIMPLE, LEVEL_MAIN },
	{ "AR",     "ARTICLENUMBER", SIMPLE, LEVEL_MAIN }, /* AR=article number that Phys. Rev. B uses instead of page numbers */
	{ "BP",     "PAGESTART", SIMPLE, LEVEL_MAIN },
	{ "EP",     "PAGEEND",   SIMPLE, LEVEL_MAIN },
	{ "PG",     "TOTALPAGES", SIMPLE, LEVEL_MAIN },
	{ "IS",     "ISSUE",     SIMPLE, LEVEL_MAIN },
	{ "SN",     "SERIALNUMBER", SERIALNO, LEVEL_HOST },
	{ "AB",     "ABSTRACT",  SIMPLE, LEVEL_MAIN },
	{ "NF",     "NOTES",     SIMPLE, LEVEL_MAIN },
	{ "DE",     "KEYWORD",   ISI_KEYWORD, LEVEL_MAIN }, /* author keywords */
	{ "ID",     "KEYWORD",   ISI_KEYWORD, LEVEL_MAIN }, /* new ISI keywords */
	{ "LA",     "LANGUAGE",  SIMPLE, LEVEL_MAIN },
	{ "TC",     "TIMESCITED", SIMPLE, LEVEL_MAIN },
	{ "NR",     "NUMBERREFS", SIMPLE, LEVEL_MAIN },
	{ "CR",     "CITEDREFS",  SIMPLE, LEVEL_MAIN },
	{ "PT",     " ",         TYPE,   LEVEL_HOST },
	{ "DT",     "DOCUMENTTYPE", TYPE, LEVEL_MAIN },
	{ "GA",     "ISIDELIVERNUM", SIMPLE, LEVEL_MAIN}, /*ISI document delivery number */
	{ "UT",     "ISIREFNUM", SIMPLE, LEVEL_MAIN }, /* ISI unique article identifer */
	{ "DI",     "DOI",       SIMPLE, LEVEL_MAIN },

	{ " ",      "TYPE|ARTICLE",           ALWAYS, LEVEL_MAIN },
	{ " ",      "ISSUANCE|continuing",    ALWAYS, LEVEL_HOST },
	{ " ",      "RESOURCE|text",          ALWAYS, LEVEL_MAIN },
	{ " ",      "GENRE|periodical",       ALWAYS, LEVEL_HOST },
	{ " ",      "GENRE|academic journal", ALWAYS, LEVEL_HOST }
};

static lookups book[] = {
	{ "AU",     "AUTHOR",    PERSON, LEVEL_MAIN },
	{ "AF",     "AUTHOR",    PERSON, LEVEL_MAIN },
	{ "TI",     "TITLE",     TITLE,  LEVEL_MAIN },
	{ "SO",     "TITLE",     TITLE,  LEVEL_HOST }, /* full journal name */
	{ "JI",     "SHORTTITLE",TITLE,  LEVEL_HOST }, /* abbr journal name */
	{ "J9",     "SHORTTITLE",TITLE,  LEVEL_HOST }, /* 29char journal name */
	{ "PU",     "PUBLISHER", SIMPLE, LEVEL_MAIN },
	{ "PI",     "ADDRESS",   SIMPLE, LEVEL_MAIN }, /* publisher city */
	{ "C1",     "ADDRESS",   SIMPLE, LEVEL_MAIN }, /* author address */
	{ "PA",     "ADDRESS",   SIMPLE, LEVEL_MAIN }, /* publisher address */
	{ "RP",     "REPRINTADDRESS", SIMPLE, LEVEL_MAIN },
	{ "PY",     "YEAR",      SIMPLE, LEVEL_MAIN },
	{ "PD",     "MONTH",     SIMPLE, LEVEL_MAIN },
	{ "VL",     "VOLUME",    SIMPLE, LEVEL_MAIN },
	{ "BP",     "PAGESTART", SIMPLE, LEVEL_MAIN },
	{ "EP",     "PAGEEND",   SIMPLE, LEVEL_MAIN },
	{ "PG",     "TOTALPAGES", SIMPLE, LEVEL_MAIN },
	{ "IS",     "ISSUE",     SIMPLE, LEVEL_MAIN },
	{ "SN",     "SERIALNUMBER", SERIALNO, LEVEL_HOST },
	{ "AB",     "ABSTRACT",  SIMPLE, LEVEL_MAIN },
	{ "NF",     "NOTES",     SIMPLE, LEVEL_MAIN },
	{ "DE",     "KEYWORD",   ISI_KEYWORD, LEVEL_MAIN }, /* author keywords */
	{ "ID",     "KEYWORD",   ISI_KEYWORD, LEVEL_MAIN }, /* new ISI keywords */
	{ "LA",     "LANGUAGE",  SIMPLE, LEVEL_MAIN },
	{ "TC",     "TIMESCITED", SIMPLE, LEVEL_MAIN },
	{ "NR",     "NUMBERREFS", SIMPLE, LEVEL_MAIN },
	{ "CR",     "CITEDREFS",  SIMPLE, LEVEL_MAIN },
	{ "PT",     " ",         TYPE,   LEVEL_MAIN },
	{ "DT",     "DOCUMENTTYPE", TYPE, LEVEL_MAIN },
	{ "GA",     "ISIDELIVERNUM", SIMPLE, LEVEL_MAIN}, /*ISI document delivery number */
	{ "UT",     "ISIREFNUM", SIMPLE, LEVEL_MAIN }, /* ISI unique article identifer */
	
	{ "PT",     " ",         TYPE,   LEVEL_HOST },
	{ "DI",     "DOI",       SIMPLE, LEVEL_MAIN },
	{ " ",         "TYPE|BOOK",       ALWAYS, LEVEL_MAIN },
	{ " ",         "ISSUANCE|monographic",    ALWAYS, LEVEL_MAIN },
	{ " ",         "RESOURCE|text",   ALWAYS, LEVEL_MAIN },
	{ " ",         "GENRE|book",      ALWAYS, LEVEL_MAIN }
};

static lookups inbook[] = {
	{ "AU",     "AUTHOR",    PERSON, LEVEL_MAIN },
	{ "AF",     "AUTHOR",    PERSON, LEVEL_MAIN },
	{ "TI",     "TITLE",     TITLE,  LEVEL_MAIN },
	{ "SO",     "TITLE",     TITLE,  LEVEL_HOST }, /* full journal name */
	{ "JI",     "SHORTTITLE",TITLE,  LEVEL_HOST }, /* abbr journal name */
	{ "J9",     "SHORTTITLE",TITLE,  LEVEL_HOST }, /* 29char journal name */
	{ "PU",     "PUBLISHER", SIMPLE, LEVEL_HOST },
	{ "PI",     "ADDRESS",   SIMPLE, LEVEL_HOST }, /* publisher city */
	{ "C1",     "ADDRESS",   SIMPLE, LEVEL_MAIN }, /* author address */
	{ "PA",     "ADDRESS",   SIMPLE, LEVEL_HOST }, /* publisher address */
	{ "RP",     "REPRINTADDRESS", SIMPLE, LEVEL_MAIN },
	{ "PY",     "PARTYEAR",  SIMPLE, LEVEL_MAIN },
	{ "PD",     "PARTMONTH", SIMPLE, LEVEL_MAIN },
	{ "VL",     "VOLUME",    SIMPLE, LEVEL_MAIN },
	{ "BP",     "PAGESTART", SIMPLE, LEVEL_MAIN },
	{ "EP",     "PAGEEND",   SIMPLE, LEVEL_MAIN },
	{ "PG",     "TOTALPAGES", SIMPLE, LEVEL_HOST },
	{ "IS",     "ISSUE",     SIMPLE, LEVEL_MAIN },
	{ "SN",     "SERIALNUMBER", SERIALNO, LEVEL_HOST },
	{ "AB",     "ABSTRACT",  SIMPLE, LEVEL_MAIN },
	{ "NF",     "NOTES",     SIMPLE, LEVEL_MAIN },
	{ "DE",     "KEYWORD",   ISI_KEYWORD, LEVEL_MAIN }, /* author keywords */
	{ "ID",     "KEYWORD",   ISI_KEYWORD, LEVEL_MAIN }, /* new ISI keywords */
	{ "LA",     "LANGUAGE",  SIMPLE, LEVEL_MAIN },
	{ "TC",     "TIMESCITED", SIMPLE, LEVEL_MAIN },
	{ "NR",     "NUMBERREFS", SIMPLE, LEVEL_MAIN },
	{ "CR",     "CITEDREFS",  SIMPLE, LEVEL_MAIN },
	{ "PT",     " ",         TYPE,   LEVEL_HOST },
	{ "DT",     "DOCUMENTTYPE", TYPE, LEVEL_MAIN },
	{ "GA",     "ISIDELIVERNUM", SIMPLE, LEVEL_MAIN}, /*ISI document delivery number */
	{ "UT",     "ISIREFNUM", SIMPLE, LEVEL_MAIN }, /* ISI unique article identifer */
	{ "DI",     "DOI",       SIMPLE, LEVEL_MAIN },
	{ " ",         "TYPE|INBOOK",   ALWAYS, LEVEL_MAIN },
	{ " ",         "RESOURCE|text", ALWAYS, LEVEL_MAIN },
	{ " ",         "ISSUANCE|monographic",    ALWAYS, LEVEL_HOST },
	{ " ",         "GENRE|book",    ALWAYS, LEVEL_HOST }
};

static lookups bookinseries[] = {
	{ "AU",     "AUTHOR",    PERSON, LEVEL_MAIN },
	{ "AF",     "AUTHOR",    PERSON, LEVEL_MAIN },
	{ "TI",     "TITLE",     TITLE,  LEVEL_MAIN },
	{ "SO",     "TITLE",     TITLE,  LEVEL_MAIN },
	{ "SE",     "TITLE",     TITLE,  LEVEL_HOST },
	{ "BS",     "SUBTITLE",  TITLE,  LEVEL_HOST },
	{ "JI",     "SHORTTITLE",TITLE,  LEVEL_HOST }, /* abbr journal name */
	{ "J9",     "SHORTTITLE",TITLE,  LEVEL_HOST }, /* 29char journal name */
	{ "PU",     "PUBLISHER", SIMPLE, LEVEL_HOST },
	{ "PI",     "ADDRESS",   SIMPLE, LEVEL_HOST }, /* publisher city */
	{ "C1",     "ADDRESS",   SIMPLE, LEVEL_MAIN }, /* author address */
	{ "PA",     "ADDRESS",   SIMPLE, LEVEL_HOST }, /* publisher address */
	{ "RP",     "REPRINTADDRESS", SIMPLE, LEVEL_MAIN },
	{ "PY",     "PARTYEAR",  SIMPLE, LEVEL_MAIN },
	{ "PD",     "PARTMONTH", SIMPLE, LEVEL_MAIN },
	{ "VL",     "VOLUME",    SIMPLE, LEVEL_MAIN },
	{ "BP",     "PAGESTART", SIMPLE, LEVEL_MAIN },
	{ "EP",     "PAGEEND",   SIMPLE, LEVEL_MAIN },
	{ "PG",     "TOTALPAGES", SIMPLE, LEVEL_MAIN },
	{ "IS",     "ISSUE",     SIMPLE, LEVEL_MAIN },
	{ "SN",     "SERIALNUMBER", SERIALNO, LEVEL_HOST },
	{ "AB",     "ABSTRACT",  SIMPLE, LEVEL_MAIN },
	{ "NF",     "NOTES",     SIMPLE, LEVEL_MAIN },
	{ "DE",     "KEYWORD",   ISI_KEYWORD, LEVEL_MAIN }, /* author keywords */
	{ "ID",     "KEYWORD",   ISI_KEYWORD, LEVEL_MAIN }, /* new ISI keywords */
	{ "LA",     "LANGUAGE",  SIMPLE, LEVEL_MAIN },
	{ "TC",     "TIMESCITED", SIMPLE, LEVEL_MAIN },
	{ "NR",     "NUMBERREFS", SIMPLE, LEVEL_MAIN },
	{ "CR",     "CITEDREFS",  SIMPLE, LEVEL_MAIN },
	{ "PT",     " ",         TYPE,   LEVEL_HOST },
	{ "DT",     "DOCUMENTTYPE", TYPE, LEVEL_MAIN },
	{ "GA",     "ISIDELIVERNUM", SIMPLE, LEVEL_MAIN}, /*ISI document delivery number */
	{ "UT",     "ISIREFNUM", SIMPLE, LEVEL_MAIN }, /* ISI unique article identifer */
	{ "DI",     "DOI",       SIMPLE, LEVEL_MAIN },

	{ " ",      "TYPE|INCOLLECTION",      ALWAYS, LEVEL_MAIN },
	{ " ",      "ISSUANCE|monographic",   ALWAYS, LEVEL_HOST },
	{ " ",      "RESOURCE|text",          ALWAYS, LEVEL_MAIN },
	{ " ",      "GENRE|collection",       ALWAYS, LEVEL_MAIN }
};



variants isi_all[] = {
	{ "Journal", &(article[0]), sizeof(article)/sizeof(lookups)},
	{ "J", &(article[0]), sizeof(article)/sizeof(lookups)},
	{ "Book", &(book[0]), sizeof(book)/sizeof(lookups)},
	{ "B", &(book[0]), sizeof(book)/sizeof(lookups)},
	{ "Chapter", &(inbook[0]), sizeof(inbook)/sizeof(lookups)},
	{ "S", &(bookinseries[0]), sizeof(bookinseries)/sizeof(lookups)},
};

int isi_nall = sizeof( isi_all ) / sizeof( variants );

#ifdef NOCOMPILE
int
get_reftype( char *p, long refnum )
{
	int i;
	while ( is_ws( *p ) ) p++;
	for ( i=0; i<nall; ++i )
		if ( strncasecmp( all[i].type, p, strlen(all[i].type) ) == 0 ) return i;
	fprintf( stderr, "Warning: Did not recognize '%s' of refnum %ld, defaulting to article.\n",
		p, refnum );
	return 0;
}
#endif
