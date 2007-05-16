
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#include <iostream>

#include <libgnomevfsmm.h>
#include <glibmm/i18n.h>

#include "TagList.h"
#include "DocumentList.h"
#include "LibraryParser.h"
#include "ProgressDialog.h"
#include "Utility.h"

#include "Library.h"


Library::Library (TagWindow &tagwindow)
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
	std::ostringstream out;
	
	out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	out << "<library>\n";
	
	out << "<manage_target"
		<< " braces=\""
		<< (manage_braces_ ? "true" : "false")
		<< "\" utf8=\""
		<< (manage_utf8_ ? "true" : "false")
	<< "\">"
	<< Glib::Markup::escape_text (manage_target_)
	<< "</manage_target>\n";
	
	taglist_->writeXML (out);
	doclist_->writeXML (out);
	
	out << "</library>\n";
	
	return out.str ();
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
		std::cerr << "Exception on line " << context.get_line_number () << ", character " << context.get_char_number () << ": '" << ex.what () << "'\n";
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



// True on success
bool Library::load (Glib::ustring const &libfilename)
{
	Gnome::Vfs::Handle libfile;

	Glib::RefPtr<Gnome::Vfs::Uri> liburi = Gnome::Vfs::Uri::create (libfilename);

	ProgressDialog progress (tagwindow_);

	/*progress.setLabel (
		"<b><big>Opening "
		+ liburi->extract_short_name ()
		+ "</big></b>\n\nThis process may take some time, particularly\n"
		+ "if the library has been moved since it was last opened.");*/
	progress.setLabel (String::ucompose (_("Opening %1"), liburi->extract_short_name ()));

	// If we get an exception and return, progress::~Progress should
	// take care of calling finish() for us.
	progress.start ();

	try {
		libfile.open (libfilename, Gnome::Vfs::OPEN_READ);
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "opening library '"
			+ Utility::uriToDisplayFileName (libfilename) + "'");
		return false;
	}

	Glib::RefPtr<Gnome::Vfs::FileInfo> fileinfo;
	fileinfo = libfile.get_file_info ();

	char *buffer = (char *) malloc (sizeof(char) * (fileinfo->get_size() + 1));
	if (!buffer) {
		std::cerr << "Warning: TagWindow::loadLibrary: couldn't allocate buffer\n";
		return false;
	}

	try {
		libfile.read (buffer, fileinfo->get_size());
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "reading library '"
			+ Utility::uriToDisplayFileName (libfilename) + "'");
		free (buffer);
		return false;
	}
	buffer[fileinfo->get_size()] = 0;
	
	progress.update (0.1);

	Glib::ustring rawtext = buffer;
	free (buffer);
	libfile.close ();

	progress.getLock ();
	std::cerr << "Reading XML...\n";
	if (!readXML (rawtext)) {
		progress.releaseLock ();
		return false;
	}
	progress.releaseLock ();
	std::cerr << "Done, got " << doclist_->getDocs ().size() << " docs\n";

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
		progress.getLock ();

		Glib::ustring const absfilename = docit->getFileName();

		Glib::ustring relfilename;
		if (!docit->getRelFileName().empty()) {
			relfilename = Glib::build_filename (
				Glib::path_get_dirname (libfilename),
				docit->getRelFileName());
		}
		
		std::cerr << "Abs: " << absfilename
			<< "\nRel: " << relfilename << "\n\n";
		
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
							Utility::uriToDisplayFileName (absfilename),
							Utility::uriToDisplayFileName (relfilename)
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

		progress.releaseLock ();
	}

	progress.finish ();

	return true;
}


// True on success
bool Library::save (Glib::ustring const &libfilename)
{
	Gnome::Vfs::Handle libfile;

	Glib::ustring tmplibfilename = libfilename + ".save-tmp";

	try {
		libfile.create (tmplibfilename, Gnome::Vfs::OPEN_WRITE,
			false, Gnome::Vfs::PERM_USER_READ | Gnome::Vfs::PERM_USER_WRITE);
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "opening file '" + tmplibfilename + "'");
		return false;
	}


	std::cerr << "Updating relative filenames...\n";
	DocumentList::Container &docs = doclist_->getDocs ();
	DocumentList::Container::iterator docit = docs.begin ();
	DocumentList::Container::iterator const docend = docs.end ();
	for (; docit != docend; ++docit) {
		if (Utility::fileExists (docit->getFileName())) {
			docit->updateRelFileName (libfilename);
		}
	}
	std::cerr << "Done.\n";

	std::cerr << "Generating XML...\n";
	Glib::ustring rawtext = writeXML ();
	std::cerr << "Done.\n";

	try {
		libfile.write (rawtext.c_str(), strlen(rawtext.c_str()));
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "writing to file '" + tmplibfilename + "'");
		return false;
	}

	libfile.close ();

	// Forcefully move our tmp file into its real position
	try {
		Gnome::Vfs::Handle::move (tmplibfilename, libfilename, true);
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "moving '"
			+ tmplibfilename + "' to '" + libfilename + "'");
		return false;
	}

	// Having successfully saved the library, write the bibtex if needed
	if (!manage_target_.empty ()) {
		// manage_target_ is either an absolute URI or a relative URI	
		Glib::ustring const bibtextarget = 
			Gnome::Vfs::Uri::make_full_from_relative (
				libfilename,
				manage_target_);
		
		std::vector<Document*> docs;
		DocumentList::Container &docrefs = doclist_->getDocs ();
		DocumentList::Container::iterator it = docrefs.begin();
		DocumentList::Container::iterator const end = docrefs.end();
		for (; it != end; it++) {
			docs.push_back(&(*it));
		}
		writeBibtex (bibtextarget, docs, manage_braces_, manage_utf8_);
	}

	return true;
}


void Library::writeBibtex (
	Glib::ustring const &bibfilename,
	std::vector<Document*> const &docs,
	bool const usebraces,
	bool const utf8)
{
	Glib::ustring tmpbibfilename = bibfilename + ".export-tmp";

	Gnome::Vfs::Handle bibfile;

	try {
		bibfile.create (tmpbibfilename, Gnome::Vfs::OPEN_WRITE,
			false, Gnome::Vfs::PERM_USER_READ | Gnome::Vfs::PERM_USER_WRITE);
	} catch (const Gnome::Vfs::exception ex) {
		std::cerr << "TagWindow::onExportBibtex: "
			"exception in create '" << ex.what() << "'\n";
		Utility::exceptionDialog (&ex, "opening BibTex file");
		return;
	}

	std::ostringstream bibtext;

	std::vector<Document*>::const_iterator it = docs.begin ();
	std::vector<Document*>::const_iterator const end = docs.end ();
	for (; it != end; ++it) {
		(*it)->writeBibtex (bibtext, usebraces, utf8);
	}

	try {
		bibfile.write (bibtext.str().c_str(), strlen(bibtext.str().c_str()));
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "writing to BibTex file");
		bibfile.close ();
		return;
	}

	bibfile.close ();

	// Forcefully move our tmp file into its real position
	Gnome::Vfs::Handle::move (tmpbibfilename, bibfilename, true);
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

