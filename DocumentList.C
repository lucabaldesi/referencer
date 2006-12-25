

#include <iostream>
#include <sstream>

#include <glibmm/markup.h>
#include <libgnomevfsmm.h>

#include "DocumentList.h"


Document::Document (Glib::ustring const &filename)
{
	filename_ = filename;
	displayname_ =
		Gnome::Vfs::unescape_string_for_display (
			Glib::path_get_basename (filename));
	int const maxlen = 35;
	if (displayname_.size() > maxlen) {
		displayname_ = displayname_.substr(0, maxlen) + "...";
	}
}

Document::Document (
	Glib::ustring const &filename,
	Glib::ustring const &displayname,
	std::vector<int> const &tagUids)
{
	filename_ = filename;
	displayname_ = displayname;
	tagUids_ = tagUids;
}


Glib::ustring& Document::getDisplayName()
{
	return displayname_;
}


Glib::ustring& Document::getFileName()
{
	return filename_;
}


std::vector<int>& Document::getTags()
{
	return tagUids_;
}

void Document::setTag(int uid)
{
	if (hasTag(uid)) {
		std::cerr << "Warning: Document::setTag: warning, already have tag "
			<< uid << " on " << displayname_ << std::endl;
	} else {
		tagUids_.push_back(uid);
	}
}


void Document::clearTag(int uid)
{
	std::vector<int>::iterator location =
		std::find(tagUids_.begin(), tagUids_.end(), uid);

	if (location != tagUids_.end()) {
		tagUids_.erase(location);
	} else {
		std::cerr << "Warning: Document::clearTag: didn't have tag "
			<< uid << " on " << displayname_ << std::endl;
	}
}


void Document::clearTags()
{
	tagUids_.clear();
}


bool Document::hasTag(int uid)
{
	return std::find(tagUids_.begin(), tagUids_.end(), uid) != tagUids_.end();
}


static Glib::ustring writeBibKey (Glib::ustring& key, Glib::ustring&value)
{
	// Should be doing lots of escaping here, going from UTF-8 to LaTeX
	return key + " = {" + value + "}";
}

void Document::writeBibtex (std::ostringstream& out)
{
	// BibTeX values cannot be larger than 1000 characters - should make sure of this
	// This doctype bit should be a variable
	// We should strip illegal characters from displayname in a predictable way
	out << "@article{" << displayname_ << "," << std::endl;
	// There are more fields than this, or there should be!
	out << writeBibKey ("author", bib_.getAuthor()) << ",\n";
	out << writeBibKey ("title", bib_.getTitle()) << ",\n";
	out << writeBibKey ("journal", bib_.getJournal()) << ",\n";
	out << writeBibKey ("volume", bib_.getVolume()) << ",\n";
	out << writeBibKey ("number", bib_.getNumber()) << ",\n";
	out << writeBibKey ("pages", bib_.getPages()) << ",\n";
	out << writeBibKey ("year", bib_.getYear()) << "\n";
	out << "}\n\n";
}


std::vector<Document>& DocumentList::getDocs ()
{
	return docs_;
}


Document* DocumentList::newDoc (Glib::ustring const &filename)
{
	Document newdoc(filename);
	docs_.push_back(newdoc);
	return &(docs_.back());
}


void DocumentList::loadDoc (
	Glib::ustring const &filename,
	Glib::ustring const &displayname,
	std::vector<int> const &taguids)
{
	Document newdoc (filename, displayname, taguids);
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


using Glib::Markup::escape_text;

void DocumentList::writeXML (std::ostringstream& out)
{
	out << "<doclist>\n";
	std::vector<Document>::iterator it = docs_.begin();
	std::vector<Document>::iterator const end = docs_.end();
	for (; it != end; it++) {
		out << "  <doc>\n";
		out << "    <filename>" << escape_text((*it).getFileName())
			<< "</filename>\n";
		out << "    <displayname>" << escape_text((*it).getDisplayName())
			<< "</displayname>\n";
		std::vector<int> docvec = (*it).getTags();
		for (std::vector<int>::iterator it = docvec.begin();
			   it != docvec.end(); ++it) {
			out << "    <tagged>" << (*it) << "</tagged>\n";
		}
		out << "  </doc>\n";
	}
	out << "</doclist>\n";
}
