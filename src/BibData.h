
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef BIBDATA_H
#define BIBDATA_H

#include <map>
#include <vector>
#include <functional>

#include <gtkmm.h>

struct casefoldCompare : public std::binary_function<Glib::ustring, Glib::ustring, bool> {
	bool operator () ( const Glib::ustring &lhs, const Glib::ustring &rhs ) const {
		return lhs.casefold() < rhs.casefold();
	}
};

class BibData {
	private:
	Glib::ustring type_;
	Glib::ustring doi_;
	Glib::ustring volume_;
	Glib::ustring issue_;
	Glib::ustring pages_;
	Glib::ustring authors_;
	Glib::ustring journal_;
	Glib::ustring title_;
	Glib::ustring year_;

	static std::vector<Glib::ustring> document_types;
	static Glib::ustring default_document_type;

	public:
	BibData ();

	static std::vector<Glib::ustring> &getDocTypes ();
	static Glib::ustring &getDefaultDocType ();

	void print () const;
	void clear ();

	void writeXML (std::ostringstream &out);
	void parseCrossRefXML (Glib::ustring const &xml);
	void resolveDoi ();
	void resolveArxiv ();
	
	void mergeIn (BibData const &source);

	typedef std::map <Glib::ustring, Glib::ustring, casefoldCompare> ExtrasMap;
	ExtrasMap extras_;
	void addExtra (Glib::ustring const &key, Glib::ustring const &value);
	void clearExtras ();
	ExtrasMap getExtras () const {return extras_;}
	bool hasExtras () {return !extras_.empty();}

	void setType (Glib::ustring const &type) {type_ = type;}
	Glib::ustring getType () const {return type_;} 
	void setDoi (Glib::ustring const &doi) {doi_ = doi;}
	Glib::ustring getDoi () const {return doi_;}
	void setVolume (Glib::ustring const &vol) {volume_ = vol;}
	Glib::ustring getVolume () const {return volume_;}
	void setIssue (Glib::ustring const &issue) {issue_ = issue;}
	Glib::ustring getIssue () const {return issue_;}
	void setPages (Glib::ustring const &pages) {pages_ = pages;}
	Glib::ustring getPages () const {return pages_;}
	void setAuthors (Glib::ustring const &authors) {authors_ = authors;}
	Glib::ustring getAuthors () const {return authors_;}
	void setJournal (Glib::ustring const &journal) {journal_ = journal;}
	Glib::ustring getJournal () const {return journal_;}
	void setTitle (Glib::ustring const &title) {title_ = title;}
	Glib::ustring getTitle () const {return title_;}
	void setYear (Glib::ustring const &year) {year_ = year;}
	Glib::ustring getYear () const {return year_;}

	void guessJournal (Glib::ustring const &raw);
	void guessVolumeNumberPage (Glib::ustring const &raw);
	void guessYear (Glib::ustring const &raw);
	void guessAuthors (Glib::ustring const &raw);
	void guessTitle (Glib::ustring const &raw);
	void guessDoi (Glib::ustring const &raw);
	void guessArxiv (Glib::ustring const &raw_);
};

#endif
