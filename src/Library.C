
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
		DEBUG ("Exception on line %1, character %2: '%3'",
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
	Glib::RefPtr<Gtk::Builder> xml = Gtk::Builder::create_from_file 
		(Utility::findDataFile ("libraryfolder.ui"));

	Gtk::FileChooserButton *location;
		xml->get_widget ("Location", location);

	Gtk::CheckButton *monitor;
		xml->get_widget ("AutomaticallyAddDocuments", monitor);

	Gtk::Dialog *dialog;
		xml->get_widget ("LibraryFolder", dialog);
 
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
	Glib::RefPtr<Gio::File> libfile = Gio::File::create_for_uri (libfilename);
	Glib::RefPtr<Gio::FileInfo> fileinfo = libfile->query_info ();
	Glib::RefPtr<Gio::FileInputStream> libfile_is;

	Progress progress (tagwindow_);

	progress.start (String::ucompose (
		_("Opening %1"),
		fileinfo->get_display_name ()));

	try {
		libfile_is = libfile->read();
	} catch (const Gio::Error ex) {
		Utility::exceptionDialog (&ex, "opening library '"
			+ libfile->get_parse_name() + "'");
		return false;
	}

	char *buffer = (char *) malloc (sizeof(char) * (fileinfo->get_size() + 1));
	gsize bytes_read = 0;
	if (!buffer) {
		DEBUG ("Warning: RefWindow::loadLibrary: couldn't allocate buffer");
		return false;
	}

	try {
		libfile_is->read_all (buffer, fileinfo->get_size(), bytes_read);
	} catch (const Gio::Error ex) {
		Utility::exceptionDialog (&ex, "reading library '"
			+ libfile->get_parse_name() + "'");
		free (buffer);
		return false;
	}
	buffer[bytes_read] = 0;

	progress.update (0.1);

	Glib::ustring rawtext = buffer;
	free (buffer);
	libfile_is->close();

	DEBUG ("Reading XML...");
	if (!readXML (rawtext)) {
		return false;
	}
	DEBUG (String::ucompose ("Done, got %1 docs", doclist_->getDocs().size()));

	progress.update (0.2);

	// Set filename_ on document based on relfilename
	int i = 0;
	DocumentList::Container &docs = doclist_->getDocs ();
	DocumentList::Container::iterator docit = docs.begin ();
	DocumentList::Container::iterator const docend = docs.end ();
	for (; docit != docend; ++docit) {
		progress.update (0.2 + ((double)(i++) / (double)docs.size ()) * 0.8);

		if (!docit->getRelFileName().empty()) {
			Glib::ustring full_filename;
			full_filename = Glib::build_filename (
				Glib::path_get_dirname (libfilename),
				docit->getRelFileName());
			docit->setFileName(full_filename);
		}
	}

	progress.finish ();

	return true;
}


// True on success
bool Library::save (Glib::ustring const &libfilename)
{
	DEBUG ("Saving to %1", libfilename);
	
	Glib::RefPtr<Gio::File> libfile = Gio::File::create_for_uri (libfilename);

	DEBUG ("Updating relative filenames...");
	DocumentList::Container &docs = doclist_->getDocs ();
	DocumentList::Container::iterator docit = docs.begin ();
	DocumentList::Container::iterator const docend = docs.end ();
	for (; docit != docend; ++docit) {
		docit->updateRelFileName (libfilename);
	}
	DEBUG ("Done.");

	DEBUG ("Generating XML...");
	Glib::ustring rawtext = writeXML ();
	DEBUG ("Done.");

	try {
		std::string new_etag;
		libfile->replace_contents (rawtext, "", new_etag);
	} catch (const Gio::Error ex) {
		Utility::exceptionDialog (&ex, "replacing contents of file '" + libfilename + "'");
		return false;
	}

	DEBUG ("Writing bibtex, manage_target_ = %1", manage_target_);
	// Having successfully saved the library, write the bibtex if needed
	if (!manage_target_.empty ()) {
		// manage_target_ is either an absolute URI or a relative URI
		Glib::ustring bibtextarget_uri;
		if (Glib::uri_parse_scheme(manage_target_) != "") //absolute URI
			bibtextarget_uri = manage_target_;
		else
			bibtextarget_uri = libfile->get_parent()->resolve_relative_path(manage_target_)->get_uri();

		DEBUG ("bibtextarget_uri = %1", bibtextarget_uri);

		std::vector<Document*> docs;
		DocumentList::Container &docrefs = doclist_->getDocs ();
		DocumentList::Container::iterator it = docrefs.begin();
		DocumentList::Container::iterator const end = docrefs.end();
		for (; it != end; it++) {
			docs.push_back(&(*it));
		}
		try {
		    writeBibtex (bibtextarget_uri, docs, manage_braces_, manage_utf8_);
		} catch (Glib::Exception const &ex) {
			Utility::exceptionDialog (&ex, "writing bibtex to " + bibtextarget_uri);
			return false;
		}
	}
	DEBUG ("Done.");

	return true;
}


void Library::writeBibtex (
	Glib::ustring const &biburi,
	std::vector<Document*> const &docs,
	bool const usebraces,
	bool const utf8)
{
	DEBUG ("Writing BibTex to %1", biburi);

	Glib::RefPtr<Gio::File> bibfile = Gio::File::create_for_uri (biburi);

	std::ostringstream bibtext;

	std::vector<Document*>::const_iterator it = docs.begin ();
	std::vector<Document*>::const_iterator const end = docs.end ();
	for (; it != end; ++it) {
		(*it)->writeBibtex (*this, bibtext, usebraces, utf8);
	}

	try {
		std::string new_etag;
		bibfile->replace_contents (bibtext.str(), "", new_etag);
	} catch (const Gio::Error ex) {
		Utility::exceptionDialog (&ex, "writing to BibTex file");
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

