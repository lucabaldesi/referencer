
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */

#include <iostream>

#include <glibmm/i18n.h>
#include <libgnomevfsmm.h>

#include "Document.h"
#include "DocumentList.h"
#include "icon-entry.h"
#include "Library.h"
#include "Linker.h"
#include "Preferences.h"
#include "RefWindow.h"
#include "Transfer.h"
#include "ucompose.hpp"
#include "Utility.h"

#include "DocumentView.h"

#if GTK_VERSION_LT(2,12)
#include "ev-tooltip.h"
#endif

#undef USE_TRACKER
#ifdef USE_TRACKER
#include "tracker.h"
#endif


DocumentView::~DocumentView ()
{
#if GTK_VERSION_LT(2,12)
	gtk_widget_destroy (doctooltip_);
#endif
}


DocumentView::DocumentView (
	RefWindow &refwin,
	Library &lib,
	bool const uselistview)
	
 : win_ (refwin), lib_(lib)
{
#if GTK_VERSION_LT(2,12)
	hoverdoc_ = NULL;
#endif
	ignoreSelectionChanged_ = false;

	/*
	 * Pack a vbox inside a frame inside ourself
	 */
	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);
	Gtk::Frame *iconsframe = new Gtk::Frame ();
	iconsframe->add (*vbox);
	pack_start (*iconsframe, true, true, 0);

	/*
	 * Model common to icon and list views
	 */
	Gtk::TreeModel::ColumnRecord doccols;
	doccols.add(docpointercol_);
	doccols.add(dockeycol_);
	doccols.add(doctitlecol_);
	doccols.add(docauthorscol_);
	doccols.add(docyearcol_);
	doccols.add(docthumbnailcol_);
#if GTK_VERSION_GE(2,12)
	doccols.add(doctooltipcol_);
#endif
	doccols.add(docvisiblecol_);
	doccols.add(doccaptioncol_);

	docstore_ = Gtk::ListStore::create(doccols);
	docstorefilter_ = Gtk::TreeModelFilter::create (docstore_);
	docstorefilter_->set_visible_column (docvisiblecol_);
	docstoresort_ = Gtk::TreeModelSort::create (docstorefilter_);
	docstoresort_->signal_sort_column_changed ().connect(
		sigc::mem_fun (*this, &DocumentView::onSortColumnChanged));

	std::pair<int, int> sortinfo = _global_prefs->getListSort ();
	docstoresort_->set_sort_column (sortinfo.first, (Gtk::SortType)sortinfo.second);

	/*
	 * Icon View
	 */
	Gtk::IconView *icons = Gtk::manage(new Gtk::IconView(docstoresort_));
	icons->set_markup_column (doccaptioncol_);
	icons->set_pixbuf_column (docthumbnailcol_);
#if GTK_VERSION_GE(2,12)
	// Nasty, gtkmm doesn't have a binding for passing the column object
	icons->set_tooltip_column (6);
#endif
	icons->signal_item_activated().connect (
		sigc::mem_fun (*this, &DocumentView::docActivated));

	icons->signal_button_press_event().connect(
		sigc::mem_fun (*this, &DocumentView::docClicked), false);

	icons->signal_selection_changed().connect(
		sigc::mem_fun (*this, &DocumentView::docSelectionChanged));

	icons->set_selection_mode (Gtk::SELECTION_MULTIPLE);
	icons->set_columns (-1);

	icons->set_orientation (Gtk::ORIENTATION_HORIZONTAL);
	icons->set_item_width (256);

	std::vector<Gtk::TargetEntry> dragtypes;
	Gtk::TargetEntry target;
	target.set_info (0);
	target.set_target ("text/uri-list");
	dragtypes.push_back (target);
	target.set_target ("text/x-moz-url-data");
	dragtypes.push_back (target);

	icons->drag_dest_set (
		dragtypes,
		Gtk::DEST_DEFAULT_ALL,
		Gdk::ACTION_COPY | Gdk::ACTION_MOVE | Gdk::ACTION_LINK);

	icons->signal_drag_data_received ().connect (
		sigc::mem_fun (*this, &DocumentView::onIconsDragData));
	
	#if GTK_VERSION_LT(2,12)
	icons->set_events (Gdk::POINTER_MOTION_MASK | Gdk::LEAVE_NOTIFY_MASK);
	icons->signal_motion_notify_event ().connect_notify (
		sigc::mem_fun (*this, &DocumentView::onDocMouseMotion));
	icons->signal_leave_notify_event ().connect_notify (
		sigc::mem_fun (*this, &DocumentView::onDocMouseLeave));
	#endif

	docsiconview_ = icons;

	Gtk::ScrolledWindow *iconsscroll = Gtk::manage(new Gtk::ScrolledWindow());
	iconsscroll->add(*icons);
	iconsscroll->set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	vbox->pack_start(*iconsscroll, true, true, 0);

	docsiconscroll_ = iconsscroll;

	#if GTK_VERSION_LT(2,12)
	doctooltip_ = ev_tooltip_new (GTK_WIDGET(win_.window_->gobj()));
	#endif
	/*
	 * End of icon view stuff
	 */

	/*
	 * List view stuff
	 */
	Gtk::TreeView *table = Gtk::manage (new Gtk::TreeView(docstoresort_));
	table->set_enable_search (true);
	table->set_search_column (1);
	table->set_rules_hint (true);

	table->drag_dest_set (
		dragtypes,
		Gtk::DEST_DEFAULT_ALL,
		Gdk::ACTION_COPY | Gdk::ACTION_MOVE | Gdk::ACTION_LINK);

	table->signal_drag_data_received ().connect (
		sigc::mem_fun (*this, &DocumentView::onIconsDragData));

	table->signal_row_activated ().connect (
		sigc::mem_fun (*this, &DocumentView::docListActivated));

	table->signal_button_press_event().connect(
		sigc::mem_fun (*this, &DocumentView::docClicked), false);

	docslistselection_ = table->get_selection ();
	docslistselection_->set_mode (Gtk::SELECTION_MULTIPLE);
	docslistselection_->signal_changed ().connect (
		sigc::mem_fun (*this, &DocumentView::docSelectionChanged));

	docslistview_ = table;

	Gtk::ScrolledWindow *tablescroll = Gtk::manage(new Gtk::ScrolledWindow());
	tablescroll->add(*table);
	tablescroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	vbox->pack_start(*tablescroll, true, true, 0);

	docslistscroll_ = tablescroll;

	populateColumns ();

	/*
	 * End of list view stuff
	 */
	
	setUseListView (uselistview);
	

	/*
	 * Search box
	 */
	Sexy::IconEntry *searchentry = Gtk::manage (new Sexy::IconEntry ());
	Gtk::Image *searchicon = Gtk::manage (
		new Gtk::Image (Gtk::Stock::FIND, Gtk::ICON_SIZE_BUTTON));
	searchentry->set_icon (Sexy::ICON_ENTRY_PRIMARY, searchicon);
	searchentry->signal_changed ().connect (
		sigc::mem_fun (*this, &DocumentView::onSearchChanged));
	
	searchentry_ = searchentry;
	/*
	 * End of search box
	 */

}


void DocumentView::setUseListView (bool const &list)
{
	if (list != uselistview_) {
		uselistview_ = list;
		
	if (uselistview_) {
		docsiconscroll_->hide ();
		docslistscroll_->show ();
		docslistview_->grab_focus ();
	} else {
		docslistscroll_->hide ();
		docsiconscroll_->show ();
		docsiconview_->grab_focus ();
	}

	docSelectionChanged ();
	}
}

#if GTK_VERSION_LT(2,12)
void DocumentView::onDocMouseMotion (GdkEventMotion* event)
{
	int x = (int)event->x;
	int y = (int)event->y;

	Gtk::TreeModel::Path path = docsiconview_->get_path_at_pos (x, y);
	bool havepath = path.gobj() != NULL;

	Document *doc = NULL;
	if (havepath) {
		Gtk::ListStore::iterator it = docstoresort_->get_iter (path);
		doc = (*it)[docpointercol_];
	}

	if (doc != hoverdoc_) {
		if (doc) {
			BibData &bib = doc->getBibData ();
			Glib::ustring tiptext = String::ucompose (
				// Translators: this is the format for the document tooltips
				_("<b>%1</b>\n%2\n<i>%3</i>"),
				Glib::Markup::escape_text (doc->getKey()),
				Glib::Markup::escape_text (bib.getTitle()),
				Glib::Markup::escape_text (bib.getAuthors()));

			int xoffset = (int) docsiconscroll_->get_hadjustment ()->get_value ();
			int yoffset = (int) docsiconscroll_->get_vadjustment ()->get_value ();

			ev_tooltip_set_position (EV_TOOLTIP (doctooltip_), x - xoffset, y - yoffset);
			ev_tooltip_set_text (EV_TOOLTIP (doctooltip_), tiptext.c_str());
			ev_tooltip_activate (EV_TOOLTIP (doctooltip_));
		} else {
			ev_tooltip_deactivate (EV_TOOLTIP (doctooltip_));
		}
		hoverdoc_ = doc;
	}
}


void DocumentView::onDocMouseLeave (GdkEventCrossing *event)
{
	ev_tooltip_deactivate (EV_TOOLTIP (doctooltip_));
}
#endif


void DocumentView::onIconsDragData (
	const Glib::RefPtr <Gdk::DragContext> &context,
	int n1, int n2, const Gtk::SelectionData &sel, guint n3, guint n4)
{
	std::cerr << "onIconsDragData: got '" << sel.get_data_as_string () << "'\n";
	std::cerr << "\tOf type '" << sel.get_data_type () << "'\n";

	std::vector<Glib::ustring> files;

	typedef std::vector <Glib::ustring> urilist;
	urilist uris;

	if (sel.get_data_type () == "text/uri-list") {
		uris = sel.get_uris ();
	} else if (sel.get_data_type () == "text/x-moz-url-data") {
		uris.push_back (Utility::mozUrlSelectionToUTF8(sel));
	}

	urilist::iterator it = uris.begin ();
	urilist::iterator const end = uris.end ();
	for (; it != end; ++it) {
		bool is_dir = false;
		Glib::RefPtr<Gnome::Vfs::Uri> uri = Gnome::Vfs::Uri::create (*it);

		if (!Utility::uriIsFast (uri)) {
			// It's a remote file, download it
			Glib::RefPtr<Gnome::Vfs::Uri> liburi =
				Gnome::Vfs::Uri::create (win_.getOpenedLib());
			
			/*
			 * THIS IS ALL BOLLOCKS
			 * Really we should just show the user a save dialog and let him choose
			 */

			Glib::ustring destinationdir;			
			if (liburi) {
				destinationdir =
					liburi->get_scheme ()
					+ "://"
					+	Glib::build_filename (
						liburi->extract_dirname (),
						_("Downloaded Documents"));

				if (!Gnome::Vfs::Uri::create (destinationdir)->uri_exists ()) {
					Gnome::Vfs::Handle::make_directory (destinationdir, 0757);
				}
			}

			Glib::ustring const destination = Glib::build_filename (
				destinationdir,
				uri->extract_short_path_name ());

			std::cerr << "'" << destination << "\n";
			Transfer::downloadRemoteFile (
				*it,
				destination,
				*(win_.getProgress()));
			files.push_back (destination);
		} else {
			// It's a local file, see if it's a directory
			Glib::RefPtr<Gnome::Vfs::FileInfo> info;
			try {
				info = uri->get_file_info ();
			} catch (const Gnome::Vfs::exception ex) {
				Utility::exceptionDialog (&ex,
					String::ucompose (
						_("Getting info for file '%1'"),
						uri->to_string ()));
				return;
			}
			is_dir = info->get_type() == Gnome::Vfs::FILE_TYPE_DIRECTORY;

			if (is_dir) {
				std::vector<Glib::ustring> morefiles = Utility::recurseFolder (*it);
				std::vector<Glib::ustring>::iterator it2;
				for (it2 = morefiles.begin(); it2 != morefiles.end(); ++it2) {
					files.push_back (*it2);
				}
			} else {
				files.push_back (*it);
			}
		}
	}

	win_.addDocFiles (files);
}


std::vector<Document*> DocumentView::getSelectedDocs ()
{
	std::vector<Document*> docpointers;

	if (uselistview_) {
		Gtk::TreeSelection::ListHandle_Path paths =
			docslistselection_->get_selected_rows ();

		Gtk::TreeSelection::ListHandle_Path::iterator it = paths.begin ();
		Gtk::TreeSelection::ListHandle_Path::iterator const end = paths.end ();
		for (; it != end; it++) {
			/*
			 * Explicit resolution through the proxy models is not strictly
			 * necessary, but this way we pick up on it if the models aren't
			 * in sync, rather than finding out the hard way
			 */
			Gtk::TreePath sortPath = (*it);
			Gtk::TreePath filterPath = docstoresort_->convert_path_to_child_path (sortPath);
			Gtk::TreePath realPath = docstorefilter_->convert_path_to_child_path (filterPath);

			Gtk::ListStore::iterator iter = docstore_->get_iter (realPath);
			docpointers.push_back((*iter)[docpointercol_]);
		}
	} else {
		Gtk::IconView::ArrayHandle_TreePaths paths =
			docsiconview_->get_selected_items ();

		Gtk::IconView::ArrayHandle_TreePaths::iterator it = paths.begin ();
		Gtk::IconView::ArrayHandle_TreePaths::iterator const end = paths.end ();
		for (; it != end; it++) {
			/*
			 * Explicit resolution through the proxy models is not strictly
			 * necessary, but this way we pick up on it if the models aren't
			 * in sync, rather than finding out the hard way
			 */
			Gtk::TreePath sortPath = (*it);
			Gtk::TreePath filterPath = docstoresort_->convert_path_to_child_path (sortPath);
			Gtk::TreePath realPath = docstorefilter_->convert_path_to_child_path (filterPath);

			Gtk::ListStore::iterator iter = docstore_->get_iter (realPath);
			docpointers.push_back((*iter)[docpointercol_]);
		}
	}

	return docpointers;
}


Document *DocumentView::getSelectedDoc ()
{
	if (uselistview_) {
		Gtk::TreeSelection::ListHandle_Path paths =
			docslistselection_->get_selected_rows ();

		if (paths.size() != 1) {
			std::cerr << "Warning: DocumentView::getSelectedDoc: size != 1\n";
			return NULL;
		}

		Gtk::TreePath path = (*paths.begin ());
		Gtk::ListStore::iterator iter = docstoresort_->get_iter (path);
		return (*iter)[docpointercol_];

	} else {
		Gtk::IconView::ArrayHandle_TreePaths paths =
			docsiconview_->get_selected_items ();

		if (paths.size() != 1) {
			std::cerr << "Warning: DocumentView::getSelectedDoc: size != 1\n";
			return NULL;
		}

		Gtk::TreePath path = (*paths.begin ());
		Gtk::ListStore::iterator iter = docstoresort_->get_iter (path);
		return (*iter)[docpointercol_];
	}
}


int DocumentView::getSelectedDocCount ()
{
	if (uselistview_) {
		return docslistselection_->get_selected_rows().size ();
	} else {
		return docsiconview_->get_selected_items().size ();
	}
}


int DocumentView::getVisibleDocCount ()
{
	return docstoresort_->children().size();
}

void DocumentView::invokeLinker (Linker *linker)
{
	std::vector<Document*> docs = getSelectedDocs ();

	std::vector<Document*>::iterator it = docs.begin ();
	std::vector<Document*>::iterator const end = docs.end ();
	for (; it != end; it++) {
		if (linker->canLink (*it))
			linker->doLink(*it);
	}
}

bool DocumentView::docClicked (GdkEventButton* event)
{
	/*
	 * Linkers are local to here for now, in the future they might
	 * be plugin-extensible and thus moved elsewhere
	 */
	static std::vector<Linker*> linkers;
	static DoiLinker doi;
	static ArxivLinker arxiv;
	static UrlLinker url;
	static PubmedLinker pubmed;
	static GoogleLinker google;

	/*
	 * Initialise linkers
	 */
	static Glib::RefPtr<Gdk::Pixbuf> linkerIcon;
	if (linkers.size() == 0) {
		linkers.push_back(&doi);
		linkers.push_back(&arxiv);
		linkers.push_back(&url);
		linkers.push_back(&pubmed);
		linkers.push_back(&google);

		std::vector<Linker*>::iterator it = linkers.begin ();
		std::vector<Linker*>::iterator const end = linkers.end ();
		for (; it != end; ++it)
			(*it)->createUI (&win_, this);
	}

	if((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
		/*
		 * Select what's under the pointer if it isn't already
		 * selected
		 */
		if (uselistview_) {
			Gtk::TreeModel::Path clickedpath;
			Gtk::TreeViewColumn *clickedcol;
			int cellx;
			int celly;

			bool const gotpath = docslistview_->get_path_at_pos (
				(int)event->x, (int)event->y,
				clickedpath, clickedcol,
				cellx, celly);

			if (gotpath && !docslistselection_->is_selected (clickedpath)) {
				docslistselection_->unselect_all ();
				docslistselection_->select (clickedpath);
			}
		} else {
			Gtk::TreeModel::Path clickedpath =
				docsiconview_->get_path_at_pos ((int)event->x, (int)event->y);

			if (clickedpath.gobj() != NULL
			    && !docsiconview_->path_is_selected (clickedpath)) {
				docsiconview_->unselect_all ();
				docsiconview_->select_path (clickedpath);
			}
		}

		/*
		 * Get the popup menu widget
		 */
		Gtk::Menu *popupmenu =
			(Gtk::Menu*)win_.uimanager_->get_widget("/DocPopup");

		/* Work out which linkers are applicable */
		std::vector<Document *> docs = getSelectedDocs ();
			
		std::vector<Linker*>::iterator it = linkers.begin();
		std::vector<Linker*>::iterator const end = linkers.end();
		for (; it != end; ++it) {
			Linker *linker = (*it);
			Glib::RefPtr<Gtk::Action> action = win_.actiongroup_->get_action (Glib::ustring("linker_") + linker->getName());

			bool enable = false;
			std::vector<Document*>::iterator docIt = docs.begin ();
			std::vector<Document*>::iterator const docEnd = docs.end ();
			for (; docIt != docEnd; ++docIt) {
				if (linker->canLink(*docIt))
					enable = true;
			}

			action->set_visible(enable);
		}

		/*
		 * Display the menu
		 */
		//popupmenu->show_all ();
		popupmenu->popup (event->button, event->time);

		return true;
	} else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 2)) {
		// Middle click pasting
		win_.onPasteBibtex (GDK_SELECTION_PRIMARY);
		return true;
	} else {
		return false;
	}
}


DocumentView::SubSet DocumentView::selectedDocsHaveTag (int uid)
{
	bool alltrue = true;
	bool allfalse = true;

	std::vector<Document*> docs = getSelectedDocs ();

	std::vector<Document*>::iterator it = docs.begin ();
	std::vector<Document*>::iterator const end = docs.end ();
	for (; it != end; it++) {
		if ((*it)->hasTag(uid)) {
			allfalse = false;
		} else {
			alltrue = false;
		}
	}

	if (allfalse == true)
		return NONE;
	else if (alltrue == true)
		return ALL;
	else if (alltrue == false && allfalse == false)
		return SOME;
	else
		return NONE;
}


DocumentView::Capabilities DocumentView::getDocSelectionCapabilities ()
{
	Capabilities result;

	std::vector<Document*> docs = getSelectedDocs ();

	std::vector<Document*>::iterator it = docs.begin ();
	std::vector<Document*>::iterator const end = docs.end ();

	bool const offline = _global_prefs->getWorkOffline();

	for (; it != end; it++) {
		// Allow web linking even when offline, since it might
		// just be us that's offline, not the web browser
		if ((*it)->canGetMetadata() && !offline)
			result.getmetadata = true;

		if (!(*it)->getFileName().empty())
			result.open = true;
	}

	return result;
}


/*
 * Selection changed, not our problem, let the parent deal.
 */
void DocumentView::docSelectionChanged ()
{
	if (!ignoreSelectionChanged_)
		selectionchangedsignal_.emit ();
}


/*
 * User double clicked on a document
 */
void DocumentView::docActivated (const Gtk::TreeModel::Path& path)
{
	Gtk::TreePath filterPath = docstoresort_->convert_path_to_child_path (path);
	Gtk::TreePath realPath = docstorefilter_->convert_path_to_child_path (filterPath);


	Gtk::ListStore::iterator it = docstore_->get_iter (realPath);
	Document *doc = (*it)[docpointercol_];
	// The methods we're calling should fail out safely and quietly
	// if the number of docs selected != 1
	if (!doc->getFileName ().empty ()) {
		win_.onOpenDoc ();
	} else {
		win_.onDocProperties ();
	}
}


/*
 * Populate a row in docstore_ from a Document
 */
void DocumentView::loadRow (
	Gtk::TreeModel::iterator item,
	Document * const doc)
{
	(*item)[dockeycol_] = doc->getKey();
	// PROGRAM CRASHING THIS HOLIDAY SEASON?
	// THIS LINE DID IT!
	// WHEE!  LOOK AT THIS LINE OF CODE!
	(*item)[docpointercol_] = doc;
	(*item)[docthumbnailcol_] = doc->getThumbnail();
	(*item)[doctitlecol_] = doc->getBibData().getTitle ();
	(*item)[docauthorscol_] = doc->getBibData().getAuthors ();
	(*item)[docyearcol_] = doc->getBibData().getYear ();
	#if GTK_VERSION_GE(2,12)
	(*item)[doctooltipcol_] = String::ucompose (
		// Translators: this is the format for the document tooltips
		_("<b>%1</b>\n%2\n<i>%3</i>"),
		Glib::Markup::escape_text (doc->getKey()),
		Glib::Markup::escape_text (doc->getBibData().getTitle()),
		Glib::Markup::escape_text (doc->getBibData().getAuthors()));
	#endif
	(*item)[docvisiblecol_] = isVisible (doc);

	Glib::ustring title = Utility::wrap (doc->getField ("title"), 35, 1, false);
	Glib::ustring authors =	Utility::firstAuthor(doc->getField("author"));
	authors = Utility::strip (authors, "{");
	authors = Utility::strip (authors, "}");
	Glib::ustring key = doc->getKey();
	Glib::ustring year = doc->getField("year");

	if (title.empty())
		title = " ";
	if (authors.empty())
		authors = " ";
	if (key.empty())
		key = " ";
	if (year.empty())
		year = " ";

	(*item)[doccaptioncol_] = String::ucompose (
		// Translators: this is the format for the document captions
		_("<span size='xx-small'> </span>\n<small>%1</small>\n<b>%2</b>\n%3\n<i>%4</i>"),
		Glib::Markup::escape_text (key),
		Glib::Markup::escape_text (title),
		Glib::Markup::escape_text (year),
		Glib::Markup::escape_text (authors));
}

/*
 * Return whether a document matches searches and tag filters
 */

bool DocumentView::isVisible (Document * const doc)
{
	Glib::ustring const searchtext = searchentry_->get_text ();
	bool const search = !searchtext.empty ();

	bool visible = true;
	for (std::vector<int>::iterator tagit = win_.filtertags_.begin();
	     tagit != win_.filtertags_.end(); ++tagit) {
		if (!(*tagit == ALL_TAGS_UID
		    || (*tagit == NO_TAGS_UID && doc->getTags().empty())
		    || doc->hasTag(*tagit))) {
		    	// A tag is selected that we do not match
			visible = false;
			break;
		}
	}

	if (search && visible) {
		if (!doc->matchesSearch (searchtext))
			visible = false;
	}
	
	/*
	 * TODO iterate over taggerUris 
	 */
	 
	return visible;
}

/*
 * Update the visibility of all rows
 *
 * This needs to be called:
 *  - when global conditions for visibility
 *     change: when the tag filter is changed, when search text is 
 *     changed, or when tracker returns some matches
 *
 *  Things like adding and updating docs don't need to call this as well
 *
 */
void DocumentView::updateVisible ()
{
	ignoreSelectionChanged_ = true;
	Gtk::TreeModel::iterator item = docstore_->children().begin();
	Gtk::TreeModel::iterator const end = docstore_->children().end();
	for (; item != end; ++item) {
		Document * const doc = (*item)[docpointercol_];
		(*item)[docvisiblecol_] = isVisible (doc);
	}
	ignoreSelectionChanged_ = false;
	docSelectionChanged ();
}


/*
 * Optimisation: O(N)
 *
 *  - Refresh document's row in docstore_ from Document.
 *  - Update visibility of document's row
 */
void DocumentView::updateDoc (Document * const doc)
{
	Gtk::TreeModel::iterator item = docstore_->children().begin();
	Gtk::TreeModel::iterator const end = docstore_->children().end();
	for (; item != end; ++item) {
		if ((*item)[docpointercol_] == doc) {
			loadRow (item, doc);
			return;
		}
	}
	
	std::cerr << "DocumentView::updateDoc: Warning: doc not found\n";
}


/*
 * Remove the row with docpointercol_ == doc from docstore_
 */
void DocumentView::removeDoc (Document * const doc)
{
	bool found = false;

	ignoreSelectionChanged_ = true;
	Gtk::TreeModel::iterator it = docstore_->children().begin();
	Gtk::TreeModel::iterator const end = docstore_->children().end();
	for (; it != end; ++it) {
		if ((*it)[docpointercol_] == doc) {
			docstore_->erase (it);
			found = true;
			break;
		}
	}
	ignoreSelectionChanged_ = false;
	docSelectionChanged ();
	
	if (!found)
		std::cerr << "DocumentView::removeDoc: Warning: doc not found\n";
}

/*
 * Append a row to docstore_ and load data from Document
 */
void DocumentView::addDoc (Document * doc)
{
	doc->setView(this);

	Gtk::TreeModel::iterator item = docstore_->append();
	loadRow (item, doc);
  
	Gtk::TreeModel::Path path = 
		docstoresort_->get_path (
			docstoresort_->convert_child_iter_to_iter (
				docstorefilter_->convert_child_iter_to_iter (item)));

	docslistview_->scroll_to_row (path);
	docslistselection_->unselect_all ();
	docslistselection_->select (path);

	docsiconview_->unselect_all ();
	docsiconview_->scroll_to_path (path, false, 0.0, 0.0);
	docsiconview_->select_path (path);
}


/*
 * Please, please populate tags etc before calling this with 
 * a tag-related handler connected to the selectionchanged
 * signal
 */
void DocumentView::populateDocStore ()
{
	//std::cerr << "RefWindow::populateDocStore >>\n";
	ignoreSelectionChanged_ = true;

	/* XXX not the only one any more! */
	// This is our notification that something about the documentlist
	// has changed, including its length, so update dependent sensitivities:
	win_.actiongroup_->get_action("ExportBibtex")
		->set_sensitive (lib_.doclist_->size() > 0);
	// Save initial selection
	Gtk::TreePath initialpath;
	if (uselistview_) {
		Gtk::TreeSelection::ListHandle_Path paths =
			docslistselection_->get_selected_rows ();
		if (paths.size () > 0)
			initialpath = (*paths.begin());
	} else {
		Gtk::IconView::ArrayHandle_TreePaths paths =
			docsiconview_->get_selected_items ();
		if (paths.size () > 0)
			initialpath = (*paths.begin());
	}

	docstore_->clear ();

	// Populate from library_->doclist_
	DocumentList::Container& docvec = lib_.doclist_->getDocs();
	DocumentList::Container::iterator docit = docvec.begin();
	DocumentList::Container::iterator const docend = docvec.end();
	for (; docit != docend; ++docit) {
		addDoc (&(*docit));
	}

	// Restore initial selection
	if (uselistview_) {
		docslistselection_->select (initialpath);
	} else {
		docsiconview_->select_path (initialpath);
	}

	docsiconview_->unselect_all ();
	docslistselection_->unselect_all ();
	ignoreSelectionChanged_ = false;
	docSelectionChanged ();
}


/*
 * Clear out all documents
 */
void DocumentView::clear ()
{
	docstore_->clear ();
}


void
end_search (GPtrArray * out_array,
        GError * error,
        gpointer user_data)
{
	DocumentView *docview = (DocumentView*) user_data;
	
	printf (">>end_search\n");
	
	docview->trackerUris_.clear ();
	
	
	
	if (error) {
		printf ("Tracker error\n");
		g_error_free (error);
		// TODO Call the update visibility function
		return;
	}
	
	if (out_array) {
		printf ("%d results\n", out_array->len);
		for (unsigned int i = 0; i < out_array->len; ++i) {
			char **meta = (char**) g_ptr_array_index (out_array, i);
			printf ("0x%x : %s\n", meta[0], meta[0]);
			docview->trackerUris_.push_back (meta[0]);
		}
		g_ptr_array_free (out_array, TRUE);
	}
	
	/*
	 * We're probably calling this twice: we already called it in 
	 * onSearchChanged
	 */
	docview->updateVisible ();
}


void DocumentView::onSearchChanged ()
{
	Gdk::Color yellowish ("#f7f7be");
	Gdk::Color black ("#000000");
	
	bool hasclearbutton =
		((Sexy::IconEntry*) searchentry_)->get_icon (Sexy::ICON_ENTRY_SECONDARY);
	
	if (!searchentry_->get_text ().empty()) {
	/*if (entry->priv->is_a11y_theme)
		return;*/
		searchentry_->modify_base (Gtk::STATE_NORMAL, yellowish);
		searchentry_->modify_text (Gtk::STATE_NORMAL, black);
		if (!hasclearbutton)
			((Sexy::IconEntry*) searchentry_)->add_clear_button ();
	} else {
		searchentry_->unset_base (Gtk::STATE_NORMAL);
		searchentry_->unset_text (Gtk::STATE_NORMAL);
		if (hasclearbutton)
			((Sexy::IconEntry*) searchentry_)->set_icon (Sexy::ICON_ENTRY_SECONDARY, NULL);
	}
	
	updateVisible ();
	
	
	/*
	 * Get some tracker results and spit them to the console
	 */
#ifdef USE_TRACKER
	TrackerClient *client = tracker_connect (0);
	if (!client) {
		printf ("Error in tracker_connect\n");
		return;
	}
	
	Glib::ustring searchtext = searchentry_->get_text();
	
	if (! searchtext.empty()) {
		tracker_search_text_detailed_async (client,
	                -1,
	                SERVICE_FILES,
	                searchtext.c_str(),
	                0, 10,
	                (TrackerGPtrArrayReply)end_search,
	                this);
        }
#endif

	
}


void DocumentView::onSortColumnChanged ()
{
	Gtk::SortType order;
	int column;
	docstoresort_->get_sort_column_id (column, order);
	_global_prefs->setListSort (column, order);
}

void DocumentView::onColumnEdited (
	const Glib::ustring& pathStr, 
	const Glib::ustring& newText,
	const Glib::ustring &columnName)
{
	Gtk::TreePath sortPath (pathStr);
	Gtk::TreePath filterPath = docstoresort_->convert_path_to_child_path (sortPath);
	Gtk::TreePath realPath = docstorefilter_->convert_path_to_child_path (filterPath);

	Gtk::TreeModel::iterator iter = docstore_->get_iter (realPath);
	Gtk::TreeModelColumn<Glib::ustring> col = listViewColumns_[columnName];
	(*iter)[col] = newText;

	Document *doc = (*iter)[docpointercol_];
	doc->setField (columnName, newText);

	win_.setDirty (true);
}


void DocumentView::addCol (
	Glib::ustring const &name,
	Glib::ustring const &caption,
	Gtk::TreeModelColumn<Glib::ustring> modelCol,
	bool const expand,
	bool const ellipsize)
{
	listViewColumns_[name] = modelCol;

	// Er, we're actually passing this as reference, is this the right way
	// to create it?  Will the treeview actually copy it?
	Gtk::CellRendererText *cell;
	Gtk::TreeViewColumn *col;

	col = Gtk::manage (new Gtk::TreeViewColumn (caption, modelCol));
	col->set_resizable (true);
	col->set_expand (expand);
	col->set_sort_column (modelCol);
	cell = (Gtk::CellRendererText *) col->get_first_cell_renderer ();
	if (ellipsize)
		cell->property_ellipsize () = Pango::ELLIPSIZE_END;
	cell->property_editable () = true;
	cell->signal_edited ().connect (
		sigc::bind (
			sigc::mem_fun (*this, &DocumentView::onColumnEdited),
			name)
		);
	docslistview_->append_column (*col);
}


void DocumentView::populateColumns ()
{
	addCol ("key", _("Key"), dockeycol_, false, false);
	addCol ("title", _("Title"), doctitlecol_, true, true);
	addCol ("authors", _("Authors"), docauthorscol_, false, true);
	addCol ("year", _("Year"), docyearcol_, false, false);
}
