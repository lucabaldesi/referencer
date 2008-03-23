
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef LIBRARYPARSER_H
#define LIBRARYPARSER_H

#include <gtkmm.h>
#include "Library.h"
#include "DocumentList.h"
#include "TagList.h"
#include "BibData.h"
#include "ucompose.hpp"

class LibraryParser : public Glib::Markup::Parser {
	Library &library_;
	DocumentList &doclist_;
	TagList &taglist_;

	bool inTag_;
	bool inUid_;
	bool inTagName_;

	Glib::ustring newTagUid_;
	Glib::ustring newTagName_;

	bool inDoc_;
	bool inKey_;
	bool inFileName_;
	bool inRelFileName_;
	bool inTagged_;
	bool inBibItem_;

	Glib::ustring newDocKey_;
	Glib::ustring newDocFileName_;
	Glib::ustring newDocRelFileName_;
	Glib::ustring newDocTag_;
	std::vector<int> newDocTags_;
	Glib::ustring bibText_;
	Glib::ustring bibExtraKey_;

	Glib::ustring textBuffer_;

	bool manageBraces_;
	bool manageUTF8_;

	BibData newDocBib_;

	public:
	LibraryParser (Library &library, TagList &taglist, DocumentList &doclist)
		: library_(library), doclist_(doclist), taglist_(taglist)
	{
		inTag_ = false;
		inUid_ = false;
		inTagName_ = false;

		inDoc_ = false;
		inKey_ = false;
		inFileName_ = false;
		inRelFileName_ = false;
		inTagged_ = false;
		inBibItem_ = false;

		manageBraces_ = false;
		manageUTF8_ = false;
	}

	private:

	bool boolFromStr (Glib::ustring const &str) {
		if (str == "true") {
			return true;
		} else if (str == "false") {
			return false;
		} else {
			throw new Glib::MarkupError (Glib::MarkupError::INVALID_CONTENT,
				String::ucompose (_("'%s' is not a valid boolean value"), str));
		}
	}

	virtual void on_start_element (
		Glib::Markup::ParseContext& context,
		const Glib::ustring& element_name,
		const Glib::Markup::Parser::AttributeMap& attributes)
	{
		textBuffer_ = "";
		//std::cerr << "Started element " << element_name << "\n";
		if (element_name == "tag") {
			inTag_ = true;
			newTagUid_ = "";
			newTagName_ = "";
		} else if (inTag_ && element_name == "uid") {
			inUid_ = true;
		} else if (inTag_ && element_name == "name") {
			inTagName_ = true;
		}
		else if (element_name == "doc") {
			inDoc_ = true;
			newDocFileName_ = "";
			newDocRelFileName_ = "";
			newDocKey_ = "";
			newDocTag_ = "";
			newDocTags_.clear();
			newDocBib_.clear();
		} else if (inDoc_ && element_name == "filename") {
			inFileName_ = true;
		} else if (inDoc_ && element_name == "relative_filename") {
			inRelFileName_ = true;
		} else if (inDoc_ && element_name == "key") {
			inKey_ = true;
		} else if (inDoc_ && element_name == "tagged") {
			inTagged_ = true;
		} else if (element_name == "bib_doi"
		           || element_name == "bib_type"
		           || element_name == "bib_title"
		           || element_name == "bib_authors"
		           || element_name == "bib_journal"
		           || element_name == "bib_volume"
		           || element_name == "bib_number"
		           || element_name == "bib_pages"
		           || element_name == "bib_year"
		           || element_name == "") {
			inBibItem_ = true;
			bibText_ = "";
		} else if (element_name == "bib_extra") {
			inBibItem_ = true;
			bibText_ = "";
			Glib::Markup::Parser::AttributeMap::const_iterator attrib = attributes.find ("key");
			std::pair<Glib::ustring, Glib::ustring> keyval = *attrib;
			bibExtraKey_ = keyval.second;
		} else if (element_name == "manage_target") {
			Glib::Markup::Parser::AttributeMap::const_iterator attrib = attributes.find ("braces");
			std::pair<Glib::ustring, Glib::ustring> keyval;
			if (attrib != attributes.end ()) {
				keyval = *attrib;
				manageBraces_ = boolFromStr (keyval.second);
			}
			attrib = attributes.find ("utf8");
			if (attrib != attributes.end ()) {
				keyval = *attrib;
				manageUTF8_ = boolFromStr (keyval.second);
			}
		}

	}

	virtual void	on_end_element (
		Glib::Markup::ParseContext& context,
		const Glib::ustring& element_name)
	{
		//std::cerr << "Ended element " << element_name << "\n";
		if (element_name == "tag") {
			inTag_ = false;
			// Hardcoded for ATTACH tags only
			taglist_.loadTag (newTagName_, atoi(newTagUid_.c_str()));
		} else if (inTag_ && element_name == "uid") {
			inUid_ = false;
		} else if (inTag_ && element_name == "name") {
			inTagName_ = false;
		}
		else if (element_name == "doc") {
			inDoc_ = false;
			doclist_.loadDoc (
				newDocFileName_, newDocRelFileName_, newDocKey_, newDocTags_, newDocBib_);
		} else if (inDoc_ && element_name == "filename") {
			inFileName_ = false;
		} else if (inDoc_ && element_name == "relative_filename") {
			inRelFileName_ = false;
		} else if (inDoc_ && element_name == "key") {
			inKey_ = false;
		} else if (inDoc_ && element_name == "tagged") {
			inTagged_ = false;
			newDocTags_.push_back(atoi(newDocTag_.c_str()));
			newDocTag_ = "";
		} else if (element_name == "bib_doi") {
			inBibItem_ = false;
			newDocBib_.setDoi (bibText_);
		} else if (element_name == "bib_title") {
			inBibItem_ = false;
			newDocBib_.setTitle (bibText_);
		} else if (element_name == "bib_type") {
			inBibItem_ = false;
			newDocBib_.setType (bibText_);
		} else if (element_name == "bib_authors") {
			inBibItem_ = false;
			newDocBib_.setAuthors (bibText_);
		} else if (element_name == "bib_journal") {
			inBibItem_ = false;
			newDocBib_.setJournal (bibText_);
		} else if (element_name == "bib_volume") {
			inBibItem_ = false;
			newDocBib_.setVolume (bibText_);
		} else if (element_name == "bib_number") {
			inBibItem_ = false;
			newDocBib_.setIssue (bibText_);
		} else if (element_name == "bib_pages") {
			inBibItem_ = false;
			newDocBib_.setPages (bibText_);
		} else if (element_name == "bib_year") {
			inBibItem_ = false;
			newDocBib_.setYear (bibText_);
		} else if (element_name == "bib_extra") {
			inBibItem_ = false;
			newDocBib_.addExtra (bibExtraKey_, bibText_);
		} else if (element_name == "manage_target") {
			library_.manageBibtex (textBuffer_, manageBraces_, manageUTF8_);
		}
	}

 	// Called on error, including one thrown by an overridden virtual method.
	virtual void on_error (
		Glib::Markup::ParseContext& context,
		const Glib::MarkupError& error)
	{
		std::cerr << "LibraryParser: Parse Error!\n";
	}

	virtual void on_text (
		Glib::Markup::ParseContext& context,
		const Glib::ustring& text)
	{
		if (inUid_)
			newTagUid_ += text;
		else if (inTagName_)
			newTagName_ += text;

		else if (inFileName_) {
			newDocFileName_ += text;
		} else if (inRelFileName_) {
			newDocRelFileName_ += text;
		}
		else if (inKey_)
			newDocKey_ += text;
		else if (inTagged_)
			newDocTag_ += text;
		else if (inBibItem_)
			bibText_ += text;
		else {
			textBuffer_ += text;
		}

	}
};

#endif
