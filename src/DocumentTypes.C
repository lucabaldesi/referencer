

#include <iostream>

#include <glibmm/i18n.h>

#include "DocumentTypes.h"


void DocumentTypeManager::registerField (
	Glib::ustring internalName,
	Glib::ustring displayName,
	bool shortField)
{
	DocumentField newbie (internalName, displayName, shortField);
	documentFields_[internalName] = newbie;
}

void DocumentTypeManager::addField (DocumentType &type, Glib::ustring internalName, bool required)
{
	if (documentFields_.find (internalName) == documentFields_.end())
		DEBUG ("unknown field " + internalName);

	if (required)
		type.requiredFields_.push_back (documentFields_[internalName]); 
	else 
		type.optionalFields_.push_back (documentFields_[internalName]); 
}

DocumentTypeManager::DocumentTypeManager () {
	registerField ("title", _("Title"), false);
	registerField ("author", _("Authors"), false);
	registerField ("journal", _("Journal"), false);
	registerField ("year", _("Year"), true);

	registerField ("volume", _("Volume"), true);
	registerField ("number", _("Issue"), true);
	registerField ("pages", _("Pages"), true);
	registerField ("month", _("Month"), true);
	registerField ("note", _("Note"), false);
	registerField ("key", _("Key"), true);

	registerField ("editor", _("Editor"), false);
	registerField ("publisher", _("Publisher"), false);
	registerField ("series", _("Series"), false);
	registerField ("address", _("Address"), false);
	registerField ("edition", _("Edition"), false);

	registerField ("booktitle", _("Book title"), false);
	registerField ("organisation", _("Organisation"), false);

	registerField ("howpublished", _("How published"), false);

	registerField ("school", _("School"), false);
	registerField ("institution", _("Institution"), false);
	registerField ("chapter", _("Chapter"), false);
	registerField ("type", _("Type"), false);
	registerField ("doi", _("DOI"), false);


	DocumentType article ("article", _("Article"));
	addField (article, "title", true);
	addField (article, "journal", true);
	addField (article, "year", true);
	addField (article, "author", true);
	addField (article, "volume", false);
	addField (article, "number", false);
	addField (article, "pages", false);
	addField (article, "doi", false);
	// Fields John thinks are silly
	/*
	addField (article, "month", false);
	addField (article, "note", false);
	*/
	registerType (article);

	DocumentType book ("book", _("Book"));
	addField (book, "author", true);
	addField (book, "editor", true);
	addField (book, "title", true);
	addField (book, "publisher", true);
	addField (book, "year", true);
	addField (book, "volume", false);
	addField (book, "series", false);
	addField (book, "address", false);
	addField (book, "edition", false);
	addField (book, "doi", false);
	// Fields John thinks are silly
	/*
	addField (book, "month", false);
	addField (book, "note", false);
	*/
	registerType (book);

	DocumentType inproceedings ("inproceedings", _("In Proceedings"));
	addField (inproceedings, "author", true);
	addField (inproceedings, "title", true);
	addField (inproceedings, "booktitle", true);
	addField (inproceedings, "year", true);
	addField (inproceedings, "editor", false);
	addField (inproceedings, "pages", false);
	addField (inproceedings, "organisation", false);
	addField (inproceedings, "publisher", false);
	addField (inproceedings, "address", false);
	addField (inproceedings, "doi", false);
	/*
	addField (inproceedings, "month", false);
	addField (inproceedings, "note", false);
	*/
	registerType (inproceedings);

	DocumentType misc ("misc", _("Misc"));
	addField (misc, "author", false);
	addField (misc, "title", false);
	addField (misc, "howpublished", false);
	addField (misc, "month", false);
	addField (misc, "year", false);
	addField (misc, "note", false);
	addField (misc, "doi", false);
	registerType (misc);
	
	DocumentType unpublished ("unpublished", _("Unpublished"));
	addField (unpublished, "author", true);
	addField (unpublished, "title", true);
	addField (unpublished, "note", true);
	addField (unpublished, "month", false);
	addField (unpublished, "year", false);
	addField (unpublished, "doi", false);
	registerType (unpublished);

	DocumentType mastersthesis ("mastersthesis", _("Master's thesis"));
	addField (mastersthesis, "author", true);
	addField (mastersthesis, "title", true);
	addField (mastersthesis, "school", true);
	addField (mastersthesis, "year", true);
	addField (mastersthesis, "address", false);
	addField (mastersthesis, "note", false);
	addField (mastersthesis, "month", false);
	addField (mastersthesis, "doi", false);
	registerType (mastersthesis);

	DocumentType phdthesis ("phdthesis", _("PhD thesis"));
	addField (phdthesis, "author", true);
	addField (phdthesis, "title", true);
	addField (phdthesis, "school", true);
	addField (phdthesis, "year", true);
	addField (phdthesis, "address", false);
	addField (phdthesis, "note", false);
	addField (phdthesis, "month", false);
	addField (phdthesis, "doi", false);	
	registerType (phdthesis);

	DocumentType proceedings ("proceedings", _("Proceedings"));
	addField (proceedings, "title", true);
	addField (proceedings, "year", true);
	addField (proceedings, "editor", false);
	addField (proceedings, "publisher", false);
	addField (proceedings, "organisation", false);
	addField (proceedings, "address", false);
	addField (proceedings, "month", false);
	addField (proceedings, "note", false);
	addField (proceedings, "doi", false);	
	registerType (proceedings);

	DocumentType conference ("conference", _("Conference"));
	addField (conference, "title", true);
	addField (conference, "author", false);
	addField (conference, "howpublished", false);
	addField (conference, "address", false);
	addField (conference, "month", false);
	addField (conference, "year", false);
	addField (conference, "note", false);
	addField (conference, "doi", false);	
	registerType (conference);

	DocumentType inbook ("inbook", _("In Book"));
	addField (inbook, "title", true);
	addField (inbook, "editor", true);
	addField (inbook, "author", true);
	addField (inbook, "chapter", true);
	addField (inbook, "pages", true);
	addField (inbook, "publisher", true);
	addField (inbook, "year", true);
	addField (inbook, "volume", false);
	addField (inbook, "series", false);
	addField (inbook, "address", false);
	addField (inbook, "edition", false);
	addField (inbook, "month", false);
	addField (inbook, "note", false);
	addField (inbook, "doi", false);
	registerType (inbook);

	DocumentType booklet ("booklet", _("Booklet"));
	addField (booklet, "title", true);
	addField (booklet, "author", false);
	addField (booklet, "howpublished", false);
	addField (booklet, "address", false);
	addField (booklet, "month", false);
	addField (booklet, "year", false);
	addField (booklet, "note", false);
	addField (booklet, "doi", false);
	registerType (booklet);

	DocumentType incollection ("incollection", _("In Collection"));
	addField (incollection, "author", true);
	addField (incollection, "title", true);
	addField (incollection, "booktitle", true);
	addField (incollection, "year", true);
	addField (incollection, "editor", false);
	addField (incollection, "pages", false);
	addField (incollection, "organisation", false);
	addField (incollection, "publisher", false);
	addField (incollection, "address", false);
	addField (incollection, "month", false);
	addField (incollection, "note", false);
	addField (incollection, "doi", false);
	registerType (incollection);

	DocumentType manual ("manual", _("Manual"));
	addField (manual, "title", true);
	addField (manual, "author", false);
	addField (manual, "organisation", false);
	addField (manual, "address", false);
	addField (manual, "edition", false);
	addField (manual, "month", false);
	addField (manual, "year", false);
	addField (manual, "note", false);
	addField (manual, "doi", false);
	registerType (manual);

	DocumentType techreport ("techreport", _("Technical Report"));
	addField (techreport, "title", true);
	addField (techreport, "author", true);
	addField (techreport, "institution", true);
	addField (techreport, "year", true);
	addField (techreport, "type", false);
	addField (techreport, "number", false);
	addField (techreport, "address", false);
	addField (techreport, "month", false);
	addField (techreport, "note", false);
	addField (techreport, "doi", false);
	registerType (techreport);

}


DocumentType DocumentTypeManager::getType (Glib::ustring bibtexName)
{
	return documentTypes_[bibtexName];
}



