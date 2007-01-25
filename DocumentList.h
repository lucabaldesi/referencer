
#ifndef DOCUMENTLIST_H
#define DOCUMENTLIST_H

#include <gtkmm.h>
#include <sstream>
#include <vector>

#include "BibUtils.h"

#include "Document.h"

class DocumentList {
	private:
	std::vector<Document> docs_;

	public:
	std::vector<Document>& getDocs ();
	int size () {return docs_.size();}
	Document* getDoc (Glib::ustring const &name);
	Document* newDocWithFile (Glib::ustring const &filename);
	Document* newDocWithName (Glib::ustring const &key);
	Document* newDocWithDoi (Glib::ustring const &doi);
	Document* newDocUnnamed ();
	Glib::ustring uniqueKey (Glib::ustring const &basename);
	void removeDoc (Glib::ustring const &key);
	void loadDoc (
		Glib::ustring const &filename,
		Glib::ustring const &key,
		std::vector<int> const &taguids,
		BibData const &bib);
	void print ();
	bool test ();
	void clearTag (int uid);
	void writeXML (std::ostringstream& out);
	void clear () {docs_.clear ();}
	void writeBibtex (std::ostringstream &out);

	bool import (Glib::ustring const &filename, BibUtils::Format format);
	Document parseBibUtils (BibUtils::fields *ref);
};

#endif
