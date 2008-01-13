
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
#include <glibmm/i18n.h>
#include "ucompose.hpp"

#include "Preferences.h"
#include "Progress.h"

#include "Transfer.h"

namespace Transfer {

typedef enum {
	TRANSFER_NONE = 0,
	TRANSFER_OK = 1,
	TRANSFER_FAIL_SILENT = 2,
	TRANSFER_FAIL_LOUD = 4
} Status;

static uint64_t transferStatus;

static Glib::ustring transferresults;
void fetcherThread (Glib::ustring const &filename);

void onTransferCancel ()
	{transferStatus |= TRANSFER_FAIL_SILENT;}


void promptWorkOffline ()
{
	Glib::ustring const messagetext2 = String::ucompose (
		"<b><big>%1</big></b>\n\n%2",
		_("Work offline?"),
		_("There was a problem while retrieving metadata, would you like \n"
		"to work offline?  If you choose to work offline, no further network "
		"operations will be attempted until you choose to work online again "
		"in the Preferences dialog."));
	Gtk::MessageDialog faildialog (
		messagetext2,
		true,
		Gtk::MESSAGE_WARNING,
		Gtk::BUTTONS_NONE);

	Gtk::Button *online = faildialog.add_button
		(_("_Stay Online"), Gtk::RESPONSE_NO);
	Gtk::Button *offline = faildialog.add_button
		(_("_Go Offline"), Gtk::RESPONSE_YES);
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
}


bool on_transfer_progress(
	const Gnome::Vfs::Transfer::ProgressInfo& info,
	Progress &progress)
{
	if (info.get_phase() == Gnome::Vfs::XFER_PHASE_COPYING) {
		progress.update (
			(double)info.get_bytes_copied()
			/ (double)info.get_file_size());
	} else {
		progress.update ();
	}
	
	return true;
}

void downloadRemoteFile (
	Glib::ustring const &source,
	Glib::ustring const &dest,
	Progress &progress)
{
	Glib::RefPtr<Gnome::Vfs::Uri> srcuri = Gnome::Vfs::Uri::create (source);
	Glib::RefPtr<Gnome::Vfs::Uri> desturi = Gnome::Vfs::Uri::create (dest);

	progress.start (String::ucompose(
		_("Downloading %1"),
		srcuri->extract_short_name()));
	try {
		Gnome::Vfs::Transfer::transfer (
			srcuri,
			desturi,
			Gnome::Vfs::XFER_DEFAULT,
			Gnome::Vfs::XFER_ERROR_MODE_ABORT,
			Gnome::Vfs::XFER_OVERWRITE_MODE_SKIP,
			sigc::bind (
				sigc::ptr_fun (&on_transfer_progress),
				sigc::ref(progress))
			);
	} catch (const Gnome::Vfs::exception ex) {
		progress.finish ();
		throw ex;
	}
	progress.finish ();
}


Glib::ustring &readRemoteFile (
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

	transferStatus = TRANSFER_NONE;

	Glib::Thread *fetcher = Glib::Thread::create (
		sigc::bind (sigc::ptr_fun (&fetcherThread), filename), true);

	Glib::Timer timeout;
	timeout.start ();

	double const maxTimeout = 10.0;
	double const dialogDelay = 1.0;
	bool dialogShown = false;
	while (transferStatus == TRANSFER_NONE) {
		progress.pulse ();
		while (Gnome::Main::events_pending())
			Gnome::Main::iteration ();
		Glib::usleep (100000);

		if (!dialogShown && timeout.elapsed () > dialogDelay) {
			dialog.show_all ();
			vbox->set_border_width (12);
			dialogShown = true;
		}

		if (timeout.elapsed () > maxTimeout) {
			transferStatus |= TRANSFER_FAIL_SILENT;
			break;
		}
	}

	fetcher->join ();

	if (!(transferStatus & TRANSFER_OK)) {
		/*
		promptWorkOffline ();
		throw Exception ("Transfer failed\n");
		*/
		transferresults = "";
		return transferresults;
	} else {
		return transferresults;
	}
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
		transferStatus |= TRANSFER_FAIL_SILENT;
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
		std::cerr << "readCB: result OK\n";
		if (reallyread > 0) {
			Gnome::Vfs::Async::Handle &h = const_cast<Gnome::Vfs::Async::Handle&> (handle);
			h.read ((char*)buffer + reallyread, 1024 * 256 - reallyread, sigc::ptr_fun (&readCB));
		} else {
			advance = true;
		}
	} else  {
		if (result == Gnome::Vfs::ERROR_EOF) {
			std::cerr << "readCB: EOF\n";
			advance = true;
		} else {
			std::cerr << "readCB: result error\n";
			transferStatus |= TRANSFER_FAIL_SILENT;
		}
	}
}


void closeCB (
	Gnome::Vfs::Async::Handle const &handle,
	Gnome::Vfs::Result result)
{
	if (result == Gnome::Vfs::OK) {
		std::cerr << "closeCB: result OK, closed\n";
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
		if ((transferStatus & TRANSFER_FAIL_SILENT) | (transferStatus & TRANSFER_FAIL_LOUD)) {
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
		transferStatus |= TRANSFER_FAIL_SILENT;
		Utility::exceptionDialog (&ex,
			String::ucompose (
				_("Opening URI '%1' on server"),
				Gnome::Vfs::unescape_string_for_display (filename)));
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
		transferStatus |= TRANSFER_FAIL_SILENT;
		Utility::exceptionDialog (&ex,
			String::ucompose (
				_("Reading URI '%1' on server"),
				Gnome::Vfs::unescape_string_for_display (filename)));
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
		transferStatus |= TRANSFER_FAIL_SILENT;
		Utility::exceptionDialog (&ex,
			String::ucompose (
				_("Closing URI '%1' on server"),
				Gnome::Vfs::unescape_string_for_display (filename)));
		return;
	}

	if (waitForFlag (advance))
		return;

	transferStatus |= TRANSFER_OK;
}


} // namespace Transfer
