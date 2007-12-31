
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */


#include <iostream>

#include <gtkmm.h>
#include <glibmm/i18n.h>

#include "ucompose.hpp"

#include "Document.h"
#include "Preferences.h"
#include "Transfer.h"
#include "Utility.h"

#include "CrossRefPlugin.h"

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
		if (element_name == "doi") {
			Glib::ustring const typestring = (*(attributes.find ("type"))).second;
			if (typestring == "conference_paper") {
				bib_.setType("InProceedings");
			} else {
				bib_.setType("Article");
			}
		}
	}

	virtual void	on_end_element (
		Glib::Markup::ParseContext& context,
		const Glib::ustring& element)
	{
		if (Glib::str_has_prefix (text_, "<![CDATA[") &&
				Glib::str_has_suffix (text_, "]]>")) {
			text_ = text_.substr (strlen ("<![CDATA["),
				text_.length () - strlen ("<![CDATA[" "]]>"));
		}

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
		} else if (element == "volume_title") {
			bib_.addExtra ("BookTitle", text_);
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
		text_ += text;
	}
};


bool CrossRefPlugin::resolve (Document &doc)
{
	/*
	 * Prompt for username and password if needed
	 */
	if (_global_prefs->getCrossRefUsername ().empty ()) {
		Glib::ustring message = 
			String::ucompose (
				"<b><big>%1</big></b>\n\n%2\n",
				_("CrossRef credentials not found"),
				_("To use the CrossRef service, an account is needed.  Username and password may be set in Preferences.")
				);

		Gtk::MessageDialog dialog(message, true, Gtk::MESSAGE_WARNING,
				          Gtk::BUTTONS_NONE, true);

		dialog.add_button (Gtk::Stock::CANCEL,     0);
		dialog.add_button (_("_Preferences"),      1);
		dialog.add_button (_("_Disable CrossRef"), 2);

		do {
			int response = dialog.run ();
			if (response == 1) {
				// Preferences
				_global_prefs->showDialog ();
				if (!_global_prefs->getCrossRefUsername ().empty ())
					break;
				// if they didn't give us one then we loop around
				// else we go ahead
			} else if (response == 2) {
				// Disable
				_global_prefs->disablePlugin (this);
				return false;
			} else {
				// Cancel
				return false;
			}
		} while (1);
	}

	Glib::ustring messagetext =
		String::ucompose (
			"<b><big>%1</big></b>\n\n%2\n",
			_("Downloading metadata"),
			String::ucompose (
				_("Contacting crossref.org to retrieve metadata for '%1'"),
				doc.getField("doi"))
		);

	Glib::ustring const url = 
		  Glib::ustring("http://www.crossref.org/openurl/?pid=")
		+ _global_prefs->getCrossRefUsername ()
		+ Glib::ustring(":")
		+ _global_prefs->getCrossRefPassword ()
		+ Glib::ustring("&id=doi:")
		+ doc.getField("doi")
		+ Glib::ustring ("&noredirect=true");

	std::cerr << "CrossRefPlugin::resolve: using url '" << url << "'\n";

	// FIXME: even if we don't get any metadata, 
	// an exceptionless download+parse is considered
	// a success.
	// Nobody notices as long as crossref is the last resort
	bool success = true;

	try {
		Glib::ustring &xml = Transfer::readRemoteFile (
			_("Downloading Metadata"), messagetext, url);

		CrossRefParser parser (doc.getBibData());
		Glib::Markup::ParseContext context (parser);
		try {
			context.parse (xml);
		} catch (Glib::MarkupError const ex) {
			std::cerr << "Markuperror while parsing:\n'''\n" << xml << "\n'''\n";
			Utility::exceptionDialog (&ex, _("Parsing CrossRef XML.  The DOI could be invalid, or not known to crossref.org"));
			success = false;
		}
		context.end_parse ();
	} catch (Transfer::Exception ex) {
		Utility::exceptionDialog (&ex, _("Downloading metadata"));
		success = false;
	}

	return success;
}


Glib::ustring const CrossRefPlugin::getShortName ()
{
	return Glib::ustring ("crossref");
}


Glib::ustring const CrossRefPlugin::getLongName ()
{
	return Glib::ustring (_("Crossref.org OpenURL DOI resolver"));
}


