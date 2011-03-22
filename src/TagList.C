
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
#include <libxml/xmlwriter.h>

#include "TagList.h"
#include "Utility.h"
#include "Library.h"

Tag::Tag(int const uid, const std::string& name) {
	uid_ = uid;
	name_ = name;
}

TagList::TagMap& TagList::getTags ()
{
	return tags_;
}

int TagList::newTag(const std::string& name) {
    tags_[uidCounter_] = Tag(uidCounter_, name);
	return uidCounter_++;
}

void TagList::loadTag(const std::string& name, int uid) {
    if (tagExists(name)) {
        DEBUG("Duplicate tag name '%1'", name);
	} else {
        tags_[uid] = Tag(uid, name);
		if (uid >= uidCounter_)
			uidCounter_ = uid + 1;
	}
}

void TagList::renameTag(int uid, const std::string& newname) {
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

std::string TagList::getName(int uid) {
	return tags_[uid].name_;
}

void TagList::writeXML(xmlTextWriterPtr writer) {
    xmlTextWriterStartElement(writer, CXSTR LIB_ELEMENT_TAGLIST);
	TagMap::iterator it = tags_.begin();
	TagMap::iterator const end = tags_.end();

	for (; it != end; it++) {
        xmlTextWriterStartElement(writer, CXSTR LIB_ELEMENT_TAG);
        xmlTextWriterWriteFormatElement(writer, CXSTR LIB_ELEMENT_TAG_UID, "%d", (*it).second.uid_);
        xmlTextWriterWriteElement(writer, CXSTR LIB_ELEMENT_TAG_NAME, CXSTR(*it).second.name_.c_str());
        xmlTextWriterEndElement(writer);
	}
    xmlTextWriterEndElement(writer);
}

bool TagList::tagExists(const std::string& name) {
	TagMap::iterator it = tags_.begin();
	TagMap::iterator const end = tags_.end();
	for (; it != end; it++) {
		if (it->second.name_ == name)
			return true;
	}

	return false;
}
