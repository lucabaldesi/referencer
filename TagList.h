
#ifndef TAGLIST_H
#define TAGLIST_H

#include <string>
#include <vector>

class Tag {
	public:
	typedef enum {ATTACH, SEARCH_ALL, SEARCH_AUTHOR,
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
	TagList() {uidCounter_ = 0;}
	void print();
	std::vector<Tag> getTags();
	std::vector<Tag> newTag(std::string const name, Tag::Action const action);
};

#endif
