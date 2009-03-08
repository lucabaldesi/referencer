
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef TAGLIST_H
#define TAGLIST_H

#include <string>
#include <map>
#include <sstream>
#include <gtkmm.h>


class Tag {
	public:
	Tag (int const uid, std::string const name);
	Tag () {};
	int uid_;
	std::string name_;
};

class TagList {
	public:
	typedef std::map<int, Tag> TagMap;
	TagList() {
		uidCounter_ = 0;
	}
	void print ();
	TagMap& getTags ();
	int newTag (std::string const name);
	void loadTag (std::string const name, int uid);
	void renameTag (int uid, Glib::ustring newname);
	void deleteTag (int uid);
	Glib::ustring getName (int const &uid);
	void writeXML (Glib::ustring &out);
	void clear () {tags_.clear (); uidCounter_ = 0;}
	bool tagExists (std::string const name);

	private:
	TagMap tags_;
	int uidCounter_;
};

#endif
