/*
 * endtypes.c
 *
 * Copyright (c) Chris Putnam 2003-7
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
	{ "%0", "TYPE",      TYPE,   LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON, LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE, LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE,  LEVEL_MAIN },
	{ "%J", "TITLE",     TITLE,  LEVEL_HOST }, /* for refer formats */
	{ "%E", "2ND_AUTHOR",PERSON, LEVEL_MAIN },
	{ "%B", "2ND_TITLE", SIMPLE, LEVEL_MAIN },
	{ "%C", "ADDRESS",   SIMPLE, LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE, LEVEL_MAIN }, 
	{ "%V", "VOLUME" ,   SIMPLE, LEVEL_MAIN },
	{ "%6", "NUMVOLUMES",SIMPLE, LEVEL_HOST },
	{ "%N", "NUMBER",    SIMPLE, LEVEL_MAIN },
	{ "%P", "PAGES",        PAGES,  LEVEL_MAIN },
	{ "%&", "SECTION",      SIMPLE, LEVEL_MAIN },
	{ "%Y", "3RD_AUTHOR",   PERSON, LEVEL_MAIN },
	{ "%S", "3RD_TITLE",    SIMPLE, LEVEL_MAIN },
	{ "%7", "EDITION",      SIMPLE, LEVEL_MAIN },
	{ "%8", "MONTHDATE",    DATE,   LEVEL_MAIN },
	{ "%9", "GENRE",        SIMPLE, LEVEL_MAIN },
	{ "%?", "SUB_AUTHOR",   PERSON, LEVEL_MAIN }, /* subsidiary-authors */
	{ "%!", "SHORTTITLE",   TITLE,  LEVEL_MAIN },
	{ "%@", "SERIALNUM", SERIALNO,  LEVEL_MAIN },
	{ "%(", "ORIGINALPUB",  SIMPLE, LEVEL_MAIN },
	{ "%)", "REPRINTEDITION",SIMPLE,LEVEL_MAIN },
	{ "%*", "REVIEWEDITEM", SIMPLE, LEVEL_MAIN },
	{ "%1", "CUSTOM1",      SIMPLE, LEVEL_MAIN },
	{ "%2", "CUSTOM2",      SIMPLE, LEVEL_MAIN },
	{ "%3", "CUSTOM3",      SIMPLE, LEVEL_MAIN },
	{ "%4", "CUSTOM4",      SIMPLE, LEVEL_MAIN },
	{ "%#", "CUSTOM5",      SIMPLE, LEVEL_MAIN },
	{ "%$", "CUSTOM6",      SIMPLE, LEVEL_MAIN },
	{ "%M", "ACCESSNUM",    SIMPLE, LEVEL_MAIN },
	{ "%L", "CALLNUMBER",   SIMPLE, LEVEL_MAIN },
	{ "%F", "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ "%K", "KEYWORD",      SIMPLE, LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",     SIMPLE, LEVEL_MAIN },
	{ "%O", "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "%U", "URL",          SIMPLE, LEVEL_MAIN },
	{ "%Z", "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "%+", "AUTHORADDRESS",SIMPLE, LEVEL_MAIN }
};

static lookups journalarticle[] = {
	{ "%0", "TYPE",      TYPE,     LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON,   LEVEL_MAIN },
	{ "%D", "PARTYEAR",  SIMPLE,   LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE,    LEVEL_MAIN },
	{ "%J", "TITLE",     TITLE,    LEVEL_HOST }, /* journal title */
	{ "%B", "TITLE",     TITLE,    LEVEL_HOST }, /* journal title */
	{ "%C", "ADDRESS",   SIMPLE, LEVEL_MAIN },
	{ "%V", "VOLUME" ,   SIMPLE,   LEVEL_MAIN },
	{ "%N", "ISSUE",     SIMPLE,   LEVEL_MAIN },
	{ "%P", "PAGES",     PAGES,    LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE,    LEVEL_HOST }, 
	{ "%8", "PARTMONTHDATE", DATE,     LEVEL_MAIN },
	{ "%9", "GENRE",     SIMPLE,  LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE,    LEVEL_MAIN },
	{ "%@", "SERIALNUMBER",      SERIALNO, LEVEL_HOST },
	{ "%(", "ORIGINALPUB",SIMPLE,  LEVEL_MAIN },
	{ "%)", "REPRINTEDITION",SIMPLE,LEVEL_MAIN },
	{ "%*", "REVIEWEDITEM",SIMPLE, LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE,   LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE,   LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE,   LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE,   LEVEL_MAIN },
	{ "%X", "ABSTRACT",  SIMPLE,   LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE,   LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE,   LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE,   LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE,LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "ISSUANCE|continuing",    ALWAYS, LEVEL_HOST },
	{ "  ", "RESOURCE|text",          ALWAYS, LEVEL_MAIN },
	{ "  ", "GENRE|periodical",       ALWAYS, LEVEL_HOST },
	{ "  ", "GENRE|academic journal", ALWAYS, LEVEL_HOST }
};

static lookups magazinearticle[] = {
	{ "%0", "TYPE",      TYPE,    LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON,  LEVEL_MAIN },
	{ "%D", "PARTYEAR",  SIMPLE,  LEVEL_MAIN },
	{ "%T", "TITLE",     TITLE,   LEVEL_MAIN },
	{ "%J", "TITLE",     TITLE,   LEVEL_HOST }, /* magazine name */
	{ "%B", "TITLE",     TITLE,   LEVEL_HOST }, /* magazine name */
	{ "%C", "ADDRESS",   SIMPLE, LEVEL_MAIN },
	{ "%V", "VOLUME",    SIMPLE,  LEVEL_MAIN },
	{ "%N", "ISSUE",     SIMPLE,  LEVEL_MAIN },
	{ "%P", "PAGES",     PAGES,   LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE,    LEVEL_HOST }, 
	{ "%8", "PARTMONTHDATE", DATE,    LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE,   LEVEL_MAIN },
	{ "%@", "SERIALNUMBER",      SERIALNO,LEVEL_HOST },
	{ "%(", "ORIGINALPUB",SIMPLE, LEVEL_MAIN },
	{ "%)", "REPRINTEDITION",SIMPLE,LEVEL_MAIN },
	{ "%*", "REVIEWEDITEM",SIMPLE,LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE,  LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE,  LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE,  LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE,  LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE,  LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE,  LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE,  LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE,  LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE,LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "ISSUANCE|continuing",  ALWAYS, LEVEL_HOST },
	{ "  ", "RESOURCE|text",        ALWAYS, LEVEL_MAIN },
	{ "  ", "GENRE|periodical",     ALWAYS, LEVEL_HOST },
	{ "  ", "GENRE|magazine",       ALWAYS, LEVEL_HOST }
};

static lookups newspaperarticle[] = {
	{ "%0", "TYPE",      TYPE,    LEVEL_MAIN },
	{ "%A", "REPORTER",  PERSON,  LEVEL_MAIN },
	{ "%D", "PARTYEAR",  SIMPLE,  LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE,   LEVEL_MAIN },
	{ "%J", "TITLE",     TITLE,   LEVEL_HOST }, /* newspaper name */
	{ "%B", "TITLE",     TITLE,   LEVEL_HOST }, /* newspaper name */
	{ "%V", "VOLUME" ,   SIMPLE,  LEVEL_MAIN },
	{ "%C", "ADDRESS",   SIMPLE, LEVEL_MAIN },
	{ "%N", "ISSUE",     SIMPLE,  LEVEL_MAIN },
	{ "%P", "PAGES",     PAGES,   LEVEL_MAIN },
	{ "%7", "EDITION",   SIMPLE,  LEVEL_MAIN },
	{ "%8", "PARTMONTHDATE", DATE,    LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE,   LEVEL_MAIN },
	{ "%@", "SERIALNUMBER",      SERIALNO, LEVEL_MAIN },
	{ "%(", "ORIGINALPUB",SIMPLE, LEVEL_MAIN },
	{ "%)", "REPRINTEDITION",SIMPLE, LEVEL_MAIN },
	{ "%*", "REVIEWEDITEM",SIMPLE,LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE,  LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE,  LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE,  LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE,  LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE,  LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE,  LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE,  LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE,  LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "%+", "AUTHORADDRESS",SIMPLE,LEVEL_MAIN },
	{ "  ", "ISSUANCE|continuing",    ALWAYS, LEVEL_HOST },
	{ "  ", "RESOURCE|text",          ALWAYS, LEVEL_MAIN },
	{ "  ", "GENRE|newspaper",        ALWAYS, LEVEL_HOST }
};

static lookups book[] = {
	{ "%0", "TYPE",      TYPE,      LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON,    LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE,    LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE,     LEVEL_MAIN },
	{ "%E", "AUTHOR",    PERSON,    LEVEL_HOST }, /* SERIES_AUTHOR */
	{ "%B", "TITLE",     TITLE,     LEVEL_HOST }, /* SERIES_TITLE */
	{ "%S", "TITLE",     TITLE,     LEVEL_SERIES },
	{ "%C", "ADDRESS",   SIMPLE,    LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE,    LEVEL_MAIN }, 
	{ "%V", "VOLUME" ,   SIMPLE,    LEVEL_MAIN },
	{ "%6", "NUMVOLUMES",SIMPLE,    LEVEL_MAIN },
	{ "%P", "NUMBEROFPAGES", SIMPLE,LEVEL_MAIN },
	{ "%7", "EDITION",   SIMPLE,    LEVEL_MAIN },
	{ "%?", "TRANSLATOR",PERSON,    LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE,     LEVEL_MAIN },
	{ "%@", "SERIALNUMBER",      SERIALNO,    LEVEL_MAIN },
	{ "%(", "ORIGINALPUB",SIMPLE,   LEVEL_MAIN },
	{ "%)", "REPRINTEDITION",SIMPLE,LEVEL_MAIN },
	{ "%1", "CUSTOM1",   SIMPLE,    LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE,    LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE,    LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE,    LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE,    LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE,    LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE,    LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE,    LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE,    LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE,    LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE,    LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE,    LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE,    LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE,    LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE, LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "GENRE|book", ALWAYS,   LEVEL_MAIN },
	{ "  ", "ISSUANCE|monographic", ALWAYS,   LEVEL_MAIN },
	{ "  ", "RESOURCE|text", ALWAYS, LEVEL_MAIN }
};

static lookups booksection[] = {
	{ "%0", "TYPE",      TYPE,      LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON,    LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE,    LEVEL_HOST },
	{ "%T", "TITLE" ,    TITLE,     LEVEL_MAIN },
	{ "%E", "EDITOR",    PERSON,    LEVEL_HOST }, /* editor for book */
	{ "%B", "TITLE",     TITLE,     LEVEL_HOST }, /* book title */
	{ "%C", "ADDRESS",   SIMPLE,    LEVEL_HOST },
	{ "%I", "PUBLISHER", SIMPLE,    LEVEL_HOST }, 
	{ "%V", "VOLUME" ,   SIMPLE,    LEVEL_MAIN },
	{ "%6", "NUMVOLUMES",SIMPLE,    LEVEL_HOST },
	{ "%P", "PAGES",     PAGES,     LEVEL_MAIN },
	{ "%Y", "EDITOR",    PERSON,    LEVEL_SERIES },
	{ "%S", "TITLE",     TITLE,     LEVEL_SERIES },
	{ "%7", "EDITION",   SIMPLE,    LEVEL_HOST },
	{ "%?", "TRANSLATOR",PERSON,    LEVEL_HOST },
	{ "%!", "SHORTTITLE",TITLE,     LEVEL_MAIN },
	{ "%@", "SERIALNUMBER",      SERIALNO,    LEVEL_HOST },
	{ "%(", "ORIGINALPUB",SIMPLE,   LEVEL_MAIN },
	{ "%)", "REPRINTEDITION",SIMPLE,LEVEL_HOST },
	{ "%1", "CUSTOM1",   SIMPLE,    LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE,    LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE,    LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE,    LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE,    LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE,    LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE,    LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE,    LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE,    LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE,    LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE,    LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE,    LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE,    LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE,    LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE, LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "GENRE|book", ALWAYS, LEVEL_HOST },
	{ "  ", "ISSUANCE|monographic", ALWAYS, LEVEL_HOST },
	{ "  ", "RESOURCE|text", ALWAYS, LEVEL_MAIN }
};

static lookups editedbook[] = {
	{ "%0", "TYPE",      TYPE,      LEVEL_MAIN },
	{ "%A", "EDITOR",    PERSON,    LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE,    LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE,     LEVEL_MAIN },
	{ "%E", "EDITOR",    PERSON,    LEVEL_HOST }, /* SERIES_EDITOR */
	{ "%B", "TITLE",     TITLE,     LEVEL_HOST }, /* SERIES_TITLE */
	{ "%C", "ADDRESS",   SIMPLE,    LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE,    LEVEL_MAIN }, 
	{ "%V", "VOLUME" ,   SIMPLE,    LEVEL_MAIN },
	{ "%6", "NUMVOLUMES",SIMPLE,    LEVEL_HOST },
	{ "%P", "NUMBEROFPAGES", SIMPLE,LEVEL_MAIN },
	{ "%7", "EDITION",   SIMPLE,    LEVEL_MAIN },
	{ "%?", "TRANSLATOR",PERSON,    LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE,     LEVEL_MAIN },
	{ "%@", "SERIALNUMBER",      SERIALNO,    LEVEL_MAIN },
	{ "%(", "ORIGINALPUB",SIMPLE,   LEVEL_MAIN },
	{ "%)", "REPRINTEDITION",SIMPLE,LEVEL_MAIN },
	{ "%1", "CUSTOM1",   SIMPLE,    LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE,    LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE,    LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE,    LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE,    LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE,    LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE,    LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE,    LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE,    LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE,    LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE,    LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE,    LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE,    LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE,    LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE, LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "GENRE|book", ALWAYS, LEVEL_MAIN },
	{ "  ", "ISSUANCE|monographic", ALWAYS, LEVEL_MAIN },
	{ "  ", "RESOURCE|text", ALWAYS, LEVEL_MAIN }
};

static lookups manuscript[] = {
	{ "%0", "TYPE",      TYPE,    LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON,  LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE,  LEVEL_MAIN },
	{ "%T", "TITLE",     TITLE,   LEVEL_MAIN },
	{ "%B", "TITLE",     TITLE,   LEVEL_HOST }, /* COLLECTION_TITLE */
	{ "%C", "ADDRESS",   SIMPLE,  LEVEL_MAIN },
	{ "%N", "NUMBER",    SIMPLE,  LEVEL_MAIN },
	{ "%P", "PAGES",     PAGES,   LEVEL_MAIN },
	{ "%8", "MONTHDATE", DATE,    LEVEL_MAIN },
	{ "%9", "GENRE",     SIMPLE,  LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE,   LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE,  LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE,  LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE,  LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE,  LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE,  LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE,  LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE,  LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE,  LEVEL_MAIN },
	{ "%1", "CUSTOM1",   SIMPLE,  LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE,  LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE,  LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE,  LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE,  LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE,  LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE,LEVEL_MAIN},
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "RESOURCE|text", ALWAYS, LEVEL_MAIN },
	{ "  ", "GENRE|manuscript", ALWAYS, LEVEL_MAIN }
};

static lookups communication[] = {
	{ "%0", "TYPE",      TYPE,    LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON,  LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE,  LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE,   LEVEL_MAIN },
	{ "%E", "RECIPIENT", PERSON,  LEVEL_MAIN },
	{ "%C", "ADDRESS",   SIMPLE,  LEVEL_MAIN },
	{ "%V", "VOLUME",    SIMPLE,  LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE,  LEVEL_MAIN },
	{ "%8", "MONTHDATE", DATE,    LEVEL_MAIN },
	{ "%9", "GENRE",     SIMPLE,  LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE,   LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE,  LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE,  LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE,  LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE,  LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE,  LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE,  LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE,  LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE,  LEVEL_MAIN },
	{ "%1", "CUSTOM1",   SIMPLE,  LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE,  LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE,  LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE,  LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE,  LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE,  LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "%+", "AUTHORADDRESS",SIMPLE,LEVEL_MAIN},
	{ "  ", "GENRE|communication", ALWAYS, LEVEL_MAIN}
};

static lookups proceedings[] = {
	{ "%0", "TYPE",      TYPE,    LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON,  LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE,  LEVEL_HOST },
	{ "%T", "TITLE" ,    TITLE,   LEVEL_MAIN },
	{ "%E", "EDITOR",    PERSON,  LEVEL_MAIN },
	{ "%B", "CONFERENCE_NAME", SIMPLE,  LEVEL_MAIN }, 
	{ "%C", "ADDRESS",   SIMPLE,  LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE,  LEVEL_MAIN }, 
	{ "%V", "VOLUME" ,   SIMPLE,  LEVEL_MAIN },
	{ "%6", "NUMVOLUMES",SIMPLE,  LEVEL_HOST },
	{ "%N", "NUMBER",    SIMPLE,  LEVEL_MAIN },
	{ "%P", "PAGES",     PAGES,   LEVEL_MAIN },
	{ "%Y", "EDITOR",    PERSON,  LEVEL_HOST }, /* SERIES_EDITOR */
	{ "%S", "TITLE",     TITLE,   LEVEL_HOST }, /* SERIES_TITLE */
	{ "%7", "EDITION",   SIMPLE,  LEVEL_HOST },
	{ "%8", "MONTHDATE", DATE,    LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE,   LEVEL_MAIN },
	{ "%1", "CUSTOM1",   SIMPLE,  LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE,  LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE,  LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE,  LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE,  LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE,  LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE,  LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE,  LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE,  LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE,  LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE,  LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE,  LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE,  LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE,  LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE,LEVEL_MAIN},
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "RESOURCE|text", ALWAYS, LEVEL_MAIN },
	{ "  ", "GENRE|conference publication", ALWAYS, LEVEL_MAIN }
};

static lookups thesis[] = {
	{ "%0", "TYPE",      TYPE, LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON   , LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE   , LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE    , LEVEL_MAIN },
	{ "%B", "ACADEMIC_DEPARTMENT", SIMPLE   , LEVEL_MAIN },
	{ "%C", "ADDRESS",   SIMPLE   , LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE   , LEVEL_MAIN }, 
	{ "%V", "VOLUME" ,   SIMPLE   , LEVEL_MAIN },
	{ "%P", "NUMPAGES",     PAGES    , LEVEL_MAIN },
/*	{ "%9", "THESIS_TYPE", SIMPLE   , LEVEL_MAIN },*/
	{ "%9", "GENRE",     SIMPLE   , LEVEL_MAIN }, /* thesis type */
	{ "%8", "MONTHDATE", DATE     , LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE    , LEVEL_MAIN },
	{ "%1", "CUSTOM1",   SIMPLE   , LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE   , LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE   , LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE   , LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE   , LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE   , LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE   , LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE   , LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE   , LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE   , LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE   , LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE   , LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE, LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "RESOURCE|text", ALWAYS, LEVEL_MAIN },
	{ "  ", "GENRE|theses",  ALWAYS, LEVEL_MAIN }
};

static lookups program[] = {
	{ "%0", "TYPE",      TYPE,  LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON   , LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE   , LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE    , LEVEL_MAIN },
	{ "%C", "ADDRESS",   SIMPLE   , LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE   , LEVEL_MAIN },
	{ "%7", "VERSION",   SIMPLE   , LEVEL_MAIN },
	{ "%9", "GENRE",     SIMPLE   , LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE    , LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE   , LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE   , LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE   , LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE   , LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE   , LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE   , LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%1", "COMPUTER",  SIMPLE   , LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE   , LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE   , LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE   , LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE   , LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE   , LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE, LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "RESOURCE|software, multimedia", ALWAYS, LEVEL_MAIN }
};

static lookups audiovisual[] = {
	{ "%0", "TYPE",      TYPE     , LEVEL_MAIN },
	{ "%A", "WRITER",    PERSON   , LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE   , LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE    , LEVEL_MAIN },
	{ "%B", "TITLE",     TITLE    , LEVEL_HOST }, /* COLLECTION_TITLE */
	{ "%C", "ADDRESS",   SIMPLE   , LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE   , LEVEL_MAIN },
	{ "%V", "EXTENTOFWORK",     SIMPLE   , LEVEL_MAIN },
	{ "%N", "NUMBER",    SIMPLE   , LEVEL_MAIN },
	{ "%8", "MONTHDATE", DATE     , LEVEL_MAIN },
	{ "%9", "GENRE",     SIMPLE   , LEVEL_MAIN },
	{ "%?", "PERFORMER", PERSON   , LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE    , LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE   , LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE   , LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE   , LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE   , LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE   , LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE   , LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%1", "CUSTOM1",   SIMPLE   , LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE   , LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE   , LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE   , LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE   , LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE   , LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE, LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "RESOURCE|mixed material", ALWAYS, LEVEL_MAIN }
};

static lookups broadcast[] = {
	{ "%0", "TYPE",      TYPE     , LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON   , LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE   , LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE    , LEVEL_MAIN },
	{ "%E", "DIRECTOR",  PERSON   , LEVEL_MAIN },
	{ "%B", "TITLE",     TITLE    , LEVEL_HOST }, /* SERIES_TITLE */
	{ "%C", "ADDRESS",   SIMPLE   , LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE   , LEVEL_MAIN },
	{ "%Y", "PRODUCER",  PERSON   , LEVEL_MAIN },
	{ "%8", "MONTHDATE", DATE     , LEVEL_MAIN },
	{ "%9", "GENRE",     SIMPLE   , LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE    , LEVEL_MAIN },
	{ "%@", "SERIALNUMBER",      SERIALNO , LEVEL_MAIN },
	{ "%?", "PERFORMER", PERSON   , LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE   , LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE   , LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE   , LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE   , LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE   , LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE   , LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%1", "CUSTOM1",   SIMPLE   , LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE   , LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE   , LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE   , LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE   , LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE   , LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE, LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "RESOURCE|moving image", ALWAYS, LEVEL_MAIN }
};

static lookups electronic[] = {
	{ "%0", "TYPE",      TYPE      , LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON    , LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE    , LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE     , LEVEL_MAIN },
	{ "%C", "ADDRESS",   SIMPLE, LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE    , LEVEL_MAIN }, 
	{ "%V", "ACCESS_YEAR",  SIMPLE , LEVEL_MAIN },
	{ "%N", "ACCESS_DATE",  DATE   , LEVEL_MAIN },
	{ "%7", "EDITION",   SIMPLE    , LEVEL_MAIN },
	{ "%8", "UPDATE_DATE", DATE    , LEVEL_MAIN },
	{ "%9", "GENRE",     SIMPLE    , LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE     , LEVEL_MAIN },
	{ "%1", "CUSTOM1",   SIMPLE    , LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE    , LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE    , LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE    , LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE    , LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE    , LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE    , LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE    , LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE    , LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE    , LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE    , LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE    , LEVEL_MAIN },
	{ "%V", "VOLUME",    SIMPLE    , LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE    , LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "%+", "AUTHORADDRESS",SIMPLE , LEVEL_MAIN },
        { " ",  "RESOURCE|software, multimedia",    ALWAYS, LEVEL_MAIN },
        { " ",  "GENRE|electronic",       ALWAYS, LEVEL_MAIN },

};

static lookups artwork[] = {
	{ "%0", "TYPE",      TYPE     , LEVEL_MAIN },
	{ "%A", "ARTIST",    PERSON   , LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE   , LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE    , LEVEL_MAIN },
	{ "%C", "ADDRESS",   SIMPLE   , LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE   , LEVEL_MAIN },
	{ "%8", "MONTHDATE", DATE     , LEVEL_MAIN },
	{ "%9", "GENRE",     SIMPLE   , LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE    , LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE   , LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE   , LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE   , LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE   , LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE   , LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE   , LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%1", "CUSTOM1",   SIMPLE   , LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE   , LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE   , LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE   , LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE   , LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE   , LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE, LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "GENRE|art original", ALWAYS, LEVEL_MAIN }
};

static lookups report[] = {
	{ "%0", "TYPE",      TYPE     , LEVEL_MAIN },
	{ "%A", "AUTHOR",    PERSON   , LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE   , LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE    , LEVEL_MAIN },
	{ "%C", "ADDRESS",   SIMPLE   , LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE   , LEVEL_MAIN },
	{ "%P", "PAGES",     PAGES    , LEVEL_MAIN },
	{ "%8", "MONTHDATE", DATE     , LEVEL_MAIN },
	{ "%9", "GENRE",     SIMPLE   , LEVEL_MAIN },
	{ "%@", "SERIALNUMBER", SERIALNO , LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE    , LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE   , LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE   , LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE   , LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE   , LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE   , LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE   , LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%1", "CUSTOM1",   SIMPLE   , LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE   , LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE   , LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE   , LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE   , LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE   , LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE, LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "RESOURCE|text", ALWAYS, LEVEL_MAIN },
	{ "  ", "GENRE|report", ALWAYS, LEVEL_MAIN }
};

static lookups map[] = {
	{ "%0", "TYPE",      TYPE     , LEVEL_MAIN },
	{ "%A", "CARTOGRAPHER",    PERSON   , LEVEL_MAIN },
	{ "%D", "YEAR",      SIMPLE   , LEVEL_MAIN },
	{ "%T", "TITLE" ,    TITLE    , LEVEL_MAIN },
	{ "%C", "ADDRESS",   SIMPLE   , LEVEL_MAIN },
	{ "%I", "PUBLISHER", SIMPLE   , LEVEL_MAIN },
	{ "%7", "EDITION",   SIMPLE   , LEVEL_MAIN },
	{ "%9", "GENRE",     SIMPLE   , LEVEL_MAIN },
	{ "%!", "SHORTTITLE",TITLE    , LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE   , LEVEL_MAIN },
	{ "%L", "CALLNUMBER",SIMPLE   , LEVEL_MAIN },
	{ "%F", "REFNUM",    SIMPLE   , LEVEL_MAIN },
	{ "%K", "KEYWORD",   SIMPLE   , LEVEL_MAIN }, 
	{ "%X", "ABSTRACT",  SIMPLE   , LEVEL_MAIN },
	{ "%O", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%U", "URL",       SIMPLE   , LEVEL_MAIN },
	{ "%Z", "NOTES",     SIMPLE   , LEVEL_MAIN },
	{ "%1", "SCALE",     SIMPLE   , LEVEL_MAIN },
	{ "%2", "CUSTOM2",   SIMPLE   , LEVEL_MAIN },
	{ "%3", "CUSTOM3",   SIMPLE   , LEVEL_MAIN },
	{ "%4", "CUSTOM4",   SIMPLE   , LEVEL_MAIN },
	{ "%#", "CUSTOM5",   SIMPLE   , LEVEL_MAIN },
	{ "%$", "CUSTOM6",   SIMPLE   , LEVEL_MAIN },
	{ "%+", "AUTHORADDRESS",SIMPLE, LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "RESOURCE|cartographic", ALWAYS, LEVEL_MAIN },
	{ "  ", "GENRE|map", ALWAYS,  LEVEL_MAIN }
};

static lookups patent[] = {
	{ "%0", "TYPE",      TYPE, LEVEL_MAIN    },
	{ "%A", "AUTHOR",    PERSON, LEVEL_MAIN  },
	{ "%D", "YEAR",      SIMPLE, LEVEL_MAIN  },
	{ "%T", "TITLE" ,    TITLE, LEVEL_MAIN   },
	{ "%B", "PUBLISHED_SOURCE", SIMPLE, LEVEL_MAIN},
	{ "%C", "COUNTRY",   SIMPLE, LEVEL_MAIN  },
	{ "%I", "ASSIGNEE",  SIMPLE, LEVEL_MAIN  },
	{ "%V", "VOLUME",    SIMPLE, LEVEL_MAIN  },
	{ "%N", "ISSUE",     SIMPLE, LEVEL_MAIN  },
	{ "%P", "PAGES",     SIMPLE, LEVEL_MAIN  },
	{ "%@", "NUMBER",    SIMPLE, LEVEL_MAIN  },
	{ "%M", "ACCESSNUM", SIMPLE, LEVEL_MAIN  },
	{ "%L", "CALLNUMBER",SIMPLE, LEVEL_MAIN  },
	{ "%F", "REFNUM",    SIMPLE, LEVEL_MAIN  },
	{ "%K", "KEYWORD",   SIMPLE, LEVEL_MAIN  }, 
	{ "%X", "ABSTRACT",  SIMPLE, LEVEL_MAIN  },
	{ "%O", "NOTES",     SIMPLE, LEVEL_MAIN  },
	{ "%U", "URL",       SIMPLE, LEVEL_MAIN  },
	{ "%Z", "NOTES",     SIMPLE, LEVEL_MAIN  },
	{ "%1", "SCALE",     SIMPLE, LEVEL_MAIN  },
	{ "%2", "CUSTOM2",   SIMPLE, LEVEL_MAIN  },
	{ "%3", "CUSTOM3",   SIMPLE, LEVEL_MAIN  },
	{ "%4", "CUSTOM4",   SIMPLE, LEVEL_MAIN  },
	{ "%#", "CUSTOM5",   SIMPLE, LEVEL_MAIN  },
	{ "%$", "CUSTOM6",   SIMPLE, LEVEL_MAIN  },
	{ "%+", "AUTHORADDRESS",SIMPLE, LEVEL_MAIN },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "RESOURCE|text", ALWAYS, LEVEL_MAIN },
	{ "  ", "GENRE|patent", ALWAYS, LEVEL_MAIN }
};

static lookups hearing[] = {
	{ "%0", "TYPE",      TYPE, LEVEL_MAIN    },
	{ "%D", "YEAR",      SIMPLE, LEVEL_MAIN  },
	{ "%T", "TITLE" ,    TITLE, LEVEL_MAIN   },
	{ "%B", "COMMITTEE", SIMPLE, LEVEL_MAIN  },
	{ "%C", "ADDRESS",   SIMPLE, LEVEL_MAIN  },
	{ "%I", "PUBLISHER", SIMPLE, LEVEL_MAIN  }, 
	{ "%6", "NUMVOLUMES",SIMPLE, LEVEL_MAIN  },
	{ "%N", "NUMBER",    SIMPLE, LEVEL_MAIN  },
	{ "%P", "NUMPAGES",  PAGES,  LEVEL_MAIN  },
	{ "%S", "LEGISLATIVEBODY", SIMPLE, LEVEL_MAIN  },
	{ "%7", "SESSION",   SIMPLE, LEVEL_MAIN  },
	{ "%8", "MONTHDATE", DATE,   LEVEL_MAIN  },
	{ "%!", "SHORTTITLE",TITLE,  LEVEL_MAIN  },
	{ "%(", "ORIGINALPUB",SIMPLE, LEVEL_MAIN },
	{ "%M", "ACCESSNUM", SIMPLE, LEVEL_MAIN  },
	{ "%L", "CALLNUMBER",SIMPLE, LEVEL_MAIN  },
	{ "%F", "REFNUM",    SIMPLE, LEVEL_MAIN  },
	{ "%K", "KEYWORD",   SIMPLE, LEVEL_MAIN  }, 
	{ "%X", "ABSTRACT",  SIMPLE, LEVEL_MAIN  },
	{ "%O", "NOTES",     SIMPLE, LEVEL_MAIN  },
	{ "%U", "URL",       SIMPLE, LEVEL_MAIN  },
	{ "%Z", "NOTES",     SIMPLE, LEVEL_MAIN  },
	{ "%1", "CUSTOM1",   SIMPLE, LEVEL_MAIN  },
	{ "%2", "CUSTOM2",   SIMPLE, LEVEL_MAIN  },
	{ "%3", "CUSTOM3",   SIMPLE, LEVEL_MAIN  },
	{ "%4", "CUSTOM4",   SIMPLE, LEVEL_MAIN  },
	{ "%9", "GENRE",     SIMPLE, LEVEL_MAIN  },
	{ "%#", "CUSTOM5",   SIMPLE, LEVEL_MAIN  },
	{ "%$", "CUSTOM6",   SIMPLE, LEVEL_MAIN  },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "GENRE|hearing", ALWAYS, LEVEL_MAIN }
};

static lookups bill[] = {
	{ "%0", "TYPE",      TYPE, LEVEL_MAIN    },
	{ "%D", "YEAR",      SIMPLE, LEVEL_MAIN  },
	{ "%T", "TITLE" ,    TITLE, LEVEL_MAIN   },
	{ "%B", "CODE",      SIMPLE, LEVEL_MAIN  },
	{ "%V", "CODEVOLUME",SIMPLE, LEVEL_MAIN   },
	{ "%N", "BILLNUMBER",SIMPLE, LEVEL_MAIN   },
	{ "%P", "CODEPAGES", SIMPLE, LEVEL_MAIN  },
	{ "%&", "CODESECTION",SIMPLE, LEVEL_MAIN  },
	{ "%S", "LEGISLATIVEBODY", SIMPLE, LEVEL_MAIN  },
	{ "%7", "SESSION",   SIMPLE, LEVEL_MAIN  },
	{ "%8", "MONTHDATE", DATE, LEVEL_MAIN    },
	{ "%?", "SPONSOR",   PERSON, LEVEL_MAIN  },
	{ "%!", "SHORTTITLE",TITLE,  LEVEL_MAIN   },
	{ "%(", "ORIGINALPUB",SIMPLE, LEVEL_MAIN  },
	{ "%M", "ACCESSNUM", SIMPLE, LEVEL_MAIN  },
	{ "%L", "CALLNUMBER",SIMPLE, LEVEL_MAIN   },
	{ "%F", "REFNUM",    SIMPLE, LEVEL_MAIN  },
	{ "%K", "KEYWORD",   SIMPLE, LEVEL_MAIN  }, 
	{ "%X", "ABSTRACT",  SIMPLE, LEVEL_MAIN  },
	{ "%O", "NOTES",     SIMPLE, LEVEL_MAIN  },
	{ "%U", "URL",       SIMPLE, LEVEL_MAIN  },
	{ "%Z", "NOTES",     SIMPLE, LEVEL_MAIN  },
	{ "%1", "CUSTOM1",   SIMPLE, LEVEL_MAIN  },
	{ "%2", "CUSTOM2",   SIMPLE, LEVEL_MAIN  },
	{ "%3", "CUSTOM3",   SIMPLE, LEVEL_MAIN  },
	{ "%4", "CUSTOM4",   SIMPLE, LEVEL_MAIN  },
	{ "%#", "CUSTOM5",   SIMPLE, LEVEL_MAIN  },
	{ "%$", "CUSTOM6",   SIMPLE, LEVEL_MAIN  },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "RESOURCE|text", ALWAYS, LEVEL_MAIN },
	{ "  ", "GENRE|legislation", ALWAYS, LEVEL_MAIN }
};

static lookups statute[] = {
	{ "%0", "TYPE",      TYPE, LEVEL_MAIN    },
	{ "%D", "YEAR",      SIMPLE, LEVEL_MAIN  },
	{ "%T", "TITLE" ,    TITLE, LEVEL_MAIN   },
	{ "%B", "CODE",      SIMPLE, LEVEL_MAIN  },
	{ "%V", "CODENUMBER",SIMPLE, LEVEL_MAIN   },
	{ "%N", "PUBLICLAWNUMBER",    SIMPLE, LEVEL_MAIN  },
	{ "%P", "PAGES",     PAGES, LEVEL_MAIN    },
	{ "%&", "SECTION",   SIMPLE, LEVEL_MAIN  },
	{ "%7", "SESSION",   SIMPLE, LEVEL_MAIN  },
	{ "%8", "MONTHDATE", DATE, LEVEL_MAIN    },
	{ "%!", "SHORTTITLE",TITLE,  LEVEL_MAIN   },
	{ "%(", "ORIGINALPUB",SIMPLE, LEVEL_MAIN  },
	{ "%M", "ACCESSNUM", SIMPLE, LEVEL_MAIN  },
	{ "%L", "CALLNUMBER",SIMPLE, LEVEL_MAIN   },
	{ "%F", "REFNUM",    SIMPLE, LEVEL_MAIN  },
	{ "%K", "KEYWORD",   SIMPLE, LEVEL_MAIN  }, 
	{ "%X", "ABSTRACT",  SIMPLE, LEVEL_MAIN  },
	{ "%O", "NOTES",     SIMPLE, LEVEL_MAIN  },
	{ "%U", "URL",       SIMPLE, LEVEL_MAIN  },
	{ "%Z", "NOTES",     SIMPLE, LEVEL_MAIN  },
	{ "%1", "CUSTOM1",   SIMPLE, LEVEL_MAIN  },
	{ "%2", "CUSTOM2",   SIMPLE, LEVEL_MAIN  },
	{ "%3", "CUSTOM3",   SIMPLE, LEVEL_MAIN  },
	{ "%4", "CUSTOM4",   SIMPLE, LEVEL_MAIN  },
	{ "%#", "CUSTOM5",   SIMPLE, LEVEL_MAIN  },
	{ "%$", "CUSTOM6",   SIMPLE, LEVEL_MAIN  },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "RESOURCE|text", ALWAYS, LEVEL_MAIN },
	{ "  ", "GENRE|legislation", ALWAYS, LEVEL_MAIN }
};

static lookups lawcase[] = {
	{ "%0", "TYPE",      TYPE, LEVEL_MAIN    },
	{ "%D", "YEAR",      SIMPLE, LEVEL_MAIN  },
	{ "%T", "TITLE" ,    TITLE, LEVEL_MAIN   },
	{ "%E", "REPORTER",  TITLE, LEVEL_HOST   }, /*Reporter is name of book*/
	{ "%B", "CODE",      SIMPLE, LEVEL_MAIN  },
	{ "%I", "COURT",     SIMPLE, LEVEL_MAIN  },
	{ "%V", "VOLUME",    SIMPLE, LEVEL_MAIN   },
	{ "%N", "PUBLICLAWNUMBER",    SIMPLE, LEVEL_MAIN  },
	{ "%P", "STARTPAGE",     SIMPLE, LEVEL_MAIN  },
	{ "%8", "MONTHDATE", DATE, LEVEL_MAIN    },
	{ "%?", "COUNSEL",   PERSON, LEVEL_MAIN  },
	{ "%!", "SHORTTITLE",TITLE,  LEVEL_MAIN   },
	{ "%(", "ORIGINALPUB",SIMPLE, LEVEL_MAIN  },
	{ "%M", "ACCESSNUM", SIMPLE, LEVEL_MAIN  },
	{ "%L", "CALLNUMBER", SIMPLE, LEVEL_MAIN  },
	{ "%F", "REFNUM",    SIMPLE, LEVEL_MAIN  },
	{ "%K", "KEYWORD",   SIMPLE, LEVEL_MAIN  }, 
	{ "%X", "ABSTRACT",  SIMPLE, LEVEL_MAIN  },
	{ "%O", "NOTES",     SIMPLE, LEVEL_MAIN  },
	{ "%U", "URL",       SIMPLE, LEVEL_MAIN  },
	{ "%Z", "NOTES",     SIMPLE, LEVEL_MAIN  },
	{ "%1", "CUSTOM1",   SIMPLE, LEVEL_MAIN  },
	{ "%2", "CUSTOM2",   SIMPLE, LEVEL_MAIN  },
	{ "%3", "CUSTOM3",   SIMPLE, LEVEL_MAIN  },
	{ "%4", "CUSTOM4",   SIMPLE, LEVEL_MAIN  },
	{ "%#", "CUSTOM5",   SIMPLE, LEVEL_MAIN  },
	{ "%$", "CUSTOM6",   SIMPLE, LEVEL_MAIN  },
	{ "%W", "PHYSICALLOC",  SIMPLE, LEVEL_MAIN },  /* physical location */
	{ "  ", "RESOURCE|text", ALWAYS, LEVEL_MAIN },
	{ "  ", "GENRE|legal case and case notes", ALWAYS, LEVEL_MAIN }
};

/* order is important....."Book" matches "Book" and "Book Section", hence
 * "Book Section must come first */

variants end_all[] = {
	{"Generic", &(generic[0]), sizeof(generic)/sizeof(lookups)},
	{"Journal Article", &(journalarticle[0]), sizeof(journalarticle)/sizeof(lookups)},
	{"Book Section", &(booksection[0]),sizeof(booksection)/sizeof(lookups)},
	{"Book", &(book[0]), sizeof(book)/sizeof(lookups)},
	{"Edited Book", &(editedbook[0]),sizeof(editedbook)/sizeof(lookups)},
	{"Magazine Article", &(magazinearticle[0]), sizeof(magazinearticle)/sizeof(lookups)},
	{"Newspaper Article", &(newspaperarticle[0]), sizeof(newspaperarticle)/sizeof(lookups)},
	{"Manuscript", &(manuscript[0]), sizeof(manuscript)/sizeof(lookups)},
	{"Personal Communication", &(communication[0]), sizeof(communication)/sizeof(lookups)},
	{"Conference Proceeding", &(proceedings[0]), sizeof(proceedings)/sizeof(lookups)},
	{"Thesis", &(thesis[0]), sizeof(thesis)/sizeof(lookups)},
	{"Computer Program", &(program[0]), sizeof(program)/sizeof(lookups)},
	{"Audiovisual Material", &(audiovisual[0]), sizeof(audiovisual)/sizeof(lookups)},
	{"Film or Broadcast", &(broadcast[0]), sizeof(broadcast)/sizeof(lookups)},
	{"Artwork", &(artwork[0]), sizeof(artwork)/sizeof(lookups)},
	{"Electronic Source", &(electronic[0]), sizeof(electronic)/sizeof(lookups)},
	{"Report", &(report[0]), sizeof(report)/sizeof(lookups)},
	{"Map", &(map[0]), sizeof(map)/sizeof(lookups)},
	{"Patent", &(patent[0]), sizeof(patent)/sizeof(lookups)},
	{"Hearing", &(hearing[0]), sizeof(hearing)/sizeof(lookups)},
	{"Bill", &(bill[0]), sizeof(bill)/sizeof(lookups)},
	{"Statute", &(statute[0]), sizeof(statute)/sizeof(lookups)},
	{"Case", &(lawcase[0]), sizeof(lawcase)/sizeof(lookups)}
};


int end_nall = sizeof( end_all ) / sizeof( variants );

#ifdef NOCOMPILE
int
get_reftype( char *p, long refnum )
{
	int i;
	while ( is_ws( *p ) ) p++;
	for ( i=0; i<nall; ++i ) {
		if ( !strncasecmp( all[i].type, p, strlen(all[i].type)) ){
/*			fprintf( stderr,"SUCCESS strncasecmp('%s', '%s', %d )\n", all[i].type,p,strlen(all[i].type)); */
			return i;
		}else {
/*			fprintf( stderr,"FAILURE strncasecmp('%s', '%s', %d )\n", all[i].type,p,strlen(all[i].type));*/
		}
	}
	fprintf( stderr, "Warning: Did not recognize type '%s' of refnum %ld, defaulting to generic.\n",
			p, refnum );
	return 0;
}
#endif

