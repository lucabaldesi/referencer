
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
#include "ev-tooltip.h"
#include "icon-entry.h"
#include "Library.h"
#include "Preferences.h"
#include "RefWindow.h"
#include "Transfer.h"
#include "ucompose.hpp"
#include "Utility.h"

#include "DocumentView.h"


DocumentView::~DocumentView ()
{
	gtk_widget_destroy (doctooltip_);
}


DocumentView::DocumentView (
	RefWindow &refwin,
	Library &lib,
	bool const uselistview)
	
 : win_ (refwin), lib_(lib)
{
	hoverdoc_ = NULL;

	// The iconview side
	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);
	Gtk::Frame *iconsframe = new Gtk::Frame ();
	iconsframe->add (*vbox);
	
	pack_start (*iconsframe, true, true, 0);

	// Create the store for the document icons
	Gtk::TreeModel::ColumnRecord iconcols;
	iconcols.add(docpointercol_);
	iconcols.add(dockeycol_);
	iconcols.add(doctitlecol_);
	iconcols.add(docauthorscol_);
	iconcols.add(docyearcol_);
	iconcols.add(docthumbnailcol_);
	docstore_ = Gtk::ListStore::create(iconcols);

	// Create the IconView for the document icons
	Gtk::IconView *icons = Gtk::manage(new Gtk::IconView(docstore_));
	icons->set_text_column(dockeycol_);
	icons->set_pixbuf_column(docthumbnailcol_);
	icons->signal_item_activated().connect (
		sigc::mem_fun (*this, &DocumentView::docActivated));

	icons->signal_button_press_event().connect(
		sigc::mem_fun (*this, &DocumentView::docClicked), false);

	icons->signal_selection_changed().connect(
		sigc::mem_fun (*this, &DocumentView::docSelectionChanged));

	icons->set_selection_mode (Gtk::SELECTION_MULTIPLE);
	icons->set_columns (-1);

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
	icons->set_events (Gdk::POINTER_MOTION_MASK | Gdk::LEAVE_NOTIFY_MASK);
	icons->signal_motion_notify_event ().connect_notify (
		sigc::mem_fun (*this, &DocumentView::onDocMouseMotion));
	icons->signal_leave_notify_event ().connect_notify (
		sigc::mem_fun (*this, &DocumentView::onDocMouseLeave));

	docsiconview_ = icons;

	Gtk::ScrolledWindow *iconsscroll = Gtk::manage(new Gtk::ScrolledWindow());
	iconsscroll->add(*icons);
	iconsscroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	vbox->pack_start(*iconsscroll, true, true, 0);

	docsiconscroll_ = iconsscroll;

	doctooltip_ = ev_tooltip_new (GTK_WIDGET(win_.window_->gobj()));

	// The TreeView for the document list
	Gtk::TreeView *table = Gtk::manage (new Gtk::TreeView(docstore_));
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

	// Er, we're actually passing this as reference, is this the right way
	// to create it?  Will the treeview actually copy it?
	Gtk::CellRendererText *cell;
	Gtk::TreeViewColumn *col;
	col = Gtk::manage (new Gtk::TreeViewColumn (_("Key"), dockeycol_));
	col->set_resizable (true);
	col->set_sort_column (dockeycol_);
	table->append_column (*col);
	col = Gtk::manage (new Gtk::TreeViewColumn (_("Title"), doctitlecol_));
	col->set_resizable (true);
	col->set_expand (true);
	col->set_sort_column (doctitlecol_);
	cell = (Gtk::CellRendererText *) col->get_first_cell_renderer ();
	cell->property_ellipsize () = Pango::ELLIPSIZE_END;
	table->append_column (*col);
	col = Gtk::manage (new Gtk::TreeViewColumn (_("Authors"), docauthorscol_));
	col->set_resizable (true);
	//col->set_expand (true);
	col->set_sort_column (docauthorscol_);
	cell = (Gtk::CellRendererText *) col->get_first_cell_renderer ();
	cell->property_ellipsize () = Pango::ELLIPSIZE_END;
	table->append_column (*col);
	col = Gtk::manage (new Gtk::TreeViewColumn (_("Year  "), docyearcol_));
	col->set_resizable (true);
	col->set_sort_column (docyearcol_);
	table->append_column (*col);

	docslistview_ = table;

	Gtk::ScrolledWindow *tablescroll = Gtk::manage(new Gtk::ScrolledWindow());
	tablescroll->add(*table);
	tablescroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	vbox->pack_start(*tablescroll, true, true, 0);

	docslistscroll_ = tablescroll;
	
	setUseListView (uselistview);
	
	
	
	Sexy::IconEntry *searchentry = Gtk::manage (new Sexy::IconEntry ());
	Gtk::Image *searchicon = Gtk::manage (
		new Gtk::Image (Gtk::Stock::FIND, Gtk::ICON_SIZE_BUTTON));
	searchentry->set_icon (Sexy::ICON_ENTRY_PRIMARY, searchicon);
	searchentry->signal_changed ().connect (
		sigc::mem_fun (*this, &DocumentView::onSearchChanged));
	
	searchentry_ = searchentry;
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


void DocumentView::onDocMouseMotion (GdkEventMotion* event)
{
	// Guh, it's giving me these in the iconview, so doesn't work when scrolled down
	int x = (int)event->x;
	int y = (int)event->y;

	Gtk::TreeModel::Path path = docsiconview_->get_path_at_pos (x, y);
	bool havepath = path.gobj() != NULL;

	Document *doc = NULL;
	if (havepath) {
		Gtk::ListStore::iterator it = docstore_->get_iter (path);
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
			Glib::ustring const destinationdir =
				liburi->get_scheme ()
				+ "://"
				+	Glib::build_filename (
					liburi->extract_dirname (),
					_("Downloaded documents"));

			if (!Gnome::Vfs::Uri::create (destinationdir)->uri_exists ()) {
				Gnome::Vfs::Handle::make_directory (destinationdir, 0757);
			}

			std::cerr << "'" << destinationdir << "\n";
			std::cerr << "'" <<  uri->to_string () << "\n";
			std::cerr << "'" <<  uri->extract_short_path_name () << "\n";

			Glib::ustring const destination = Glib::build_filename (
				destinationdir,
				uri->extract_short_path_name ());

			std::cerr << "'" << destination << "\n";
			Transfer::downloadRemoteFile (*it, destination);
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
			Gtk::TreePath path = (*it);
			Gtk::ListStore::iterator iter = docstore_->get_iter (path);
			docpointers.push_back((*iter)[docpointercol_]);
		}
	} else {
		Gtk::IconView::ArrayHandle_TreePaths paths =
			docsiconview_->get_selected_items ();

		Gtk::IconView::ArrayHandle_TreePaths::iterator it = paths.begin ();
		Gtk::IconView::ArrayHandle_TreePaths::iterator const end = paths.end ();
		for (; it != end; it++) {
			Gtk::TreePath path = (*it);
			Gtk::ListStore::iterator iter = docstore_->get_iter (path);
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
			return false;
		}

		Gtk::TreePath path = (*paths.begin ());
		Gtk::ListStore::iterator iter = docstore_->get_iter (path);
		return (*iter)[docpointercol_];

	} else {
		Gtk::IconView::ArrayHandle_TreePaths paths =
			docsiconview_->get_selected_items ();

		if (paths.size() != 1) {
			std::cerr << "Warning: DocumentView::getSelectedDoc: size != 1\n";
			return false;
		}

		Gtk::TreePath path = (*paths.begin ());
		Gtk::ListStore::iterator iter = docstore_->get_iter (path);
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
	return docstore_->children().size();
}


bool DocumentView::docClicked (GdkEventButton* event)
{
  if((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
  	if (uselistview_) {
		  Gtk::TreeModel::Path clickedpath;
		  Gtk::TreeViewColumn *clickedcol;
		  int cellx;
		  int celly;
	  	bool const gotpath = docslistview_->get_path_at_pos (
	  		(int)event->x, (int)event->y,
	  		clickedpath, clickedcol,
	  		cellx, celly);
			if (gotpath) {
				if (!docslistselection_->is_selected (clickedpath)) {
					docslistselection_->unselect_all ();
					docslistselection_->select (clickedpath);
				}
			}
  	} else {
			Gtk::TreeModel::Path clickedpath =
				docsiconview_->get_path_at_pos ((int)event->x, (int)event->y);

			if (clickedpath.gobj() != NULL) {
				if (!docsiconview_->path_is_selected (clickedpath)) {
					docsiconview_->unselect_all ();
					docsiconview_->select_path (clickedpath);
				}
			}
		}

		Gtk::Menu *popupmenu =
			(Gtk::Menu*)win_.uimanager_->get_widget("/DocPopup");
		popupmenu->popup (event->button, event->time);

		return true;
  } else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 2)) {
  	// Epic middle click pasting
  	win_.onPasteBibtex (GDK_SELECTION_PRIMARY);

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
		if ((*it)->canWebLink())
			result.weblink = true;

		if ((*it)->canGetMetadata() && !offline)
			result.getmetadata = true;

		if (!(*it)->getFileName().empty())
			result.open = true;
	}

	return result;
}


void DocumentView::docSelectionChanged ()
{
	selectionchangedsignal_.emit ();
}


void DocumentView::docActivated (const Gtk::TreeModel::Path& path)
{
	Gtk::ListStore::iterator it = docstore_->get_iter (path);
	Document *doc = (*it)[docpointercol_];
	// The methods we're calling should fail out safely and quietly
	// if the number of docs selected != 1
	if (!doc->getFileName ().empty ()) {
		win_.onOpenDoc ();
	} else if (doc->canWebLink ()) {
		win_.onWebLinkDoc ();
	} else {
		win_.onDocProperties ();
	}
}


void DocumentView::populateDocStore ()
{
	//std::cerr << "RefWindow::populateDocStore >>\n";

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

	Glib::ustring const searchtext = searchentry_->get_text ();
	bool const search = !searchtext.empty ();

	// Populate from library_->doclist_
	DocumentList::Container& docvec = lib_.doclist_->getDocs();
	DocumentList::Container::iterator docit = docvec.begin();
	DocumentList::Container::iterator const docend = docvec.end();
	for (; docit != docend; ++docit) {
		bool filtered = true;
		for (std::vector<int>::iterator tagit = win_.filtertags_.begin();
		     tagit != win_.filtertags_.end(); ++tagit) {
			if (*tagit == ALL_TAGS_UID
			    || (*tagit == NO_TAGS_UID && (*docit).getTags().empty())
			    || (*docit).hasTag(*tagit)) {
				filtered = false;
				break;
			}
		}

		if (search && !filtered) {
			if (!(*docit).matchesSearch (searchtext))
				filtered = true;
		}

		if (filtered)
			continue;

		Gtk::TreeModel::iterator item = docstore_->append();
		(*item)[dockeycol_] = (*docit).getKey();
		// PROGRAM CRASHING THIS HOLIDAY SEASON?
		// THIS LINE DID IT!
		// WHEE!  LOOK AT THIS LINE OF CODE!
		(*item)[docpointercol_] = &(*docit);
		(*item)[docthumbnailcol_] = (*docit).getThumbnail();
		(*item)[doctitlecol_] = (*docit).getBibData().getTitle ();
		(*item)[docauthorscol_] = (*docit).getBibData().getAuthors ();
		(*item)[docyearcol_] = (*docit).getBibData().getYear ();
	}

	// Restore initial selection
	if (uselistview_) {
		docslistselection_->select (initialpath);
	} else {
		docsiconview_->select_path (initialpath);
	}

	// If we set the selection in a valid way
	// this probably already got called, although not for
	// opening library etc.
	win_.updateStatusBar ();
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
	populateDocStore ();
}
