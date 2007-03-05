
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#include <iostream>

#include <libgnomeuimm.h>
#include <libgnomevfsmm.h>

#include "Preferences.h"

#include "Transfer.h"

namespace Transfer {

static bool transfercomplete;
static bool transferfail;
static Glib::ustring transferresults;

void onTransferCancel ()
{
	transferfail = true;
}

void fetcherThread (Glib::ustring const &filename);

Glib::ustring &getRemoteFile (
	Glib::ustring const &title,
	Glib::ustring const &messagetext,
	Glib::ustring const &filename)
{
	Gtk::Dialog dialog (title, true, false);

	Gtk::VBox *vbox = dialog.get_vbox ();
	vbox->set_spacing (12);

	Gtk::Label label ("", false);
	label.set_markup (messagetext);
	vbox->pack_start (label, true, true, 0);

	Gtk::ProgressBar progress;
	vbox->pack_start (progress, false, false, 0);

	Gtk::Button *cancelbutton = dialog.add_button (Gtk::Stock::CANCEL, 0);
	cancelbutton->signal_clicked().connect(
		sigc::ptr_fun (&onTransferCancel));

	dialog.show_all ();
	vbox->set_border_width (12);

	transfercomplete = false;
	transferfail = false;

	Glib::Thread *fetcher = Glib::Thread::create (
		sigc::bind (sigc::ptr_fun (&fetcherThread), filename), true);

	Glib::Timer timeout;
	timeout.start ();

	double const max_timeout = 10.0;
	while (transfercomplete == false && transferfail == false) {
		progress.pulse ();
		while (Gnome::Main::events_pending())
			Gnome::Main::iteration ();
		Glib::usleep (100000);

		if (timeout.elapsed () > max_timeout) {
			transferfail = true;
			break;
		}
	}

	fetcher->join ();

	if (transferfail) {
		Glib::ustring const messagetext2 = "<b><big>Work offline?</big></b>\n\n"
			"There was a problem while retrieving metadata, would you like \n"
			"to work offline?  If you choose to work offline, no further network "
			"operations will be attempted until you choose to work online again "
			"in the Preferences dialog.";
		Gtk::MessageDialog faildialog (
			messagetext2,
			true,
			Gtk::MESSAGE_WARNING,
			Gtk::BUTTONS_NONE);

		Gtk::Button *online = faildialog.add_button
			("_Stay Online", Gtk::RESPONSE_NO);
		Gtk::Button *offline = faildialog.add_button
			("_Go Offline", Gtk::RESPONSE_YES);
		faildialog.set_default_response (Gtk::RESPONSE_YES);

		Gtk::Image *onlineicon = Gtk::manage (
			new Gtk::Image (Gtk::Stock::CONNECT, Gtk::ICON_SIZE_BUTTON));
		online->set_image (*onlineicon);
		Gtk::Image *offlineicon = Gtk::manage (
			new Gtk::Image (Gtk::Stock::DISCONNECT, Gtk::ICON_SIZE_BUTTON));
		offline->set_image (*offlineicon);

		if (faildialog.run () == Gtk::RESPONSE_YES) {
			_global_prefs->setWorkOffline (true);
		};
		throw Exception ("Transfer failed\n");
	}

	return transferresults;
}


volatile static bool advance;
Gnome::Vfs::Async::Handle bibfile;

void openCB (
	Gnome::Vfs::Async::Handle const &handle,
	Gnome::Vfs::Result result)
{
	if (result == Gnome::Vfs::OK) {
		std::cerr << "openCB: result OK, opened\n";
		advance = true;
	} else {
		std::cerr << "openCB: result not OK\n";
		transferfail = true;
	}
}


void readCB (
	Gnome::Vfs::Async::Handle const &handle,
	Gnome::Vfs::Result result,
	gpointer buffer,
	Gnome::Vfs::FileSize requested,
	Gnome::Vfs::FileSize reallyread)
{
	std::cerr << "Woo, read " << reallyread << " bytes\n";
	char *charbuf = (char *) buffer;
	charbuf [reallyread] = 0;
	if (result == Gnome::Vfs::OK) {
		std::cerr << "readCB: result OK, read\n";
		advance = true;
	} else {
		std::cerr << "readCB: result not OK\n";
		transferfail = true;
	}
}


void closeCB (
	Gnome::Vfs::Async::Handle const &handle,
	Gnome::Vfs::Result result)
{
	if (result == Gnome::Vfs::OK) {
		std::cerr << "readCB: result OK, closed\n";
	} else {
		// Can't really do much about this
		std::cerr << "closeCB: warning: result not OK\n";
	}

	advance = true;
}


// Return true if all is well
// Return false if we should give up and go home
static bool waitForFlag (volatile bool &flag)
{
	while (flag == false) {
		std::cerr << "Waiting...\n";
		Glib::usleep (100000);
		if (transferfail) {
			// The parent decided we've timed out or cancelled and should give up
			//     libgnomevfsmm-WARNING **: gnome-vfsmm Async::Handle::cancel():
			//     This method currently leaks memory
			bibfile.cancel ();
			// Should close it if it's open
			std::cerr << "waitForFlag completed due to transferfail\n";
			return true;
		}
	}
	std::cerr << "Done!\n";
	return false;
}


void fetcherThread (Glib::ustring const &filename)
{
	Glib::RefPtr<Gnome::Vfs::Uri> biburi = Gnome::Vfs::Uri::create (filename);

	advance = false;
	try {
		bibfile.open (filename,
		              Gnome::Vfs::OPEN_READ,
		              0,
		              sigc::ptr_fun(&openCB));
	} catch (const Gnome::Vfs::exception ex) {
		std::cerr << "Got an exception from open\n";
		transferfail = true;
		Utility::exceptionDialog (&ex, "opening file on server");
		return;
	}

	if (waitForFlag (advance))
		// Opening failed
		return;

	// Crossref can fuck off if it thinks the metadata for a
	// paper is more than 256kB.  If it really is then we will
	// later try and parse incomplete XML, and should fail
	// there probably.
	int const maxsize = 1024 * 256;
	char *buffer = (char *) malloc (sizeof (char) * maxsize);

	advance = false;
	try {
		bibfile.read (buffer, maxsize, sigc::ptr_fun (&readCB));
	} catch (const Gnome::Vfs::exception ex) {
		std::cerr << "Got an exception from read\n";
		// should close handle?
		transferfail = true;
		Utility::exceptionDialog (&ex, "reading from file on server");
		return;
	}

	if (waitForFlag (advance))
		// Aargh shouldn't we be closing?
		return;

	transferresults = buffer;
	free (buffer);
	advance = false;
	try {
		bibfile.close (sigc::ptr_fun (&closeCB));
	} catch (const Gnome::Vfs::exception ex) {
		std::cerr << "Got an exception from close\n";
		transferfail = true;
		Utility::exceptionDialog (&ex, "closing file on server");
		return;
	}

	if (waitForFlag (advance))
		return;

	transfercomplete = true;
}


} // namespace Transfer
