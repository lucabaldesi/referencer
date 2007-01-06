

#include "BibData.h"

#include <iostream>

#include <time.h>
#include <boost/regex.hpp>

#include <libgnomevfsmm.h>

#include "CrossRefParser.h"

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
	std::cout << "Number: " << number_ << std::endl;	
	std::cout << "Pages: " << pages_ << std::endl;	
	std::cout << "Year: " << year_ << std::endl;	
}

using Glib::Markup::escape_text;

void BibData::writeXML (std::ostringstream &out)
{
	out << "    <bib_doi>" << escape_text(doi_) << "</bib_doi>\n";
	out << "    <bib_title>" << escape_text(title_) << "</bib_title>\n";
	out << "    <bib_authors>" << escape_text(authors_) << "</bib_authors>\n";
	out << "    <bib_journal>" << escape_text(journal_) << "</bib_journal>\n";
	out << "    <bib_volume>" << escape_text(volume_) << "</bib_volume>\n";
	out << "    <bib_number>" << escape_text(number_) << "</bib_number>\n";
	out << "    <bib_pages>" << escape_text(pages_) << "</bib_pages>\n";
	out << "    <bib_year>" << escape_text(year_) << "</bib_year>\n";
}


void BibData::parseCrossRefXML (Glib::ustring const &xml)
{
	CrossRefParser parser (*this);
	Glib::Markup::ParseContext context (parser);
	try {
		context.parse (xml);
	} catch (Glib::MarkupError const ex) {
		std::cerr << "Exception on line " << context.get_line_number () << ", character " << context.get_char_number () << ", code ";
		switch (ex.code()) {
			case Glib::MarkupError::BAD_UTF8:
				std::cerr << "Bad UTF8\n";
				break;
			case Glib::MarkupError::EMPTY:
				std::cerr << "Empty\n";
				break;
			case Glib::MarkupError::PARSE:
				std::cerr << "Parse error\n";
				break;
			default:
				std::cerr << (int)ex.code() << "\n";
		}
	}
	context.end_parse ();
}


/*
 * Parse the XML metadata embedded in some PDFs
 * in the hopes it contains some interesting fields
 */
void BibData::parseMetadata (Glib::ustring const &meta, FieldMask mask)
{
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

	//boost::regex expression("[Dd][Oo][Ii]: ?([^ \\n]*)");
	boost::regex expression("[Dd][Oo][Ii]: ?([0-9a-zA-Z\\.\\/]*)");

	std::string::const_iterator start, end;
	start = raw.begin();
	end = raw.end(); 
	boost::match_results<std::string::const_iterator> what; 
	boost::match_flag_type flags = boost::match_default; 
	while(regex_search(start, end, what, expression, flags)) { 
		Glib::ustring gstr = std::string(what[1]);
		setDoi (gstr);
		return;
	}
}


void BibData::getCrossRef ()
{
	Gnome::Vfs::Handle bibfile;
	
	/*http://www.crossref.org/openurl/?id=doi:10.1103/PhysRevB.73.014404&noredirect=true*/
	
	Glib::ustring bibfilename =
		"http://www.crossref.org/openurl/?id=doi:"
		+ doi_
		+ "&noredirect=true";
	//"/home/jcspray/Projects/pdfdivine/crossref_eg.xml";
		
	Glib::RefPtr<Gnome::Vfs::Uri> biburi = Gnome::Vfs::Uri::create (bibfilename);
	
	bool exists = biburi->uri_exists ();
	if (!exists) {
		std::cerr << "BibData::getCrossRef: bib XML file '" <<
			bibfilename << "' not found\n";
		return;
	}
	
	bibfile.open (bibfilename, Gnome::Vfs::OPEN_READ);

	Glib::RefPtr<Gnome::Vfs::FileInfo> fileinfo;
	fileinfo = bibfile.get_file_info ();
	
	char *buffer = (char *) malloc (sizeof(char) * (fileinfo->get_size() + 1));
	bibfile.read (buffer, fileinfo->get_size());
	buffer[fileinfo->get_size()] = 0;
	
	Glib::ustring rawtext = buffer;
	free (buffer);
	bibfile.close ();
	parseCrossRefXML (rawtext);
}


void BibData::clear ()
{
	doi_ = "";
	volume_ = "";
	number_ = "";
	pages_ = "";
	authors_ = "";
	journal_ = "";
	title_ = "";
	year_ = "";
}
