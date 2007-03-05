
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef CROSSREFPARSER_H
#define CROSSREFPARSER_H

#include <gtkmm.h>
#include "DocumentList.h"
#include "TagList.h"

class CrossRefParser : public Glib::Markup::Parser {
	BibData &bib_;

	Glib::ustring text_;

	public:
	CrossRefParser (BibData &bib)
		: bib_(bib)
	{

	}

	private:
	virtual void on_start_element (
		Glib::Markup::ParseContext& context,
		const Glib::ustring& element_name,
		const Glib::Markup::Parser::AttributeMap& attributes)
	{
		//std::cerr << "CrossRefParser: Started element '" << element_name << "'\n";
		text_ = "";
		// Should use a more reliable check than this
		if (element_name == "html" || element_name == "HTML") {
			std::cerr << "html tag found, throwing error\n";
			Glib::MarkupError error (
				Glib::MarkupError::INVALID_CONTENT,
				"Looks like a HTML document, not an XML document");
			throw error;
		}
	}

	virtual void	on_end_element (
		Glib::Markup::ParseContext& context,
		const Glib::ustring& element)
	{
		//std::cerr << "CrossRefParser: Ended element '" << element << "'\n";
		if (element == "doi") {
			bib_.setDoi (text_);
		} else if (element == "article_title") {
			bib_.setTitle (text_);
		// Should handle multiple 'author' elements
		} else if (element == "author") {
			bib_.setAuthors (text_);
		} else if (element == "journal_title") {
			bib_.setJournal (text_);
		} else if (element == "volume") {
			bib_.setVolume (text_);
		} else if (element == "issue") {
			bib_.setIssue (text_);
		} else if (element == "first_page") {
			bib_.setPages (text_);
		} else if (element == "year") {
			bib_.setYear (text_);
		}
	}

 	// Called on error, including one thrown by an overridden virtual method.
	virtual void on_error (
		Glib::Markup::ParseContext& context,
		const Glib::MarkupError& error)
	{
		std::cerr << "CrossRefParser: Parse Error!\n";
	}

	virtual void on_text (
		Glib::Markup::ParseContext& context,
		const Glib::ustring& text)
	{
		if (Glib::str_has_prefix (text, "<![CDATA[") &&
				Glib::str_has_suffix (text, "]]>")) {
			text_ += text.substr (strlen ("<![CDATA["),
				text.length () - strlen ("<![CDATA[" "]]>"));
		} else {
			text_ += text;	
		}
	}
};

#endif
