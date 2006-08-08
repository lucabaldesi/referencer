
#include <gtkmm.h>

class BibData {
	private:
	Glib::ustring volume_;
	Glib::ustring number_;
	Glib::ustring pages_;
	Glib::ustring authors_;
	Glib::ustring journal_;
	Glib::ustring title_;
	public:
	void setVolume (Glib::ustring &vol) {volume_ = vol;}
	Glib::ustring getVolume () {return volume_;}
	void setNumber (Glib::ustring &num) {number_ = num;}
	Glib::ustring getNumber () {return number_;}
	void setPages (Glib::ustring &pages) {pages_ = pages;}
	Glib::ustring getPages () {return pages_;}
	void setAuthors (Glib::ustring &authors) {authors_ = authors;}
	Glib::ustring getAuthors () {return authors_;}
	void setJournal (Glib::ustring &journal) {journal_ = journal;}
	Glib::ustring getJournal () {return journal_;}
	void setTitle (Glib::ustring &title) {title_ = title;}
	Glib::ustring getTitle () {return title_;}

	void print ();
};
