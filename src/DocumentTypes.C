


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


	DocumentType article;
	article.bibtexName_ = "article";
	article.displayName_ = _("Article");
	addField (article, "title", true);
	addField (article, "journal", true);
	addField (article, "year", true);
	addField (article, "author", true);
	addField (article, "volume", false);
	addField (article, "number", false);
	addField (article, "pages", false);
	// Fields John thinks are silly
	/*
	addField (article, "month", false);
	addField (article, "note", false);
	addField (article, "key", false);
	*/

	documentTypes_["article"] = article;

	DocumentType book;
	book.bibtexName_ = "book";
	book.displayName_ = _("Book");
	addField (book, "author", true);
	addField (book, "editor", true);
	addField (book, "title", true);
	addField (book, "publisher", true);
	addField (book, "year", true);
	addField (book, "volume", false);
	addField (book, "series", false);
	addField (book, "address", false);
	addField (book, "edition", false);
	// Fields John thinks are silly
	/*
	addField (book, "month", false);
	addField (book, "note", false);
	addField (book, "key", false);
	*/

	documentTypes_["book"] = book;
}


DocumentType DocumentTypeManager::getType (Glib::ustring bibtexName)
{
	return documentTypes_[bibtexName];
}



