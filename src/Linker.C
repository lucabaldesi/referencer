
#include <gtkmm.h>
#include <glibmm/i18n.h>

#include "Document.h"
#include "DocumentView.h"
#include "RefWindow.h"
#include "Utility.h"

#include "Linker.h"


void Linker::createUI (RefWindow *window, DocumentView *view)
{

	static bool iconCreated = false;
	if (!iconCreated) {
		/* Factory for creating stock icons */
		Glib::RefPtr<Gtk::IconFactory> iconFactory = Gtk::IconFactory::create();
		iconFactory->add_default ();

		Gtk::IconSource iconSource;
		iconSource.set_pixbuf( Utility::getThemeIcon ("web-browser") );
		iconSource.set_size(Gtk::ICON_SIZE_SMALL_TOOLBAR);
		iconSource.set_size_wildcarded(); //Icon may be scaled.
		Gtk::IconSet iconSet;
		iconSet.add_source (iconSource);
		Gtk::StockID stockId = Gtk::StockID ("web-browser");
		iconFactory->add (stockId, iconSet);

		iconCreated = true;
	}

	Glib::ustring action = Glib::ustring("linker_") + getName();
	window->actiongroup_->add (
		Gtk::Action::create (action, Gtk::StockID("web-browser"), getLabel()),
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

Glib::ustring Linker::getURL (Document *doc)
{
	DEBUG ("Linker::getURL called erroneously!");
	return "";
}

void Linker::doLink (Document *doc)
{
	Glib::ustring url = getURL(doc);
	Gio::AppInfo::launch_default_for_uri (url);
}

bool DoiLinker::canLink (Document const *doc)
{
	return doc->hasField("doi");
}


Glib::ustring DoiLinker::getURL (Document *doc)
{
	Glib::ustring url = Glib::ustring("http://dx.doi.org/") + doc->getField("doi");
	return url;
}

Glib::ustring DoiLinker::getLabel ()
{
	return Glib::ustring (_("DOI Link"));
}



bool ArxivLinker::canLink (Document const *doc)
{
	return doc->hasField("eprint");
}


Glib::ustring ArxivLinker::getURL (Document *doc)
{
	Glib::ustring url = Glib::ustring("http://arxiv.org/abs/") + doc->getField ("eprint");
	return url;
}



Glib::ustring ArxivLinker::getLabel ()
{
	return Glib::ustring (_("arXiv Link"));
}



bool UrlLinker::canLink (Document const *doc)
{
	return doc->hasField("url");
}


Glib::ustring UrlLinker::getURL (Document *doc)
{
	Glib::ustring url = doc->getField("url");
	return url;
}

Glib::ustring UrlLinker::getLabel ()
{
	return Glib::ustring (_("URL Link"));
}



bool PubmedLinker::canLink (Document const *doc)
{
	return doc->hasField("pmid");
}


Glib::ustring PubmedLinker::getURL (Document *doc)
{
	Glib::ustring url = Glib::ustring ("http://www.ncbi.nlm.nih.gov/pubmed/") + doc->getField("pmid");
	return url;
}

Glib::ustring PubmedLinker::getLabel ()
{
	return Glib::ustring (_("PubMed Link"));
}



bool GoogleLinker::canLink (Document const *doc)
{
	return doc->hasField("doi") || doc->hasField("title");
}


Glib::ustring GoogleLinker::getURL (Document *doc)
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


	Glib::ustring escaped = Glib::uri_escape_string (searchTerm);
	DEBUG (escaped);
	Glib::ustring url = Glib::ustring ("http://scholar.google.co.uk/scholar?q=") + escaped + Glib::ustring("&btnG=Search");
	return url;
}

Glib::ustring GoogleLinker::getLabel ()
{
	return Glib::ustring (_("Google Scholar"));
}
