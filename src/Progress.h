
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

class RefWindow;

class Progress {
	public:
	Progress (RefWindow &tagwindow);
	~Progress ();

	void start (Glib::ustring const &text);
	void update (double status);
	void update ();
	void finish ();

	private:
	void flushEvents ();

	bool finished_;
	RefWindow &win_;
	int msgid_;
};

#endif

