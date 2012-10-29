
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

#include "config.h"
#include "CrossRefPlugin.h"

class CrossRefParser : public Glib::Markup::Parser {
	BibData &bib_;

	Glib::ustring text_;
	Glib::ustring given_name_;
	std::vector<Glib::ustring> authors_;

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
		text_ = "";
		// Should use a more reliable check than this
		if (element_name == "html" || element_name == "HTML") {
			DEBUG ("html tag found, throwing error");
			Glib::MarkupError error (
				Glib::MarkupError::INVALID_CONTENT,
				"Looks like a HTML document, not an XML document");
			throw error;
		}

		if (element_name == "query") {
			Glib::ustring statusString;
			Glib::Markup::Parser::AttributeMap::const_iterator found = attributes.find ("status");
			if (found != attributes.end())
				statusString = found->second;


			if (statusString == "unresolved") {
				DEBUG ("CrossRefParser: query failed, throwing error");
				Glib::MarkupError error (
					Glib::MarkupError::INVALID_CONTENT,
					"Looks like a HTML document, not an XML document");
				throw error;
			}
		}

		if (element_name == "doi") {
			Glib::ustring typeString;
			Glib::Markup::Parser::AttributeMap::const_iterator found = attributes.find ("type");
			if (found != attributes.end())
				typeString = found->second;

			if (typeString == "conference_paper") {
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

		if (element == "doi") {
			bib_.setDoi (text_);
		} else if (element == "article_title") {
			bib_.setTitle (text_);
		/* FIXME: assuming given_name precedes surname */
		} else if (element == "given_name") {
			given_name_ = text_;
		} else if (element == "surname") {
			if (!given_name_.empty()) {
				text_ = text_ + ", " + given_name_;
				given_name_ = "";
			}
			authors_.push_back (text_);
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
		} else if (element == "body") {
			/* End of entry */
			Glib::ustring authorString;
			std::vector<Glib::ustring>::iterator it = authors_.begin ();
			for (; it != authors_.end(); ++it) {
				if (it != authors_.begin()) {
					authorString += " and ";
				}
				authorString += *it;
			}
			if (!authorString.empty())
				bib_.setAuthors (authorString);
		}
	}

 	// Called on error, including one thrown by an overridden virtual method.
	virtual void on_error (
		Glib::Markup::ParseContext& context,
		const Glib::MarkupError& error)
	{
		DEBUG ("CrossRefParser: Parse Error!");
	}

	virtual void on_text (
		Glib::Markup::ParseContext& context,
		const Glib::ustring& text)
	{
		text_ += text;
	}
};


CrossRefPlugin::CrossRefPlugin () {
	loaded_ = true;
	cap_.add(PluginCapability::DOI);

	xml_ = Gtk::Builder::create_from_file (Utility::findDataFile ("crossref.ui"));

	xml_->get_widget ("Crossref", dialog_);
	xml_->get_widget ("Username", usernameEntry_);
	xml_->get_widget ("Password", passwordEntry_);

	usernameEntry_->signal_changed().connect (
		sigc::mem_fun (*this, &CrossRefPlugin::onPrefsChanged));
	passwordEntry_->signal_changed().connect (
		sigc::mem_fun (*this, &CrossRefPlugin::onPrefsChanged));
}


int CrossRefPlugin::canResolve (Document &doc) {
	if (doc.hasField("doi"))
		return 10; /* Return low priority */
	return -1;
}


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
				_("To use the CrossRef service, a free account is needed.  "
				  "Login information may be set in Preferences, or the CrossRef plugin "
				  "may be disabled.")
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
				doConfigure ();
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

	Glib::ustring const username = _global_prefs->getCrossRefUsername ();
	Glib::ustring const password = _global_prefs->getCrossRefPassword ();

	Glib::ustring const url = 
		  Glib::ustring("http://www.crossref.org/openurl/?pid=")
		+ username
		+ (password.empty() ? "" : ":")
		+ password 
		+ Glib::ustring("&id=doi:")
		+ Glib::uri_escape_string(doc.getField("doi"))
		+ Glib::ustring ("&noredirect=true");

	DEBUG ("CrossRefPlugin::resolve: using url '%1'", url);

	// FIXME: even if we don't get any metadata, 
	// an exceptionless download+parse is considered
	// a success.
	// Nobody notices as long as crossref is the last resort
	bool success = true;

	try {
		Glib::ustring &xml = Transfer::readRemoteFile (
			_("Downloading Metadata"), messagetext, url);

		DEBUG (xml);

		// XXX
		// Test for "Missing WWW-Authenticate header" for bad username/password
		// Test for "No DOI found" for bad DOI

		CrossRefParser parser (doc.getBibData());
		Glib::Markup::ParseContext context (parser);
		try {
			context.parse (xml);
			context.end_parse ();
		} catch (Glib::MarkupError const ex) {
			DEBUG ("Markuperror while parsing:\n'''%1\n'''", xml);
			//Utility::exceptionDialog (&ex, _("Parsing CrossRef XML.  The DOI could be invalid, or not known to crossref.org"));
			success = false;
		}
	} catch (Transfer::Exception ex) {
		//Utility::exceptionDialog (&ex, _("Downloading metadata"));
		success = false;
	}

	DEBUG ("resolve returning %1", success);
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


Glib::ustring const CrossRefPlugin::getAuthor ()
{
	return Glib::ustring ("John Spray");
}


Glib::ustring const CrossRefPlugin::getVersion ()
{
	return Glib::ustring (VERSION);
}

void CrossRefPlugin::onPrefsChanged ()
{
	if (ignoreChanges_)
		return;

	_global_prefs->setCrossRefUsername (usernameEntry_->get_text ());
	_global_prefs->setCrossRefPassword (passwordEntry_->get_text ());
}

void CrossRefPlugin::doConfigure ()
{
	ignoreChanges_ = true;
	usernameEntry_->set_text (_global_prefs->getCrossRefUsername ());
	passwordEntry_->set_text (_global_prefs->getCrossRefPassword ());
	ignoreChanges_ = false;

	dialog_->run ();
	dialog_->hide ();
}
