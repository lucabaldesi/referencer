
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef BIBUTILS_WRAPPER_H
#define BIBUTILS_WRAPPER_H

#include <gtkmm.h>

#include "Document.h"

extern "C" {
namespace BibUtils {
#include "libbibutils/bibutils.h"

typedef enum {
	FORMAT_BIBTEX = BIBL_BIBTEXIN,
	FORMAT_ENDNOTE = BIBL_ENDNOTEIN,
	FORMAT_RIS = BIBL_RISIN,
	FORMAT_MODS = BIBL_MODSIN,
	FORMAT_UNKNOWN = -1
} Format;

std::string formatType (fields *info);
int getType (fields *info);

std::string formatPeople(fields *info, char *tag, char *ctag, int level);
std::string formatPerson (std::string const &munged);

Document parseBibUtils (BibUtils::fields *ref);

Format guessFormat (std::string const &rawtext);

void biblFromString (
	bibl &b,
	std::string const &rawtext,
	Format format,
	param &p);

}
}

#endif
