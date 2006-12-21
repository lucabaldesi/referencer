
#include <iostream>

#include "TagList.h"

Tag::Tag (int const uid, std::string const name, Tag::Action const action)
{
	uid_ = uid;
	name_ = name;
	action_ = action;
}

std::vector<Tag> TagList::getTags()
{
	return tags_;
}

void TagList::newTag(std::string const name, Tag::Action const action)
{
	Tag newtag (uidCounter_++, name, action);
	tags_.push_back(newtag);
}

void TagList::print()
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
