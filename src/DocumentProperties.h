
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

#include <gtkmm.h>

#include "CaseFoldCompare.h"
#include "DocumentTypes.h"

class Document;
class RefWindow;

class DocumentProperties {
	private:
	Glib::RefPtr<Gtk::Builder> xml_;
	RefWindow &win_;

	/*
	 * Local working copy
	 */
	Gtk::Dialog *dialog_;
	Gtk::Entry *keyentry_;
	Gtk::FileChooserButton *filechooser_;
	Gtk::Image *iconImage_;
	Gtk::Button *iconButton_;

	Gtk::Button *crossrefbutton_;
	Gtk::Button *pastebibtexbutton_;
	Gtk::ToolButton *newextrafieldbutton_;
	Gtk::ToolButton *deleteextrafieldbutton_;
	Gtk::ToolButton *editextrafieldbutton_;
	Gtk::Expander *extrafieldsexpander_;
	Gtk::TreeView *extrafieldsview_;
	Glib::RefPtr<Gtk::TreeSelection> extrafieldssel_;

	Gtk::ComboBox *typeCombo_;
	Gtk::TreeModelColumn<Glib::ustring> typelabelcol_;
	Gtk::TreeModelColumn<Glib::ustring> typebibtexnamecol_;
	Gtk::TreeModel::ColumnRecord typecombocols_;
	Glib::RefPtr< Gtk::ListStore > typecombostore_;
	sigc::connection typecombochanged_;
	typedef std::map <Glib::ustring, Gtk::Entry*, casefoldCompare> FieldEntryMap;
	FieldEntryMap fieldEntries_;

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
	void onFileChanged ();
	void onIconButtonClicked ();
	bool ignoreTypeChanged_;
	DocumentTypeManager typeManager_;

	public:
		bool show (Document *doc);
		DocumentProperties (RefWindow &refwin);
};

#endif
