
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

#include "BibData.h"

#include <libgnomeuimm.h>

class Library;

class Document {
	private:
	Glib::ustring filename_;
	Glib::ustring relfilename_;
	Glib::ustring key_;
	std::vector<int> tagUids_;
	Glib::RefPtr<Gdk::Pixbuf> thumbnail_;
	static const Glib::ustring defaultKey_;

	void setupThumbnail ();

	BibData bib_;

	public:
	Document ();
	Document (Glib::ustring const &filename);
	Document (
		Glib::ustring const &filename,
		Glib::ustring const &relfilename,
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
	Glib::RefPtr<Gdk::Pixbuf> getThumbnail () {return thumbnail_;}
	std::vector<int>& getTags ();
	void setTag (int uid);
	void clearTag (int uid);
	void clearTags ();

	bool hasTag (int uid);
	bool canWebLink ();
	bool canGetMetadata ();
	bool matchesSearch (Glib::ustring const &search);

	void writeBibtex (
		Library const &lib,
		std::ostringstream& out,
		bool const usebraces,
		bool const utf8);
	void writeXML (std::ostringstream &out);
	void readPDF ();
	void getMetaData ();
	void renameFromKey ();

	BibData& getBibData () {return bib_;}
	void setBibData (BibData& bib){bib_ = bib;}

	static Glib::RefPtr<Gdk::Pixbuf> defaultthumb_;
	static Glib::RefPtr<Gdk::Pixbuf> thumbframe_;
	static Glib::RefPtr<Gnome::UI::ThumbnailFactory> thumbfac_;

	Glib::ustring generateKey ();

	void setField (Glib::ustring const &field, Glib::ustring const &value);
	Glib::ustring getField (Glib::ustring const &field) const;
};

#endif
