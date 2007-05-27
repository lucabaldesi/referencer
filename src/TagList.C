
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

Tag::Tag (int const uid, std::string const name, Tag::Action const action)
{
	uid_ = uid;
	name_ = name;
	action_ = action;
}

std::vector<Tag>& TagList::getTags ()
{
	return tags_;
}

int TagList::newTag (std::string const name, Tag::Action const action)
{
	Tag newtag (uidCounter_, name, action);
	tags_.push_back(newtag);
	return uidCounter_++;
}


void TagList::loadTag (std::string const name, Tag::Action const action, int uid)
{
	Tag newtag (uid, name, action);
	tags_.push_back(newtag);
	if (uid >= uidCounter_)
		uidCounter_ = uid + 1;
}


void TagList::renameTag (int uid, Glib::ustring newname)
{
	std::vector<Tag>::iterator it = tags_.begin();
	std::vector<Tag>::iterator const end = tags_.end();
	for (; it != end; it++) {
		if ((*it).uid_ == uid) {
			(*it).name_ = newname;
			return;
		}
	}

	std::cerr << "Warning: TagList::renameTag: uid " << uid << "not found\n";
}


void TagList::print ()
{
	std::vector<Tag>::iterator it = tags_.begin();
	std::vector<Tag>::iterator const end = tags_.end();
	for (; it != end; it++) {
		std::cerr << (*it).uid_ << " ";
		std::cerr << (*it).name_ << " ";
		std::cerr << (*it).action_ << " ";
		std::cerr << std::endl;
	}
}


void TagList::deleteTag (int uid)
{
	std::vector<Tag>::iterator it = tags_.begin();
	std::vector<Tag>::iterator const end = tags_.end();
	for (; it != end; it++) {
		if ((*it).uid_ == uid) {
			tags_.erase(it);
			return;
		}
	}

	std::cerr << "Warning:: TagList::deleteTag: tag uid "
		<< uid << "not found\n";
}


Glib::ustring TagList::getName (int const &uid)
{
	std::vector<Tag>::iterator it = tags_.begin();
	std::vector<Tag>::iterator const end = tags_.end();
	for (; it != end; it++) {
		if ((*it).uid_ == uid) {
			return (*it).name_;
		}
	}
	return Glib::ustring();
}


using Glib::Markup::escape_text;


void TagList::writeXML (std::ostringstream& out)
{
	out << "<taglist>\n";
	std::vector<Tag>::iterator it = tags_.begin();
	std::vector<Tag>::iterator const end = tags_.end();
	for (; it != end; it++) {
		out << "  <tag>\n";
		out << "    <uid>" << (*it).uid_ << "</uid>\n";
		out << "    <name>" << escape_text((*it).name_) << "</name>\n";
		// For now assume all tags are ATTACH, when they're not we need
		// to put some more information here
		out << "  </tag>\n";
	}
	out << "</taglist>\n";
}
