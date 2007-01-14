

#ifndef UTILITY_H
#define UTILITY_H

#include <gtkmm.h>
#include <utility>

namespace Utility {
	typedef std::pair <Glib::ustring, Glib::ustring> StringPair;

	bool DOIURLValid (Glib::ustring const &url);
	StringPair twoWaySplit (Glib::ustring const &str, Glib::ustring const &divider);
	Glib::ustring strip (Glib::ustring const &victim, Glib::ustring const &unwanted);
}

#endif
