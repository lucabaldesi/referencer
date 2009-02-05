
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#include <iostream>

#include <glibmm/i18n.h>
#include <string>
#include "ucompose.hpp"

#include "Document.h"
#include "DocumentList.h"
#include "DocumentTypes.h"
#include "Preferences.h"

#include "DocumentProperties.h"


DocumentProperties::DocumentProperties ()
{
	xml_ = Gnome::Glade::Xml::create (
		Utility::findDataFile ("documentproperties.glade"));

	dialog_ = (Gtk::Dialog *) xml_->get_widget ("DocumentProperties");
	filechooser_ = (Gtk::FileChooserButton *) xml_->get_widget ("File");
	keyentry_ = (Gtk::Entry *) xml_->get_widget ("Key");
	iconImage_ = (Gtk::Image *) xml_->get_widget ("Icon");

	crossrefbutton_ = (Gtk::Button *) xml_->get_widget ("Lookup");
	crossrefbutton_->signal_clicked().connect(
		sigc::mem_fun (*this, &DocumentProperties::onMetadataLookup));

	pastebibtexbutton_ = (Gtk::Button *) xml_->get_widget ("PasteBibtex");
	pastebibtexbutton_->signal_clicked().connect(
		sigc::mem_fun (*this, &DocumentProperties::onPasteBibtex));

	Gtk::Button *clearButton = (Gtk::Button *) xml_->get_widget ("Clear");
	clearButton->signal_clicked().connect (
		sigc::mem_fun (*this, &DocumentProperties::onClear));

	extrafieldsexpander_ =
		(Gtk::Expander *) xml_->get_widget ("ExtraFieldsExpander");
	newextrafieldbutton_ = (Gtk::Button *) xml_->get_widget ("NewExtraField");
	deleteextrafieldbutton_ = (Gtk::Button *) xml_->get_widget ("DeleteExtraField");
	editextrafieldbutton_ = (Gtk::Button *) xml_->get_widget ("EditExtraField");

	newextrafieldbutton_->signal_clicked ().connect (
		sigc::mem_fun (*this, &DocumentProperties::onNewExtraField));

	deleteextrafieldbutton_->signal_clicked ().connect (
		sigc::mem_fun (*this, &DocumentProperties::onDeleteExtraField));

	editextrafieldbutton_->signal_clicked ().connect (
		sigc::mem_fun (*this, &DocumentProperties::onEditExtraField));

	extrafieldsview_ = (Gtk::TreeView *) xml_->get_widget ("ExtraFields");
	cols_.add (extrakeycol_);
	cols_.add (extravalcol_);

	extrafieldssel_ = extrafieldsview_->get_selection ();
	extrafieldssel_->signal_changed ().connect (
		sigc::mem_fun (*this, &DocumentProperties::onExtraFieldsSelectionChanged));

	extrafieldsstore_ = Gtk::ListStore::create (cols_);

	extrafieldsview_->set_model (extrafieldsstore_);
	extrafieldsview_->append_column ("Name", extrakeycol_);
	extrafieldsview_->append_column_editable ("Value", extravalcol_);

	// looks like watch for a cell-renderer is the only way 
	// to get information that the cell(s) changed 
	Gtk::CellRendererText* extrafieldsrenderer = 
		dynamic_cast<Gtk::CellRendererText*> (
		extrafieldsview_->get_column(1)->get_first_cell_renderer() );
	extrafieldsrenderer->signal_edited ().connect (
		sigc::mem_fun (*this, &DocumentProperties::onExtraFieldEdited));

	ignoreTypeChanged_ = false;
}


bool DocumentProperties::show (Document *doc)
{
	if (!doc) {
		DEBUG ("DocumentProperties::show: NULL doc pointer");
		return false;
	}

	update (*doc);

	extrafieldsexpander_->set_expanded (extrafieldsstore_->children().size() > 0);

	keyentry_->grab_focus ();

	int result = dialog_->run ();

	if (result == Gtk::RESPONSE_OK) {
		save (*doc);
	}

	dialog_->hide ();

	if (result == Gtk::RESPONSE_OK)
		return true;
	else
		return false;
}

DocumentTypeManager typeManager;

void DocumentProperties::update (Document &doc)
{
	filechooser_->set_uri (doc.getFileName());
	keyentry_->set_text (doc.getKey());
	iconImage_->set (doc.getThumbnail());

	setupFields (doc.getBibData().getType());

	bool const ignore = ignoreTypeChanged_;

	ignoreTypeChanged_ = true;
	if (typeManager.getTypes().find(doc.getBibData().getType()) != typeManager.getTypes().end()) {
		DocumentType type = typeManager.getTypes()[doc.getBibData().getType()];
		typeCombo_->set_active_text (type.bibtexName_);
	} else {
		typeCombo_->append_text (doc.getBibData().getType());
		typeCombo_->set_active_text (doc.getBibData().getType());
	}
	ignoreTypeChanged_ = ignore;

	extrafieldsstore_->clear ();

	std::map<Glib::ustring, Glib::ustring> fields = doc.getFields();
	std::map<Glib::ustring, Glib::ustring>::iterator field = fields.begin ();
	std::map<Glib::ustring, Glib::ustring>::iterator const endField = fields.end ();
	for (; field != endField; ++field) {
		Glib::ustring const key = (*field).first;
		Glib::ustring const value = (*field).second;

		if (fieldEntries_.find(key) != fieldEntries_.end()) {
			fieldEntries_[key]->set_text (value);
		} else {
			Gtk::ListStore::iterator row = extrafieldsstore_->append ();
			(*row)[extrakeycol_] = key;
			(*row)[extravalcol_] = value;
		}
	}

	updateSensitivity ();
}

void DocumentProperties::save (Document &doc)
{
	Glib::ustring filename = filechooser_->get_uri ();
	doc.setFileName (filename);
	doc.setKey (keyentry_->get_text ());

	doc.getBibData().setType (typeCombo_->get_active_text ());

	doc.clearFields ();
	FieldEntryMap::iterator entry = fieldEntries_.begin();
	FieldEntryMap::iterator const endEntry = fieldEntries_.end();
	for (; entry != endEntry; ++entry) {
		Glib::ustring key = (*entry).first;
		Glib::ustring value = ((*entry).second)->get_text ();
		if (!value.empty())
			doc.setField (key, value);
	}

	Gtk::ListStore::iterator it = extrafieldsstore_->children().begin ();
	Gtk::ListStore::iterator const end = extrafieldsstore_->children().end ();
	for (; it != end; ++it)
		doc.setField ((*it)[extrakeycol_], (*it)[extravalcol_]);
}


void DocumentProperties::setupFields (Glib::ustring const &docType)
{
	Gtk::VBox *metadataBox = (Gtk::VBox *) xml_->get_widget ("MetadataBox");
	if (metadataBox->children().size()) {
		metadataBox->children().erase(metadataBox->children().begin());
	}

	DocumentType type = typeManager.getType (docType);

	int const nRows = type.requiredFields_.size() + type.optionalFields_.size();
	Gtk::Table *metadataTable = new Gtk::Table (nRows, 4, false);
	metadataTable->set_col_spacings (6);
	metadataTable->set_row_spacings (6);

	fieldEntries_.clear ();

	Gtk::Label *typeLabel = Gtk::manage (new Gtk::Label (_("_Type:"), Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, true));
	typeCombo_ = Gtk::manage (new Gtk::ComboBoxEntryText);
	typeCombo_->signal_changed().connect (
			sigc::mem_fun (*this, &DocumentProperties::onTypeChanged));
	metadataTable->attach (*typeLabel, 0, 1, 0, 1, Gtk::FILL, Gtk::SHRINK | Gtk::FILL, 0, 0);
	metadataTable->attach (*typeCombo_, 1, 4, 0, 1, Gtk::FILL, Gtk::SHRINK | Gtk::FILL, 0, 0);

	for (DocumentTypeManager::TypesMap::iterator it = typeManager.getTypes().begin();
			it != typeManager.getTypes().end();
			++it) {
		typeCombo_->append_text ((*it).second.bibtexName_);
	}

	int row = 1;
	for (
	     std::vector<DocumentField>::iterator it = type.requiredFields_.begin();
	     it != type.requiredFields_.end();
	     ++it) {

		if (it->shortField_)
			continue;

		Gtk::Label *label = Gtk::manage (new Gtk::Label (it->displayName_ + ":", Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false));
		Gtk::Entry *entry = Gtk::manage (new Gtk::Entry ());

		fieldEntries_[it->internalName_] = entry;

		metadataTable->attach (*label, 0, 1, row, row + 1, Gtk::FILL | Gtk::EXPAND, Gtk::SHRINK | Gtk::FILL, 0, 0);
		metadataTable->attach (*entry, 1, 4, row, row + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL, 0, 0);
		++row;
	}
	for (
	     std::vector<DocumentField>::iterator it = type.optionalFields_.begin();
	     it != type.optionalFields_.end();
	     ++it) {

		if (it->shortField_)
			continue;

		Gtk::Label *label = Gtk::manage (new Gtk::Label (it->displayName_ + ":", Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false));
		Gtk::Entry *entry = Gtk::manage (new Gtk::Entry ());

		fieldEntries_[it->internalName_] = entry;

		metadataTable->attach (*label, 0, 1, row, row + 1, Gtk::FILL | Gtk::EXPAND, Gtk::SHRINK | Gtk::FILL, 0, 0);
		metadataTable->attach (*entry, 1, 4, row, row + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL, 0, 0);
		++row;
	}

	int col = 0;
	for (
	     std::vector<DocumentField>::iterator it = type.requiredFields_.begin();
	     it != type.requiredFields_.end();
	     ++it) {

		if (!it->shortField_)
			continue;

		Gtk::Label *label = Gtk::manage (new Gtk::Label (it->displayName_ + ":", Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false));
		Gtk::Entry *entry = Gtk::manage (new Gtk::Entry ());

		fieldEntries_[it->internalName_] = entry;

		metadataTable->attach (*label, 0 + col * 2, 1 + col * 2, row, row + 1, Gtk::FILL | Gtk::EXPAND, Gtk::SHRINK | Gtk::FILL, 0, 0);
		metadataTable->attach (*entry, 1 + col * 2, 2 + col * 2, row, row + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL, 0, 0);
		++col;
		if (col > 1) {
			++row;
			col = 0;
		}
	}
	for (
	     std::vector<DocumentField>::iterator it = type.optionalFields_.begin();
	     it != type.optionalFields_.end();
	     ++it) {

		if (!it->shortField_)
			continue;

		Gtk::Label *label = Gtk::manage (new Gtk::Label (it->displayName_ + ":", Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false));
		Gtk::Entry *entry = Gtk::manage (new Gtk::Entry ());

		fieldEntries_[it->internalName_] = entry;

		metadataTable->attach (*label, 0 + col * 2, 1 + col * 2, row, row + 1, Gtk::FILL | Gtk::EXPAND, Gtk::SHRINK | Gtk::FILL, 0, 0);
		metadataTable->attach (*entry, 1 + col * 2, 2 + col * 2, row, row + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL, 0, 0);
		++col;
		if (col > 1) {
			++row;
			col = 0;
		}
	}

	metadataBox->pack_start (*metadataTable);
	metadataBox->show_all ();
}



void DocumentProperties::onNewExtraField ()
{
	Gtk::Dialog dialog ("New Field", true, false);

	Gtk::VBox *vbox = dialog.get_vbox ();

	Gtk::HBox hbox;
	hbox.set_spacing (12);
	vbox->pack_start (hbox, true, true, 0);

	Gtk::Label label ("Field name:", false);
	hbox.pack_start (label, false, false, 0);

	Gtk::Entry entry;
	entry.set_activates_default (true);
	hbox.pack_start (entry, true, true, 0);

	dialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button (Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
	dialog.set_default_response (Gtk::RESPONSE_ACCEPT);

	dialog.show_all ();
	vbox->set_border_width (12);

	if (dialog.run () == Gtk::RESPONSE_ACCEPT) {
		Gtk::ListStore::iterator row = extrafieldsstore_->append ();
		(*row)[extrakeycol_] = entry.get_text ();
		(*row)[extravalcol_] = "";
	}
}


void DocumentProperties::onDeleteExtraField ()
{
	// Oh dear, this may crash if this button was sensitive at the wrong time
	extrafieldsstore_->erase (extrafieldssel_->get_selected ());
	updateSensitivity();
}


void DocumentProperties::onEditExtraField ()
{
	Gtk::TreeSelection::ListHandle_Path paths =
		extrafieldssel_->get_selected_rows ();

	if (paths.empty ()) {
		DEBUG ("Warning: DocumentProperties::onEditExtraField: none selected");
		return;
	} else if (paths.size () > 1) {
		DEBUG ("Warning: DocumentProperties::onEditExtraField: too many selected");
		return;
	}

	Gtk::TreePath path = (*paths.begin ());
	extrafieldsview_->set_cursor (path, *extrafieldsview_->get_column (1), true);

	updateSensitivity();
}


void DocumentProperties::onExtraFieldsSelectionChanged ()
{
	bool const enable = extrafieldssel_->count_selected_rows () > 0;
	deleteextrafieldbutton_->set_sensitive (enable);
	editextrafieldbutton_->set_sensitive (enable);
}


void DocumentProperties::updateSensitivity()
{
	Document doc;
	save (doc);
	crossrefbutton_->set_sensitive (doc.canGetMetadata ());
}

void DocumentProperties::onExtraFieldEdited (const Glib::ustring& path, const Glib::ustring& text)
{
	updateSensitivity ();
}


void DocumentProperties::onMetadataLookup ()
{
	Document doc;
	save (doc);
	if (doc.getMetaData ())
		update (doc);
}


void DocumentProperties::onPasteBibtex ()
{
	GdkAtom const selection = GDK_SELECTION_CLIPBOARD;

	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get (selection);

	Glib::ustring clipboardtext = clipboard->wait_for_text ();

	DocumentList doclist;
	int const imported = doclist.import (clipboardtext, BibUtils::FORMAT_BIBTEX);

	DEBUG1 ("DocumentProperties::onPasteBibtex: Imported %1 references", imported);

	if (imported) {
		DocumentList::Container &docs = doclist.getDocs ();
		DocumentList::Container::iterator it = docs.begin ();

		/*
		 * This will lose the key from the bibtex since it's
		 * in Document not BibData
		 */
		Document doc;
		save(doc);
		doc.getBibData().mergeIn (it->getBibData());
		update (doc);
	} else {
		Glib::ustring message;
	       
		if (imported < 1) {
			message = String::ucompose (
				"<b><big>%1</big></b>",
				_("No references found on clipboard.\n"));
		} else {
			message = String::ucompose (
				"<b><big>%1</big></b>",
				_("Multiple references found on clipboard.\n"));
		}

		Gtk::MessageDialog dialog (
			message, true,
			Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE, true);

		dialog.run ();
	}
}


void DocumentProperties::onClear ()
{
	std::map <Glib::ustring, Gtk::Entry*>:: iterator entry = fieldEntries_.begin();
	std::map <Glib::ustring, Gtk::Entry*>:: iterator const endEntry = fieldEntries_.end();
	for (; entry != endEntry; ++entry)
		((*entry).second)->set_text (Glib::ustring());

	extrafieldsstore_->clear ();
}

void DocumentProperties::onTypeChanged ()
{
	if (ignoreTypeChanged_)
		return;
	Document doc;
	save (doc);
	update (doc);
}
