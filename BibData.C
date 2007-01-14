

#include "BibData.h"

#include <iostream>

#include <time.h>
#include <boost/regex.hpp>

#include <libgnomeuimm.h>
#include <libgnomevfsmm.h>

#include "Preferences.h"

#include "CrossRefParser.h"


/*
 * Dump all fields to standard out
 */
void BibData::print ()
{
	std::cout << "DOI: " << doi_ << std::endl;
	std::cout << "Title: " << title_ << std::endl;
	std::cout << "Authors: " << authors_ << std::endl;
	std::cout << "Journal: " << journal_ << std::endl;
	std::cout << "Volume: " << volume_ << std::endl;
	std::cout << "Number: " << issue_ << std::endl;
	std::cout << "Pages: " << pages_ << std::endl;
	std::cout << "Year: " << year_ << std::endl;
}

using Glib::Markup::escape_text;

void BibData::writeXML (std::ostringstream &out)
{
	out << "    <bib_doi>" << escape_text(doi_) << "</bib_doi>\n";
	out << "    <bib_title>" << escape_text(title_) << "</bib_title>\n";
	out << "    <bib_authors>" << escape_text(authors_) << "</bib_authors>\n";
	out << "    <bib_journal>" << escape_text(journal_) << "</bib_journal>\n";
	out << "    <bib_volume>" << escape_text(volume_) << "</bib_volume>\n";
	out << "    <bib_number>" << escape_text(issue_) << "</bib_number>\n";
	out << "    <bib_pages>" << escape_text(pages_) << "</bib_pages>\n";
	out << "    <bib_year>" << escape_text(year_) << "</bib_year>\n";
}


void BibData::parseCrossRefXML (Glib::ustring const &xml)
{
	CrossRefParser parser (*this);
	Glib::Markup::ParseContext context (parser);
	try {
		context.parse (xml);
	} catch (Glib::MarkupError const ex) {
		std::cerr << "Exception on line " << context.get_line_number () << ", character " << context.get_char_number () << ", code ";
		switch (ex.code()) {
			case Glib::MarkupError::BAD_UTF8:
				std::cerr << "Bad UTF8\n";
				break;
			case Glib::MarkupError::EMPTY:
				std::cerr << "Empty\n";
				break;
			case Glib::MarkupError::PARSE:
				std::cerr << "Parse error\n";
				break;
			default:
				std::cerr << (int)ex.code() << "\n";
		}
	}
	context.end_parse ();
}


/*
 * Parse the XML metadata embedded in some PDFs
 * in the hopes it contains some interesting fields
 */
void BibData::parseMetadata (Glib::ustring const &meta, FieldMask mask)
{
}

/*
 *  Try to fill the journal field by doing a text search
 *  for a known long form of a journal name and then
 *  using the corresponding short form.
 *
 *  Citations always use the short form, so by searching
 *  for the long form we should only pick up the name
 *  of this paper's journal.
 */
void BibData::guessJournal (Glib::ustring const &raw)
{
	int const n_titles = 2;
	Glib::ustring search_titles[n_titles]= {
		"PHYSICAL REVIEW B",
		"JOURNAL OF APPLIED PHYSICS"
	};
	Glib::ustring bib_titles[n_titles] = {
		"Phys. Rev. B",
		"J. Appl. Phys."
	};

	for (int i = 0; i < n_titles; ++i) {
		// TODO: we would like a fast, case insensitive,
		// encoding-correct search.
		if (strstr (raw.c_str(), search_titles[i].c_str())) {
			setJournal (bib_titles[i]);
			break;
		}
	}
}

/*
 * Try to fill the volume, number and pages fields
 * using the raw text -- these are lumped together
 * because in some cases our search expressions
 * may include all three.
 *
 * (Even page? --jcs)
 */
void BibData::guessVolumeNumberPage (Glib::ustring const &raw)
{
		/*
		 * Do regex magic to learn what we can
		 * Interested in strings like:
		 * "VOLUME 95, NUMBER 3"
		 * "PHYSICAL REVIEW B 71, 064413"
		 *
		 * When searching for abbreviations like Vol. 1, No. 2
		 * we must be very careful not to pick up citations
		 * by accident
		 */
}

/*
 * Try to guess the year of the paper from the raw text
 */
void BibData::guessYear (Glib::ustring const &raw_)
{
	/*
	 * Take the greatest four-letter numeric which is between
	 * 1990 and the current year.  This will occasionally
	 * pick up a 'number' or 'page' instead, but should
	 * almost always be the year.
	 */


	std::string const &raw = raw_;

	boost::regex expression("\\W([0-9][0-9][0-9][0-9])\\W");

	int latestyear = 0;
	Glib::ustring latestyear_str;

	int const dawn_of_ejournals = 1990;
	time_t const timesecs = time(NULL);
	struct tm *UTC = gmtime (&timesecs);
	int const present_day = UTC->tm_year + 1900;

	std::string::const_iterator start, end;
	start = raw.begin();
	end = raw.end();
	boost::match_results<std::string::const_iterator> what;
	boost::match_flag_type flags = boost::match_default;
	while(regex_search(start, end, what, expression, flags)) {
		Glib::ustring yearstring = std::string(what[1]);
		int yearval = atoi(yearstring.c_str());
		if (yearval > dawn_of_ejournals && yearval <= present_day) {
			if (yearval > latestyear) {
				latestyear = yearval;
				latestyear_str = yearstring;
			}
		}
	  // update search position:
	  start = what[0].second;
	  // update flags:
	  flags |= boost::match_prev_avail;
	  flags |= boost::match_not_bob;
	}

	if (latestyear)
		setYear (latestyear_str);
}


/*
 * Try to guess the authors of the paper from the raw text
 */
void BibData::guessAuthors (Glib::ustring const &raw_)
{
	std::string const &raw = raw_;

	boost::regex expression("^(\\D{5,}\n\\D{5,})$");

	std::string::const_iterator start, end;
	start = raw.begin();
	end = raw.end();
	boost::match_results<std::string::const_iterator> what;
	boost::match_flag_type flags = boost::match_default;
	while(regex_search(start, end, what, expression, flags)) {
		Glib::ustring authors = std::string(what[1]);
		std::cout << "Got authors = '" << authors << "'\n" << std::endl;
		setAuthors (authors);
		return;

	  // update search position:
	  start = what[0].second;
	  // update flags:
	  flags |= boost::match_prev_avail;
	  flags |= boost::match_not_bob;
	}
}

/*
 * Try to guess the title of the paper from the raw text
 */
void BibData::guessTitle (Glib::ustring const &raw)
{
}


/*
 * Try to guess the DOI of the paper from the raw text
 */
void BibData::guessDoi (Glib::ustring const &raw_)
{
	std::string const &raw = raw_;

	boost::regex expression(
		"[Dd][Oo][Ii]:? *"
		"("
		"[^\\.\\s]+"
		"\\."
		"[^\\/\\s]+"
		"\\/"
		"[^\\s]+"
		")"
	);

	std::string::const_iterator start, end;
	start = raw.begin();
	end = raw.end();
	boost::match_results<std::string::const_iterator> what;
	boost::match_flag_type flags = boost::match_default;
	while(regex_search(start, end, what, expression, flags)) {
		Glib::ustring gstr = std::string(what[1]);
		setDoi (gstr);
		return;
	}
}

static bool transfercomplete;
static bool transferfail;
static Glib::ustring transferresults;

void BibData::onCrossRefCancel ()
{
	transferfail = true;
}

void BibData::getCrossRef ()
{
	std::cerr << ">> BibData::getCrossRef\n";
	if (doi_.empty() || _global_prefs->getWorkOffline())
		return;

	Gtk::Dialog dialog ("Retrieving Metadata", true, false);

	Gtk::VBox *vbox = dialog.get_vbox ();
	vbox->set_spacing (12);

	Glib::ustring messagetext =
		"<b><big>Retrieving metadata</big></b>\n\n"
		"Contacting crossref.org to retrieve metadata for '"
		+ doi_ + "'\n";

	Gtk::Label label ("", false);
	label.set_markup (messagetext);

	vbox->pack_start (label, true, true, 0);

	Gtk::ProgressBar progress;

	vbox->pack_start (progress, false, false, 0);

	Gtk::Button *cancelbutton = dialog.add_button (Gtk::Stock::CANCEL, 0);
	cancelbutton->signal_clicked().connect(
		sigc::mem_fun (*this, &BibData::onCrossRefCancel));

	dialog.show_all ();
	vbox->set_border_width (12);

	transfercomplete = false;
	transferfail = false;

	Glib::Thread *fetcher = Glib::Thread::create (
		sigc::mem_fun (*this, &BibData::fetcherThread), true);

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

	if (!transferfail) {
		parseCrossRefXML (transferresults);
	} else {
		messagetext = "<b><big>Work offline?</big></b>\n\n"
			"There was a problem while retrieving metadata, would you like \n"
			"to work offline?  If you choose to work offline, no further network "
			"operations will be attempted until you choose to work online again "
			"in the Preferences dialog.";
		Gtk::MessageDialog faildialog (
			messagetext,
			true,
			Gtk::MESSAGE_WARNING,
			Gtk::BUTTONS_NONE);

		Gtk::Button *online = faildialog.add_button ("_Stay Online", 0);
		Gtk::Button *offline = faildialog.add_button ("_Go Offline", 1);
		faildialog.set_default_response (1);

		Gtk::Image *onlineicon = Gtk::manage (
			new Gtk::Image (Gtk::Stock::CONNECT, Gtk::ICON_SIZE_BUTTON));
		online->set_image (*onlineicon);
		Gtk::Image *offlineicon = Gtk::manage (
			new Gtk::Image (Gtk::Stock::DISCONNECT, Gtk::ICON_SIZE_BUTTON));
		offline->set_image (*offlineicon);
		
		if (faildialog.run ()) {
			_global_prefs->setWorkOffline (true);
		};
	}
}


static bool advance;
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
static bool waitForFlag (bool &flag)
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


void BibData::fetcherThread ()
{
	Utility::StringPair ends = _global_prefs->getMetadataLookup ();

	Glib::ustring bibfilename =
		ends.first
		+ doi_
		+ ends.second;

	Glib::RefPtr<Gnome::Vfs::Uri> biburi = Gnome::Vfs::Uri::create (bibfilename);

	advance = false;
	try {
		bibfile.open (bibfilename,
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
	// later try and parse incomplete XML, and should will quietly
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


void BibData::clear ()
{
	doi_ = "";
	volume_ = "";
	issue_ = "";
	pages_ = "";
	authors_ = "";
	journal_ = "";
	title_ = "";
	year_ = "";
}
