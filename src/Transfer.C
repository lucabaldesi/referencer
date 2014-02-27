
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#include <iostream>
#include <stdint.h>

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
void fetcherThread (Glib::ustring const &filename, Glib::RefPtr<Gio::Cancellable> cancellable);

void onTransferCancel (Glib::RefPtr<Gio::Cancellable> cancellable)
{
	DEBUG("Cancelling...");
	cancellable->cancel();
}


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

	Glib::RefPtr<Gio::Cancellable> cancellable = Gio::Cancellable::create();

	Gtk::Button *cancelbutton = dialog.add_button (Gtk::Stock::CANCEL, 0);
	cancelbutton->signal_clicked().connect(
		sigc::bind (sigc::ptr_fun (&onTransferCancel), cancellable));

	transferStatus = TRANSFER_NONE;


	Glib::Thread *fetcher = Glib::Thread::create (
		sigc::bind (sigc::ptr_fun (&fetcherThread), filename, cancellable), true);

	Glib::Timer timeout;
	timeout.start ();

	double const maxTimeout = 30.0;
	double const dialogDelay = 1.0;
	bool dialogShown = false;
	while (transferStatus == TRANSFER_NONE) {
		progress.pulse ();
		while (Gtk::Main::events_pending())
			Gtk::Main::iteration ();
		Glib::usleep (100000);

		if (!dialogShown && timeout.elapsed () > dialogDelay) {
			dialog.show_all ();
			vbox->set_border_width (12);
			dialogShown = true;
		}

		if (timeout.elapsed () > maxTimeout) {
			cancellable->cancel();
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

void fetcherThread (Glib::ustring const &filename, Glib::RefPtr<Gio::Cancellable> cancellable)
{
	char *buffer = NULL;
	gsize len = 0;

	try {
		Glib::RefPtr<Gio::File> file =
			Gio::File::create_for_uri (filename);
		file->load_contents(cancellable, buffer, len);
	} catch (const Glib::Error ex) {
		DEBUG ("Got an exception from load_contents", ex.what());
		transferStatus |= TRANSFER_FAIL_SILENT;
		return;
	}

	DEBUG("Received '%1' bytes", len);

	transferresults = Glib::ustring(buffer);

	transferStatus |= TRANSFER_OK;
}


} // namespace Transfer
