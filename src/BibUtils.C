
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#include <iostream>

#include "Utility.h"

#include "BibUtils.h"

extern "C" {

namespace BibUtils {

char progname[] = "referencer";

lists asis  = { 0, 0, NULL };
lists corps = { 0, 0, NULL };

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


enum {
	TYPE_UNKNOWN = 0,
	TYPE_ARTICLE,
	TYPE_INBOOK,
	TYPE_INPROCEEDINGS,
	TYPE_PROCEEDINGS,
	TYPE_INCOLLECTION,
	TYPE_COLLECTION,
	TYPE_BOOK,
	TYPE_PHDTHESIS,
	TYPE_MASTERSTHESIS,
	TYPE_REPORT,
	TYPE_MANUAL,
	TYPE_UNPUBLISHED,
	TYPE_MISC
};


static int
bibtexout_type( fields *info)
{
	char *genre;
	int type = TYPE_UNKNOWN, i, maxlevel, level;

	/* determine bibliography type */
	for ( i=0; i<info->nfields; ++i ) {
		if ( strcasecmp( info->tag[i].data, "GENRE" ) &&
		     strcasecmp( info->tag[i].data, "NGENRE" ) ) continue;
		genre = info->data[i].data;
		level = info->level[i];
		if ( !strcasecmp( genre, "periodical" ) ||
		     !strcasecmp( genre, "academic journal" ) ||
		     !strcasecmp( genre, "magazine" ) )
			type = TYPE_ARTICLE;
		else if ( !strcasecmp( genre, "instruction" ) )
			type = TYPE_MANUAL;
		else if ( !strcasecmp( genre, "unpublished" ) )
			type = TYPE_UNPUBLISHED;
		else if ( !strcasecmp( genre, "conference publication" ) ) {
			if ( level==0 ) type=TYPE_PROCEEDINGS;
			else type = TYPE_INPROCEEDINGS;
		} else if ( !strcasecmp( genre, "collection" ) ) {
			if ( level==0 ) type=TYPE_COLLECTION;
			else type = TYPE_INCOLLECTION;
		} else if ( !strcasecmp( genre, "report" ) )
			type = TYPE_REPORT;
		else if ( !strcasecmp( genre, "book" ) ) {
			if ( level==0 ) type=TYPE_BOOK;
			else type=TYPE_INBOOK;
		} else if ( !strcasecmp( genre, "theses" ) ) {
			if ( type==TYPE_UNKNOWN ) type=TYPE_PHDTHESIS;
		} else if ( !strcasecmp( genre, "Ph.D. thesis" ) )
			type = TYPE_PHDTHESIS;
		else if ( !strcasecmp( genre, "Masters thesis" ) )
			type = TYPE_MASTERSTHESIS;
	}
	if ( type==TYPE_UNKNOWN ) {
		for ( i=0; i<info->nfields; ++i ) {
			if ( strcasecmp( info->tag[i].data, "ISSUANCE" ) ) continue;
			if ( !strcasecmp( info->data[i].data, "monographic" ) ) {
				if ( info->level[i]==0 ) type = TYPE_BOOK;
				else if ( info->level[i]==1 ) type=TYPE_INBOOK;
			}
		}
	}

	/* default to BOOK type */
	if ( type==TYPE_UNKNOWN ) {
		maxlevel = fields_maxlevel( info );
		if ( maxlevel > 0 ) type = TYPE_INBOOK;
		else {
			fprintf( stderr, "xml2bib: cannot identify TYPE"
				" in reference");
			type = TYPE_MISC;
		}
	}
	return type;
}

typedef struct {
	int bib_type;
	char *type_name;
} typenames;


int getType (fields *info)
{
	return bibtexout_type (info);
}


std::string formatType (fields *info)
{
	int const type = bibtexout_type (info);

	typenames types[] = {
		{ TYPE_ARTICLE, (char*)"Article" },
		{ TYPE_INBOOK, (char*)"Inbook" },
		{ TYPE_PROCEEDINGS, (char*)"Proceedings" },
		{ TYPE_INPROCEEDINGS, (char*)"InProceedings" },
		{ TYPE_BOOK, (char*)"Book" },
		{ TYPE_PHDTHESIS, (char*)"PhdThesis" },
		{ TYPE_MASTERSTHESIS, (char*)"MastersThesis" },
		{ TYPE_REPORT, (char*)"TechReport" },
		{ TYPE_MANUAL, (char*)"Manual" },
		{ TYPE_COLLECTION, (char*)"Collection" },
		{ TYPE_INCOLLECTION, (char*)"InCollection" },
		{ TYPE_UNPUBLISHED, (char*)"Unpublished" },
		{ TYPE_MISC, (char*)"Misc" } };

	int i, ntypes = sizeof( types ) / sizeof( types[0] );
	char *s = NULL;
	for ( i=0; i<ntypes; ++i ) {
		if ( types[i].bib_type == type ) {
			s = types[i].type_name;
			break;
		}
	}
	if ( !s ) s = types[ntypes-1].type_name; /* default to TYPE_MISC */

	return std::string (s);
}


std::string formatTitle (BibUtils::fields *ref, int level)
{
	int kmain;
	int ksub;

	kmain = fields_find (ref, (char*)"TITLE", level);
	ksub = fields_find (ref, (char*)"SUBTITLE", level);

	std::string title;

	if (kmain >= 0) {
		title = ref->data[kmain].data;
		ref->used[kmain] = 1;
		if (ksub >= 0) {
			title += ": ";
			title += std::string (ref->data[ksub].data);
			ref->used[ksub] = 1;
		}
	}

	return title;
}


std::string formatPerson (std::string const &munged)
{
	std::string output;
	int nseps = 0, nch;
	char *p = (char *) munged.c_str ();
	while ( *p ) {
		nch = 0;
		if ( nseps ) output = output + " ";
		while ( *p && *p!='|' ) {
			output = output + *p++;
			nch++;
		}
		if ( *p=='|' ) p++;
		if ( nseps==0 ) output = output + ",";
		else if ( nch==1 ) output = output + ".";
		nseps++;
	}

	return output;
}


std::string formatPeople(fields *info, char *tag, char *ctag, int level)
{
	int i, npeople, person, corp;

	std::string output;

	/* primary citation authors */
	npeople = 0;
	for ( i=0; i<info->nfields; ++i ) {
		if ( level!=-1 && info->level[i]!=level ) continue;
		person = ( strcasecmp( info->tag[i].data, tag ) == 0 );
		corp   = ( strcasecmp( info->tag[i].data, ctag ) == 0 );
		if ( person || corp ) {
			if (npeople > 0)
				output += " and ";

			if (corp)
				output += std::string (info->data[i].data);
			else
				output += formatPerson (info->data[i].data);

			npeople++;
		}
	}

	return output;
}

/**
	IMPORTANT
	=========
	
	All the newdoc.getBibData.().setFoo have Glib::ustring 
	arguments.  That means that the std::strings we're using 
	have to be in utf8: there is NO conversion, not even from 
	the current locale.
	
	Current (3.32) version of bibutils seems to be using utf8 
	internally even when importing a file from latin1.
*/

Document parseBibUtils (BibUtils::fields *ref)
{
	std::pair<std::string,std::string> a[]={
		std::make_pair("PARTDAY", "Day"),
		std::make_pair("PARTMONTH", "Month"),
		std::make_pair("KEYWORD", "Keywords"),
		std::make_pair("DEGREEGRANTOR", "School"),
		std::make_pair("DEGREEGRANTOR:ASIS", "School"),
		std::make_pair("DEGREEGRANTOR:CORP", "School"),
		std::make_pair("NOTES", "Note")
	};

	std::map<std::string,std::string> replacements (
		a,a + (sizeof(a) / sizeof(*a)));

	Document newdoc;

	int type = 	BibUtils::getType (ref);
	newdoc.getBibData().setType (formatType (ref));

	if (type == TYPE_INBOOK) {
		newdoc.getBibData().addExtra ("Chapter", formatTitle (ref, 0));
	} else {
		newdoc.getBibData().setTitle (formatTitle (ref, 0));
	}

	if ( type==TYPE_ARTICLE )
		newdoc.getBibData().setJournal (formatTitle (ref, 1));
	else if ( type==TYPE_INBOOK )
		newdoc.getBibData().setTitle (formatTitle (ref, 1));
	else if ( type==TYPE_INPROCEEDINGS || type==TYPE_INCOLLECTION )
		newdoc.getBibData().addExtra ("BookTitle", formatTitle (ref, 1));
	else if ( type==TYPE_BOOK || type==TYPE_COLLECTION || type==TYPE_PROCEEDINGS )
		newdoc.getBibData().addExtra ("Series", formatTitle (ref, 1));

	std::string authors = formatPeople (ref, (char*)"AUTHOR", (char*)"CORPAUTHOR", 0);
	std::string editors = formatPeople (ref, (char*)"EDITOR", (char*)"CORPEDITOR", -1);
	std::string translators = formatPeople (ref, (char*)"TRANSLATOR", (char*)"CORPTRANSLATOR", -1);
	newdoc.getBibData().setAuthors (authors);
	if (!editors.empty ()) {
		newdoc.getBibData().addExtra ("Editor", editors);
	}
	if (!translators.empty ()) {
		newdoc.getBibData().addExtra ("Translator", translators);
	}

	for (int j = 0; j < ref->nfields; ++j) {
		std::string key = ref->tag[j].data;
		std::string value = ref->data[j].data;

		int used = 1;
		if (key == "REFNUM") {
			newdoc.setKey (value);
		} else if (key == "VOLUME") {
			newdoc.getBibData().setVolume (value);
		} else if (key == "NUMBER" || key == "ISSUE") {
			newdoc.getBibData().setIssue (value);
		} else if (key == "YEAR" || key == "PARTYEAR") {
			newdoc.getBibData().setYear (value);
		} else if (key == "PAGESTART") {
			newdoc.getBibData().setPages (value + newdoc.getBibData().getPages ());
		} else if (key == "PAGEEND") {
			newdoc.getBibData().setPages (newdoc.getBibData().getPages () + "-" + value);
		} else if (key == "ARTICLENUMBER") {
			/* bibtex normally avoid article number, so output as page */
			newdoc.getBibData().setPages (value);
		} else if (key == "RESOURCE" || key == "ISSUANCE" || key == "GENRE"
		        || key == "AUTHOR" || key == "EDITOR" || key == "CORPAUTHOR"
		        || key == "CORPEDITOR" || key == "TYPE") {
			// Don't add them as "extra fields"
		} else {
			used = 0;
		}
		if (used)
			ref->used[j] = 1;

		if (!ref->used[j]) {
			if (key == "TITLE") {
				if (type == TYPE_INCOLLECTION) {
					// Special case: Chapters in InCollection get added as "Title" level 0
					key = "Chapter";
				} else if (type == TYPE_INPROCEEDINGS) {
					// Special case: Series in InProceedings get added as "Title" level 0
					key = "Series";
				} else {
					DEBUG ("unexpected TITLE element %1:%2 (%3)",
						key, value, ref->level[j]);
					// Don't overwrite existing title field
					if (!newdoc.getBibData().getTitle().empty()) {
						continue;
					}
				}
			}

			if (!replacements[key].empty()) {
				key = replacements[key];
			} else {
				key = Utility::firstCap (key);
			}

			int level = ref->level[j];
			if (!value.empty ()) {
				newdoc.getBibData().addExtra (key, value);
			}
		}
	}


	return newdoc;
}


Format guessFormat (std::string const &rawtext)
{
	return (Format) BIBL_BIBTEXIN;
}

static void writerThread (std::string const &raw, int pipe, volatile bool *advance)
{
	int len = strlen (raw.c_str());
	if (len <= 0) {
		*advance = true;
		close (pipe);
		return;
	}

	// Writing more than 65536 freezes in write()
	int block = 1024;
	if (block > len)
		block = len;

	for (int i = 0; i < len / block; ++i) {
		write (pipe, raw.c_str() + i * block, block);
		*advance = true;
	}
	if (len % block > 0) {
		write (pipe, raw.c_str() + (len / block) * block, len % block);
	}

	close (pipe);
}


void biblFromString (
	bibl &b,
	std::string const &rawtext,
	Format format,
	param &p
	)
{
	int handles[2];
	if (pipe(handles)) {
		throw Glib::IOChannelError (
			Glib::IOChannelError::BROKEN_PIPE,
			"Couldn't get pipe in biblFromString");
	}
	int pipeout = handles[0];
	int pipein = handles[1];

	volatile bool advance = false;

	Glib::Thread *writer = Glib::Thread::create (
		sigc::bind (sigc::ptr_fun (&writerThread), rawtext, pipein, &advance), true);

	while (!advance) {}

	FILE *otherend = fdopen (pipeout, "r");
	BibUtils::bibl_read(&b, otherend, "My Pipe", format, &p );
	fclose (otherend);
	close (pipeout);

	writer->join ();
}

}

}
