
#ifndef DOCUMENTPROPERTIES_H
#define DOCUMENTPROPERTIES_H

#include <libglademm.h>
#include <gtkmm.h>

class Document;

class DocumentProperties {
	private:
	Glib::RefPtr<Gnome::Glade::Xml> xml_;

	Document *doc_;

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
	Gtk::FileChooserButton *filechooser_;
	Gtk::Button *crossrefbutton_;

	void update ();
	void save ();
	void onCrossRefLookup ();

	public:
		bool show (Document *doc);
		DocumentProperties ();
};

#endif
