
#ifndef BIBDATA_H
#define BIBDATA_H

#include <gtkmm.h>

class BibData {
	private:
	Glib::ustring doi_;
	Glib::ustring volume_;
	Glib::ustring issue_;
	Glib::ustring pages_;
	Glib::ustring authors_;
	Glib::ustring journal_;
	Glib::ustring title_;
	Glib::ustring year_;

	public:
	typedef enum {
		FIELD_VOLUME = 1<<0,
		FIELD_NUMBER = 1<<1,
		FIELD_PAGES = 1<<2,
		FIELD_AUTHORS = 1<<3,
		FIELD_JOURNAL = 1<<4,
		FIELD_TITLE = 1<<5,
		FIELD_YEAR = 1<<6,
		FIELD_ALL = 1<<7
	} FieldMask;

	BibData () {}

	void writeXML (std::ostringstream &out);
	void parseCrossRefXML (Glib::ustring const &xml);
	void getCrossRef ();
	void fetcherThread ();

	void setDoi (Glib::ustring const &doi) {doi_ = doi;} 
	Glib::ustring getDoi () {return doi_;}

	void setVolume (Glib::ustring const &vol) {volume_ = vol;}
	Glib::ustring getVolume () {return volume_;}
	void setIssue (Glib::ustring const &issue) {issue_ = issue;}
	Glib::ustring getIssue () {return issue_;}
	void setPages (Glib::ustring const &pages) {pages_ = pages;}
	Glib::ustring getPages () {return pages_;}
	void setAuthors (Glib::ustring const &authors) {authors_ = authors;}
	Glib::ustring getAuthors () {return authors_;}
	void setJournal (Glib::ustring const &journal) {journal_ = journal;}
	Glib::ustring getJournal () {return journal_;}
	void setTitle (Glib::ustring const &title) {title_ = title;}
	Glib::ustring getTitle () {return title_;}
	void setYear (Glib::ustring const &year) {year_ = year;}
	Glib::ustring getYear () {return year_;}

	void print ();
	
	void clear ();
	
	void parseMetadata (Glib::ustring const &meta, FieldMask mask);
	void guessJournal (Glib::ustring const &raw);
	void guessVolumeNumberPage (Glib::ustring const &raw);
	void guessYear (Glib::ustring const &raw);
	void guessAuthors (Glib::ustring const &raw);
	void guessTitle (Glib::ustring const &raw);
	void guessDoi (Glib::ustring const &raw);
};


#endif
