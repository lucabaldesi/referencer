
#include "Utility.h"

namespace Utility {

bool DOIURLValid (Glib::ustring const &url) {
	return url.find ("<!DOI!>") != Glib::ustring::npos;
}

StringPair twoWaySplit (Glib::ustring const &str, Glib::ustring const &divider)
{
	StringPair retval;
	int pos = str.find (divider);
	if (pos == Glib::ustring::npos) {
		retval.first = str;
	} else {
		retval.first = str.substr (0, pos);	
		retval.second = str.substr (pos + divider.length(), str.length ());
	}

	return retval;
}

}
