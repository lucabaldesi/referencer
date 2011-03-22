
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef DOCUMENTLIST_H
#define DOCUMENTLIST_H

#include <gtkmm.h>
#include <sstream>
#include <list>
#include <libxml/xmlwriter.h>

#include "BibUtils.h"

#include "Document.h"



class DocumentList {
	public:
	typedef std::list<Document> Container;

	private:
	Container docs_;

	public:
	Container& getDocs ();
	int size () {return docs_.size();}
	Document* newDocWithFile (Glib::ustring const &filename);
	Document* newDocWithName (Glib::ustring const &key);
	Document* newDocUnnamed ();
	Document* insertDoc (Document const &doc);

	bool docExists (
		Glib::ustring const &name,
		Document const *exclusion);
	Glib::ustring uniqueKey (
		Glib::ustring const &basename);
	Glib::ustring uniqueKey (
		Glib::ustring const &basename,
		Document const *exclusion);
	Glib::ustring sanitizedKey (
		Glib::ustring const &key);

	void removeDoc (Document* const addr);
	void loadDoc (
		Glib::ustring const &filename,
		Glib::ustring const &relfilename,
		Glib::ustring const &notes,
		Glib::ustring const &key,
		std::vector<int> const &taguids,
		BibData const &bib);
	void print ();
	void clearTag (int uid);
	void writeXML (xmlTextWriterPtr writer);
	void clear () {docs_.clear ();}

	int importFromFile (Glib::ustring const &filename, BibUtils::Format format);
	int import (Glib::ustring const &rawtext, BibUtils::Format format);
	Document parseBibUtils (BibUtils::fields *ref);
};

#endif
