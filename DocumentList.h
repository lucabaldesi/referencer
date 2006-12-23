
#ifndef DOCUMENTLIST_H
#define DOCUMENTLIST_H

#include <gtkmm.h>
#include <sstream>

class Document {
	private:
	Glib::ustring filename_;
	Glib::ustring displayname_;
	std::vector<int> tagUids_;

	public:
	Document (Glib::ustring const filename);
	Glib::ustring getDisplayName();
	Glib::ustring getFileName();
	std::vector<int> getTags ();
	void setTag (int uid);
	void clearTag (int uid);
	void clearTags ();
	bool hasTag (int uid);
};

class DocumentList {
	private:
	std::vector<Document> docs_;
	
	public:
	std::vector<Document>& getDocs ();
	Document* newDoc (Glib::ustring const filename);
	void print ();
	bool test ();
	void clearTag (int uid);
	void writeXML (std::ostringstream& out);
};

#endif
