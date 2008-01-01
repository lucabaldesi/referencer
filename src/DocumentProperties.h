
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef DOCUMENTPROPERTIES_H
#define DOCUMENTPROPERTIES_H

#include <map>

#include <libglademm.h>
#include <gtkmm.h>

class Document;

class DocumentProperties {
	private:
	Glib::RefPtr<Gnome::Glade::Xml> xml_;

	/*
	 * Local working copy
	 */
	Gtk::Dialog *dialog_;
	Gtk::Entry *keyentry_;
	Gtk::FileChooserButton *filechooser_;
	Gtk::Image *iconImage_;

	Gtk::Button *crossrefbutton_;
	Gtk::Button *pastebibtexbutton_;
	Gtk::Button *newextrafieldbutton_;
	Gtk::Button *deleteextrafieldbutton_;
	Gtk::Button *editextrafieldbutton_;
	Gtk::Expander *extrafieldsexpander_;
	Gtk::TreeView *extrafieldsview_;
	Glib::RefPtr<Gtk::TreeSelection> extrafieldssel_;

	Gtk::ComboBoxEntryText *typeCombo_;
	std::map <Glib::ustring, Gtk::Entry*> fieldEntries_;

	Gtk::TreeModelColumn<Glib::ustring> extrakeycol_;
	Gtk::TreeModelColumn<Glib::ustring> extravalcol_;
	Gtk::TreeModel::ColumnRecord cols_;
	Glib::RefPtr< Gtk::ListStore > extrafieldsstore_;

	void update (Document &doc);
	void save (Document &doc);
	void setupFields (Glib::ustring const &docType);
	void onMetadataLookup ();
	void onPasteBibtex ();
	void onClear ();
	void onNewExtraField ();
	void onDeleteExtraField ();
	void onEditExtraField ();
	void onExtraFieldsSelectionChanged ();
	void onExtraFieldEdited (const Glib::ustring& path, const Glib::ustring& text);
	void onDoiEntryChanged ();
	void updateSensitivity();
	void onTypeChanged ();
	bool ignoreTypeChanged_;

	public:
		bool show (Document *doc);
		DocumentProperties ();
};

#endif
