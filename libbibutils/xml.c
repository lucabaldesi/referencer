/*
 * xml.c
 *
 * Copyright (c) Chris Putnam 2004-5
 *
 * Source code released under the GPL
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "is_ws.h"
#include "strsearch.h"
#include "newstr.h"
#include "xml.h"

#define TRUE (1)
#define FALSE (0)

static xml_attrib *
xmlattrib_new( void )
{
	xml_attrib *a = (xml_attrib *) malloc( sizeof( xml_attrib ) );
	if ( a ) {
		lists_init( &(a->attrib) );
		lists_init( &(a->value) );
	}
	return a;
}

static void
xmlattrib_add( xml_attrib *a, char *attrib, char *value  )
{
	lists_add( &(a->attrib), attrib );
	lists_add( &(a->value), value );
}

static void
xmlattrib_free( xml_attrib *a )
{
	lists_free( &(a->attrib) );
	lists_free( &(a->value ) );
}

static xml *
xml_new( void )
{
	xml *x = ( xml * ) malloc( sizeof( xml ) );
	if ( x ) xml_init( x );
	return x;
}

void
xml_free( xml *x )
{
	if ( x->tag ) newstr_free( x->tag );
	if ( x->value ) newstr_free( x->value );
	if ( x->a ) xmlattrib_free( x->a );
	if ( x->down ) xml_free( x->down );
	if ( x->next ) xml_free( x->next );
}

void
xml_init( xml *x )
{
	x->tag = newstr_new();
	x->value = newstr_new();
	x->a = NULL;
	x->down = NULL;
	x->next = NULL;
	if ( !(x->tag) || !(x->value) ) {
		fprintf(stderr,"xml_init: memory error.\n");
		exit( EXIT_FAILURE );
	}
#ifdef COUNT_TRAVERSAL
	x->count = 0;
#endif
}

enum {
	XML_DESCRIPTOR,
	XML_COMMENT,
	XML_OPEN,
	XML_CLOSE,
	XML_OPENCLOSE
};

static int
xml_terminator( char *p, int *type )
{
	if ( *p=='>' ) {
		return 1;
	} else if ( *p=='/' && *(p+1)=='>' ) {
		if ( *type==XML_OPENCLOSE ) return 1;
		else if ( *type==XML_OPEN ) {
			*type = XML_OPENCLOSE;
			return 1;
		}
	} else if ( *p=='?' && *(p+1)=='>' && *type==XML_DESCRIPTOR ) {
		return 1;
	} else if ( *p=='!' && *(p+1)=='>' && *type==XML_COMMENT ) {
		return 1;
	}
	return 0;
}

static char *
xml_processattrib( char *p, xml_attrib **ap, int *type )
{
	xml_attrib *a = NULL;
	char quote_character = '\"';
	int inquotes = 0;
	newstr aname, aval;
	newstr_init( &aname );
	newstr_init( &aval );
	while ( *p && !xml_terminator(p,type) ) {
		/* get attribute name */
		while ( *p==' ' || *p=='\t' ) p++;
		while ( *p && !strchr( "= \t", *p ) && !xml_terminator(p,type)){
			newstr_addchar( &aname, *p );
			p++;
		}
		while ( *p==' ' || *p=='\t' ) p++;
		if ( *p=='=' ) p++;
		/* get attribute value */
		while ( *p==' ' || *p=='\t' ) p++;
		if ( *p=='\"' || *p=='\'' ) {
			if ( *p=='\'' ) quote_character = *p;
			inquotes=1;
			p++;
		}
		while ( *p && ((!xml_terminator(p,type) && !strchr("= \t", *p ))||inquotes)){
			if ( *p==quote_character ) inquotes=0;
			else newstr_addchar( &aval, *p );
			p++;
		}
		if ( aname.len ) {
			if ( !a ) a = xmlattrib_new();
			xmlattrib_add( a, aname.data, aval.data );
		}
		newstr_empty( &aname );
		newstr_empty( &aval );
	}
	newstr_free( &aname );
	newstr_free( &aval );
	*ap = a;
	return p;
}

/*
 * xml_processtag
 *
 *      XML_COMMENT   <!-- ....  -->
 * 	XML_DESCRIPTOR   <?.....>
 * 	XML_OPEN      <A>
 * 	XML_CLOSE     </A>
 * 	XML_OPENCLOSE <A/>
 */
static char *
xml_processtag( char *p, newstr *tag, xml_attrib **attrib, int *type )
{
	*attrib = NULL;
	if ( *p=='<' ) p++;
	if ( *p=='!' ) {
		while ( *p && *p!='>' ) newstr_addchar( tag, *p++ );
		*type = XML_COMMENT;
	} else if ( *p=='?' ) {
		*type = XML_DESCRIPTOR;
		p++; /* skip '?' */
		while ( *p && !strchr( " \t", *p ) && !xml_terminator(p,type) )
			newstr_addchar( tag, *p++ );
		if ( *p==' ' || *p=='\t' )
			p = xml_processattrib( p, attrib, type );
	} else if ( *p=='/' ) {
		while ( *p && !strchr( " \t", *p ) && !xml_terminator(p,type) )
			newstr_addchar( tag, *p++ );
		*type = XML_CLOSE;
		if ( *p==' ' || *p=='\t' ) 
			p = xml_processattrib( p, attrib, type );
	} else {
		*type = XML_OPEN;
		while ( *p && !strchr( " \t", *p ) && !xml_terminator(p,type) )
			newstr_addchar( tag, *p++ );
		if ( *p==' ' || *p=='\t' ) 
			p = xml_processattrib( p, attrib, type );
	}
	while ( *p && *p!='>' ) p++;
	if ( *p=='>' ) p++;
	return p;
}

static void
xml_appendnode( xml *onode, xml *nnode )
{
	if ( !onode->down ) onode->down = nnode;
	else {
		xml *p = onode->down;
		while ( p->next ) p = p->next;
		p->next = nnode;
	}
}

char *
xml_tree( char *p, xml *onode )
{
	newstr tag;
	xml_attrib *attrib;
	int type, is_style = 0;

	newstr_init( &tag );

	while ( *p ) {
		/* retain white space for <style> tags in endnote xml */
		if ( onode->tag && onode->tag->data && 
			!strcasecmp(onode->tag->data,"style") ) is_style=1;
		while ( *p && *p!='<' ) {
			if ( onode->value->len>0 || is_style || !is_ws( *p ) )
				newstr_addchar( onode->value, *p );
			p++;
		}
		if ( *p=='<' ) {
			newstr_empty( &tag );
			p = xml_processtag( p, &tag, &attrib, &type );
			if ( type==XML_OPEN || type==XML_OPENCLOSE ||
			     type==XML_DESCRIPTOR ) {
				xml *nnode = xml_new();
				newstr_newstrcpy( nnode->tag, &tag );
				nnode->a = attrib;
				xml_appendnode( onode, nnode );
				if ( type==XML_OPEN )
					p = xml_tree( p, nnode );
			} else if ( type==XML_CLOSE ) {
				/*check to see if it's closing for this one*/
				return p; /* assume it's right for now*/
			}
		}
	}
	newstr_free( &tag );
	return p;
}

void
xml_draw( xml *x, int n )
{
	int i,j;
	if ( !x ) return;
	for ( i=0; i<n; ++i ) printf( "    " );
	printf("n=%d tag='%s' value='%s'\n", n, x->tag->data, x->value->data );
	if ( x->a ) {
		for ( j=0; j<x->a->value.n; ++j ) {
			for ( i=0; i<n; ++i ) printf( "    " );
			printf("    attrib='%s' value='%s'\n",
				(x->a)->attrib.str[j].data,
				(x->a)->value.str[j].data );
		}
	}
	if ( x->down ) xml_draw( x->down, n+1 );
	if ( x->next ) xml_draw( x->next, n );
}


#define XML_BUFSIZE (512)

char *
xml_findstart( char *buffer, char *tag )
{
	char starttag[XML_BUFSIZE], *startptr, *p = NULL;
	int length = strlen( tag );
	if ( length < XML_BUFSIZE-4 ) {
		sprintf( starttag, "<%s>", tag );
		p = strsearch( buffer, starttag );
		if ( !p ) {
			sprintf( starttag, "<%s ",tag );
			p = strsearch( buffer, starttag );
		}
	} else {
		startptr = (char *) malloc( sizeof(char) * (length+4) );
		if ( startptr ) {
			sprintf( startptr, "<%s", tag );
			p = strsearch( buffer, startptr );
			free( startptr );
		}
	}
	return p;
}

char *
xml_findend( char *buffer, char *tag )
{
	char endtag[XML_BUFSIZE], *p = NULL;
	int length = strlen( tag );
	if ( length<XML_BUFSIZE-4 ) {
		sprintf( endtag, "</%s>", tag );
		p = strsearch( buffer, endtag );
	} else {
		char *endptr = (char *) malloc( sizeof(char) * (length+5) );
		if ( endptr ) {
			sprintf( endptr, "</%s>", tag );
			p = strsearch( buffer, endptr );
			free( endptr );
		}
	}
	if ( p && *p ) {
		if ( *p ) p++;  /* skip <random_tag></end> combo */
		while ( *p && *(p-1)!='>' ) p++;
	}
	return p;
}

int
xml_tagexact( xml *node, char *s )
{
	unsigned int slen = strlen( s );

/*#ifdef COUNT_TRAVERSAL
	node->count++;
	fprintf( stderr, "xml_tagexact checking node tag='%s' value='%s' %d for tag='%s'\n",node->tag->data,node->value->data,node->count,s);
#endif*/
	if ( node->tag->len==slen && !strcasecmp( node->tag->data, s ) ) {
/*#ifdef COUNT_TRAVERSAL
		node->count++;
#endif*/
		return 1;
	}
	return 0;
}

int
xml_tag_attrib( xml *node, char *s, char *attrib, char *value )
{
	xml_attrib *na = node->a;
	int i, nattrib = 0;
	if ( !na ) return 0;
	else nattrib = na->attrib.n;

#ifdef COUNT_TRAVERSAL
	node->count++;
	fprintf( stderr, "xml_tag_attrib checking node tag='%s' value='%s' %d for attrib='%s' value='%s'\n",node->tag->data,node->value->data,node->count,attrib,value);
#endif
	if ( node->tag->len!=strlen(s) || strcasecmp( node->tag->data, s ) )
		return 0;
	for ( i=0; i<nattrib; ++i ) {
		if ( !na->attrib.str[i].data || !na->value.str[i].data )
			continue;
		if ( !strcasecmp( na->attrib.str[i].data, attrib ) &&
		     !strcasecmp( na->value.str[i].data, value ) )
			return 1;
	}
	return 0;
}

newstr *
xml_getattrib( xml *node, char *attrib )
{
	newstr *ns = NULL;
	xml_attrib *na = node->a;
	int i, nattrib;
	if ( na ) {
		nattrib = na->attrib.n;
		for ( i=0; i<nattrib; ++i )
			if ( !strcasecmp( na->attrib.str[i].data, attrib ) )
				ns = &(na->value.str[i]);
	}
	return ns;
}

#ifdef COUNT_TRAVERSAL
void
xml_reporttraversal( xml *node, int depth )
{
	int i;
	for ( i=0; i<depth; ++i ) fprintf(stderr,"  ");
	fprintf(stderr,"node: " );
	if ( node->tag && node->tag->data ) 
		fprintf(stderr,"tag='%s' ",node->tag->data );
	if ( node->value && node->value->data ) 
		fprintf(stderr,"value='%s' ", node->value->data);
	fprintf(stderr,"%d\n", node->count);
	if ( node->down ) xml_reporttraversal( node->down, depth+1 );
	if ( node->next ) xml_reporttraversal( node->next, depth );
}
#endif
