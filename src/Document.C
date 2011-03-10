
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


const Glib::ustring Document::defaultKey_ = _("Unnamed");
Glib::RefPtr<Gdk::Pixbuf> Document::loadingthumb_;


Document::~Document ()
{
	ThumbnailGenerator::instance().deregisterRequest (this);
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
	Glib::ustring const &notes,
	Glib::ustring const &key,
	std::vector<int> const &tagUids,
	BibData const &bib)
{
	view_ = NULL;
	setFileName (filename);
	setNotes (notes);
	key_ = key;
	tagUids_ = tagUids;
	bib_ = bib;
	relfilename_ = relfilename;
}

Glib::ustring Document::keyReplaceDialogNotUnique (
	Glib::ustring const &original,
	Glib::ustring const &replacement)
{
	return keyReplaceDialog(original, replacement,
		_("The chosen key conflicts with an "
			"existing one.  Replace '%1' with '%2'?"));
}

Glib::ustring Document::keyReplaceDialogInvalidChars (
	Glib::ustring const &original,
	Glib::ustring const &replacement)
{
	return keyReplaceDialog(original, replacement,
		_("The chosen key contained invalid characters."
			"  Replace '%1' with '%2'?"));
}

Glib::ustring Document::keyReplaceDialog (
	Glib::ustring const &original,
	Glib::ustring const &replacement,
	const char *message_text)
{
	Glib::ustring message = String::ucompose (
		"<big><b>%1</b></big>\n\n%2",
		_("Key naming conflict"),
		String::ucompose (
			message_text,
			original, replacement));

	Gtk::MessageDialog dialog (message, true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
	Gtk::Button *button;

	button = dialog.add_button (_("_Ignore"), Gtk::RESPONSE_CANCEL);
	Gtk::Image *noImage = Gtk::manage (new Gtk::Image (Gtk::Stock::NO, Gtk::ICON_SIZE_BUTTON));
	button->set_image (*noImage);

	button = dialog.add_button (_("_Replace"), Gtk::RESPONSE_ACCEPT);
	Gtk::Image *yesImage = Gtk::manage (new Gtk::Image (Gtk::Stock::YES, Gtk::ICON_SIZE_BUTTON));
	button->set_image (*yesImage);
	dialog.set_default_response (Gtk::RESPONSE_ACCEPT);

	if (dialog.run () == Gtk::RESPONSE_ACCEPT)
		return replacement;
	else
		return original;
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
		Glib::ustring filename = Gio::File::create_for_uri(filename_)->query_info()->get_display_name();

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


	ThumbnailGenerator::instance().registerRequest (filename_, this);
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
	ThumbnailGenerator::instance().deregisterRequest (this);

	if (filename != filename_) {
		filename_ = filename;
		setupThumbnail ();
	} else if (!thumbnail_) {
		setupThumbnail ();
	}
}

Glib::ustring const & Document::getNotes () const
{
	return notes_;
}

void Document::setNotes (Glib::ustring const &notes)
{
	if (notes != notes_)
		notes_ = notes;
}


void Document::updateRelFileName (Glib::ustring const &libfilename)
{
	Glib::ustring const newrelfilename =
		Utility::relPath (libfilename, getFileName ());
	if (!newrelfilename.empty())
		relfilename_ = Utility::relPath (libfilename, getFileName ());
	DEBUG (String::ucompose ("Set refilename_ '%1'", relfilename_));
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
		std::ostringstream num;
		num << uid;
	} else {
		tagUids_.push_back(uid);
	}
}


void Document::clearTag(int uid)
{
	std::vector<int>::iterator location =
		std::find(tagUids_.begin(), tagUids_.end(), uid);

	if (location != tagUids_.end())
		tagUids_.erase(location);
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


/**
 * Temporarily duplicating functionality in printBibtex and 
 * writeBibtex -- the difference is that writeBibtex requires a 
 * Library reference in order to resolve tag uids to names.
 * In order to be usable from PythonDocument printBibtex just 
 * doesn't bother printing tags at all.
 *
 * This will get fixed when the ill-conceived tag ID system is 
 * replaced with lists of strings
 */
Glib::ustring Document::printBibtex (
	bool const useBraces,
	bool const utf8)
{
	std::ostringstream out;

	// BibTeX values cannot be larger than 1000 characters - should make sure of this
	// We should strip illegal characters from key in a predictable way
	out << "@" << bib_.getType() << "{" << key_ << ",\n";

	BibData::ExtrasMap extras = bib_.getExtras ();
	BibData::ExtrasMap::iterator it = extras.begin ();
	BibData::ExtrasMap::iterator const end = extras.end ();
	for (; it != end; ++it) {
		// Exceptions to useBraces are editor and author because we
		// don't want "Foo, B.B. and John Bar" to be literal
		writeBibKey (
			out,
			(*it).first,
			(*it).second,
			((*it).first.lowercase () != "editor") && useBraces, utf8);
	}

	// Ideally should know what's a list of human names and what's an
	// institution name be doing something different for non-human-name authors?
	writeBibKey (out, "author",  bib_.getAuthors(), false, utf8);
	writeBibKey (out, "title",   bib_.getTitle(), useBraces, utf8);
	writeBibKey (out, "journal", bib_.getJournal(), useBraces, utf8);
	writeBibKey (out, "volume",  bib_.getVolume(), useBraces, utf8);
	writeBibKey (out, "number",  bib_.getIssue(), useBraces, utf8);
	writeBibKey (out, "pages",   bib_.getPages(), useBraces, utf8);
	writeBibKey (out, "year",    bib_.getYear(), useBraces, utf8);
	writeBibKey (out, "doi",    bib_.getDoi(), useBraces, utf8);

	out << "}\n\n";

	return out.str();
}


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
		out << "\ttags = \"";
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

void Document::writeXML (Glib::ustring &out)
{
	out += "  <doc>\n";
	out += "    <relative_filename>" + escape_text(getRelFileName())
		+ "</relative_filename>\n";
	out += "    <key>" + escape_text(getKey())
		+ "</key>\n";
	out += "    <notes>" + escape_text(getNotes())
		+ "</notes>\n";

	std::vector<int> docvec = getTags();
	for (std::vector<int>::iterator it = docvec.begin();
		   it != docvec.end(); ++it) {
		std::ostringstream stream;
		stream << (*it);
		out += "    <tagged>" + stream.str() + "</tagged>\n";
	}

	getBibData().writeXML (out);

	out += "  </doc>\n";
}


bool Document::readPDF ()
{
	if (filename_.empty()) {
		DEBUG ("Document::readPDF: has no filename");
		return false;
	}

	std::string contentType = Gio::File::create_for_uri(filename_)->query_info("standard::content-type")->get_content_type();
	if (contentType != "application/pdf")
		return false;

	GError *error = NULL;
	PopplerDocument *popplerdoc = poppler_document_new_from_file (filename_.c_str(), NULL, &error);
	if (popplerdoc == NULL) {
		DEBUG ("Document::readPDF: Failed to load '%1'", filename_);
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

	DEBUG ("%1", textdump);
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


/* [bert] hack this to handle searches with a space like iTunes. For
 * now we just treat the whole search string as the boolean "AND" of
 * each individual component, and check the whole string against each
 * document.
 *
 * A better way to do this would be to change the whole search
 * procedure to narrow the search progressively by term, so that
 * additional terms search only through an already-filtered list.
 * But this would require changing higher-level code, for example, 
 * the DocumentView::isVisible() method that calls this function would
 * have to change substantially.
 */

bool Document::matchesSearch (Glib::ustring const &search)
{
    /* This is a bit of a hack, I guess, but it's a low-impact way of 
     * implementing the change. If the search term contains a space, I 
     * iteratively decompose it into substrings and pass those onto
     * this function. If anything doesn't match, we return a failure.
     */
    if (search.find(' ') != Glib::ustring::npos) {
        Glib::ustring::size_type p1 = 0;
        Glib::ustring::size_type p2;

        do {

            /* Find the next space in the string, if any.
             */
            p2 = search.find(' ', p1);

            /* Extract the appropriate substring.
             */
            Glib::ustring const searchTerm = search.substr(p1, p2);

            /* If the term is empty, ignore it and move on. It might just be a 
             * trailing or duplicate space character.
             */
            if (searchTerm.empty()) {
                break;
            }

            /* Now that we have the substring, which is guaranteed to be
             * free of spaces, we can pass it recursively into this function.
             * If the term does NOT match, fail the entire comparison right away.
             */
            if (!matchesSearch(searchTerm)) {
                return false;
            }

            p1 = p2 + 1;		/* +1 to skip over the space */

        } while (p2 != Glib::ustring::npos); /* Terminate at end of string */
        return true;		/* All matched, so OK. */
    }

    Glib::ustring const searchNormalised = search.casefold();

    FieldMap fields = getFields ();
    FieldMap::iterator fieldIter = fields.begin ();
    FieldMap::iterator const fieldEnd = fields.end ();
    for (; fieldIter != fieldEnd; ++fieldIter) {
        if (fieldIter->second.casefold().find(searchNormalised) != Glib::ustring::npos)
            return true;
    }

    if (notes_.casefold().find(searchNormalised) != Glib::ustring::npos)
        return true;

    if (key_.casefold().find(searchNormalised) != Glib::ustring::npos)
        return true;

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

	if (hasField("url"))
		potentials.add(PluginCapability::URL);

	if (potentials.empty())
		return false;

	PluginManager *pluginManager = _global_plugins;

	std::list<Plugin*> plugins = pluginManager->getEnabledPlugins();
	std::list<Plugin*>::iterator it = plugins.begin ();
	std::list<Plugin*>::iterator end = plugins.end ();

	bool success = false;

	for (; it != end; ++it) {
		if (!(*it)->cap_.hasAny(potentials)) {
			DEBUG ("Document::getMetaData: module '%1' has no "
				"suitable capabilities", (*it)->getShortName());
			continue;
		}

		DEBUG ("Document::getMetaData: trying module '%1'",
			(*it)->getShortName());
		success = (*it)->resolve(*this);

		if (success) {
			DEBUG ("BibData::resolveDoi: paydirt with module '%1'",
				(*it)->getShortName());
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

	Glib::RefPtr<Gio::File> oldfile = Gio::File::create_for_uri(getFileName());

	Glib::ustring shortname = oldfile->query_info()->get_display_name();
	DEBUG ("Shortname = %1", shortname);
	Glib::RefPtr<Gio::File> parentdir = oldfile->get_parent();

	Glib::ustring::size_type pos = shortname.rfind (".");
	Glib::ustring extension = "";
	if (pos != Glib::ustring::npos)
		extension = shortname.substr (pos, shortname.length() - 1);

	Glib::ustring newfilename = getKey() + extension;
	DEBUG ("Newfilename = %1", newfilename);

	Glib::RefPtr<Gio::File> newfile = parentdir->get_child(newfilename);

	try {
		oldfile->move(newfile);
		setFileName (newfile->get_uri ());
	} catch (Gio::Error &ex) {
		Utility::exceptionDialog (&ex,
			String::ucompose (_("Moving '%1' to '%2'"),
				oldfile->get_uri (),
				newfile->get_uri ())
			);
	}
}


void Document::setField (Glib::ustring const &field, Glib::ustring const &value)
{
	DEBUG (String::ucompose ("%1 : %2", field, value));
	if (field == "doi")
		bib_.setDoi (value);
	else if (field.lowercase() == "title")
		bib_.setTitle (value);
	else if (field.lowercase() == "volume")
		bib_.setVolume (value);
	else if (field.lowercase() == "number")
		bib_.setIssue (value);
	else if (field.lowercase() == "journal")
		bib_.setJournal (value);
	else if (field.lowercase() == "author")
		bib_.setAuthors (value);
	else if (field.lowercase() == "year")
		bib_.setYear (value);
	else if (field.lowercase() == "pages")
		bib_.setPages (value);
	else if (field == "key")
		setKey (value);
	else {
		/* The extras map uses a case-folding comparator */
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
	else if (field == "key")
		return getKey();
	else {
		if (bib_.extras_.find(field) != bib_.extras_.end()) {
			const Glib::ustring _field = field;
			return bib_.extras_[_field];
		} else {
			DEBUG ("Document::getField: WARNING: unknown field %1", field);
			throw std::range_error("Document::getField: unknown field");
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
