/*
 * is_ws.c
 *
 * Copyright (c) Chris Putnam 2003-7
 *
 * Source code released under the GPL
 *
 */
#include "is_ws.h"

/* is_ws(), is whitespace */
int 
is_ws( char ch )
{
	if (ch==' ' || ch=='\n' || ch=='\t' || ch=='\r' ) return 1;
	else return 0;
}

