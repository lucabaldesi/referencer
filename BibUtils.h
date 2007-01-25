
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
	FORMAT_UNKNOWN
} Format;

Glib::ustring formatType (fields *info);
int getType (fields *info);

Glib::ustring formatPeople(fields *info, char *tag, char *ctag, int level);
Glib::ustring formatPerson (Glib::ustring const &munged);

Document parseBibUtils (BibUtils::fields *ref);

Format guessFormat (Glib::ustring const &rawtext);

}
}

#endif
