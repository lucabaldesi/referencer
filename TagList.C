
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
			break;
		}
	}
}


void TagList::writeXML (std::ostringstream& out)
{
	// Should escape this stuff
	out << "<taglist>\n";
	std::vector<Tag>::iterator it = tags_.begin();
	std::vector<Tag>::iterator const end = tags_.end();
	for (; it != end; it++) {
		out << "  <tag>\n";
		out << "    <uid>" << (*it).uid_ << "</uid>\n";
		out << "    <name>" << (*it).name_ << "</uid>\n";
		// For now assume all tags are ATTACH, when they're not we need
		// to put some more information here
		out << "  </tag>\n";
	}
	out << "</taglist>\n";
}
