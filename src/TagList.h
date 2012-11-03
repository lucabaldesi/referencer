
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
#include <libxml/xmlwriter.h>

class Tag {
	public:
    Tag(int uid, const std::string& name);

    Tag() {
    }

public:
	int uid_;
	std::string name_;
};

class TagList {
	public:
	typedef std::map<int, Tag> TagMap;

	TagList() {
		uidCounter_ = 0;
	}
    void print();
    TagMap& getTags();
    int newTag(const std::string& name);
    void loadTag(const std::string& name, int uid);
    void renameTag(int uid, const std::string& newname);
    void deleteTag(int uid);
    std::string getName(int uid);
    /**
     * Dumps this tag list's data into an XML document.
     * @param writer the XML writer, which actually does all the XML writing and
     * formatting.
     */
    void writeXML(xmlTextWriterPtr writer);

    void clear() {
        tags_.clear();
        uidCounter_ = 0;
    }
    bool tagExists(const std::string& name);
    int getTagUid(const std::string& name);

	private:
	TagMap tags_;
	int uidCounter_;
};

#endif
