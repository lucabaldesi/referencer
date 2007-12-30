

#include <gtkmm.h>

#include <libgnomevfsmm.h>
#include <glibmm/i18n.h>


#include "Document.h"
#include "Utility.h"

#include "Linker.h"


void Linker::doLink (Document *doc)
{
	std::cerr << "Linker::doLink called erroneously!\n";
}

bool DoiLinker::canLink (Document const *doc)
{
	return doc->hasField("doi");
}


void DoiLinker::doLink (Document *doc)
{
	Glib::ustring url = Glib::ustring("http://dx.doi.org/") + doc->getField("doi");
	Gnome::Vfs::url_show (url);
}

Glib::ustring DoiLinker::getLabel ()
{
	return Glib::ustring (_("DOI Link"));
}



bool ArxivLinker::canLink (Document const *doc)
{
	return doc->hasField("eprint");
}


void ArxivLinker::doLink (Document *doc)
{
	Glib::ustring url = Glib::ustring("http://arxiv.org/abs/") + doc->getField ("eprint");
	Gnome::Vfs::url_show (url);
}



Glib::ustring ArxivLinker::getLabel ()
{
	return Glib::ustring (_("arXiv Link"));
}



bool UrlLinker::canLink (Document const *doc)
{
	return doc->hasField("url");
}


void UrlLinker::doLink (Document *doc)
{
	Glib::ustring url = doc->getField("url");
	Gnome::Vfs::url_show (url);
}

Glib::ustring UrlLinker::getLabel ()
{
	return Glib::ustring (_("URL Link"));
}



bool MedlineLinker::canLink (Document const *doc)
{
	return doc->hasField("pmid");
}


void MedlineLinker::doLink (Document *doc)
{
	Glib::ustring url = Glib::ustring ("http://www.ncbi.nlm.nih.gov/pubmed/") + doc->getField("pmid");

	Gnome::Vfs::url_show (url);
}

Glib::ustring MedlineLinker::getLabel ()
{
	return Glib::ustring (_("MedLine Link"));
}



bool GoogleLinker::canLink (Document const *doc)
{
	return doc->hasField("doi") || doc->hasField("title");
}


void GoogleLinker::doLink (Document *doc)
{

	Glib::ustring searchTerm;
	if (doc->hasField ("doi")) {
		searchTerm = doc->getField ("doi");
	} else {
		searchTerm = doc->getField ("title") + Glib::ustring(" ") + Utility::firstAuthor(doc->getField ("authors"));
	}

	Glib::ustring escaped = Gnome::Vfs::escape_string (searchTerm);
	std::cerr << escaped << "\n";
	Glib::ustring url = Glib::ustring ("http://scholar.google.co.uk/scholar?q=") + escaped + Glib::ustring("&btnG=Search");

	Gnome::Vfs::url_show (url);
}

Glib::ustring GoogleLinker::getLabel ()
{
	return Glib::ustring (_("Google Scholar"));
}
