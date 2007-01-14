
#include "Utility.h"

namespace Utility {

bool DOIURLValid (
	Glib::ustring const &url)
{
	return url.find ("<!DOI!>") != Glib::ustring::npos;
}


StringPair twoWaySplit (
	Glib::ustring const &str,
	Glib::ustring const &divider)
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


Glib::ustring strip (
	Glib::ustring const &victim,
	Glib::ustring const &unwanted)
{
	Glib::ustring stripped = victim;
	while (stripped.find (unwanted) != Glib::ustring::npos) {
		int pos = stripped.find (unwanted);
		stripped = stripped.substr (0, pos)
			+ stripped.substr (pos + unwanted.length (), stripped.length ());
	}

	return stripped;
}


Glib::ustring ensureExtension (
	Glib::ustring const &filename,
	Glib::ustring const &extension)
{
	if (filename.find (".") != Glib::ustring::npos)
		return filename;
	else {
		return filename + "." + extension;
	}
}

}
