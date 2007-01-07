
#ifndef DOCUMENTPROPERTIES_H
#define DOCUMENTPROPERTIES_H

#include <libglademm.h>
#include <gtkmm.h>

class Document;

class DocumentProperties {
	private:
	Glib::RefPtr<Gnome::Glade::Xml> xml_;

	Gtk::Dialog *dialog_;

	Gtk::Entry *doientry_;
	Gtk::Entry *keyentry_;
	Gtk::Entry *titleentry_;
	Gtk::Entry *authorsentry_;
	Gtk::Entry *journalentry_;
	Gtk::Entry *volumeentry_;
	Gtk::Entry *issueentry_;
	Gtk::Entry *pagesentry_;
	Gtk::Entry *yearentry_;

	public:
		void show (Document *doc);
		DocumentProperties ();
};

#endif
