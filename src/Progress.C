
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#include <iostream>

#include "RefWindow.h"

#include "Progress.h"


Progress::Progress (RefWindow &refwindow)
	: win_ (refwindow)
{
	finished_ = true;
}


Progress::~Progress ()
{
	if (!finished_)
		finish ();
}


void Progress::start (Glib::ustring const &text)
{
	// Flag that the loop thread waits for
	finished_ = false;
	win_.setSensitive (false);
	win_.getProgressBar()->set_fraction (0.0);
	msgid_ = win_.getStatusBar()->push (text);
}


void Progress::finish ()
{
	finished_ = true;
	win_.getProgressBar()->set_fraction (1.0);
	win_.setSensitive (true);
	win_.getStatusBar()->remove_message (msgid_);
}


void Progress::update (double status)
{
	win_.getProgressBar()->set_fraction (status);
	flushEvents ();
}


void Progress::update ()
{
	win_.getProgressBar()->pulse ();
	flushEvents ();
}


void Progress::flushEvents ()
{
	while (Gtk::Main::events_pending())
		Gtk::Main::iteration ();
}


