
#include <iostream>
#include <sstream>

#include <glibmm/markup.h>
#include <libgnomevfsmm.h>
#include <libgnomeuimm.h>

// Libpoppler stuff
#include <PDFDoc.h>
#include <goo/gtypes.h>
#include <goo/GooString.h>
#include <GlobalParams.h>
#include <TextOutputDev.h>

#include "Utility.h"

#include "Document.h"

Glib::RefPtr<Gdk::Pixbuf> Document::defaultthumb_;

Document::Document (Glib::ustring const &filename)
{
	filename_ = filename;
	setupThumbnail ();
}


Document::Document ()
{
	// Pick up the default thumbnail
	setupThumbnail ();
}


Document::Document (
	Glib::ustring const &filename,
	Glib::ustring const &key,
	std::vector<int> const &tagUids,
	BibData const &bib)
{
	filename_ = filename;
	setupThumbnail ();
	key_ = key;
	tagUids_ = tagUids;
	bib_ = bib;
}


Glib::ustring Document::generateKey ()
{
	// Ideally Chambers06
	// If not then pap104
	// If not then Unnamed-5
	Glib::ustring name;

	if (!bib_.getAuthors().empty ()) {
		Glib::ustring year = bib_.getYear ();
		if (year.size() == 4)
			year = year.substr (2,3);

		Glib::ustring authors = bib_.getAuthors ();

		// Should:
		// Strip spaces from authors
		// Truncate it at the first "et al", "and", or ","

		name = authors + year;
	} else if (!filename_.empty ()) {
		Glib::ustring filename = Gnome::Vfs::unescape_string_for_display (
			Glib::path_get_basename (filename_));

		unsigned int periodpos = filename.find_last_of (".");
		if (periodpos != std::string::npos) {
			filename = filename.substr (0, periodpos);
		}

		name = filename;

	} else {
		name = "Unnamed";
	}

	// Don't confuse LaTeX
	name = Utility::strip (name, " ");
	name = Utility::strip (name, "&");
	name = Utility::strip (name, "$");
	name = Utility::strip (name, "%");
	name = Utility::strip (name, "#");
	name = Utility::strip (name, "_");
	name = Utility::strip (name, "{");
	name = Utility::strip (name, "}");

	unsigned int const maxlen = 12;
	if (name.size() > maxlen) {
		name = name.substr(0, maxlen) + "...";
	}

	return name;
}


void Document::setupThumbnail ()
{
	Glib::RefPtr<Gnome::Vfs::Uri> uri = Gnome::Vfs::Uri::create (filename_);
	if (!filename_.empty () && uri->is_local() && uri->uri_exists()) {
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
			thumbnail_ = Utility::getThemeIcon ("gnome-mime-application-pdf");
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


Glib::ustring& Document::getKey()
{
	return key_;
}


Glib::ustring& Document::getFileName()
{
	return filename_;
}


void Document::setFileName (Glib::ustring &filename)
{
	if (filename == filename_)
		return;
	filename_ = filename;
	setupThumbnail ();
}


void Document::setKey (Glib::ustring const &key)
{
	key_ = key;
}


std::vector<int>& Document::getTags()
{
	return tagUids_;
}

void Document::setTag(int uid)
{
	if (hasTag(uid)) {
		std::cerr << "Warning: Document::setTag: warning, already have tag "
			<< uid << " on " << key_ << std::endl;
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
			<< uid << " on " << key_ << std::endl;
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


using Utility::writeBibKey;

void Document::writeBibtex (std::ostringstream& out)
{
	// BibTeX values cannot be larger than 1000 characters - should make sure of this
	// This doctype bit should be a variable
	// We should strip illegal characters from key in a predictable way
	out << "@" << bib_.getType() << "{" << key_ << "," << std::endl;

	BibData::ExtrasMap extras = bib_.getExtras ();
	BibData::ExtrasMap::iterator it = extras.begin ();
	BibData::ExtrasMap::iterator const end = extras.end ();
	for (; it != end; ++it) {
		out << writeBibKey ((*it).first, (*it).second) << ",\n";
	}

// Should be doing something different for non-human-name authors?
	out << writeBibKey ("author",  bib_.getAuthors()) << ",\n";
	out << writeBibKey ("title",   bib_.getTitle()) << ",\n";
	out << writeBibKey ("journal", bib_.getJournal()) << ",\n";
	out << writeBibKey ("volume",  bib_.getVolume()) << ",\n";
	out << writeBibKey ("number",  bib_.getIssue()) << ",\n";
	out << writeBibKey ("pages",   bib_.getPages()) << ",\n";
	out << writeBibKey ("year",    bib_.getYear()) << "\n";

	out << "}\n\n";
}


using Glib::Markup::escape_text;

void Document::writeXML (std::ostringstream &out)
{
	out << "  <doc>\n";
	out << "    <filename>" << escape_text(getFileName())
		<< "</filename>\n";
	out << "    <key>" << escape_text(getKey())
		<< "</key>\n";

	std::vector<int> docvec = getTags();
	for (std::vector<int>::iterator it = docvec.begin();
		   it != docvec.end(); ++it) {
		out << "    <tagged>" << (*it) << "</tagged>\n";
	}

	getBibData().writeXML (out);

	out << "  </doc>\n";
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
		int const lastpage = popplerdoc->getNumPages ();
		popplerdoc->displayPages(output, firstpage, lastpage, 72, 72, 0,
			gTrue, gFalse, gFalse);
	}

	delete output;
	delete popplerdoc;

	if (!textdump.empty()) {
		bib_.guessDoi (textdump);
		if (bib_.getDoi().empty ())
			bib_.guessArxiv (textdump);
		//bib_.guessAuthors (textdump);
		bib_.guessYear (textdump);
	} else {
		std::cerr << "Document::ReadPDF: Could not extract text from '"
			<< filename_ << "'\n";
	}
}


bool Document::canWebLink ()
{
	if (
		   !bib_.getDoi ().empty ()
		|| !bib_.getExtras ()["eprint"].empty()
	   )
	{
		return true;
	} else {
		return false;
	}
}


bool Document::canGetMetadata ()
{
	if (
		   !bib_.getDoi ().empty ()
		|| !bib_.getExtras ()["eprint"].empty()
	   )
	{
		return true;
	} else {
		return false;
	}
}


bool Document::matchesSearch (Glib::ustring const &search)
{
	if (bib_.getTitle().uppercase().find(search.uppercase()) != Glib::ustring::npos)
		return true;
		
	if (bib_.getAuthors().uppercase().find(search.uppercase()) != Glib::ustring::npos)
		return true;
		
	if (key_.uppercase().find(search.uppercase()) != Glib::ustring::npos)
		return true;
	
	return false;
}
