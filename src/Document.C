
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
#include <glibmm/i18n.h>
#include "ucompose.hpp"
#include <poppler.h>

#include "config.h"

#include "BibUtils.h"
#include "DocumentView.h"
#include "Library.h"
#include "PluginManager.h"
#include "Preferences.h"
#include "TagList.h"
#include "ThumbnailGenerator.h"
#include "Utility.h"


#include "Document.h"

static ThumbnailGenerator *thumbnailGenerator;

const Glib::ustring Document::defaultKey_ = _("Unnamed");
Glib::RefPtr<Gdk::Pixbuf> Document::loadingthumb_;


Document::~Document ()
{
	if (thumbnailGenerator)
		thumbnailGenerator->deregisterRequest (this);
}

Document::Document (Document const &x)
{
	view_ = NULL;
	*this = x;
	setupThumbnail ();
}

Document::Document (Glib::ustring const &filename)
{
	view_ = NULL;
	setFileName (filename);
}


Document::Document ()
{
	view_ = NULL;
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
	view_ = NULL;
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

		Glib::ustring::size_type comma = authors.find (",");
		Glib::ustring::size_type space = authors.find (" ");
		Glib::ustring::size_type snip = Glib::ustring::npos;

		if (comma != Glib::ustring::npos)
			snip = comma;
		if (space != Glib::ustring::npos && space < comma)
			snip = space;

		if (snip != Glib::ustring::npos)
			authors = authors.substr(0, snip);


		if (authors.size() > maxlen - 2) {
			authors = authors.substr(0, maxlen - 2);
		}

		// Should:
		// Truncate it at the first "et al", "and", or ","

		name = authors + year;
	} else if (!filename_.empty ()) {
		Glib::ustring filename = Gnome::Vfs::unescape_string_for_display (
			Glib::path_get_basename (filename_));

		Glib::ustring::size_type periodpos = filename.find_last_of (".");
		if (periodpos != std::string::npos) {
			filename = filename.substr (0, periodpos);
		}

		name = filename;
		if (name.size() > maxlen) {
			name = name.substr(0, maxlen);
		}

	} else {
		name = defaultKey_;
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


void Document::setThumbnail (Glib::RefPtr<Gdk::Pixbuf> thumb)
{
	thumbnail_ = thumb;
	if (view_)
		view_->updateDoc(this);
}


void Document::setupThumbnail ()
{
	if (!loadingthumb_) {
		loadingthumb_ = Utility::getThemeIcon ("image-loading");
		if (!loadingthumb_)
			loadingthumb_ = Gdk::Pixbuf::create_from_file
				(Utility::findDataFile ("unknown-document.png"));

		if (loadingthumb_) {
			/* FIXME magic number duplicated elsewhere */
			float const desiredwidth = 64.0 + 9;
			int oldwidth = loadingthumb_->get_width ();
			int oldheight = loadingthumb_->get_height ();
			int newwidth = (int)desiredwidth;
			int newheight = (int)((float)oldheight * (desiredwidth / (float)oldwidth));
			loadingthumb_ = loadingthumb_->scale_simple (
				newwidth, newheight, Gdk::INTERP_BILINEAR);
		}

	}
	thumbnail_ = loadingthumb_;
	if (!thumbnailGenerator)
		thumbnailGenerator = new ThumbnailGenerator ();
 
	thumbnailGenerator->registerRequest (filename_, this);

}


Glib::ustring const & Document::getKey() const
{
	return key_;
}


Glib::ustring const & Document::getFileName() const
{
	return filename_;
}


Glib::ustring const & Document::getRelFileName() const
{
	return relfilename_;
}


void Document::setFileName (Glib::ustring const &filename)
{
	if (thumbnailGenerator)
		thumbnailGenerator->deregisterRequest (this);

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
	Library const &lib,
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
	writeBibKey (out, "doi",    bib_.getDoi(), usebraces, utf8);
	
	if (tagUids_.size () > 0) {
		out << "\tkeywords = \"";
		std::vector<int>::iterator tagit = tagUids_.begin ();
		std::vector<int>::iterator const tagend = tagUids_.end ();
		for (; tagit != tagend; ++tagit) {
			if (tagit != tagUids_.begin ())
				out << ", ";
			out << lib.taglist_->getName (*tagit);
		}
		out << "\"\n";
	}

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


bool Document::readPDF ()
{
	if (filename_.empty()) {
		std::cerr << "Document::readPDF: has no filename\n";
		return false;
	}

	GError *error = NULL;

	PopplerDocument *popplerdoc = poppler_document_new_from_file (filename_.c_str(), NULL, &error);
	if (popplerdoc == NULL) {
		std::cerr << "Document::readPDF: Failed to load '"
			<< filename_ << "'\n";
		g_error_free (error);
		return false;
	}

	Glib::ustring textdump;
	int num_pages = poppler_document_get_n_pages (popplerdoc);
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
		#ifdef OLD_POPPLER
			#warning Using poppler <= 0.5
			textdump += poppler_page_get_text (page, rect);
		#else
			#warning Using poppler >= 0.6
			textdump += poppler_page_get_text (page, POPPLER_SELECTION_GLYPH, rect);
		#endif

		poppler_rectangle_free (rect);
		g_object_unref (page);

		// When we read the first page, see if it has the doc ID
		// and if it does then don't bother reading the rest.
		if (i == 0) {
			bib_.guessYear (textdump);
			bib_.guessDoi (textdump);
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
		bib_.guessArxiv (textdump);
		// We might have picked this up on the first page
		if (bib_.getYear ().empty())
			bib_.guessYear (textdump);
	}

	return !(textdump.empty ());
}


bool Document::canGetMetadata ()
{
	if (hasField("doi")
		|| hasField ("eprint")
		|| hasField ("pmid")
	   )
	{
		return true;
	} else {
		return false;
	}
}


bool Document::matchesSearch (Glib::ustring const &search)
{
	std::map <Glib::ustring, Glib::ustring> fields = getFields ();
	std::map <Glib::ustring, Glib::ustring>::iterator fieldIter = fields.begin ();
	std::map <Glib::ustring, Glib::ustring>::iterator const fieldEnd = fields.end ();
	for (; fieldIter != fieldEnd; ++fieldIter) {
		if (fieldIter->second.uppercase().find(search.uppercase()) != Glib::ustring::npos)
			return true;
	}

	return false;
}

/*
 * Returns true on success
 */
bool Document::getMetaData ()
{
	if (_global_prefs->getWorkOffline())
		return false;

	PluginCapability potentials;

	if (hasField("doi"))
		potentials.add(PluginCapability::DOI);

	if (hasField("eprint"))
		potentials.add(PluginCapability::ARXIV);

	if (hasField("pmid"))
		potentials.add(PluginCapability::PUBMED);

	if (potentials.empty())
		return false;

	PluginManager *pluginManager = _global_plugins;

	std::list<Plugin*> plugins = pluginManager->getEnabledPlugins();
	std::list<Plugin*>::iterator it = plugins.begin ();
	std::list<Plugin*>::iterator end = plugins.end ();

	bool success = false;

	for (; it != end; ++it) {
		if (!(*it)->cap_.hasAny(potentials)) {
			std::cerr << "Document::getMetaData: module '"
				<< (*it)->getShortName() << "' has no "
				"suitable capabilities\n";
			continue;
		}

		std::cerr << "Document::getMetaData: trying module '"
			<< (*it)->getShortName() << "'\n";
		success = (*it)->resolve(*this);

		if (success) {
			std::cerr << "BibData::resolveDoi: paydirt with module '"
				<< (*it)->getShortName() << "'\n";
			break;
		}
	}

	/*
	 * Set up the key if it was never set to begin with
	 */
	if (success) {
		if (getKey().substr(0, defaultKey_.size()) == defaultKey_)
			setKey(generateKey ());
	}

	return success;
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

	Glib::ustring::size_type pos = shortname.rfind (".");
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


void Document::setField (Glib::ustring const &field, Glib::ustring const &value)
{
	if (field == "doi")
		bib_.setDoi (value);
	else if (field == "title")
		bib_.setTitle (value);
	else if (field == "volume")
		bib_.setVolume (value);
	else if (field == "number")
		bib_.setIssue (value);
	else if (field == "journal")
		bib_.setJournal (value);
	else if (field == "author")
		bib_.setAuthors (value);
	else if (field == "year")
		bib_.setYear (value);
	else if (field == "pages")
		bib_.setPages (value);
	else {
		bib_.extras_[field] = value;
	}
}


/* Can't be const because of std::map[] */
Glib::ustring Document::getField (Glib::ustring const &field)
{
	if (field == "doi")
		return bib_.getDoi ();
	else if (field == "title")
		return bib_.getTitle ();
	else if (field == "volume")
		return bib_.getVolume ();
	else if (field == "number")
		return bib_.getIssue ();
	else if (field == "journal")
		return bib_.getJournal ();
	else if (field == "author")
		return bib_.getAuthors ();
	else if (field == "year")
		return bib_.getYear ();
	else if (field == "pages")
		return bib_.getPages ();
	else {
		if (bib_.extras_.find(field) != bib_.extras_.end()) {
			const Glib::ustring _field = field;
			return bib_.extras_[_field];
		} else {
			std::cerr << "Document::getField: WARNING: unknown field "
				<< field << "\n";
		}
	}
}


bool Document::hasField (Glib::ustring const &field) const
{
	if (field == "doi")
		return !bib_.getDoi ().empty();
	else if (field == "title")
		return !bib_.getTitle ().empty();
	else if (field == "volume")
		return !bib_.getVolume ().empty();
	else if (field == "number")
		return !bib_.getIssue ().empty();
	else if (field == "journal")
		return !bib_.getJournal ().empty();
	else if (field == "author")
		return !bib_.getAuthors ().empty();
	else if (field == "year")
		return !bib_.getYear ().empty();
	else if (field == "pages")
		return !bib_.getPages ().empty();
	else {
		if (bib_.extras_.find(field) != bib_.extras_.end())
			return true;
		else
			return false;
	}
}


/*
 * Metadata fields.  Does not include document key or type
 */
std::map <Glib::ustring, Glib::ustring> Document::getFields ()
{
	std::map <Glib::ustring, Glib::ustring> fields;

	if (!bib_.getDoi ().empty())
		fields["doi"] = bib_.getDoi();

	if (!bib_.getTitle ().empty())
		fields["title"] = bib_.getTitle();

	if (!bib_.getVolume ().empty())
		fields["volume"] = bib_.getVolume();

	if (!bib_.getIssue ().empty())
		fields["number"] = bib_.getIssue();

	if (!bib_.getJournal ().empty())
		fields["journal"] = bib_.getJournal();

	if (!bib_.getAuthors ().empty())
		fields["author"] = bib_.getAuthors();

	if (!bib_.getYear ().empty())
		fields["year"] = bib_.getYear();

	if (!bib_.getPages ().empty())
		fields["pages"] = bib_.getPages();


		
	BibData::ExtrasMap::iterator it = bib_.extras_.begin ();
	BibData::ExtrasMap::iterator end = bib_.extras_.end ();
	for (; it != end; ++it) {
		fields[(*it).first] = (*it).second;
	}

	return fields;
}


void Document::clearFields ()
{
	bib_.extras_.clear ();
	setField ("doi", "");
	setField ("title", "");
	setField ("volume", "");
	setField ("number", "");
	setField ("journal", "");
	setField ("author", "");
	setField ("year", "");
	setField ("pages", "");
}


bool Document::parseBibtex (Glib::ustring const &bibtex)
{
	BibUtils::param p;
	BibUtils::bibl b;
	BibUtils::bibl_init( &b );
	BibUtils::bibl_initparams( &p, BibUtils::FORMAT_BIBTEX, BIBL_MODSOUT);

	try {
		/* XXX should convert this to latin1? */
		BibUtils::biblFromString (b, bibtex, BibUtils::FORMAT_BIBTEX, p);
		if (b.nrefs != 1)
			return false;

		Document newdoc = BibUtils::parseBibUtils (b.ref[0]);

		getBibData().mergeIn (newdoc.getBibData());	
		
		BibUtils::bibl_free( &b );
	} catch (Glib::Error ex) {
		BibUtils::bibl_free( &b );
		Utility::exceptionDialog (&ex, _("Parsing BibTeX"));
		return false;
	}
}
