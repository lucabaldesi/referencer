
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
	progress_ = tagwindow_.getProgressBar ();
}


Progress::~Progress ()
{
	if (!finished_)
		finish ();
}


void Progress::setLabel (Glib::ustring const &markup)
{
	tagwindow_.setStatusText (markup);
}


void Progress::start ()
{
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
		//flushEvents ();
		Glib::usleep (100000);
	}
}

/*
 * A DEFUNCT note on threading:
 * If the caller is inside a gtk_threads_enter block, for
 * example if the main loop is thus surrounded, then this 
 * is liable to freeze.
*/

void Progress::flushEvents ()
{
	//gdk_threads_enter ();
	while (Gnome::Main::events_pending())
		Gnome::Main::iteration ();
	//gdk_threads_leave ();
}


