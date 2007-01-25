
#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "BibData.h"

class Document {
	private:
	Glib::ustring filename_;
	Glib::ustring key_;
	std::vector<int> tagUids_;
	Glib::RefPtr<Gdk::Pixbuf> thumbnail_;

	void setupThumbnail ();
	static Glib::RefPtr<Gdk::Pixbuf> getThemeIcon(Glib::ustring const &iconname);

	BibData bib_;

	public:
	Document ();
	Document (Glib::ustring const &filename);
	Document (
		Glib::ustring const &filename,
		Glib::ustring const &key,
		std::vector<int> const &tagUids,
		BibData const &bib);
	Glib::ustring& getKey();
	Glib::ustring& getFileName();
	void setFileName (Glib::ustring &filename);
	void setKey (Glib::ustring const &key);
	Glib::RefPtr<Gdk::Pixbuf> getThumbnail () {return thumbnail_;}
	std::vector<int>& getTags ();
	void setTag (int uid);
	void clearTag (int uid);
	void clearTags ();
	bool hasTag (int uid);

	bool canWebLink ();

	void writeBibtex (std::ostringstream& out);
	void writeXML (std::ostringstream &out);
	void readPDF ();

	BibData& getBibData () {return bib_;}
	void setBibData (BibData& bib){bib_ = bib;}

	static Glib::RefPtr<Gdk::Pixbuf> defaultthumb_;

	Glib::ustring generateKey ();
};

#endif
