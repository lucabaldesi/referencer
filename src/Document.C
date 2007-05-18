
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

#include <glibmm/markup.h>
#include <libgnomevfsmm.h>
#include <libgnomeuimm.h>
#include <glibmm/i18n.h>
#include "ucompose.hpp"

#include <poppler.h>

#include "Utility.h"

#include "Document.h"

Glib::RefPtr<Gdk::Pixbuf> Document::defaultthumb_;
Glib::RefPtr<Gdk::Pixbuf> Document::thumbframe_;

Document::Document (Glib::ustring const &filename)
{
	setFileName (filename);
}


Document::Document ()
{
	// Pick up the default thumbnail
	setupThumbnail ();
}


Document::Document (
	Glib::ustring const &filename,
	Glib::ustring const &relfilename,
	Glib::ustring const &key,
	std::vector<int> const &tagUids,
	BibData const &bib)
{
	setFileName (filename);
	key_ = key;
	tagUids_ = tagUids;
	bib_ = bib;
	relfilename_ = relfilename;
}


Glib::ustring Document::generateKey ()
{
	// Ideally Chambers06
	// If not then pap104
	// If not then Unnamed-5
	Glib::ustring name;

	Glib::ustring::size_type const maxlen = 14;

	if (!bib_.getAuthors().empty ()) {
		Glib::ustring year = bib_.getYear ();
		if (year.size() == 4)
			year = year.substr (2,3);

		Glib::ustring authors = bib_.getAuthors ();
		if (authors.size() > maxlen - 2) {
			authors = authors.substr(0, maxlen - 2);
		}

		// Should:
		// Truncate it at the first "et al", "and", or ","

		name = authors + year;
	} else if (!filename_.empty ()) {
		Glib::ustring filename = Gnome::Vfs::unescape_string_for_display (
			Glib::path_get_basename (filename_));

		int periodpos = filename.find_last_of (".");
		if (periodpos != std::string::npos) {
			filename = filename.substr (0, periodpos);
		}

		name = filename;
		if (name.size() > maxlen) {
			name = name.substr(0, maxlen);
		}

	} else {
		name = _("Unnamed");
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
	name = Utility::strip (name, ",");
	name = Utility::strip (name, "@");

	return name;
}


void Document::setupThumbnail ()
{
	thumbnail_.clear ();
	Glib::RefPtr<Gnome::Vfs::Uri> uri = Gnome::Vfs::Uri::create (filename_);
	std::cerr << uri->get_scheme () << "\n";
	if (!filename_.empty () && Utility::uriIsFast (uri) && uri->uri_exists()) {
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
		if (defaultthumb_) {
			thumbnail_ = Document::defaultthumb_;
		} else {
			thumbnail_ = Utility::getThemeIcon ("gnome-mime-application-pdf");
			Document::defaultthumb_ = thumbnail_;
		}
	} else {
		float const desiredheight = 96.0;
		int oldwidth = thumbnail_->get_width ();
		int oldheight = thumbnail_->get_height ();
		int newheight = (int)desiredheight;
		int newwidth = (int)((float)oldwidth * (desiredheight / (float)oldheight));
		thumbnail_ = thumbnail_->scale_simple (
			newwidth, newheight, Gdk::INTERP_BILINEAR);

		if (!thumbframe_) {
			thumbframe_ = Gdk::Pixbuf::create_from_file (
				Utility::findDataFile ("thumbnail_frame.png"));
		}

		int const left_offset = 3;
		int const top_offset = 3;
		int const right_offset = 6;
		int const bottom_offset = 6;
		thumbnail_ = Utility::eelEmbedImageInFrame (
			thumbnail_, thumbframe_,
			left_offset, top_offset, right_offset, bottom_offset);
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


Glib::ustring& Document::getRelFileName()
{
	return relfilename_;
}


void Document::setFileName (Glib::ustring const &filename)
{
	std::cerr << "Document::setFileName: got '" << filename << "'\n";
	if (filename != filename_) {
		filename_ = filename;
		setupThumbnail ();
	} else if (!thumbnail_) {
		setupThumbnail ();
	}
}


void Document::updateRelFileName (Glib::ustring const &libfilename)
{
	Glib::ustring const newrelfilename =
		Utility::relPath (libfilename, getFileName ());
	if (!newrelfilename.empty())
		relfilename_ = Utility::relPath (libfilename, getFileName ());
	std::cerr << "Set relfilename_ = " << relfilename_ << "\n";
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

void Document::writeBibtex (
	std::ostringstream& out,
	bool const usebraces,
	bool const utf8)
{
	// BibTeX values cannot be larger than 1000 characters - should make sure of this
	// We should strip illegal characters from key in a predictable way
	out << "@" << bib_.getType() << "{" << key_ << ",\n";

	BibData::ExtrasMap extras = bib_.getExtras ();
	BibData::ExtrasMap::iterator it = extras.begin ();
	BibData::ExtrasMap::iterator const end = extras.end ();
	for (; it != end; ++it) {
		// Exceptions to usebraces are editor and author because we
		// don't want "Foo, B.B. and John Bar" to be literal
		writeBibKey (
			out,
			(*it).first,
			(*it).second,
			((*it).first.lowercase () != "editor") && usebraces, utf8);
	}

	// Ideally should know what's a list of human names and what's an
	// institution name be doing something different for non-human-name authors?
	writeBibKey (out, "author",  bib_.getAuthors(), false, utf8);
	writeBibKey (out, "title",   bib_.getTitle(), usebraces, utf8);
	writeBibKey (out, "journal", bib_.getJournal(), usebraces, utf8);
	writeBibKey (out, "volume",  bib_.getVolume(), usebraces, utf8);
	writeBibKey (out, "number",  bib_.getIssue(), usebraces, utf8);
	writeBibKey (out, "pages",   bib_.getPages(), usebraces, utf8);
	writeBibKey (out, "year",    bib_.getYear(), usebraces, utf8);

	out << "}\n\n";
}


using Glib::Markup::escape_text;

void Document::writeXML (std::ostringstream &out)
{
	out << "  <doc>\n";
	out << "    <filename>" << escape_text(getFileName())
		<< "</filename>\n";
	out << "    <relative_filename>" << escape_text(getRelFileName())
		<< "</relative_filename>\n";
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

	GError *error = NULL;

	PopplerDocument *popplerdoc = poppler_document_new_from_file (filename_.c_str(), NULL, &error);
	if (popplerdoc == NULL) {
		std::cerr << "Document::readPDF: Failed to load '"
			<< filename_ << "'\n";
		g_error_free (error);
		return;
	}

	Glib::ustring textdump;
	int num_pages = poppler_document_get_n_pages (popplerdoc);
	char **page_text;
	bool got_id = false;

	for (int i = 0; i < num_pages; i++) {
		PopplerPage *page;
		PopplerRectangle *rect;
		double width, height;

		page = poppler_document_get_page (popplerdoc, i);
		poppler_page_get_size (page, &width, &height);

		rect = poppler_rectangle_new ();
		rect->x1 = 0.;
		rect->y1 = 0.;
		rect->x2 = width;
		rect->y2 = height;

		// FIXME: add something before/after appending text to signal pagebreak?
		textdump += poppler_page_get_text (page, rect); 

		poppler_rectangle_free (rect);
		g_object_unref (page);
		
		// When we read the first page, see if it has the doc ID
		// and if it does then don't bother reading the rest.
		if (i == 0) {
			bib_.guessYear (textdump);
			bib_.guessDoi (textdump);
			if (bib_.getDoi ().empty ())
				bib_.guessArxiv (textdump);
			if (!bib_.getDoi ().empty () || !bib_.getExtras ()["eprint"].empty ()) {
				got_id = true;
				break;
			}
		}
	}

	g_object_unref (popplerdoc);

	// If we didn't find the ID on the first page and we have some text
	// then search the whole document text for the ID and year
	if (!got_id && !textdump.empty()) {
		bib_.guessDoi (textdump);
		if (bib_.getDoi ().empty ())
			bib_.guessArxiv (textdump);
		// We might have picked this up on the first page
		if (bib_.getYear ().empty())
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


void Document::getMetaData ()
{
	if (!bib_.getDoi().empty ())
		bib_.getCrossRef ();
	else if (!bib_.getExtras()["eprint"].empty())
		bib_.getArxiv ();
}


void Document::renameFromKey ()
{
	if (getFileName().empty () || getKey().empty ())
		return;

	Glib::RefPtr<Gnome::Vfs::Uri> olduri = Gnome::Vfs::Uri::create (getFileName());

	Glib::ustring shortname = olduri->extract_short_name ();
	std::cerr << "Shortname = " << shortname << "\n";
	Glib::ustring dirname = olduri->extract_dirname ();
	std::cerr << "Dirname = " << dirname << "\n";

	int pos = shortname.rfind (".");
	Glib::ustring extension = "";
	if (pos != Glib::ustring::npos)
		extension = shortname.substr (pos, shortname.length() - 1);

	Glib::ustring newfilename = getKey() + extension;
	std::cerr << "Newfilename = " << newfilename << "\n";

	//Glib::RefPtr<Gnome::Vfs::Uri> newuri = Gnome::Vfs::Uri::create (newfilename);
	Glib::RefPtr<Gnome::Vfs::Uri> newuri =
		Gnome::Vfs::Uri::create (dirname)->append_file_name (newfilename);

	try {
		Gnome::Vfs::Handle::move (
			olduri,
			newuri,
			false /*force replace*/);
		setFileName (newuri->to_string ());
	} catch (Gnome::Vfs::exception &ex) {
		Utility::exceptionDialog (&ex,
			String::ucompose (_("Moving '%1' to '%2'"),
				olduri->to_string (),
				newuri->to_string ())
			);
	}
}

