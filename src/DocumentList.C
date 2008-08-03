
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */




#include <iostream>
#include <sstream>

#include <libgnomevfsmm.h>
#include <glibmm/i18n.h>
#include "ucompose.hpp"

#include "Utility.h"
#include "DocumentList.h"
#include "Document.h"

DocumentList::Container& DocumentList::getDocs ()
{
	return docs_;
}


Document* DocumentList::newDocWithFile (Glib::ustring const &filename)
{
	Container::iterator it = docs_.begin ();
	Container::iterator const end = docs_.end ();
	for (; it != end; ++it) {
		if ((*it).getFileName() == filename) {
			return NULL;
		}
	}

	Document newdoc(filename);
	docs_.push_back(newdoc);
	return &(docs_.back());
}


Document* DocumentList::newDocUnnamed ()
{
	Document newdoc;
	docs_.push_back(newdoc);
	return &(docs_.back());
}


Glib::ustring DocumentList::uniqueKey (
	Glib::ustring const &basename)
{
	return uniqueKey (basename, NULL);
}


/**
 * Return a key mangled to be unique with respect to
 * all documents except 'exclusion'
 */
Glib::ustring DocumentList::uniqueKey (
	Glib::ustring const &basename,
	Document const *exclusion)
{
	std::ostringstream name;
	int extension = -1;
	do {
		++extension;
		name.str ("") ;
		name << basename;
		if (extension)
			name << "-" << extension;
	} while (docExists(name.str(), exclusion));

	return name.str();
}


bool DocumentList::docExists (
	Glib::ustring const &name,
	Document const *exclusion)
{
	Container::iterator it = docs_.begin ();
	Container::iterator const end = docs_.end ();
	for (; it != end; ++it) {
		if ((&(*it)) != exclusion && (*it).getKey() == name)
			return true;
	}

	return false;
}


Document* DocumentList::newDocWithName (Glib::ustring const &key)
{
	Document newdoc;
	newdoc.setKey (key);
	docs_.push_back(newdoc);
	return &(docs_.back());
}


void DocumentList::loadDoc (
	Glib::ustring const &filename,
	Glib::ustring const &relfilename,
	Glib::ustring const &notes,
	Glib::ustring const &key,
	std::vector<int> const &taguids,
	BibData const &bib)
{
	Document newdoc (filename, relfilename, notes, key, taguids, bib);
	docs_.push_back(newdoc);
}


void DocumentList::removeDoc (Document * const addr)
{
	Container::iterator it = docs_.begin();
	Container::iterator const end = docs_.end();
	for (; it != end; it++) {
		if (&(*it) == addr) {
			docs_.erase(it);
			return;
		}
	}

	std::cerr << "Warning: DocumentList::removeDoc: couldn't find '"
		<< addr << "' to erase it\n";
}


void DocumentList::print()
{
	Container::iterator it = docs_.begin();
	Container::iterator const end = docs_.end();
	for (; it != end; it++) {
		std::cerr << (*it).getFileName() << " ";
		std::cerr << (*it).getKey() << " ";
		std::vector<int> docvec = (*it).getTags();
		for (std::vector<int>::iterator it = docvec.begin();
			   it != docvec.end(); ++it) {
			std::cerr << (*it);
		}
		std::cerr << std::endl;
	}
}


void DocumentList::clearTag (int uid)
{
	Container::iterator it = docs_.begin();
	Container::iterator const end = docs_.end();
	for (; it != end; it++) {
		(*it).clearTag(uid);
	}
}


void DocumentList::writeXML (Glib::ustring &out)
{
	out += "<doclist>\n";
	Container::iterator it = docs_.begin();
	Container::iterator const end = docs_.end();
	for (; it != end; it++) {
		(*it).writeXML (out);
	}
	out += "</doclist>\n";
}


// Returns the number of references imported
int DocumentList::importFromFile (
	Glib::ustring const & filename,
	BibUtils::Format format)
{
	std::string rawtext;
	Gnome::Vfs::Handle importfile;

	Glib::RefPtr<Gnome::Vfs::Uri> liburi = Gnome::Vfs::Uri::create (filename);

	try {
		importfile.open (filename, Gnome::Vfs::OPEN_READ);
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex,
			String::ucompose (
				_("Opening file '%1'"),
				Glib::filename_to_utf8 (filename)));
		return false;
	}

	Glib::RefPtr<Gnome::Vfs::FileInfo> fileinfo;
	fileinfo = importfile.get_file_info ();

	char *buffer = (char *) malloc (sizeof(char) * (fileinfo->get_size() + 1));
	if (!buffer) {
		std::cerr << "Warning: DocumentList::import: couldn't get buffer\n";
		return false;
	}

	try {
		importfile.read (buffer, fileinfo->get_size());
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex,
			String::ucompose (
				_("Reading file '%1'"),
				Glib::filename_to_utf8 (filename)));
		free (buffer);
		return false;
	}
	buffer[fileinfo->get_size()] = 0;

	rawtext = buffer;
	
	free (buffer);
	importfile.close ();

	Glib::ustring utf8text = rawtext;
	if (!utf8text.validate()) {
		std::cerr << "DocumentList::importFromFile: input not utf-8, trying latin1\n";
		/* Upps, it's not utf8, assume it's latin1 */
		try {
			utf8text = Glib::convert (rawtext, "UTF8", "iso-8859-1");
		} catch (Glib::ConvertError const &ex) {
			Utility::exceptionDialog (&ex,
				String::ucompose (
					_("converting file %1 to utf8 from (guessed) latin1"),
					Glib::filename_to_utf8(filename)));

			return false;
		}
	} else {
		std::cerr << "DocumentList::importFromFile: validated input as utf-8\n";
	}

	return import(utf8text, format);
}


// Returns the number of references imported
int DocumentList::import (
	Glib::ustring const & rawtext,
	BibUtils::Format format)
{
	if (format == BibUtils::FORMAT_UNKNOWN)
		format = BibUtils::guessFormat (rawtext);

	BibUtils::param p;

	BibUtils::bibl b;
	BibUtils::bibl_init( &b );
	// BIBL_* are #defines, so not in namespace
	BibUtils::bibl_initparams( &p, format, BIBL_MODSOUT);
	p.charsetin = BIBL_CHARSET_UNICODE;
	p.utf8in = 1;

	try {
		BibUtils::biblFromString (b, rawtext, format, p);
	} catch (Glib::Error ex) {
		BibUtils::bibl_free( &b );
		Utility::exceptionDialog (&ex, _("Parsing import"));
		return 0;
	}

	// Make a copy to return after we free b	
	int const nrefs = b.nrefs;
	
	for (int i = 0; i < nrefs; ++i) {
		try {
			docs_.push_back (BibUtils::parseBibUtils (b.ref[i]));
		} catch (Glib::Error ex) {
			BibUtils::bibl_free( &b );
			Utility::exceptionDialog (&ex,
				String::ucompose(_("Extracting document %1 from bibutils structure"), i));
			return 0;
		}
	}

	BibUtils::bibl_free( &b );
	return nrefs;
}



