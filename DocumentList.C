

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
	/*newdoc.setKey (
		uniqueKey (
			newdoc.generateKey()));*/
	docs_.push_back(newdoc);
	return &(docs_.back());
}


Document* DocumentList::newDocUnnamed ()
{
	Document newdoc;
	/*newdoc.setKey (
		uniqueKey ("Unnamed"));*/
	docs_.push_back(newdoc);
	return &(docs_.back());
}


Document* DocumentList::newDocWithDoi (Glib::ustring const &doi)
{
	Document *newdoc = newDocUnnamed ();
	newdoc->getBibData().setDoi (doi);
	return &(docs_.back());
}


Glib::ustring DocumentList::uniqueKey (Glib::ustring const &basename)
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
		if ((*it).getKey() == name) {
			return &(*it);
		}
	}

	// Fall through
	std::cerr << "Warning: DocumentList::getDoc: couldn't find"
		" doc with name '" << name << "'\n";
	return NULL;
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
	Glib::ustring const &key,
	std::vector<int> const &taguids,
	BibData const &bib)
{
	Document newdoc (filename, key, taguids, bib);
	docs_.push_back(newdoc);
}


void DocumentList::removeDoc (Glib::ustring const &key)
{
	std::vector<Document>::iterator it = docs_.begin();
	std::vector<Document>::iterator const end = docs_.end();
	for (; it != end; it++) {
		if ((*it).getKey() == key) {
			docs_.erase(it);
			return;
		}
	}

	std::cerr << "Warning: DocumentList::removeDoc: couldn't find '"
		<< key << "' to erase it\n";
}


void DocumentList::print()
{
	std::vector<Document>::iterator it = docs_.begin();
	std::vector<Document>::iterator const end = docs_.end();
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


bool DocumentList::test ()
{
	/*newDoc ("/somewhere/foo.pdf");
	newDoc ("/somewhere/bar.pdf");
	std::vector<Document> &docvec = getDocs();
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


void writerThread (Glib::ustring const &raw, int pipe, bool *advance)
{
	int len = strlen (raw.c_str());
	// Writing more than 65536 freezes in write()
	int block = 1024;
	if (block > len)
		block = len;

	for (int i = 0; i < len / block; ++i) {
		write (pipe, raw.c_str() + i * block, block);
		*advance = true;
	}
	if (len % block > 0) {
		write (pipe, raw.c_str() + (len / block) * block, len % block);
	}
	
	close (pipe);
}


bool DocumentList::import (
	Glib::ustring const & filename,
	BibUtils::Format format)
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

	if (format == BibUtils::FORMAT_UNKNOWN)
		format = BibUtils::guessFormat (rawtext);

	BibUtils::param p;
	BibUtils::bibl b;
	BibUtils::bibl_init( &b );
	// BIBL_* are #defines, so not in namespace
	BibUtils::bibl_initparams( &p, format, BIBL_MODSOUT);

	int handles[2];
	if (pipe(handles)) {
		std::cerr << "Warning: DocumentList::import: couldn't get pipe\n";
		return false;
	}
	int pipeout = handles[0];
	int pipein = handles[1];

	bool advance = false;

	Glib::Thread *writer = Glib::Thread::create (
		sigc::bind (sigc::ptr_fun (&writerThread), rawtext, pipein, &advance), true);

	while (!advance) {}

	FILE *otherend = fdopen (pipeout, "r");
	BibUtils::bibl_read( &b, otherend, "My Pipe", format, &p );
	fclose (otherend);
	close (pipeout);
	
	writer->join ();

	for (int i = 0; i < b.nrefs; ++i) {
		docs_.push_back (BibUtils::parseBibUtils (b.ref[i]));
	}

	BibUtils::bibl_free( &b );

	return true;
}


