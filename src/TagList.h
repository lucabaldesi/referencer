
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
#include <vector>
#include <sstream>
#include <gtkmm.h>


class Tag {
	public:
	typedef enum {ATTACH = 0, SEARCH_ALL, SEARCH_AUTHOR,
		SEARCH_ABSTRACT, SEARCH_TITLE} Action;
	Tag (int const uid, std::string const name, Tag::Action const action);
	int uid_;
	std::string name_;

	Action action_;
};

class TagList {
	int uidCounter_;
	private:
	std::vector<Tag> tags_;

	public:
	TagList() {
		uidCounter_ = 0;
	}
	void print ();
	std::vector<Tag>& getTags ();
	int newTag (std::string const name, Tag::Action const action);
	void loadTag (std::string const name, Tag::Action const action, int uid);
	void renameTag (int uid, Glib::ustring newname);
	void deleteTag (int uid);
	void writeXML (std::ostringstream& out);
	void clear () {tags_.clear (); uidCounter_ = 0;}

};

#endif
