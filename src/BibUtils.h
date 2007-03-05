
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

Glib::ustring formatType (fields *info);
int getType (fields *info);

Glib::ustring formatPeople(fields *info, char *tag, char *ctag, int level);
Glib::ustring formatPerson (Glib::ustring const &munged);

Document parseBibUtils (BibUtils::fields *ref);

Format guessFormat (Glib::ustring const &rawtext);

void biblFromString (
	bibl &b,
	Glib::ustring const &rawtext,
	Format format,
	param &p);

}
}

#endif
