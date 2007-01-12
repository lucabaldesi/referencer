
#include <iostream>

#include "Preferences.h"


Preferences::Preferences ()
{
	xml_ = Gnome::Glade::Xml::create ("preferences.glade");

	dialog_ = (Gtk::Dialog *) xml_->get_widget ("Preferences");
	workofflinecheck_ = (Gtk::CheckButton *) xml_->get_widget ("WorkOffline");
	workofflinecheck_->signal_toggled().connect (
		sigc::mem_fun (*this, &Preferences::onWorkOfflineToggled));

	doilaunchentry_ = (Gtk::Entry *) xml_->get_widget ("DoiLaunch");
	doilaunchentry_->signal_changed().connect (
		sigc::mem_fun (*this, &Preferences::onURLChanged));
	metadatalookupentry_ = (Gtk::Entry *) xml_->get_widget ("MetadataLookup");
	metadatalookupentry_->signal_changed().connect (
		sigc::mem_fun (*this, &Preferences::onURLChanged));

	Gtk::Button *button = (Gtk::Button *) xml_->get_widget ("ResetToDefaults");
	button->signal_clicked().connect (
		sigc::mem_fun (*this, &Preferences::onResetToDefaults));

	doilaunchdefault_ = "http://dx.doi.org/<!DOI!>";
	metadatalookupdefault_ =
		"http://www.crossref.org/openurl/?id=doi:<!DOI!>&noredirect=true";

}


void Preferences::showDialog ()
{
	workofflinecheck_->set_active (workoffline_);
	doilaunchentry_->set_text (doilaunch_);
	metadatalookupentry_->set_text (metadatalookup_);

	dialog_->run ();
}


void Preferences::onWorkOfflineToggled ()
{
	workoffline_ = workofflinecheck_->get_active ();
	std::cerr << "Preferences: workoffline_ = " << workoffline_ << "\n";
}


void Preferences::onURLChanged ()
{
	// Should check these for validity
	doilaunch_ = doilaunchentry_->get_text ();
	metadatalookup_ = metadatalookupentry_->get_text ();
}


void Preferences::onResetToDefaults ()
{
	doilaunch_ = doilaunchdefault_;
	metadatalookup_ = metadatalookupdefault_;
}
