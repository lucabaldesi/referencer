
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef PROGRESS_H
#define PROGRESS_H

#include <gtkmm.h>

class TagWindow;

class Progress {
	public:
	Progress (TagWindow &tagwindow);
	~Progress ();

	void setLabel (Glib::ustring const &text);
	void start ();
	void update (double status);
	void update ();
	void finish ();

	private:
	void loop ();
	void flushEvents ();

	Gtk::ProgressBar *progress_;
	bool finished_;
	Glib::Thread *loopthread_;
	TagWindow &tagwindow_;
};

#endif

/*

{

	Progress prog (window_);

	Progress.setLabel ("Working, please show some patience");
	Progress.start ();

	for (int i = 0; i < 100; ++i) {
		Progress.update ((double)i / 100.0);
		fuck_about_a_bit (i);
	}

	Progress.finish ();

}

*/
