
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
	void deleteTag (int uid);
	void writeXML (std::ostringstream& out);
};

#endif
