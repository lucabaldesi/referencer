
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef LIBRARY_H
#define LIBRARY_H

#include <glibmm/ustring.h>

class DocumentList;
class TagList;

class Library {
	public:
	DocumentList *doclist_;
	TagList *taglist_;

	Library ();
	~Library ();

	void clear ();
	bool load (Glib::ustring const &libfilename);
	bool save (Glib::ustring const &libfilename);

	Glib::ustring writeXML ();
	bool readXML (Glib::ustring XML);

	void writeBibtex (
		Glib::ustring const &bibfilename,
		std::vector<Document*> const &docs,
		bool const usebraces);
};

#endif
