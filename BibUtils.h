
#ifndef BIBUTILS_WRAPPER_H
#define BIBUTILS_WRAPPER_H

#include <gtkmm.h>

#include "Document.h"

extern "C" {
namespace BibUtils {
#include "libbibutils/bibutils.h"

Glib::ustring formatType (fields *info);
int getType (fields *info);

Glib::ustring formatPeople(fields *info, char *tag, char *ctag, int level);
Glib::ustring formatPerson (Glib::ustring const &munged);

Document parseBibUtils (BibUtils::fields *ref);

}
}

#endif
