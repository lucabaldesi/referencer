
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */




#include "ProgressDialog.h"

#include <libgnomemm/main.h>

#include <iostream>

ProgressDialog::ProgressDialog ()
{
	dialog_ = new Gtk::Dialog ();
	dialog_->set_modal (true);
	dialog_->set_has_separator (false);

	Gtk::VBox *vbox = dialog_->get_vbox ();
	Gtk::VBox *myvbox = Gtk::manage (new Gtk::VBox (false, 12));
	vbox->pack_start (*myvbox);
	vbox = myvbox;
	vbox->set_border_width (6);

	progress_ = Gtk::manage (new Gtk::ProgressBar);
	label_ = Gtk::manage (new Gtk::Label ("", false));

	vbox->pack_start (*label_, true, true, 0);
	vbox->pack_start (*progress_, false, false, 0);
}


ProgressDialog::~ProgressDialog ()
{
	if (!finished_)
		finish ();
	delete dialog_;
}


void ProgressDialog::setLabel (Glib::ustring const &markup)
{
	label_->set_markup (markup);
}


void ProgressDialog::start ()
{
	dialog_->show_all ();

	// Flag that the loop thread waits for
	finished_ = false;

	// Spawn the loop thread
	loopthread_ = Glib::Thread::create (
		sigc::mem_fun (this, &ProgressDialog::loop), true);
}


void ProgressDialog::finish ()
{
	finished_ = true;
	loopthread_->join ();
}


void ProgressDialog::update (double status)
{
	//std::cerr << "Update: " << status << "\n";
	progress_->set_fraction (status);
	flushEvents ();
}


void ProgressDialog::update ()
{
	progress_->pulse ();
	flushEvents ();
}


void ProgressDialog::loop ()
{
	while (!finished_) {
		getLock ();
		flushEvents ();
		releaseLock ();
		Glib::usleep (100000);
	}
}


void ProgressDialog::flushEvents ()
{
	while (Gnome::Main::events_pending())
		Gnome::Main::iteration ();
	//std::cerr << "Refreshed UI\n";
}


void ProgressDialog::getLock ()
{
	lock_.lock ();
}


void ProgressDialog::releaseLock ()
{
	lock_.unlock ();
}


