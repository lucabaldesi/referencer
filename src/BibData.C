
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
#include <libxml/xmlwriter.h>
#include "ucompose.hpp"

#include "Transfer.h"
#include "Preferences.h"

#include "BibData.h"
#include "Library.h"

Glib::ustring BibData::default_document_type;

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
void BibData::print () const
{
	DEBUG (String::ucompose ("%1: %2\n", "DOI: ", doi_));
	DEBUG (String::ucompose ("%1: %2\n", "Title: ", title_));
	DEBUG (String::ucompose ("%1: %2\n", "Authors: ", authors_));
	DEBUG (String::ucompose ("%1: %2\n", "Journal: ", journal_));
	DEBUG (String::ucompose ("%1: %2\n", "Volume: ", volume_));
	DEBUG (String::ucompose ("%1: %2\n", "Number: ", issue_));
	DEBUG (String::ucompose ("%1: %2\n", "Pages: ", pages_));
	DEBUG (String::ucompose ("%1: %2\n", "Year: ", year_));
	
	ExtrasMap::const_iterator it = extras_.begin ();
	ExtrasMap::const_iterator const end = extras_.end ();
	for (; it != end; ++it) {
		DEBUG (String::ucompose ("%1: %2\n", it->first, it->second));
	}
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
	if (!key.validate()) {
		throw (new std::runtime_error (
			std::string("Invalid UTF-8 in key in ") + std::string(__FUNCTION__)));
	}

	if (!value.validate()) {
		throw (new std::runtime_error (
			std::string("Invalid UTF-8 in value in ") + std::string(__FUNCTION__)));
	}

	if ( key == "Keywords" && !extras_[key].empty() ) {
		extras_[key] = extras_[key] + "; " + value;
	} else {
		extras_[key] = value;
	}
}


void BibData::clearExtras ()
{
	extras_.clear ();
}

void BibData::writeXML (xmlTextWriterPtr writer)
{
	xmlTextWriterWriteElement(writer, BAD_CAST LIB_ELEMENT_DOC_BIB_TYPE, BAD_CAST type_.c_str());
	xmlTextWriterWriteElement(writer, BAD_CAST LIB_ELEMENT_DOC_BIB_DOI, BAD_CAST doi_.c_str());
	xmlTextWriterWriteElement(writer, BAD_CAST LIB_ELEMENT_DOC_BIB_TITLE, BAD_CAST title_.c_str());
	xmlTextWriterWriteElement(writer, BAD_CAST LIB_ELEMENT_DOC_BIB_AUTHORS, BAD_CAST authors_.c_str());
	xmlTextWriterWriteElement(writer, BAD_CAST LIB_ELEMENT_DOC_BIB_JOURNAL, BAD_CAST journal_.c_str());
	xmlTextWriterWriteElement(writer, BAD_CAST LIB_ELEMENT_DOC_BIB_VOLUME, BAD_CAST volume_.c_str());
	xmlTextWriterWriteElement(writer, BAD_CAST LIB_ELEMENT_DOC_BIB_NUMBER, BAD_CAST issue_.c_str());
	xmlTextWriterWriteElement(writer, BAD_CAST LIB_ELEMENT_DOC_BIB_PAGES, BAD_CAST pages_.c_str());
	xmlTextWriterWriteElement(writer, BAD_CAST LIB_ELEMENT_DOC_BIB_YEAR, BAD_CAST year_.c_str());

	ExtrasMap::iterator it = extras_.begin ();
	ExtrasMap::iterator const end = extras_.end ();
	for (; it != end; ++it) {
		xmlTextWriterStartElement(writer, BAD_CAST LIB_ELEMENT_DOC_BIB_EXTRA);
		xmlTextWriterWriteAttribute(writer, BAD_CAST LIB_ELEMENT_DOC_BIB_EXTRA_KEY, BAD_CAST (*it).first.c_str());
		xmlTextWriterWriteString(writer, BAD_CAST (*it).second.c_str());
		xmlTextWriterEndElement(writer);
	}
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
 * Try to guess the DOI of the paper from the raw text
 */
void BibData::guessDoi (Glib::ustring const &graw)
{
	std::string const &raw = graw;
	boost::regex expression(
		"\\(?"
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
		Glib::ustring wholeMatch = std::string(what[0]);
		Glib::ustring gstr = std::string(what[1]);

		// Special case to chop off trailing comma to deal with
		// "doi: foo, available online" in JCompPhys
		// Note that commas ARE legal suffix characters in Doi spec
		// But there's nothing in the spec about regexing them
		// out of PDFS :-) -jcs
		if (gstr[gstr.size() - 1] == ',') {
			gstr = gstr.substr (0, gstr.size() - 1);
		};

		/*
		 * Special case to chop off trailing parenthesis
		 * in (doi:foo.foo/bar) case
		 */
		if (wholeMatch[0] == '(' && wholeMatch[wholeMatch.size() - 1] == ')') {
			gstr = gstr.substr (0, gstr.size() - 1);
		}

		setDoi (gstr);
		return;
	}
}


/*
 * Try to extract the Arxiv eprint value of the paper from the raw text
 */
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



/*
 * Take values from the source BibData for fields of ours
 * which are currently empty.
 */
void BibData::mergeIn (BibData const &source)
{
	type_ = source.getType ();
	if (!source.getDoi ().empty ())
		doi_ = source.getDoi ();
	if (!source.getVolume().empty ())
		volume_ = source.getVolume ();
	if (!source.getIssue().empty ())
		issue_ = source.getIssue ();
	if (!source.getPages().empty ())
		pages_ = source.getPages ();
	if (!source.getAuthors().empty ())
		authors_ = source.getAuthors ();
	if (!source.getJournal().empty ())
		journal_ = source.getJournal ();
	if (!source.getTitle().empty ())
		title_ = source.getTitle ();
	if (!source.getYear().empty ())
		year_ = source.getYear ();
		
	ExtrasMap sourceextras = source.getExtras();
	ExtrasMap::iterator it = sourceextras.begin ();
	ExtrasMap::iterator const end = sourceextras.end ();
	for (; it != end; ++it) {
		if (extras_[it->first].empty()) {
			addExtra (it->first, it->second);
		}
	}
}

