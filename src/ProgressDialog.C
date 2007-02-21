

#include "ProgressDialog.h"

ProgressDialog::ProgressDialog()
{
	dialog_ = new Gtk::Dialog ();
	dialog_->set_modal (true);
	dialog_->set_has_separator (false);
	
	Gtk::VBox *vbox = dialog_->get_vbox ();
	vbox->set_spacing (12);
	
	progress_ = Gtk::manage (new Gtk::ProgressBar);
	label_ = Gtk::manage (new Gtk::Label ("", false));
	
	vbox->pack_start (label_, true, true, 0);
	vbox->pack_start (ProgressDialog_, false, false, 0);
}


ProgressDialog::~ProgressDialog ()
{
	delete dialog_;
}


ProgressDialog::setLabel (Glib::ustring const &markup)
{
	label_->set_markup (markup);
}


void ProgressDialog::start ()
{
	dialog_->show_all ();
	dialog_->get_vbox ()->set_border_width (12);
	
	finished_ = false;
	
	// Spawn the loop thread
	
	loopthread_ = Glib::Thread::create (
		sigc::mem_fun (this, &ProgressDialog::fetcherThread), true);
}


void ProgressDialog::finish ()
{
	finished_ = true;
	loopthread_->join ();
}


void ProgressDialog::update (double status)
{
	progress_->set_fraction (status);
}


void ProgressDialog::update ()
{
	progress_->bounce ();
}


void ProgressDialog::loop ()
{
	while (!finished_) {
		while (Gnome::Main::events_pending())
			Gnome::Main::iteration ();
		Glib::usleep (100000);
	}
}
