/*
 * bibutils.c
 *
 * Copyright (c) Chris Putnam 2005-7
 *
 * Source code released under the GPL
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "bibutils.h"

/* internal includes */
#include "reftypes.h"
#include "bibtexin.h"
#include "bibtexout.h"
#include "copacin.h"
#include "endin.h"
#include "endxmlin.h"
#include "endout.h"
#include "isiin.h"
#include "isiout.h"
#include "medin.h"
#include "modsin.h"
#include "risin.h"
#include "risout.h"
#include "modsout.h"
#include "wordout.h"
#include "newstr_conv.h"

typedef struct convert_rules {
	int  (*readf)(FILE*,char*,int,int*,newstr*,newstr*,int*);
	int  (*processf)(fields*,char*,char*,long);
	void (*cleanf)(bibl*);
	int  (*typef) (fields*,char*,int,variants*,int);
	void (*convertf)(fields*,fields*,int,int,variants*,int);
	void (*headerf)(FILE*,int);
	void (*footerf)(FILE*);
	void (*writef)(fields*,FILE*,int,unsigned long);
	variants *all;
	int  nall;
} convert_rules;

void
bibl_initparams( param *p, int readmode, int writemode )
{
	p->format_opts      = 0;
	p->charsetin        = BIBL_CHARSET_DEFAULT;
	p->charsetin_src    = BIBL_SRC_DEFAULT;
	p->charsetout       = BIBL_CHARSET_DEFAULT;
	p->charsetout_src   = BIBL_SRC_DEFAULT;
	p->latexin          = 0;
	p->latexout         = 0;
	p->utf8in           = 0;
	p->utf8out          = 0;
	p->xmlout           = 0;
	p->verbose          = 0;
	p->addcount         = 0;
	p->singlerefperfile = 0;
	p->output_raw       = 0;	/* keep MODS tags for output filter */

	if ( readmode == BIBL_MEDLINEIN ) {
		/* default medline to UTF8 Unicode */
		p->charsetin = BIBL_CHARSET_UNICODE;
		p->utf8in = 1;
	} else if ( readmode == BIBL_BIBTEXIN ) {
		p->latexin = 1;
	} else if ( readmode == BIBL_MODSIN ) {
		p->xmlin = 1;
		p->utf8in = 1;
		p->output_raw = BIBL_RAW_WITHMAKEREFID |
				BIBL_RAW_WITHCHARCONVERT;
		p->charsetin = BIBL_CHARSET_UNICODE;
	} else if ( readmode == BIBL_MEDLINEIN ) {
		p->xmlin = 1;
		p->output_raw = BIBL_RAW_WITHMAKEREFID |
				BIBL_RAW_WITHCHARCONVERT;
	} else if ( readmode == BIBL_ENDNOTEXMLIN ) {
		p->xmlin = 1;
		p->utf8in = 1;
	}

	if ( writemode == BIBL_BIBTEXOUT ) {
		p->latexout = 1;
	} else if ( writemode == BIBL_MODSOUT ) {
		if ( !p->utf8out ) p->xmlout = 1;
		p->charsetout = BIBL_CHARSET_UNICODE;
	} else if ( writemode == BIBL_WORD2007OUT ) {
		if ( !p->utf8out ) p->xmlout = 1;
		p->charsetout = BIBL_CHARSET_UNICODE;
	} else p->xmlout = 0;

}

void
bibl_reporterr( int err )
{
	fprintf( stderr, "Bibutils: " );
	switch( err ) {
		case BIBL_OK:
			fprintf( stderr, "No error." ); break;
		case BIBL_ERR_BADINPUT:
			fprintf( stderr, "Bad input." ); break;
		case BIBL_ERR_MEMERR:
			fprintf( stderr, "Memory error." ); break;
		case BIBL_ERR_CANTOPEN:
			fprintf( stderr, "Can't open." ); break;
		default:
			fprintf( stderr, "Cannot identify error code."); break;
	}
	fprintf( stderr, "\n" );
}

static int
bibl_illegalinmode( int mode )
{
	if ( mode < BIBL_FIRSTIN || mode > BIBL_LASTIN ) return 1;
	else return 0;
}

static int
bibl_illegaloutmode( int mode )
{
	if ( mode < BIBL_FIRSTOUT || mode > BIBL_LASTOUT ) return 1;
	else return 0;
}

void
bibl_verbose2( fields *info, char *filename, long nrefs )
{
	int i;
	fprintf( stderr, "======== %s %ld : converted\n", filename, nrefs );
	for ( i=0; i<info->nfields; ++i ) {
		fprintf( stderr, "'%s'='%s' level=%d\n", info->tag[i].data,
				info->data[i].data , info->level[i]);
	}
	fprintf( stderr, "\n" );
	fflush( stderr );
}

void
bibl_verbose1( fields *info, fields *orig, char *filename, long nrefs )
{
	int i;
	fprintf( stderr, "======== %s %ld : processed\n", filename, nrefs );
	for ( i=0; i<orig->nfields; ++i ) {
		fprintf( stderr, "'%s'='%s' level=%d\n", orig->tag[i].data,
				orig->data[i].data , orig->level[i]);
	}
	bibl_verbose2( info, filename, nrefs );
}

void
bibl_verbose0( bibl *bin )
{
	int i;
	for ( i=0; i<bin->nrefs; ++i )
		bibl_verbose2( bin->ref[i], "", i+1 );
}

static void
process_alwaysadd( fields *info, int reftype, convert_rules *r )
{
        char tag[512], data[512], *p, *q;
        int i, process, level;
        for ( i=0; i<r->all[reftype].ntags; ++i ) {
                process = ((r->all[reftype]).tags[i]).processingtype;
                if ( process!=ALWAYS ) continue;
                level   = ((r->all[reftype]).tags[i]).level;
                p = ((r->all[reftype]).tags[i]).newstr;
                q = tag;
                while ( p && *p && *p!='|' ) {
                        *q = *p;
                        q++;
                        p++;
                }
                *q = '\0';
                q = data;
                if ( p && *p=='|' ) p++;
                while ( p && *p ) {
                        *q = *p;
                        q++;
                        p++;
                }
                *q = '\0';
                fields_add( info, tag, data, level );
        }
}

static int
read_ref( FILE *fp, bibl *bin, char *filename, convert_rules *r, param *p )
{
	newstr reference, line;
	fields *ref;
	char buf[256]="";
	int nrefs = 0, bufpos = 0, fcharset;
	newstr_init( &reference );
	newstr_init( &line );
	while ( r->readf( fp, buf, sizeof(buf), &bufpos, &line, &reference, &fcharset ) ) {
		if ( reference.len==0 ) continue;
		ref = fields_new();
		if ( !ref ) return BIBL_ERR_MEMERR;
		if ( r->processf( ref, reference.data, filename, nrefs+1 )){
			bibl_addref( bin, ref );
		} else {
			fields_free( ref );
			free( ref );
		}
		newstr_empty( &reference );
		if ( fcharset!=CHARSET_UNKNOWN ) {
			/* charset from file takes priority over default, but
			 * not user-specified */
			if ( p->charsetin_src!=BIBL_SRC_USER ) {
				p->charsetin_src = BIBL_SRC_FILE;
				p->charsetin = fcharset;
				if ( fcharset!=CHARSET_UNICODE ) p->utf8in = 0;
			}
		}
	}
	newstr_free( &line );
	newstr_free( &reference );
	return BIBL_OK;
}

static void
bibl_fixcharsets( bibl *b, param *p )
{
	fields *ref;
	long i, j;
	int swap = 0;
	int latexout = p->latexout;
	int latexin  = p->latexin;
	for ( i=0; i<b->nrefs; ++i ) {
		ref = b->ref[i];
		for ( j=0; j<ref->nfields; ++j ) {
			if ( latexin || latexout ) {
				/* don't texify/detexify URL's and the like */
				if ( !strcasecmp( ref->tag[j].data, "DOI" ) ||
				     !strcasecmp( ref->tag[j].data, "URL" ) ||
				     !strcasecmp( ref->tag[j].data, "REFNUM" )){
					latexin  = 0;
					latexout = 0;
					swap = 1;
				}
			}
			newstr_convert( &(ref->data[j]),
				p->charsetin,  latexin,  p->utf8in,  
				p->xmlin,
				p->charsetout, latexout, p->utf8out, 
				p->xmlout );
			if ( swap ) {
				swap = 0;
				latexin  = p->latexin;
				latexout = p->latexout;
			}
		}
	}
}

static int
build_refnum( fields *info, long nrefs )
{
	newstr refnum;
	char *p, num[512];
	int y, a;
	newstr_init( &refnum );
	y = fields_find( info, "YEAR", -1 );
	if ( y==-1 ) y = fields_find( info, "PARTYEAR", -1 );
	a = fields_find( info, "AUTHOR", -1 );
	if ( a==-1 ) a = fields_find( info, "AUTHOR:CORP", -1 );
	if ( a!=-1 && y!=-1 ) {
		p = info->data[a].data;
		while ( p && *p && *p!='|' )
			newstr_addchar( &refnum, *p++ );
		p = info->data[y].data;
		while ( p && *p && *p!=' ' && *p!='\t' )
			newstr_addchar( &refnum, *p++ );
	} else {
		sprintf( num, "%ld", nrefs );
		newstr_strcpy( &refnum, "ref" );
		newstr_strcat( &refnum, num );
	}
	fields_add( info, "REFNUM", refnum.data, 0 );
	newstr_free( &refnum );
	return fields_find( info, "REFNUM", 0 );
}

static void
bibl_checkrefid( bibl *b, param *p )
{
	fields *ref;
	long i;
	char buf[512];
	int n;
	for ( i=0; i<b->nrefs; ++i ) {
		ref = b->ref[i];
		n = fields_find( ref, "REFNUM", 0 );
		if ( n==-1 ) n = build_refnum( ref, i+1 );
		if ( p->addcount ) {
			sprintf( buf, "_%ld", i+1 );
			newstr_strcat( &(ref->data[n]), buf );
		}
	}
}

static int 
convert_ref( bibl *bin, char *fname, bibl *bout, convert_rules *r, param *p )
{
	fields *rin, *rout;
	long i;
	int reftype;
	if ( r->cleanf ) r->cleanf( bin );
	for ( i=0; i<bin->nrefs; ++i ) {
		rin = bin->ref[i];
		rout = fields_new();
		if ( !rout ) return BIBL_ERR_MEMERR;
		if ( r->typef ) 
			reftype = r->typef( rin, fname, i+1, r->all, r->nall );
		else reftype = 0;
		r->convertf( rin, rout, reftype, p->verbose, r->all, r->nall );
		if ( r->all ) process_alwaysadd( rout, reftype, r );
		if ( p->verbose>1 ) 
			bibl_verbose1( rout, rin, fname, i+1 );
		bibl_addref( bout, rout );
	}
	return BIBL_OK;
}

static void
rules_init( convert_rules *r, int mode )
{
	switch ( mode ) {
		case BIBL_BIBTEXIN:
			r->readf    = bibtexin_readf;
			r->processf = bibtexin_processf;
			r->cleanf   = bibtexin_cleanf;
			r->typef    = bibtexin_typef;
			r->convertf = bibtexin_convertf;
			r->all      = bibtex_all;
			r->nall     = bibtex_nall;
			break;
		case BIBL_BIBTEXOUT:
			r->headerf = NULL;
			r->footerf = NULL;
			r->writef  = bibtexout_write;
			break;
		case BIBL_ENDNOTEIN:
			r->readf    = endin_readf;
			r->cleanf   = NULL;
			r->processf = endin_processf;
			r->typef    = endin_typef;
			r->convertf = endin_convertf;
			r->all      = end_all;
			r->nall     = end_nall;
			break;
		case BIBL_ENDNOTEXMLIN:
			r->readf    = endxmlin_readf;
			r->cleanf   = NULL;
			r->processf = endxmlin_processf;
			r->typef    = endin_typef;
			r->convertf = endin_convertf;
			r->all      = end_all;
			r->nall     = end_nall;
			break;
		case BIBL_ENDNOTEOUT:
			r->headerf = NULL;
			r->footerf = NULL;
			r->writef  = endout_write;
			break;
		case BIBL_RISIN:
			r->readf    = risin_readf;
			r->cleanf   = NULL;
			r->processf = risin_processf;
			r->typef    = risin_typef;
			r->convertf = risin_convertf;
			r->all      = ris_all;
			r->nall     = ris_nall;
			break;
		case BIBL_RISOUT:
			r->headerf = NULL;
			r->footerf = NULL;
			r->writef  = risout_write;
			break;
		case BIBL_ISIIN:
			r->readf    = isiin_readf;
			r->cleanf   = NULL;
			r->processf = isiin_processf;
			r->typef    = isiin_typef;
			r->convertf = isiin_convertf;
			r->all      = isi_all;
			r->nall     = isi_nall;
			break;
		case BIBL_ISIOUT:
			r->headerf  = NULL;
			r->footerf  = NULL;
			r->writef   = isi_write;
			break;
		case BIBL_COPACIN:
			r->readf    = copacin_readf;
			r->cleanf   = NULL;
			r->processf = copacin_processf;
			r->typef    = NULL;
			r->convertf = copacin_convertf;
			r->all      = copac_all;
			r->nall     = copac_nall;
			break;
		case BIBL_MEDLINEIN:
			r->readf    = medin_readf;
			r->cleanf   = NULL;
			r->processf = medin_processf;
			r->typef    = NULL;
			r->convertf = NULL;
			r->all      = NULL;
			r->nall     = 0;
			break;
		case BIBL_MODSIN:
			r->readf    = modsin_readf;
			r->cleanf   = NULL;
			r->processf = modsin_processf;
			r->typef    = NULL;
			r->convertf = NULL;
			r->all      = NULL;
			r->nall     = 0;
			break;
		case BIBL_MODSOUT:
			r->headerf = modsout_writeheader;
			r->footerf = modsout_writefooter;
			r->writef  = modsout_write;
			break;
		case BIBL_WORD2007OUT:
			r->headerf = wordout_writeheader;
			r->footerf = wordout_writefooter;
			r->writef  = wordout_write;
	}
}

static void
bibl_setreadparams( param *np, param *op, int mode )
{
	if ( !op ) bibl_initparams( np, mode, 0 );
	else {
		np->latexin = op->latexin;
		np->utf8in = op->utf8in;
		np->xmlin = op->xmlin;
		np->charsetin = op->charsetin;
		np->charsetin_src = op->charsetin_src;
		np->verbose = op->verbose;
		np->format_opts = op->format_opts;
		np->addcount = op->addcount;
		np->output_raw = op->output_raw;
	}
	np->utf8out = 1;
	np->charsetout = BIBL_CHARSET_UNICODE;
	np->charsetout_src = BIBL_SRC_DEFAULT;
	np->xmlout = 0;
	np->latexout = 0;
}

int
bibl_read( bibl *b, FILE *fp, char *filename, int mode, param *p )
{
	convert_rules r;
	param lp;
	bibl bin;
	if ( !b || !fp || bibl_illegalinmode( mode ) ) return BIBL_ERR_BADINPUT;
	bibl_setreadparams( &lp, p, mode );
	bibl_init( &bin );
	rules_init( &r, mode );
	read_ref( fp, &bin, filename, &r, &lp );
	if ( !lp.output_raw || ( lp.output_raw & BIBL_RAW_WITHCHARCONVERT ) )
		bibl_fixcharsets( &bin, &lp );
	if ( !lp.output_raw )
		convert_ref( &bin, filename, b, &r, &lp );
	else {
		if ( p->verbose > 1 ) bibl_verbose0( &bin );
		bibl_copy( b, &bin );
	}
	if ( !lp.output_raw || ( lp.output_raw & BIBL_RAW_WITHMAKEREFID ) )
		bibl_checkrefid( b, &lp );
	bibl_free( &bin );
	return BIBL_OK;
}

static FILE *
singlerefname( fields *reffields, long nref, int mode )
{
	char outfile[2048];
	char suffix[5] = "xml";
	FILE *fp;
	long count;
	int  found;
	if ( mode==BIBL_BIBTEXOUT ) strcpy( suffix, "bib" );
	else if ( mode==BIBL_RISOUT ) strcpy( suffix, "ris" );
	else if ( mode==BIBL_ENDNOTEOUT ) strcpy( suffix, "end" );
	found = fields_find( reffields, "REFNUM", 0 );
	/* find new filename based on reference */
	if ( found!=-1 ) {
		sprintf( outfile,"%s.%s",reffields->data[found].data, suffix );
	} else  sprintf( outfile,"%ld.%s",nref, suffix );
	count = 0;
	fp = fopen( outfile, "r" );
	while ( fp ) {
		fclose(fp);
		count++;
		if ( count==60000 ) return NULL;
		if ( found!=-1 )
			sprintf( outfile, "%s_%ld.%s", 
				reffields->data[found].data, count, suffix  );
		else sprintf( outfile,"%ld_%ld.%s",nref, count, suffix );
		fp = fopen( outfile, "r" );
	}
	fp = fopen( outfile, "w" );
	if ( !fp ) return NULL;
	return fp;
}

static int
output_bibl( FILE *fp, bibl *b, convert_rules *r, param *p, int mode )
{
	long i;
	if ( !p->singlerefperfile && r->headerf )
		r->headerf( fp, p->format_opts );
	for ( i=0; i<b->nrefs; ++i ) {
		if ( p->singlerefperfile ) { 
			fp = singlerefname( b->ref[i], i, mode );
			if ( fp ) {
				if ( r->headerf ) r->headerf(fp,p->format_opts);
			} else return BIBL_ERR_CANTOPEN;
		}
		r->writef( b->ref[i], fp, p->format_opts, i );
		if ( p->singlerefperfile ) {
			if ( r->footerf ) r->footerf( fp );
			fclose( fp );
		}
	}
	if ( !p->singlerefperfile && r->footerf ) 
		r->footerf( fp );
	return 1;
}

static void
bibl_setwriteparams( param *np, param *op, int mode )
{
	if ( !op ) bibl_initparams( np, 0, mode );
	else {
		np->utf8out = op->utf8out;
		np->charsetout = op->charsetout;
		np->charsetout_src = op->charsetout_src;
		np->latexout = op->latexout;
		np->xmlout = op->xmlout;
		np->verbose = op->verbose;
		np->format_opts = op->format_opts;
		np->addcount = op->addcount;
		np->output_raw = op->output_raw;
		np->singlerefperfile = op->singlerefperfile;
	}
	np->xmlin = 0;
	np->latexin = 0;
	np->utf8in = 1;
	np->charsetin = BIBL_CHARSET_UNICODE;
	np->charsetin_src = BIBL_SRC_DEFAULT;
}

int
bibl_write( bibl *b, FILE *fp, int mode, param *p )
{
	convert_rules r;
	param lp;
	if ( !b || bibl_illegaloutmode( mode ) ) return BIBL_ERR_BADINPUT;
	if ( !fp && ( !p || !p->singlerefperfile ) ) return BIBL_ERR_BADINPUT;
	rules_init( &r, mode );
	bibl_setwriteparams( &lp, p, mode );
	bibl_fixcharsets( b, &lp );
	output_bibl( fp, b, &r, &lp, mode );
	return BIBL_OK;
}

