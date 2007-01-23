

#include <iostream>
#include <sstream>

//#include <glibmm/markup.h>
#include <libgnomevfsmm.h>

#include "Utility.h"
#include "DocumentList.h"
#include "Document.h"

std::vector<Document>& DocumentList::getDocs ()
{
	return docs_;
}


Document* DocumentList::newDocWithFile (Glib::ustring const &filename)
{
	Document newdoc(filename);
	/*newdoc.setDisplayName (
		uniqueDisplayName (
			newdoc.generateKey()));*/
	docs_.push_back(newdoc);
	return &(docs_.back());
}


Document* DocumentList::newDocUnnamed ()
{
	Document newdoc;
	/*newdoc.setDisplayName (
		uniqueDisplayName ("Unnamed"));*/
	docs_.push_back(newdoc);
	return &(docs_.back());
}


Document* DocumentList::newDocWithDoi (Glib::ustring const &doi)
{
	Document *newdoc = newDocUnnamed ();
	newdoc->getBibData().setDoi (doi);
	return &(docs_.back());
}


Glib::ustring DocumentList::uniqueDisplayName (Glib::ustring const &basename)
{
	std::ostringstream name;
	int extension = -1;
	do {
		++extension;
		name.str ("") ;
		name << basename;
		if (extension)
			name << "-" << extension;
	} while (getDoc(name.str()));

	return name.str();
}


Document* DocumentList::getDoc (Glib::ustring const &name)
{
	std::vector<Document>::iterator it = docs_.begin ();
	std::vector<Document>::iterator const end = docs_.end ();
	for (; it != end; ++it) {
		if ((*it).getDisplayName() == name) {
			return &(*it);
		}
	}

	// Fall through
	std::cerr << "Warning: DocumentList::getDoc: couldn't find"
		" doc with name '" << name << "'\n";
	return NULL;
}


Document* DocumentList::newDocWithName (Glib::ustring const &displayname)
{
	Document newdoc;
	newdoc.setDisplayName (displayname);
	docs_.push_back(newdoc);
	return &(docs_.back());
}


void DocumentList::loadDoc (
	Glib::ustring const &filename,
	Glib::ustring const &displayname,
	std::vector<int> const &taguids,
	BibData const &bib)
{
	Document newdoc (filename, displayname, taguids, bib);
	docs_.push_back(newdoc);
}


void DocumentList::removeDoc (Glib::ustring const &displayname)
{
	std::vector<Document>::iterator it = docs_.begin();
	std::vector<Document>::iterator const end = docs_.end();
	for (; it != end; it++) {
		if ((*it).getDisplayName() == displayname) {
			docs_.erase(it);
			return;
		}
	}

	std::cerr << "Warning: DocumentList::removeDoc: couldn't find '"
		<< displayname << "' to erase it\n";
}


void DocumentList::print()
{
	std::vector<Document>::iterator it = docs_.begin();
	std::vector<Document>::iterator const end = docs_.end();
	for (; it != end; it++) {
		std::cerr << (*it).getFileName() << " ";
		std::cerr << (*it).getDisplayName() << " ";
		std::vector<int> docvec = (*it).getTags();
		for (std::vector<int>::iterator it = docvec.begin();
			   it != docvec.end(); ++it) {
			std::cerr << (*it);
		}
		std::cerr << std::endl;
	}
}


bool DocumentList::test ()
{
	/*newDoc ("/somewhere/foo.pdf");
	newDoc ("/somewhere/bar.pdf");
	std::vector<Document> docvec = getDocs();
	for (std::vector<Document>::iterator it = docvec.begin; it != docvec.end*/
	return true;
}


void DocumentList::clearTag (int uid)
{
	std::vector<Document>::iterator it = docs_.begin();
	std::vector<Document>::iterator const end = docs_.end();
	for (; it != end; it++) {
		(*it).clearTag(uid);
	}
}


void DocumentList::writeXML (std::ostringstream& out)
{
	out << "<doclist>\n";
	std::vector<Document>::iterator it = docs_.begin();
	std::vector<Document>::iterator const end = docs_.end();
	for (; it != end; it++) {
		(*it).writeXML (out);
	}
	out << "</doclist>\n";
}


void DocumentList::writeBibtex (std::ostringstream& out)
{
	std::vector<Document>::iterator it = docs_.begin();
	std::vector<Document>::iterator const end = docs_.end();
	for (; it != end; it++) {
		(*it).writeBibtex (out);
	}
}




using namespace BibUtils;


bool DocumentList::import (Glib::ustring const & filename)
{
	Glib::ustring rawtext;
	{
		Gnome::Vfs::Handle importfile;

		Glib::RefPtr<Gnome::Vfs::Uri> liburi = Gnome::Vfs::Uri::create (filename);

		try {
			importfile.open (filename, Gnome::Vfs::OPEN_READ);
		} catch (const Gnome::Vfs::exception ex) {
			Utility::exceptionDialog (&ex, "opening file '" + filename + "'");
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
			Utility::exceptionDialog (&ex, "reading file '" + filename + "'");
			free (buffer);
			return false;
		}
		buffer[fileinfo->get_size()] = 0;

		rawtext = buffer;
		free (buffer);
		importfile.close ();
	}
	param p;
	bibl b;
	bibl_init( &b );
	bibl_initparams( &p, BIBL_BIBTEXIN, BIBL_MODSOUT);

	int handles[2];
	if (pipe(handles)) {
		std::cerr << "Warning: DocumentList::import: couldn't get pipe\n";
		return false;
	}
	int pipeout = handles[0];
	int pipein = handles[1];
	write (pipein, rawtext.c_str(), strlen(rawtext.c_str()));
	close (pipein);
	
	FILE *otherend = fdopen (pipeout, "r");
	bibl_read( &b, otherend, "My Pipe", BIBL_BIBTEXIN, &p );
	fclose (otherend);
	close (pipeout);

	for (int i = 0; i < b.nrefs; ++i) {
		docs_.push_back (parseBibUtils (b.ref[i]));
	}

	/*bibl_write( &b, stdout, BIBL_MODSOUT, &p );
	fflush( stdout );*/
	fprintf( stderr, "Processed %ld references.\n", b.nrefs );
	bibl_free( &b );

	return true;
}


Document DocumentList::parseBibUtils (BibUtils::fields *ref)
{
	Document newdoc;
	int k = fields_find (ref, "TITLE", 0);
	if (k >= 0) {
		newdoc.getBibData().setTitle (ref->data[k].data);
	}
	for (int j = 0; j < ref->nfields; ++j) {
		Glib::ustring key = ref->tag[j].data;
		Glib::ustring value = ref->data[j].data;

		if (key == "REFNUM") {
			newdoc.setDisplayName (value);
		/*} else if (key == "TITLE") {
			if (ref->level[j] == 0) {
				newdoc.getBibData().setTitle (value);
			} else if (ref->level[j] == 1) {
				newdoc.getBibData().setJournal (value);
			}*/
		} else if (key == "TYPE") {
			newdoc.getBibData().setType (value);
		} else if (key == "VOLUME") {
			newdoc.getBibData().setVolume (value);
		} else if (key == "NUMBER") {
			newdoc.getBibData().setIssue (value);
		} else if (key == "YEAR" || key == "PARTYEAR") {
			newdoc.getBibData().setYear (value);
		} else if (key == "PAGESTART") {
			newdoc.getBibData().setPages (value + newdoc.getBibData().getPages ());
		} else if (key == "PAGEEND") {
			newdoc.getBibData().setPages (newdoc.getBibData().getPages () + "-" + value);
		} else if (key == "AUTHOR") {
			if (newdoc.getBibData().getAuthors().empty ()) {
				newdoc.getBibData().setAuthors (value);
			} else {
				newdoc.getBibData().setAuthors (
					newdoc.getBibData().getAuthors () + " and " + value);
			}
		} else {
			std::cerr << key << " = " << value << "\n";
			std::cerr << "\t" << "used = " << ref->used[j] << "\n";
			std::cerr << "\t" << "level = " << ref->level[j] << "\n";
		}
	}
	return newdoc;
}
