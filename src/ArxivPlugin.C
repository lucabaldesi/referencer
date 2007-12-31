
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

#include "BibData.h"
#include "BibUtils.h"
#include "Preferences.h"
#include "Transfer.h"
#include "Utility.h"

#include "ArxivPlugin.h"

bool ArxivPlugin::resolve (Document &doc)
{
	if (!doc.hasField("eprint") || _global_prefs->getWorkOffline())
		return false;

	Glib::ustring messagetext =
		String::ucompose (
			"<b><big>%1</big></b>\n\n%2\n",
			_("Retrieving metadata"),
			String::ucompose (
				_("Contacting citebase.org to retrieve metadata for '%1'"),
				doc.getField("eprint"))
		);

	Glib::ustring arxivid = doc.getField("eprint");
	Glib::ustring::size_type index = arxivid.find ("v");
	if (index != Glib::ustring::npos) {
		arxivid = arxivid.substr (0, index);
	}

	arxivid = Glib::Markup::escape_text (arxivid);

	Glib::ustring const filename = "http://www.citebase.org/openurl?url_ver=Z39.88-2004&svc_id=bibtex&rft_id=oai%3AarXiv.org%3A" + arxivid;

	std::cerr << filename << "\n";

	Glib::ustring *rawtext;
	try {
		rawtext = &Transfer::readRemoteFile (
			_("Downloading Metadata"), messagetext, filename);

		std::cerr << "Raw citebase:\n" << *rawtext << "\n----\n\n";
	} catch (Transfer::Exception ex) {
		Utility::exceptionDialog (&ex, _("Downloading metadata"));
		return false;
	}

	BibUtils::param p;
	BibUtils::bibl b;
	BibUtils::bibl_init( &b );
	BibUtils::bibl_initparams( &p, BibUtils::FORMAT_BIBTEX, BIBL_MODSOUT);

	try {
		BibUtils::biblFromString (b, *rawtext, BibUtils::FORMAT_BIBTEX, p);

		Document newdoc = BibUtils::parseBibUtils (b.ref[0]);

		// Sometimes citebase gives us an URL which is just a doi
		Glib::ustring const url = newdoc.getBibData().extras_["Url"];
		std::cerr << "url = " << url << "\n";
		std::cerr << "substr = " <<  url.substr (0, 4) << "\n";
		if (url.size() >= 5 && url.substr (0, 4) == Glib::ustring("doi:")) {
			if (newdoc.getBibData().getDoi().empty()) {
				newdoc.getBibData().setDoi (url.substr(4, url.size()));
				BibData::ExtrasMap::iterator it = newdoc.getBibData().extras_.find("Url");
				newdoc.getBibData().extras_.erase(it);
			}
		}

		doc.getBibData().mergeIn (newdoc.getBibData());	
		
		BibUtils::bibl_free( &b );
	} catch (Glib::Error ex) {
		BibUtils::bibl_free( &b );
		Utility::exceptionDialog (&ex, _("Parsing BibTeX"));
		return false;
	}

	return true;
}


Glib::ustring const ArxivPlugin::getShortName ()
{
	return Glib::ustring ("arxiv");
}


Glib::ustring const ArxivPlugin::getLongName ()
{
	return Glib::ustring (_("Arxiv.org ArXiv-ID resolver"));
}


