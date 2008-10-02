
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#include <iostream>
#include <cstring>

#include <libgnomevfsmm.h>
#include <glibmm/i18n.h>

#include "TagList.h"
#include "DocumentList.h"
#include "LibraryParser.h"
#include "Progress.h"
#include "Utility.h"

#include "Library.h"


Library::Library (RefWindow &tagwindow)
	: tagwindow_ (tagwindow)
{
	doclist_ = new DocumentList ();
	taglist_ = new TagList ();
	manage_braces_ = false;
	manage_utf8_ = false;
}


Library::~Library ()
{
	delete doclist_;
	delete taglist_;
}


Glib::ustring Library::writeXML ()
{
	/* FIXME: composing entire file in memory before writing to 
	 * disk, this would fall over for really massive files */

	Glib::ustring out;

	out += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	out += "<library>\n";

	out += Glib::ustring("<manage_target")
		+ " braces=\""
		+ (manage_braces_ ? "true" : "false")
		+ "\" utf8=\""
		+ (manage_utf8_ ? "true" : "false")
		+ "\">"
		+ Glib::Markup::escape_text (manage_target_)
		+ "</manage_target>\n";

	out += Glib::ustring("<library_folder")
		+ " monitor=\""
		+ (library_folder_monitor_ ? "true" : "false")
		+ "\">"
		+ Glib::Markup::escape_text (library_folder_uri_)
		+ "</library_folder>\n";

	taglist_->writeXML (out);
	doclist_->writeXML (out);

	out += "</library>\n";

	return out;
}


bool Library::readXML (Glib::ustring XMLtext)
{
	TagList *newtags = new TagList ();
	DocumentList *newdocs = new DocumentList ();

	LibraryParser parser (*this, *newtags, *newdocs);
	Glib::Markup::ParseContext context (parser);
	try {
		context.parse (XMLtext);
	} catch (Glib::MarkupError const ex) {
		DEBUG3 ("Exception on line %1, character %2: '%3'",
			context.get_line_number (), context.get_char_number (), ex.what ());
		Utility::exceptionDialog (&ex, _("Parsing Library XML"));

		delete newtags;
		delete newdocs;
		return false;
	}

	context.end_parse ();

	delete taglist_;
	taglist_ = newtags;

	delete doclist_;
	doclist_ = newdocs;

	return true;
}


void Library::clear ()
{
	taglist_->clear ();
	doclist_->clear ();
	manage_target_ = "";
	manage_braces_ = false;
	manage_utf8_ = false;
}


/**
 * Show a dialog prompting the user for a folder in which
 * to download documents and optionally to monitor for new documents
 * Returns true if there was a change.
 */
bool Library::libraryFolderDialog ()
{
	Glib::RefPtr<Gnome::Glade::Xml> xml = Gnome::Glade::Xml::create (
			Utility::findDataFile ("libraryfolder.glade"));

	Gtk::FileChooserButton *location =
		(Gtk::FileChooserButton *) xml->get_widget ("Location");

	Gtk::CheckButton *monitor =
		(Gtk::CheckButton *) xml->get_widget ("AutomaticallyAddDocuments");

	Gtk::Dialog *dialog = 
		(Gtk::Dialog *) xml->get_widget ("LibraryFolder");
 
	bool oldMonitorState = library_folder_monitor_;
	Glib::ustring const oldFolder = library_folder_uri_;

	monitor->set_active (library_folder_monitor_);	
	location->select_uri (library_folder_uri_);

	dialog->run ();

	library_folder_monitor_ = monitor->get_active ();
	if (!location->get_uri().empty())
	        library_folder_uri_ = location->get_uri ();

	DEBUG (library_folder_uri_);

	dialog->hide ();
	return ((oldMonitorState != library_folder_monitor_) || 
		(oldFolder != library_folder_uri_));
}



// True on success
bool Library::load (Glib::ustring const &libfilename)
{
	Gnome::Vfs::Handle libfile;

	Glib::RefPtr<Gnome::Vfs::Uri> liburi = Gnome::Vfs::Uri::create (libfilename);

	Progress progress (tagwindow_);

	progress.start (String::ucompose (
		_("Opening %1"),
		liburi->extract_short_name ()));

	try {
		libfile.open (libfilename, Gnome::Vfs::OPEN_READ);
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "opening library '"
			+ Gnome::Vfs::Uri::format_for_display (libfilename) + "'");
		return false;
	}

	Glib::RefPtr<Gnome::Vfs::FileInfo> fileinfo;
	fileinfo = libfile.get_file_info ();

	char *buffer = (char *) malloc (sizeof(char) * (fileinfo->get_size() + 1));
	if (!buffer) {
		DEBUG ("Warning: RefWindow::loadLibrary: couldn't allocate buffer");
		return false;
	}

	try {
		libfile.read (buffer, fileinfo->get_size());
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "reading library '"
			+ Gnome::Vfs::Uri::format_for_display (libfilename) + "'");
		free (buffer);
		return false;
	}
	buffer[fileinfo->get_size()] = 0;

	progress.update (0.1);

	Glib::ustring rawtext = buffer;
	free (buffer);
	libfile.close ();

	DEBUG ("Reading XML...");
	if (!readXML (rawtext)) {
		return false;
	}
	DEBUG (String::ucompose ("Done, got %1 docs", doclist_->getDocs().size()));

	progress.update (0.2);


	// Resolve relative paths
	// ======================

	typedef enum {
		NONE,
		ORIG_LOC,
		NEW_LOC
	} ambiguouschoice;

	ambiguouschoice ambiguous_action_all = NONE;

	int i = 0;
	DocumentList::Container &docs = doclist_->getDocs ();
	DocumentList::Container::iterator docit = docs.begin ();
	DocumentList::Container::iterator const docend = docs.end ();
	for (; docit != docend; ++docit) {
		progress.update (0.2 + ((double)(i++) / (double)docs.size ()) * 0.8);

		Glib::ustring const absfilename = docit->getFileName();

		Glib::ustring relfilename;
		if (!docit->getRelFileName().empty()) {
			relfilename = Glib::build_filename (
				Glib::path_get_dirname (libfilename),
				docit->getRelFileName());
		}
		
		bool const absexists = Utility::fileExists (absfilename);
		bool const relexists = Utility::fileExists (relfilename);

		if (!absexists && relexists) {
			docit->setFileName (relfilename);
		} else if (absexists && relexists && relfilename != absfilename) {
			ambiguouschoice action = ambiguous_action_all;
			if (action == NONE) {
				// Put up some UI to ask the user which one they want
				Glib::ustring const shortname =
					Gnome::Vfs::Uri::create (absfilename)->extract_short_name ();

				Glib::ustring const message =
					String::ucompose (
						"<b><big>%1</big></b>\n\n%2",
						_("Document location ambiguity"),
						String::ucompose (
							_("The file '%1' exists in two locations:\n"
							"\tOriginal: <b>%2</b>\n\tNew: <b>%3</b>\n\n"
							"Do you want to keep the original location, or update it "
							"to the new one?"
							),
							shortname,
							Gnome::Vfs::Uri::format_for_display (absfilename),
							Gnome::Vfs::Uri::format_for_display (relfilename)
						)
					);

				Gtk::MessageDialog dialog (
					message, true,
					Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE,
					true);

				Gtk::CheckButton applytoall (_("Apply choice to all ambiguous locations"));
				dialog.get_vbox ()->pack_start (applytoall, 0, false, false);
				applytoall.show ();

				dialog.add_button (_("Keep _Original"), ORIG_LOC);
				dialog.add_button (_("Use _New"), NEW_LOC);
				dialog.set_default_response (ORIG_LOC);

				action = (ambiguouschoice) dialog.run ();
				if (applytoall.get_active ()) {
					ambiguous_action_all = action;
				}
			}

			if (action == NEW_LOC) {
				docit->setFileName (relfilename);
			}
		}
	}

	progress.finish ();

	return true;
}


// True on success
bool Library::save (Glib::ustring const &libfilename)
{
	Gnome::Vfs::Handle libfile;

	Glib::ustring tmplibfilename = libfilename + ".save-tmp";

	DEBUG2 ("%1 ~ %2", libfilename, tmplibfilename);

	try {
		libfile.create (tmplibfilename, Gnome::Vfs::OPEN_WRITE,
			false, Gnome::Vfs::PERM_USER_READ | Gnome::Vfs::PERM_USER_WRITE);
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "opening file '" + tmplibfilename + "'");
		return false;
	}


	DEBUG ("Updating relative filenames...");
	DocumentList::Container &docs = doclist_->getDocs ();
	DocumentList::Container::iterator docit = docs.begin ();
	DocumentList::Container::iterator const docend = docs.end ();
	for (; docit != docend; ++docit) {
		if (Utility::fileExists (docit->getFileName())) {
			docit->updateRelFileName (libfilename);
		}
	}
	DEBUG ("Done.");

	DEBUG ("Generating XML...");
	Glib::ustring rawtext = writeXML ();
	DEBUG ("Done.");

	try {
		libfile.write (rawtext.c_str(), strlen(rawtext.c_str()));
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "writing to file '" + tmplibfilename + "'");
		return false;
	}

	try {
	    libfile.close ();
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "Closing file '" + tmplibfilename + "'");
		return false;
	}

	// Forcefully move our tmp file into its real position
	try {
		Gnome::Vfs::Handle::move (tmplibfilename, libfilename, true);
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "moving '"
			+ tmplibfilename + "' to '" + libfilename + "'");
		return false;
	}

	DEBUG1 ("Writing bibtex, manage_target_ = %1", manage_target_);
	// Having successfully saved the library, write the bibtex if needed
	if (!manage_target_.empty ()) {
		// manage_target_ is either an absolute URI or a relative URI
		Glib::ustring const bibtextarget =
			Gnome::Vfs::Uri::make_full_from_relative (
				libfilename,
				manage_target_);
		DEBUG1 ("bibtextarget = %1", bibtextarget);

		std::vector<Document*> docs;
		DocumentList::Container &docrefs = doclist_->getDocs ();
		DocumentList::Container::iterator it = docrefs.begin();
		DocumentList::Container::iterator const end = docrefs.end();
		for (; it != end; it++) {
			docs.push_back(&(*it));
		}
		try {
		    writeBibtex (bibtextarget, docs, manage_braces_, manage_utf8_);
		} catch (Glib::Exception const &ex) {
			Utility::exceptionDialog (&ex, "writing bibtex to " + bibtextarget);
			return false;
		}
	}
	DEBUG ("Done.");

	return true;
}


void Library::writeBibtex (
	Glib::ustring const &bibfilename,
	std::vector<Document*> const &docs,
	bool const usebraces,
	bool const utf8)
{
	Glib::ustring tmpbibfilename = bibfilename + ".export-tmp";

	DEBUG2 ("%1 ~ %2", bibfilename, tmpbibfilename);

	Gnome::Vfs::Handle bibfile;

	try {
		bibfile.create (tmpbibfilename, Gnome::Vfs::OPEN_WRITE,
			false, Gnome::Vfs::PERM_USER_READ | Gnome::Vfs::PERM_USER_WRITE);
	} catch (const Gnome::Vfs::exception ex) {
		DEBUG ("exception in create " + ex.what());
		Utility::exceptionDialog (&ex, "opening BibTex file");
		return;
	}

	std::ostringstream bibtext;

	std::vector<Document*>::const_iterator it = docs.begin ();
	std::vector<Document*>::const_iterator const end = docs.end ();
	for (; it != end; ++it) {
		(*it)->writeBibtex (*this, bibtext, usebraces, utf8);
	}

	try {
		bibfile.write (bibtext.str().c_str(), strlen(bibtext.str().c_str()));
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "writing to BibTex file");
		bibfile.close ();
		return;
	}

	bibfile.close ();

	try {
	    // Forcefully move our tmp file into its real position
	    Gnome::Vfs::Handle::move (tmpbibfilename, bibfilename, true);
	} catch (Glib::Exception const &ex) {
		Utility::exceptionDialog (&ex, "moving bibtex file from " + tmpbibfilename + " to " + bibfilename);
		return;
	}
}


void Library::manageBibtex (
	Glib::ustring const &target,
	bool const braces,
	bool const utf8)
{
	manage_target_ = target;
	manage_braces_ = braces;
	manage_utf8_ = utf8;
}

