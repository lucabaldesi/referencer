

#include "BibData.h"

#include <iostream>

#include <time.h>
#include <boost/regex.hpp>

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

	for (unsigned int i = 0; i < n_titles; ++i) {
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
	 * Take the first four-letter numeric which is between
	 * 1990 and the current year.  This will occasionally 
	 * pick up a 'number' or 'page' instead, but should
	 * almost always be the year
	 */

	std::string const &raw = raw_;

	boost::regex expression("\\W([0-9][0-9][0-9][0-9])\\W"); 

	int const dawn_of_ejournals = 1990;
	time_t const time = time(NULL);
	struct tm UTC;
	gmtime_r(&time, &UTC);
	int const present_day = UTC.tm_year;

	std::string::const_iterator start, end; 
	start = raw.begin();
	end = raw.end(); 
	boost::match_results<std::string::const_iterator> what; 
	boost::match_flag_type flags = boost::match_default; 
	while(regex_search(start, end, what, expression, flags)) 
	{ 
		std::string yearstring = what[0];
		int yearval = atoi(yearstring.c_str());
		if (yearval > dawn_of_ejournals && yearval < present_day) {
			year_ = yearstring;
			break;
		}
	  // update search position: 
	  start = what[0].second; 
	  // update flags: 
	  flags |= boost::match_prev_avail; 
	  flags |= boost::match_not_bob; 
	}
}

/*
 * Try to guess the authors of the paper from the raw text
 */
void BibData::guessAuthors (Glib::ustring const &raw_)
{
	/*
	 * Take the first four-letter numeric which is between
	 * 1990 and the current year.  This will occasionally 
	 * pick up a 'number' or 'page' instead, but should
	 * almost always be the year
	 */

	std::string const &raw = raw_;

	boost::regex expression("^(\\D{5,}\n\\D{5,})$"); 

	std::string::const_iterator start, end; 
	start = raw.begin();
	end = raw.end(); 
	boost::match_results<std::string::const_iterator> what; 
	boost::match_flag_type flags = boost::match_default; 
	while(regex_search(start, end, what, expression, flags)) { 
		std::string authors = what[0];
		std::cout << authors << std::endl;

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
	std::string doistring;

//	boost::regex expression("\\WDOI: (.*)\\W");
		boost::regex expression("[Dd][Oo][Ii]: ?([^[ \\n]]*)[ \\n]");  

	std::string::const_iterator start, end;
	start = raw.begin();
	end = raw.end(); 
	boost::match_results<std::string::const_iterator> what; 
	boost::match_flag_type flags = boost::match_default; 
	while(regex_search(start, end, what, expression, flags)) 
	{ 
		doistring = what[1];
		std::cerr << "BibData::guessDoi: got '" << doistring << "'\n";
	  // update search position: 
	  start = what[0].second; 
	  // update flags: 
	  flags |= boost::match_prev_avail; 
	  flags |= boost::match_not_bob; 
	}
	
	// And use whatever we got last... which is probably a bad idea
	// if we're reading the refs at the end of a paper
	Glib::ustring gstr = doistring;
	setDoi (gstr);
}

