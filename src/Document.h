
#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "BibData.h"

class Document {
	private:
	Glib::ustring filename_;
	Glib::ustring relfilename_;
	Glib::ustring key_;
	std::vector<int> tagUids_;
	Glib::RefPtr<Gdk::Pixbuf> thumbnail_;

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
	Glib::ustring& getKey();
	Glib::ustring& getFileName();
	// RelFileName is NOT kept up to date in general, it's
	// used during loading and saving only
	Glib::ustring& getRelFileName();
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

	void writeBibtex (std::ostringstream& out, bool const usebraces);
	void writeXML (std::ostringstream &out);
	void readPDF ();
	void getMetaData ();
	void renameFromKey ();

	BibData& getBibData () {return bib_;}
	void setBibData (BibData& bib){bib_ = bib;}

	static Glib::RefPtr<Gdk::Pixbuf> defaultthumb_;
	static Glib::RefPtr<Gdk::Pixbuf> thumbframe_;

	Glib::ustring generateKey ();
};

#endif
