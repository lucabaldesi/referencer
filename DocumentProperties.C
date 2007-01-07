
#include <iostream>

#include "Document.h"
#include "DocumentProperties.h"

DocumentProperties::DocumentProperties ()
{
	xml_ = Gnome::Glade::Xml::create ("documentproperties.glade");
}


void DocumentProperties::show (Document *doc)
{
	if (!doc) {
		std::cerr << "DocumentProperties::show: NULL doc pointer\n";
		return;
	}

	dialog_ = (Gtk::Dialog *) xml_->get_widget ("DocumentProperties");

	BibData &bib = doc->getBibData();

	doientry_ = (Gtk::Entry *) xml_->get_widget ("Doi");
	doientry_->set_text (bib.getDoi());
	keyentry_ = (Gtk::Entry *) xml_->get_widget ("Key");
	keyentry_->set_text (doc->getDisplayName());

	dialog_->run ();

	dialog_->hide ();
}
