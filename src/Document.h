
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <glibmm.h>

#include "BibData.h"

class DocumentView;
class Library;

class Document {
	private:
	Glib::ustring filename_;
	Glib::ustring relfilename_;
	Glib::ustring key_;
	Glib::ustring notes_;
	std::vector<int> tagUids_;
	Glib::RefPtr<Gdk::Pixbuf> thumbnail_;
	static const Glib::ustring defaultKey_;
	static Glib::RefPtr<Gdk::Pixbuf> loadingthumb_;

	void setupThumbnail ();
	DocumentView *view_;

	BibData bib_;

	public:
	~Document ();
	Document ();
	Document (Document const & x);
	Document (Glib::ustring const &filename);
	Document (
		Glib::ustring const &filename,
		Glib::ustring const &relfilename,
		Glib::ustring const &notes,
		Glib::ustring const &key,
		std::vector<int> const &tagUids,
		BibData const &bib);
	Glib::ustring const & getKey() const;
	Glib::ustring const & getFileName() const;
	// RelFileName is NOT kept up to date in general, it's
	// used during loading and saving only
	Glib::ustring const & getRelFileName() const;
	void setFileName (Glib::ustring const &filename);
	void updateRelFileName (Glib::ustring const &libfilename);
	void setKey (Glib::ustring const &key);
	
	//Notes set and get
	Glib::ustring const & getNotes() const;
	void setNotes(Glib::ustring const &notes);

	std::vector<int>& getTags ();
	void setTag (int uid);
	void clearTag (int uid);
	void clearTags ();

	Glib::RefPtr<Gdk::Pixbuf> getThumbnail () {return thumbnail_;}
	void setThumbnail (Glib::RefPtr<Gdk::Pixbuf> thumb);
	void setView (DocumentView *view) {view_ = view;}

	bool hasTag (int uid);
	bool canGetMetadata ();
	bool matchesSearch (Glib::ustring const &search);

	void writeBibtex (
		Library const &lib,
		std::ostringstream& out,
		bool const usebraces,
		bool const utf8);
	void writeXML (std::ostringstream &out);
	bool readPDF ();
	bool getMetaData ();
	void renameFromKey ();

	BibData& getBibData () {return bib_;}
	void setBibData (BibData& bib){bib_ = bib;}

	Glib::ustring generateKey ();

	bool parseBibtex (Glib::ustring const &bibtex);

	void setField (Glib::ustring const &field, Glib::ustring const &value);
	Glib::ustring getField (Glib::ustring const &field);
	bool hasField (Glib::ustring const &field) const;
	std::map <Glib::ustring, Glib::ustring> getFields ();
	void clearFields ();

	static Glib::ustring keyReplaceDialog (Glib::ustring const &, Glib::ustring const &);
};

#endif
