
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#include <iostream>
#include <sstream>

#include "TagList.h"
#include "Utility.h"

Tag::Tag (int const uid, std::string const name)
{
	uid_ = uid;
	name_ = name;
}

TagList::TagMap& TagList::getTags ()
{
	return tags_;
}

int TagList::newTag (std::string const name)
{
	Tag newtag (uidCounter_, name);
	tags_[newtag.uid_] = newtag;
	return uidCounter_++;
}


void TagList::loadTag (std::string const name, int uid)
{
	Tag newtag (uid, name);
	tags_[uid] = newtag;
	if (uid >= uidCounter_)
		uidCounter_ = uid + 1;
}


void TagList::renameTag (int uid, Glib::ustring newname)
{
	tags_[uid].name_ = newname;
}


void TagList::print ()
{
	TagMap::iterator it = tags_.begin();
	TagMap::iterator const end = tags_.end();
	for (; it != end; it++) {
		DEBUG((*it).second.uid_ + " " + (*it).second.name_);
	}
}


void TagList::deleteTag (int uid)
{
	TagMap::iterator found = tags_.find (uid);
	if (found == tags_.end()) {
		DEBUG ("TagList::deleteTag: tried to delete non-existent "
			"tag %1", uid);
	} else {
		tags_.erase (tags_.find (uid));
	}
}


Glib::ustring TagList::getName (int const &uid)
{
	return tags_[uid].name_;
}


using Glib::Markup::escape_text;


void TagList::writeXML (Glib::ustring &out)
{
	out += "<taglist>\n";
	TagMap::iterator it = tags_.begin();
	TagMap::iterator const end = tags_.end();

	for (; it != end; it++) {
		std::ostringstream num;
		num << (*it).second.uid_;

		out += "  <tag>\n";
		out += "    <uid>" + num.str() + "</uid>\n";
		out += "    <name>" + escape_text((*it).second.name_) + "</name>\n";
		out += "  </tag>\n";
	}
	out += "</taglist>\n";
}

bool TagList::tagExists (std::string const name)
{
	TagMap::iterator it = tags_.begin();
	TagMap::iterator const end = tags_.end();
	for (; it != end; it++) {
		if (it->second.name_ == name)
			return true;
	}

	return false;
}
