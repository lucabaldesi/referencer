
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */






#include <iostream>

#include <time.h>
#include <boost/regex.hpp>
#include <glibmm/i18n.h>
#include "ucompose.hpp"

#include "Transfer.h"
#include "Preferences.h"
#include "CrossRefParser.h"

#include "BibData.h"


std::vector<Glib::ustring> BibData::document_types;
Glib::ustring BibData::default_document_type;

std::vector<Glib::ustring> &BibData::getDocTypes ()
{
	if (document_types.size() == 0) {
		document_types.push_back ("Article");
		document_types.push_back ("Book");
		document_types.push_back ("Booklet");
		document_types.push_back ("Conference");
		document_types.push_back ("InBook");
		document_types.push_back ("InCollection");
		document_types.push_back ("InProceedings");
		document_types.push_back ("Manual");
		document_types.push_back ("MastersThesis");
		document_types.push_back ("Misc");
		document_types.push_back ("PhDThesis");
		document_types.push_back ("Proceedings");
		document_types.push_back ("TechReport");
		document_types.push_back ("Unpublished");
	}

	return document_types;
}


Glib::ustring &BibData::getDefaultDocType ()
{
	if (default_document_type.empty ())
		default_document_type = "Article";

	return default_document_type;
}


BibData::BibData ()
{
	// The only field that actually has a default value
	type_ = getDefaultDocType ();
}


/*
 * Dump all fields to standard out
 */
void BibData::print ()
{
	std::cout << "DOI: " << doi_ << std::endl;
	std::cout << "Title: " << title_ << std::endl;
	std::cout << "Authors: " << authors_ << std::endl;
	std::cout << "Journal: " << journal_ << std::endl;
	std::cout << "Volume: " << volume_ << std::endl;
	std::cout << "Number: " << issue_ << std::endl;
	std::cout << "Pages: " << pages_ << std::endl;
	std::cout << "Year: " << year_ << std::endl;
}


void BibData::clear ()
{
	doi_ = "";
	volume_ = "";
	issue_ = "";
	pages_ = "";
	authors_ = "";
	journal_ = "";
	title_ = "";
	year_ = "";
	extras_.clear ();
}


void BibData::addExtra (Glib::ustring const &key, Glib::ustring const &value)
{
	// Should add something to our map of extra keys
//	std::cerr << "addExtra: '" << key << ":" << value << "'\n";
	extras_[key] = value;
}


void BibData::clearExtras ()
{
	extras_.clear ();
}


using Glib::Markup::escape_text;

void BibData::writeXML (std::ostringstream &out)
{
	out << "    <bib_type>" << escape_text(type_) << "</bib_type>\n";
	out << "    <bib_doi>" << escape_text(doi_) << "</bib_doi>\n";
	out << "    <bib_title>" << escape_text(title_) << "</bib_title>\n";
	out << "    <bib_authors>" << escape_text(authors_) << "</bib_authors>\n";
	out << "    <bib_journal>" << escape_text(journal_) << "</bib_journal>\n";
	out << "    <bib_volume>" << escape_text(volume_) << "</bib_volume>\n";
	out << "    <bib_number>" << escape_text(issue_) << "</bib_number>\n";
	out << "    <bib_pages>" << escape_text(pages_) << "</bib_pages>\n";
	out << "    <bib_year>" << escape_text(year_) << "</bib_year>\n";

	ExtrasMap::iterator it = extras_.begin ();
	ExtrasMap::iterator const end = extras_.end ();
	for (; it != end; ++it) {
		out << "    <bib_extra key=\""
		    << escape_text((*it).first) << "\">"
		    << escape_text((*it).second) << "</bib_extra>\n";
	}
}


void BibData::parseCrossRefXML (Glib::ustring const &xml)
{
	CrossRefParser parser (*this);
	Glib::Markup::ParseContext context (parser);
	try {
		context.parse (xml);
	} catch (Glib::MarkupError const ex) {
		std::cerr << "Markuperror while parsing:\n'''\n" << xml << "\n'''\n";
		Utility::exceptionDialog (&ex, _("Parsing CrossRef XML.  The DOI could be invalid, or not known to crossref.org"));
	}
	context.end_parse ();
}


/*
 *  Try to fill the journal field by doing a text search
 *  for a known long form of a journal name and then
 *  using the corresponding short form.
 *
 *  Citations always use the short form, so by searching
 *  for the long form we should only pick up the name
 *  of this paper's journal.
 */
void BibData::guessJournal (Glib::ustring const &raw)
{
	int const n_titles = 2;
	Glib::ustring search_titles[n_titles]= {
		"PHYSICAL REVIEW B",
		"JOURNAL OF APPLIED PHYSICS"
	};
	Glib::ustring bib_titles[n_titles] = {
		"Phys. Rev. B",
		"J. Appl. Phys."
	};

	for (int i = 0; i < n_titles; ++i) {
		// TODO: we would like a fast, case insensitive,
		// encoding-correct search.
		if (strstr (raw.c_str(), search_titles[i].c_str())) {
			setJournal (bib_titles[i]);
			break;
		}
	}
}

/*
 * Try to fill the volume, number and pages fields
 * using the raw text -- these are lumped together
 * because in some cases our search expressions
 * may include all three.
 *
 * (Even page? --jcs)
 */
void BibData::guessVolumeNumberPage (Glib::ustring const &raw)
{
		/*
		 * Do regex magic to learn what we can
		 * Interested in strings like:
		 * "VOLUME 95, NUMBER 3"
		 * "PHYSICAL REVIEW B 71, 064413"
		 *
		 * When searching for abbreviations like Vol. 1, No. 2
		 * we must be very careful not to pick up citations
		 * by accident
		 */
}

/*
 * Try to guess the year of the paper from the raw text
 */
void BibData::guessYear (Glib::ustring const &raw_)
{
	/*
	 * Take the greatest four-letter numeric which is between
	 * 1990 and the current year.  This will occasionally
	 * pick up a 'number' or 'page' instead, but should
	 * almost always be the year.
	 */


	std::string const &raw = raw_;

	boost::regex expression("\\W([0-9][0-9][0-9][0-9])\\W");

	int latestyear = 0;
	Glib::ustring latestyear_str;

	int const dawn_of_ejournals = 1990;
	time_t const timesecs = time(NULL);
	struct tm *UTC = gmtime (&timesecs);
	int const present_day = UTC->tm_year + 1900;

	std::string::const_iterator start, end;
	start = raw.begin();
	end = raw.end();
	boost::match_results<std::string::const_iterator> what;
	boost::match_flag_type flags = boost::match_default;
	while(regex_search(start, end, what, expression, flags)) {
		Glib::ustring yearstring = std::string(what[1]);
		int yearval = atoi(yearstring.c_str());
		if (yearval > dawn_of_ejournals && yearval <= present_day) {
			if (yearval > latestyear) {
				latestyear = yearval;
				latestyear_str = yearstring;
			}
		}
	  // update search position:
	  start = what[0].second;
	  // update flags:
	  flags |= boost::match_prev_avail;
	  flags |= boost::match_not_bob;
	}

	if (latestyear)
		setYear (latestyear_str);
}


/*
 * Try to guess the authors of the paper from the raw text
 */
void BibData::guessAuthors (Glib::ustring const &raw_)
{
	std::string const &raw = raw_;

	boost::regex expression("^(\\D{5,}\n\\D{5,})$");

	std::string::const_iterator start, end;
	start = raw.begin();
	end = raw.end();
	boost::match_results<std::string::const_iterator> what;
	boost::match_flag_type flags = boost::match_default;
	while(regex_search(start, end, what, expression, flags)) {
		Glib::ustring authors = std::string(what[1]);
		std::cout << "Got authors = '" << authors << "'\n" << std::endl;
		setAuthors (authors);
		return;

	  // update search position:
	  start = what[0].second;
	  // update flags:
	  flags |= boost::match_prev_avail;
	  flags |= boost::match_not_bob;
	}
}

/*
 * Try to guess the title of the paper from the raw text
 */
void BibData::guessTitle (Glib::ustring const &raw)
{
}


/*
 * Try to guess the DOI of the paper from the raw text
 */
void BibData::guessDoi (Glib::ustring const &raw_)
{
	std::string const &raw = raw_;

	boost::regex expression(
		"(?:"
		"(?:[Dd][Oo][Ii]:? *)"
		"|"		
		"(?:[Dd]igital *[Oo]bject *[Ii]denti(?:fi|ﬁ)er:? *)"
		")"		
		"("
		"[^\\.\\s]+"
		"\\."
		"[^\\/\\s]+"
		"\\/"
		"[^\\s]+"
		")"
	);

	std::string::const_iterator start, end;
	start = raw.begin();
	end = raw.end();
	boost::match_results<std::string::const_iterator> what;
	boost::match_flag_type flags = boost::match_default;
	while(regex_search(start, end, what, expression, flags)) {
		Glib::ustring gstr = std::string(what[1]);
		int len = gstr.size ();
		// Special case to chop off trailing comma to deal with
		// "doi: foo, available online" in JCompPhys
		// Note that commas ARE legal suffix characters in Doi spec
		// But there's nothing in the spec about regexing them
		// out of PDFS :-) -jcs
		if (gstr[len - 1] == ',') {
			gstr = gstr.substr (0, len - 1);
		};
		setDoi (gstr);
		return;
	}
}


/*
 * Try to extract the Arxiv eprint value of the paper from the raw text
 */
// Should really only be looking on first page of doc for this one
void BibData::guessArxiv (Glib::ustring const &raw_)
{
	std::string const &raw = raw_;

	boost::regex expression(
	"arXiv:"
	"("
	"[^\\/\\s]+"
	"[\\/\\.]"
	"[^\\s]+"
	")"
	);

	std::string::const_iterator start, end;
	start = raw.begin();
	end = raw.end();
	boost::match_results<std::string::const_iterator> what;
	boost::match_flag_type flags = boost::match_default;
	while(regex_search(start, end, what, expression, flags)) {
		Glib::ustring gstr = std::string(what[1]);
		addExtra ("eprint", gstr);
		return;
	}
}


void BibData::getCrossRef ()
{
	if (doi_.empty() || _global_prefs->getWorkOffline())
		return;
		
	Glib::ustring messagetext =
	String::ucompose (
		"<b><big>%1</big></b>\n\n%2\n",
		_("Downloading metadata"),
		String::ucompose (
			_("Contacting crossref.org to retrieve metadata for '%1'"),
			doi_)
		);

	Utility::StringPair ends = _global_prefs->getMetadataLookup ();

	Glib::ustring const bibfilename =
		ends.first
		+ doi_
		+ ends.second;

	try {
		Glib::ustring &rawtext = Transfer::getRemoteFile (
			_("Downloading Metadata"), messagetext, bibfilename);
		parseCrossRefXML (rawtext);
	} catch (Transfer::Exception ex) {
		Utility::exceptionDialog (&ex, _("Downloading metadata"));
	}
}


void BibData::getArxiv ()
{
	if (extras_["eprint"].empty() || _global_prefs->getWorkOffline())
		return;

	Glib::ustring messagetext =
		String::ucompose (
			"<b><big>%1</big></b>\n\n%2\n",
			_("Retrieving metadata"),
			String::ucompose (
				_("Contacting citebase.org to retrieve metadata for '%1'"),
				extras_["eprint"])
		);

	Glib::ustring arxivid = extras_["eprint"];
	int index = arxivid.find ("v");
	if (index != Glib::ustring::npos) {
		arxivid = arxivid.substr (0, index);
	}

	arxivid = Glib::Markup::escape_text (arxivid);

	Glib::ustring const filename = "http://www.citebase.org/openurl?url_ver=Z39.88-2004&svc_id=bibtex&rft_id=oai%3AarXiv.org%3A" + arxivid;

	std::cerr << filename << "\n";

	Glib::ustring *rawtext;
	try {
		rawtext = &Transfer::getRemoteFile (
			_("Downloading Metadata"), messagetext, filename);
		std::cerr << *rawtext << "\n\n\n";
	} catch (Transfer::Exception ex) {
		Utility::exceptionDialog (&ex, _("Downloading metadata"));
		return;
	}

	BibUtils::param p;
	BibUtils::bibl b;
	BibUtils::bibl_init( &b );
	BibUtils::bibl_initparams( &p, BibUtils::FORMAT_BIBTEX, BIBL_MODSOUT);

	try {
		BibUtils::biblFromString (b, *rawtext, BibUtils::FORMAT_BIBTEX, p);
		Document newdoc = BibUtils::parseBibUtils (b.ref[0]);
		newdoc.getBibData().print ();

		title_ = newdoc.getBibData().getTitle ();
		authors_ = newdoc.getBibData().getAuthors ();
		year_ = newdoc.getBibData().getYear ();
		if (extras_["Url"].empty())
			addExtra ("Url", newdoc.getBibData().getExtras ()["Url"]);

		BibUtils::bibl_free( &b );
	} catch (Glib::Error ex) {
		BibUtils::bibl_free( &b );
		Utility::exceptionDialog (&ex, _("Parsing BibTeX"));
		return;
	}
}

