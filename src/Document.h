
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
#include <libxml/xmlwriter.h>

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
        /**
         * Creates a document by extracting information from the provided XML
         * node.
         * @param docNode the XML DOM node representing the element that
         * contains information about the document, such as the title, authors,
         * filename etc.
         */
        Document(xmlNodePtr docNode);
	Glib::ustring const & getKey() const;
	Glib::ustring const & getFileName() const;
	// RelFileName is NOT kept up to date in general, it's
	// used during loading and saving only
	Glib::ustring const & getRelFileName() const;
	void setFileName (Glib::ustring const &filename);
	void updateRelFileName (Glib::ustring const &libfilename);
	void setKey (Glib::ustring const &key);
	/**
	 * <p>Sets the relative path to the file as is.</p>
	 * <p>This function is used when loading stored documents from the 'reflib'
	 * library XML file.</p>
	 */
	void setRelFileName(const Glib::ustring& relFileName) {
		this->relfilename_ = relFileName;
	}
	
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

	Glib::ustring printBibtex (
		bool const useBraces,
		bool const utf8);
		
	void writeXML (xmlTextWriterPtr writer);
        /**
         * Extracts document data from the given XML DOM node.
         * @param docNode the \c document node from the parsed library XML file.
         * It contains information about the document such as authors, title,
         * filename etc.
         */
        void readXML(xmlNodePtr docNode);
	bool readPDF ();
	bool getMetaData ();
	void renameFromKey ();

	BibData& getBibData () {return bib_;}
	void setBibData (BibData& bib){bib_ = bib;}

	Glib::ustring generateKey ();

	bool parseBibtex (Glib::ustring const &bibtex);

	typedef std::map <Glib::ustring, Glib::ustring> FieldMap;

	void setField (Glib::ustring const &field, Glib::ustring const &value);
	Glib::ustring getField (Glib::ustring const &field);
	bool hasField (Glib::ustring const &field) const;
	FieldMap getFields ();
	void clearFields ();

	static Glib::ustring keyReplaceDialogNotUnique (Glib::ustring const &, Glib::ustring const &);
	static Glib::ustring keyReplaceDialogInvalidChars (Glib::ustring const &, Glib::ustring const &);
	static Glib::ustring keyReplaceDialog (Glib::ustring const &, Glib::ustring const &, const char *);
};

#endif
