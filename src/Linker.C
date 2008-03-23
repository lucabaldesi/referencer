
#include <iostream>

#include <gtkmm.h>

#include <libgnomevfsmm.h>
#include <glibmm/i18n.h>


#include "Document.h"
#include "DocumentView.h"
#include "RefWindow.h"
#include "Utility.h"

#include "Linker.h"


void Linker::createUI (RefWindow *window, DocumentView *view)
{
//	linkerIcon = Utility::getThemeMenuIcon("web-browser");
	static Gtk::Widget *image = NULL;

	Glib::ustring action = Glib::ustring("linker_") + getName();
//	std::cerr << "Linker::createUI: creating action " << action << "\n";
	window->actiongroup_->add (
		Gtk::Action::create (action, getLabel()),
		sigc::bind(
			sigc::mem_fun (view, &DocumentView::invokeLinker),
			this));

	Glib::ustring ui = 
                "<ui>"
                "<popup name='DocPopup'>"
                "  <placeholder name='WebLinkDocPopupActions'>"
		"    <menuitem action='";
	ui += action;
	ui += "'/>"
                "  </placeholder>"
                "</popup>"
                "</ui>";
	window->uimanager_->add_ui_from_string (ui);
}

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



bool PubmedLinker::canLink (Document const *doc)
{
	return doc->hasField("pmid");
}


void PubmedLinker::doLink (Document *doc)
{
	Glib::ustring url = Glib::ustring ("http://www.ncbi.nlm.nih.gov/pubmed/") + doc->getField("pmid");

	Gnome::Vfs::url_show (url);
}

Glib::ustring PubmedLinker::getLabel ()
{
	return Glib::ustring (_("PubMed Link"));
}



bool GoogleLinker::canLink (Document const *doc)
{
	return doc->hasField("doi") || doc->hasField("title");
}


void GoogleLinker::doLink (Document *doc)
{
	/*
	 * We assert in canLink that we at least have doi or title
	 */

	Glib::ustring searchTerm;
	if (doc->hasField ("doi")) {
		searchTerm = doc->getField ("doi");
	} else {
		searchTerm = doc->getField ("title");
	}

	if (doc->hasField ("authors")) {
		searchTerm += Glib::ustring (" ");
		searchTerm += Utility::firstAuthor(doc->getField ("authors"));
	}

	if (doc->hasField ("year")) {
		searchTerm += Glib::ustring (" ");
		searchTerm += doc->getField ("year");
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
