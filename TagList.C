
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

std::vector<Tag> TagList::newTag(std::string const name, Tag::Action const action)
{
	Tag const *newtag = new Tag(uidCounter_++, name, action);
	tags_.push_back(*newtag);
}

void TagList::print()
{
	std::vector<Tag>::iterator it = tags_.begin();
	std::vector<Tag>::iterator const end = tags_.end();
	for (; it != end; it++) {
		std::cout << (*it).uid_;
	}
}
