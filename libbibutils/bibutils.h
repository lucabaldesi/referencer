/*
 * bibutils.h
 *
 * Copyright (c) Chris Putnam 2005-7
 *
 */
#ifndef BIBUTILS_H
#define BIBUTILS_H

#include <stdio.h>
#include "lists.h"
#include "bibl.h"

#define BIBL_OK           (0)
#define BIBL_ERR_BADINPUT (-1)
#define BIBL_ERR_MEMERR   (-2)
#define BIBL_ERR_CANTOPEN (-3)

#define BIBL_FIRSTIN      (100)
#define BIBL_MODSIN       (BIBL_FIRSTIN)
#define BIBL_BIBTEXIN     (BIBL_FIRSTIN+1)
#define BIBL_RISIN        (BIBL_FIRSTIN+2)
#define BIBL_ENDNOTEIN    (BIBL_FIRSTIN+3)
#define BIBL_COPACIN      (BIBL_FIRSTIN+4)
#define BIBL_ISIIN        (BIBL_FIRSTIN+5)
#define BIBL_MEDLINEIN    (BIBL_FIRSTIN+6)
#define BIBL_ENDNOTEXMLIN (BIBL_FIRSTIN+7)
#define BIBL_LASTIN       (BIBL_FIRSTIN+7)

#define BIBL_FIRSTOUT     (200)
#define BIBL_MODSOUT      (BIBL_FIRSTOUT)
#define BIBL_BIBTEXOUT    (BIBL_FIRSTOUT+1)
#define BIBL_RISOUT       (BIBL_FIRSTOUT+2)
#define BIBL_ENDNOTEOUT   (BIBL_FIRSTOUT+3)
#define BIBL_ISIOUT       (BIBL_FIRSTOUT+4)
#define BIBL_WORD2007OUT  (BIBL_FIRSTOUT+5)
#define BIBL_LASTOUT      (BIBL_FIRSTOUT+5)

#define BIBL_FORMAT_VERBOSE (1)

#define BIBL_RAW_WITHCHARCONVERT (4)
#define BIBL_RAW_WITHMAKEREFID   (8)

#define BIBL_CHARSET_UNKNOWN (-1)
#define BIBL_CHARSET_UNICODE (-2)
#define BIBL_CHARSET_DEFAULT (66)  /* Latin-1/ISO8859-1 */

#define BIBL_SRC_DEFAULT (0)  /* value from program default */
#define BIBL_SRC_FILE    (1)  /* value from file, priority over default */
#define BIBL_SRC_USER    (2)  /* value from user, priority over file, default */


typedef struct param {

	int charsetin;
	int charsetin_src; /* BIBL_SRC_DEFAULT, BIBL_SRC_FILE, BIBL_SRC_USER */
	int latexin;
	int utf8in;
	int xmlin;

	int charsetout;
	int charsetout_src; /* BIBL_SRC_PROG, BIBL_SRC_USER */
	int latexout;
	int utf8out;
	int xmlout;

	int output_raw;
	int format_opts; /* options for specific formats */
	int verbose;
	int addcount;  /* add reference count to reference id */
	int singlerefperfile;

} param;

extern void bibl_initparams( param *p, int readmode, int writemode );
extern int bibl_read( bibl *b, FILE *fp, char *filename, int mode, param *p );
extern int bibl_write( bibl *b, FILE *fp, int mode, param *p );
extern void bibl_reporterr( int err );

#endif

