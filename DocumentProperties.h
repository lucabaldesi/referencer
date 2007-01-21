
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
	Gtk::Button *newextrafieldbutton_;
	Gtk::Button *deleteextrafieldbutton_;
	Gtk::Button *editextrafieldbutton_;
	Gtk::ComboBoxEntryText *typecombo_;
	Gtk::Expander *extrafieldsexpander_;
	Gtk::TreeView *extrafieldsview_;
	Glib::RefPtr<Gtk::TreeSelection> extrafieldssel_;

  Gtk::TreeModelColumn<Glib::ustring> extrakeycol_;
  Gtk::TreeModelColumn<Glib::ustring> extravalcol_;
  Gtk::TreeModel::ColumnRecord cols_;
  Glib::RefPtr< Gtk::ListStore > extrafieldsstore_;

	void update ();
	void save ();
	void onCrossRefLookup ();
	void onNewExtraField ();
	void onDeleteExtraField ();
	void onEditExtraField ();
	void onExtraFieldsSelectionChanged ();

	public:
		bool show (Document *doc);
		DocumentProperties ();
};

#endif
