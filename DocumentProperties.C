
#include <iostream>

#include "Document.h"
#include "DocumentProperties.h"

#include "Preferences.h"

DocumentProperties::DocumentProperties ()
{
	xml_ = Utility::openGlade ("documentproperties.glade");

	dialog_ = (Gtk::Dialog *) xml_->get_widget ("DocumentProperties");
	filechooser_ = (Gtk::FileChooserButton *) xml_->get_widget ("File");
	doientry_ = (Gtk::Entry *) xml_->get_widget ("Doi");
	keyentry_ = (Gtk::Entry *) xml_->get_widget ("Key");
	titleentry_ = (Gtk::Entry *) xml_->get_widget ("Title");
	authorsentry_ = (Gtk::Entry *) xml_->get_widget ("Authors");
	journalentry_ = (Gtk::Entry *) xml_->get_widget ("Journal");
	volumeentry_ = (Gtk::Entry *) xml_->get_widget ("Volume");
	issueentry_ = (Gtk::Entry *) xml_->get_widget ("Issue");
	pagesentry_ = (Gtk::Entry *) xml_->get_widget ("Pages");
	yearentry_ = (Gtk::Entry *) xml_->get_widget ("Year");

	Gtk::VBox *box = (Gtk::VBox *) xml_->get_widget ("Type");
	typecombo_ = Gtk::manage (new Gtk::ComboBoxEntryText);
	box->pack_start (*typecombo_, true, true, 0);
	box->show_all ();

	std::vector<Glib::ustring>::iterator it = BibData::document_types.begin();
	for (; it != BibData::document_types.end(); ++it) {
		typecombo_->append_text (*it);
	}
	typecombo_->set_active_text (BibData::default_document_type);

	crossrefbutton_ = (Gtk::Button *) xml_->get_widget ("CrossRefLookup");
	crossrefbutton_->signal_clicked().connect(
		sigc::mem_fun (*this, &DocumentProperties::onCrossRefLookup));

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
}


bool DocumentProperties::show (Document *doc)
{
	if (!doc) {
		std::cerr << "DocumentProperties::show: NULL doc pointer\n";
		return false;
	}

	doc_ = doc;

	update ();

	int result = dialog_->run ();

	if (result == Gtk::RESPONSE_OK) {
		save ();
	}

	dialog_->hide ();

	if (result == Gtk::RESPONSE_OK)
		return true;
	else
		return false;
}


void DocumentProperties::update ()
{
	BibData &bib = doc_->getBibData();

	filechooser_->set_uri (doc_->getFileName());

	doientry_->set_text (bib.getDoi());
	keyentry_->set_text (doc_->getDisplayName());
	typecombo_->get_entry()->set_text (bib.getType());

	titleentry_->set_text (bib.getTitle());
	authorsentry_->set_text (bib.getAuthors());
	journalentry_->set_text (bib.getJournal());
	volumeentry_->set_text (bib.getVolume());
	issueentry_->set_text (bib.getIssue());
	pagesentry_->set_text (bib.getPages());
	yearentry_->set_text (bib.getYear());

	crossrefbutton_->set_sensitive (!_global_prefs->getWorkOffline ());

	extrafieldsstore_->clear ();
	BibData::ExtrasMap extras = bib.getExtras ();
	BibData::ExtrasMap::iterator it = extras.begin ();
	BibData::ExtrasMap::iterator const end = extras.end ();
	for (; it != end; ++it ) {
		Gtk::ListStore::iterator row = extrafieldsstore_->append ();
		(*row)[extrakeycol_] = (*it).first;
		(*row)[extravalcol_] = (*it).second;
	}
}


void DocumentProperties::save ()
{
	BibData &bib = doc_->getBibData();

	Glib::ustring filename = filechooser_->get_uri ();
	doc_->setFileName (filename);
	doc_->setDisplayName (keyentry_->get_text ());

	bib.setType (typecombo_->get_entry()->get_text());
	bib.setDoi (doientry_->get_text ());
	bib.setTitle (titleentry_->get_text ());
	bib.setAuthors (authorsentry_->get_text ());
	bib.setJournal (journalentry_->get_text ());
	bib.setVolume (volumeentry_->get_text ());
	bib.setIssue (issueentry_->get_text ());
	bib.setPages (pagesentry_->get_text ());
	bib.setYear (yearentry_->get_text ());

	bib.clearExtras ();
	Gtk::ListStore::iterator it = extrafieldsstore_->children().begin ();
	Gtk::ListStore::iterator const end = extrafieldsstore_->children().end ();
	for (; it != end; ++it) {
		bib.addExtra ((*it)[extrakeycol_], (*it)[extravalcol_]);
	}
}


void DocumentProperties::onCrossRefLookup ()
{
	Document *orig = doc_;
	Document spoof = *doc_;
	spoof.getBibData().setDoi (doientry_->get_text ());
	spoof.getBibData().getCrossRef ();
	doc_ = &spoof;
	update ();
	doc_ = orig;
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
	hbox.pack_start (entry, true, true, 0);

	dialog.add_button (Gtk::Stock::CANCEL, 0);
	dialog.add_button (Gtk::Stock::OK, 1);

	dialog.show_all ();
	vbox->set_border_width (12);

	if (dialog.run ()) {
		Gtk::ListStore::iterator row = extrafieldsstore_->append ();
		(*row)[extrakeycol_] = entry.get_text ();
		(*row)[extravalcol_] = "";
	}
}


void DocumentProperties::onDeleteExtraField ()
{
	// Oh dear, this may crash if this button was sensitive at the wrong time
	extrafieldsstore_->erase (extrafieldssel_->get_selected ());
}


void DocumentProperties::onEditExtraField ()
{
	Gtk::TreeSelection::ListHandle_Path paths =
		extrafieldssel_->get_selected_rows ();

	if (paths.empty ()) {
		std::cerr << "Warning: DocumentProperties::onEditExtraField: none selected\n";
		return;
	} else if (paths.size () > 1) {
		std::cerr << "Warning: DocumentProperties::onEditExtraField: too many selected\n";
		return;
	}

	Gtk::TreePath path = (*paths.begin ());
	extrafieldsview_->set_cursor (path, *extrafieldsview_->get_column (1), true);
}


void DocumentProperties::onExtraFieldsSelectionChanged ()
{
	bool const enable = extrafieldssel_->count_selected_rows () > 0;
	deleteextrafieldbutton_->set_sensitive (enable);
	editextrafieldbutton_->set_sensitive (enable);
}
