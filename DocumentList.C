

#include <iostream>
#include <sstream>

#include <glibmm/markup.h>
#include <libgnomevfsmm.h>
#include <libgnomeuimm.h>

// fwrite etc
#include <stdlib.h>

// Libpoppler stuff
#include <PDFDoc.h>
#include <goo/gtypes.h>
#include <goo/GooString.h>
#include <GlobalParams.h>
#include <TextOutputDev.h>

#include "DocumentList.h"
Glib::RefPtr<Gdk::Pixbuf> Document::defaultthumb_;

Document::Document (Glib::ustring const &filename)
{
	filename_ = filename;
	setupThumbnail ();
}


Glib::ustring Document::displayNameFromFileName ()
{
	Glib::ustring displayname;
	displayname =
		Gnome::Vfs::unescape_string_for_display (
			Glib::path_get_basename (filename_));
	unsigned int const maxlen = 12;
	if (displayname.size() > maxlen) {
		displayname = displayname.substr(0, maxlen) + "...";
	}
	
	return displayname;
}


Document::Document ()
{
	// Pick up the default thumbnail
	setupThumbnail ();
}


Document::Document (
	Glib::ustring const &filename,
	Glib::ustring const &displayname,
	std::vector<int> const &tagUids)
{
	filename_ = filename;
	setupThumbnail ();
	displayname_ = displayname;
	tagUids_ = tagUids;
}



Glib::RefPtr<Gdk::Pixbuf> Document::getThemeIcon(Glib::ustring const &iconname)
{
	Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
	if (!theme) {
		return Glib::RefPtr<Gdk::Pixbuf> (NULL);
	}

	if (!iconname.empty()) {
		if (theme->has_icon(iconname)) {
			return theme->load_icon(iconname, 48, Gtk::ICON_LOOKUP_FORCE_SVG);
		}
	}
	
	// Fall through on failure
	return Glib::RefPtr<Gdk::Pixbuf> (NULL);
}


void Document::setupThumbnail ()
{
	Glib::RefPtr<Gnome::Vfs::Uri> uri = Gnome::Vfs::Uri::create (filename_);
	if (uri->uri_exists()) {
		Glib::RefPtr<Gnome::Vfs::FileInfo> fileinfo = uri->get_file_info ();
		time_t mtime = fileinfo->get_modification_time ();
		
		Glib::RefPtr<Gnome::UI::ThumbnailFactory> thumbfac = 
			Gnome::UI::ThumbnailFactory::create (Gnome::UI::THUMBNAIL_SIZE_NORMAL);
		
		Glib::ustring thumbfile;
		thumbfile = thumbfac->lookup (filename_, mtime);
		
		// Should we be using Gnome::UI::icon_lookup_sync?
		if (thumbfile.empty()) {
			std::cerr << "Couldn't find thumbnail:'" << filename_ << "'\n";
			if (thumbfac->has_valid_failed_thumbnail (filename_, mtime)) {
				std::cerr << "Has valid failed thumbnail: '" << filename_ << "'\n";

			} else {
				std::cerr << "Generate thumbnail: '" << filename_ << "'\n";
				thumbnail_ = thumbfac->generate_thumbnail (filename_, "application/pdf");
				if (thumbnail_) {
					thumbfac->save_thumbnail (thumbnail_, filename_, mtime);
				} else {
					std::cerr << "Failed to generate thumbnail: '" << filename_ << "'\n";
					thumbfac->create_failed_thumbnail (filename_, mtime);
				}
			}

		} else {
			thumbnail_ = Gdk::Pixbuf::create_from_file (thumbfile);
		}
	}
	
	if (!thumbnail_) {
		// Should use one global pixbuf for this instead of loading
		// it separately for each doc
		if (defaultthumb_) {
			thumbnail_ = Document::defaultthumb_;
		} else {
			thumbnail_ = getThemeIcon ("gnome-mime-application-pdf");
			Document::defaultthumb_ = thumbnail_;
		}
		

	} else {
		float desiredheight = 96.0;
		int oldwidth = thumbnail_->get_width ();
		int oldheight = thumbnail_->get_height ();
		int newheight = (int)desiredheight;
		int newwidth = (int)((float)oldwidth * (desiredheight / (float)oldheight));
		thumbnail_ = thumbnail_->scale_simple (
			newwidth, newheight, Gdk::INTERP_BILINEAR);
	}
}


Glib::ustring& Document::getDisplayName()
{
	return displayname_;
}


Glib::ustring& Document::getFileName()
{
	return filename_;
}



// Unused?
/*
void Document::setFileName (Glib::ustring &filename)
{
	filename_ = filename;
	setupThumbnail ();
}
*/


void Document::setDisplayName (Glib::ustring const &displayname)
{
	displayname_ = displayname;
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


// key is not ref because it gets initted from a const char*
static Glib::ustring writeBibKey (Glib::ustring key, Glib::ustring const & value)
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
	out << writeBibKey ("author",  bib_.getAuthors()) << ",\n";
	out << writeBibKey ("title",   bib_.getTitle()) << ",\n";
	out << writeBibKey ("journal", bib_.getJournal()) << ",\n";
	out << writeBibKey ("volume",  bib_.getVolume()) << ",\n";
	out << writeBibKey ("number",  bib_.getNumber()) << ",\n";
	out << writeBibKey ("pages",   bib_.getPages()) << ",\n";
	out << writeBibKey ("year",    bib_.getYear()) << "\n";
	out << "}\n\n";
}


std::vector<Document>& DocumentList::getDocs ()
{
	return docs_;
}


Document* DocumentList::newDocWithFile (Glib::ustring const &filename)
{
	Document newdoc(filename);
	newdoc.setDisplayName (
		uniqueDisplayName (
			newdoc.displayNameFromFileName()));
	docs_.push_back(newdoc);
	return &(docs_.back());
}


Document* DocumentList::newDocUnnamed ()
{
	Document newdoc;
	newdoc.setDisplayName (
		uniqueDisplayName ("Unnamed"));
	docs_.push_back(newdoc);
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


Document* DocumentList::newDocWithDoi (Glib::ustring const &doi)
{
	Document newdoc;
	newdoc.getBibData().setDoi(doi);
	// Should get crossref metadata from the doi
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


void DocumentList::writeBibtex (std::ostringstream& out)
{
	std::vector<Document>::iterator it = docs_.begin();
	std::vector<Document>::iterator const end = docs_.end();
	for (; it != end; it++) {
		(*it).writeBibtex (out);
	}
}


static void *textfunc (void *stream, char *text, int len)
{
	Glib::ustring *str = (Glib::ustring *)stream;

	int size = len;
	if (size < 1)
		return NULL;

	// Note ustring DOESN'T WORK for addition, I think because the units of 
	// len are unicode characters and not bytes, or vice versa
	// This should break eventually and get really fixed.
	//Glib::ustring addition (text, size);
	std::string addition (text, size);
	//std::cerr << "addition = '" << addition << "'\n";
	//str->append(text, len - 1);
	*str += addition;
	
	// What is this retval used for?
	return NULL;
}


void Document::readPDF () 
{
	if (filename_.empty()) {
		std::cerr << "Document::readPDF: has no filename\n";
		return;
	}

	GooString *filename = new GooString (
		Gnome::Vfs::get_local_path_from_uri(filename_).c_str());
	PDFDoc *popplerdoc = new PDFDoc (filename, NULL, NULL);
	if (!popplerdoc->isOk()) {
		std::cerr << "Document::readPDF: Failed to load '"
			<< filename->getCString() << "'\n";
		return;
	}

	GBool physLayout = gFalse;
	GBool rawOrder = gFalse;

	Glib::ustring textdump;
	TextOutputDev *output = new TextOutputDev(
		(TextOutputFunc) textfunc,
		&textdump,
		physLayout,
		rawOrder);

	if (output->isOk()) {
		std::cerr << "Document::readPDF: Loaded, extracting text...\n";
		int const firstpage = 1;
		int const lastpage = 1;
		popplerdoc->displayPages(output, firstpage, lastpage, 72, 72, 0,
			gTrue, gFalse, gFalse);
	}

	delete output;
	delete popplerdoc;

	if (!textdump.empty()) {		
		bib_.guessDoi (textdump);
		//bib_.guessAuthors (textdump);
		bib_.guessYear (textdump);
	} else {
		std::cerr << "Document::ReadPDF: Could not extract text from '"
			<< filename_ << "'\n";
	}
}
