
#ifndef BIBUTILS_WRAPPER_H
#define BIBUTILS_WRAPPER_H

#include <gtkmm.h>

#include "Document.h"

extern "C" {
namespace BibUtils {
#include "libbibutils/bibutils.h"

Glib::ustring getType (fields *info);

Glib::ustring formatPerson (Glib::ustring const &munged);

Document parseBibUtils (BibUtils::fields *ref);

}
}

#endif
