
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

	crossrefbutton_ = (Gtk::Button *) xml_->get_widget ("CrossRefLookup");
	crossrefbutton_->signal_clicked().connect(
		sigc::mem_fun (*this, &DocumentProperties::onCrossRefLookup));
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

	titleentry_->set_text (bib.getTitle());
	authorsentry_->set_text (bib.getAuthors());
	journalentry_->set_text (bib.getJournal());
	volumeentry_->set_text (bib.getVolume());
	issueentry_->set_text (bib.getIssue());
	pagesentry_->set_text (bib.getPages());
	yearentry_->set_text (bib.getYear());

	crossrefbutton_->set_sensitive (!_global_prefs->getWorkOffline ());
}


void DocumentProperties::save ()
{
	BibData &bib = doc_->getBibData();

	Glib::ustring filename = filechooser_->get_uri ();
	doc_->setFileName (filename);
	doc_->setDisplayName (keyentry_->get_text ());

	bib.setDoi (doientry_->get_text ());
	bib.setTitle (titleentry_->get_text ());
	bib.setAuthors (authorsentry_->get_text ());
	bib.setJournal (journalentry_->get_text ());
	bib.setVolume (volumeentry_->get_text ());
	bib.setIssue (issueentry_->get_text ());
	bib.setPages (pagesentry_->get_text ());
	bib.setYear (yearentry_->get_text ());
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
