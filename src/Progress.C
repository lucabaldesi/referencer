
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */


#include "TagWindow.h"

#include "Progress.h"

#include <libgnomemm/main.h>

#include <iostream>

Progress::Progress (TagWindow &tagwindow)
	: tagwindow_ (tagwindow)
{
	dialog_ = new Gtk::Dialog ();
	dialog_->set_modal (true);
	dialog_->set_has_separator (false);

	Gtk::VBox *vbox = dialog_->get_vbox ();
	Gtk::VBox *myvbox = Gtk::manage (new Gtk::VBox (false, 12));
	vbox->pack_start (*myvbox);
	vbox = myvbox;
	vbox->set_border_width (6);

	//progress_ = Gtk::manage (new Gtk::ProgressBar);
	progress_ = tagwindow_.getProgressBar ();
	label_ = Gtk::manage (new Gtk::Label ("", false));

	vbox->pack_start (*label_, true, true, 0);
	vbox->pack_start (*progress_, false, false, 0);
}


Progress::~Progress ()
{
	if (!finished_)
		finish ();
	delete dialog_;
}


void Progress::setLabel (Glib::ustring const &markup)
{
	label_->set_markup (markup);
	tagwindow_.setStatusText (markup);
}


void Progress::start ()
{
	//dialog_->show_all ();

	// Flag that the loop thread waits for
	finished_ = false;
	
	tagwindow_.setSensitive (false);

	// Spawn the loop thread
	loopthread_ = Glib::Thread::create (
		sigc::mem_fun (this, &Progress::loop), true);
}


void Progress::finish ()
{
	finished_ = true;
	tagwindow_.getProgressBar()->set_fraction (1.0);
	tagwindow_.setSensitive (true);
	loopthread_->join ();
}


void Progress::update (double status)
{
	//std::cerr << "Update: " << status << "\n";
	progress_->set_fraction (status);
	flushEvents ();
}


void Progress::update ()
{
	progress_->pulse ();
	flushEvents ();
}


void Progress::loop ()
{
	while (!finished_) {
		/*getLock ();
		flushEvents ();
		releaseLock ();*/
		Glib::usleep (100000);
	}
}


void Progress::flushEvents ()
{
	//gdk_threads_enter ();
	while (Gnome::Main::events_pending())
		Gnome::Main::iteration ();
	//gdk_threads_leave ();
	//std::cerr << "Refreshed UI\n";
}


void Progress::getLock ()
{
	lock_.lock ();
}


void Progress::releaseLock ()
{
	lock_.unlock ();
}


