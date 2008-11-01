
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */


#include <map>
#include <cmath>
#include <iostream>
#include <fstream>

#include <gtkmm.h>
#include <libgnomeuimm.h>
#include <libgnomevfsmm.h>
#include <glibmm/i18n.h>
#include "ucompose.hpp"

#include "Document.h"
#include "DocumentList.h"
#include "DocumentProperties.h"
#include "DocumentView.h"
#include "Library.h"
#include "LibraryParser.h"
#include "Plugin.h"
#include "Preferences.h"
#include "Progress.h"
#include "TagList.h"

#include "config.h"
#include "RefWindow.h"
#include "referencer_ui.h"


RefWindow::RefWindow ()
{
	ignoreTagSelectionChanged_ = false;
	ignoreTaggerActionToggled_ = false;
	ignoreDocSelectionChanged_ = false;

	dirty_ = false;

	library_ = new Library (*this);

	docpropertiesdialog_ = new DocumentProperties ();

	constructUI ();

	progress_ = new Progress (*this);

	Glib::ustring const libfile = _global_prefs->getLibraryFilename ();

	if (!libfile.empty() && library_->load (libfile)) {
		setOpenedLib (libfile);
		populateTagList ();
		docview_->populateDocStore ();
	} else {
		onNewLibrary ();
	}

	updateStatusBar ();
}


RefWindow::~RefWindow ()
{
	_global_prefs->setLibraryFilename (openedlib_);

	delete progress_;
	delete docpropertiesdialog_;
	delete library_;	
}


void RefWindow::signalException ()
{
    try {
        throw;
    } catch (const Glib::Exception &ex) {
        Utility::exceptionDialog (&ex, "executing UI action");
    } catch (const std::exception &ex) {
        DEBUG (Glib::ustring("std::exception in signal handler, what=") + ex.what());

    } catch (...) {
        DEBUG ("Unknown type of exception in signal handler");
    }
}


void RefWindow::run ()
{
	DEBUG ("entering main loop");

        /* Connect up exception handler for UI signals */
        Glib::add_exception_handler (sigc::mem_fun (*this, &RefWindow::signalException));

        /* Enter the main event loop */
	Gnome::Main::run (*window_);
}


void RefWindow::constructUI ()
{
	/* The main window */
	window_ = new Gtk::Window(Gtk::WINDOW_TOPLEVEL);

	std::pair<int, int> size = _global_prefs->getWindowSize ();
	window_->set_default_size (size.first, size.second);
	window_->signal_delete_event().connect (
		sigc::mem_fun (*this, &RefWindow::onDelete));
	window_->signal_configure_event().connect_notify (
		sigc::mem_fun (*this, &RefWindow::onResize));

	try {
		Glib::RefPtr<Gdk::Pixbuf> icon = Gdk::Pixbuf::create_from_file(
			Utility::findDataFile("referencer.svg"));
		window_->set_icon (icon);
	} catch (Gdk::PixbufError ex) {
	}

	/* Vbox fills the whole window */
	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);
	window_->add (*vbox);

	/* The menu bar */
	constructMenu ();
	vbox->pack_start (*uimanager_->get_widget("/MenuBar"), false, false, 0);

	/* The tool bar */
	vbox->pack_start (*uimanager_->get_widget("/ToolBar"), false, false, 0);
	Gtk::Toolbar *toolbar = (Gtk::Toolbar *) uimanager_->get_widget("/ToolBar");
	Gtk::ToolItem *paditem = Gtk::manage (new Gtk::ToolItem);
	paditem->set_expand (true);
	// To put the search box on the right
	toolbar->append (*paditem);
	// In order to prevent search box falling off the edge.
	toolbar->set_show_arrow (false);

	/* Contains tags and document/notes view */
	Gtk::HPaned *hpaned = Gtk::manage(new Gtk::HPaned());
	vbox->pack_start (*hpaned, true, true, 0);
	
	/* Contains document and notes view */
	Gtk::VPaned *vpaned = Gtk::manage(new Gtk::VPaned());
	hpaned->pack2(*vpaned, Gtk::EXPAND);

	/* The statusbar */
	Gtk::HBox *statusbox = Gtk::manage (new Gtk::HBox());
	vbox->pack_start (*statusbox, false, false, 0);	

	statusbar_ = Gtk::manage (new Gtk::Statusbar ());

	offlineicon_ = Gtk::manage (new Gtk::Image ());
	updateOfflineIcon ();

	statusbox->pack_start (*offlineicon_, false, false, 6);
	statusbox->pack_start (*statusbar_, true, true, 0);

	progressbar_ = Gtk::manage (new Gtk::ProgressBar ());
	statusbar_->pack_start (*progressbar_, false, false, 0);
	
	/* The document view */
	docview_ = Gtk::manage (
		new DocumentView(
			*this,
			*library_,
			_global_prefs->getUseListView ()));
	docview_->getSelectionChangedSignal ().connect (
		sigc::mem_fun (*this, &RefWindow::docSelectionChanged));
	
	// The header and wrapper for the notes view
	notespane_ = Gtk::manage (new Gtk::VBox ());
	Gtk::HBox *notesheader = new Gtk::HBox ();
	noteslabel_ = new Gtk::Label ();
	noteslabel_->set_ellipsize(Pango::ELLIPSIZE_END);
	noteslabel_->set_alignment (0.0, 0.0);
	
	// The button to close the notes pane (seems like it could be simpler)
	Gtk::Button *notesclosebutton = new Gtk::Button ();
	Gtk::Image *tinycloseimage = Gtk::manage(new Gtk::Image ( Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU ));
	notesclosebutton->set_image(*tinycloseimage);
	notesclosebutton->set_relief(Gtk::RELIEF_NONE);
	notesclosebutton->signal_clicked ().connect_notify( 
		sigc::mem_fun(*this, &RefWindow::onNotesClose ) );
	
	// The note region itself
	Gtk::ScrolledWindow *notesscroll = new Gtk::ScrolledWindow();
	notesview_ = new Gtk::TextView;
	notesscroll->set_policy (Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	notesscroll->set_shadow_type (Gtk::SHADOW_NONE);
	notesview_->set_wrap_mode(Gtk::WRAP_WORD);
	notesbuffer_ = notesview_->get_buffer();
	notesbuffer_->signal_changed().connect(
		sigc::mem_fun (*this, &RefWindow::onNotesChanged));
	notesbufferignore_ = false;

        noteslabel_->set_mnemonic_widget (*notesview_);

	// Pack up the notes and document views
	notesheader->pack_start(*noteslabel_, true, true, 0);
	notesheader->pack_end(*notesclosebutton, false, false, 0);
	notespane_->pack_start(*notesheader, false, false, 0);
	notespane_->pack_start(*notesscroll);

	notesscroll->add(*notesview_);
	notesscroll->set_shadow_type (Gtk::SHADOW_IN);

	vpaned->pack1(*docview_, Gtk::EXPAND);
	vpaned->pack2(*notespane_, Gtk::FILL);

	docview_->signal_size_allocate().connect (
		sigc::mem_fun (*this, &RefWindow::onNotesPaneResize));
	vpaned->set_position (_global_prefs->getNotesPaneHeight() );

	/* Drop in the search box */
	Gtk::ToolItem *searchitem = Gtk::manage (new Gtk::ToolItem);
	searchitem->add (docview_->getSearchEntry());
	toolbar->append (*searchitem);
		
	/* The tags */
	vbox = Gtk::manage(new Gtk::VBox);
	Gtk::Frame *tagsframe = new Gtk::Frame ();
	tagsframe->add(*vbox);
	hpaned->pack1(*tagsframe, Gtk::FILL);

	tagpane_ = tagsframe;

	Gtk::VBox *filtervbox = Gtk::manage (new Gtk::VBox);
	vbox->pack_start (*filtervbox, true, true, 0);

	// Create the store for the tag list
	Gtk::TreeModel::ColumnRecord tagcols;
	tagcols.add(taguidcol_);
	tagcols.add(tagnamecol_);
	tagcols.add(tagfontcol_);
	tagstore_ = Gtk::ListStore::create(tagcols);

	tagstore_->set_sort_func (tagnamecol_, sigc::mem_fun (*this, &RefWindow::sortTags));
	tagstore_->set_sort_column (tagnamecol_, Gtk::SORT_ASCENDING);

	// Create the treeview for the tag list
	Gtk::TreeView *tags = Gtk::manage(new Gtk::TreeView(tagstore_));
	//tags->append_column("UID", taguidcol_);
	Gtk::CellRendererText *render = Gtk::manage(new Gtk::CellRendererText());
	render->property_editable() = true;
	render->signal_edited().connect (
		sigc::mem_fun (*this, &RefWindow::tagNameEdited));
	render->signal_editing_started().connect (
		sigc::mem_fun (*this, &RefWindow::tagNameEditingStarted));
	render->property_xalign () = 0.5;
        Gtk::TreeView::Column *namecol = Gtk::manage(
		new Gtk::TreeView::Column (_("Tags"), *render));
	namecol->set_cell_data_func (
		*render, sigc::mem_fun (*this, &RefWindow::tagCellRenderer));
	namecol->add_attribute (render->property_markup (), tagnamecol_);
	namecol->add_attribute (render->property_font_desc (), tagfontcol_);
	((Gtk::CellRendererText*)(std::vector<Gtk::CellRenderer*>(namecol->get_cell_renderers())[0]))->property_ellipsize() = Pango::ELLIPSIZE_MIDDLE;
	
	tags->append_column (*namecol);
	tags->signal_button_press_event().connect_notify(
		sigc::mem_fun (*this, &RefWindow::tagClicked));
	tags->set_row_separator_func(
		sigc::mem_fun(*this, &RefWindow::tagSeparator));
	tags->set_headers_visible (false);
	tags->set_search_column (tagnamecol_);

	Gtk::ScrolledWindow *tagsscroll = Gtk::manage(new Gtk::ScrolledWindow());
	tagsscroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	tagsscroll->set_shadow_type (Gtk::SHADOW_NONE);
	tagsscroll->add (*tags);
	filtervbox->pack_start(*tagsscroll, true, true, 0);

	tagview_ = tags;

	tagselection_ = tags->get_selection();
	tagselection_->signal_changed().connect_notify (
		sigc::mem_fun (*this, &RefWindow::tagSelectionChanged));
	tagselection_->set_mode (Gtk::SELECTION_MULTIPLE);

	Gtk::Toolbar& tagbar = (Gtk::Toolbar&) *uimanager_->get_widget("/TagBar");
	tagbar.set_toolbar_style (Gtk::TOOLBAR_BOTH_HORIZ);
	tagbar.set_orientation (Gtk::ORIENTATION_VERTICAL);
	tagbar.set_show_arrow (false);
	std::vector<Gtk::Widget*> tagbarButtons = tagbar.get_children ();
	std::vector<Gtk::Widget*>::iterator buttonIter = tagbarButtons.begin ();
	std::vector<Gtk::Widget*>::iterator const buttonEnd = tagbarButtons.end ();
	for (; buttonIter != buttonEnd; ++buttonIter) {
		Gtk::ToolButton *toolbutton = (Gtk::ToolButton*)(*buttonIter);
		Gtk::Button *button = (Gtk::Button*)toolbutton->get_child ();
		/* FIXME: should check the toolbutton is actually a toolbutton
		 * rather than letting it fail on get_child for other types */
		if (!button)
			break;
		button->set_relief (Gtk::RELIEF_NORMAL);
	}
	filtervbox->pack_start (tagbar, false, false, 0);

	window_->show_all ();
	// Update visibilities
	docview_->setUseListView (!docview_->getUseListView());
	docview_->setUseListView (!docview_->getUseListView());

	// Initialise and listen for prefs change or user input
	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("ShowTagPane"))->set_active(
				_global_prefs->getShowTagPane ());
	onShowTagPanePrefChanged ();
	_global_prefs->getShowTagPaneSignal ().connect (
		sigc::mem_fun (*this, &RefWindow::onShowTagPanePrefChanged));

	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("ShowTagPane"))->signal_toggled ().connect (
				sigc::mem_fun(*this, &RefWindow::onShowTagPaneToggled));
				
	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("ShowNotesPane"))->set_active(
				_global_prefs->getShowNotesPane ());
	onShowNotesPanePrefChanged ();
	_global_prefs->getShowNotesPaneSignal ().connect (
		sigc::mem_fun (*this, &RefWindow::onShowNotesPanePrefChanged));

	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("ShowNotesPane"))->signal_toggled ().connect (
				sigc::mem_fun(*this, &RefWindow::onShowNotesPaneToggled));

	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("WorkOffline"))->set_active(
				_global_prefs->getWorkOffline());
	_global_prefs->getWorkOfflineSignal ().connect (
		sigc::mem_fun (*this, &RefWindow::onWorkOfflinePrefChanged));

	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("WorkOffline"))->signal_toggled ().connect (
				sigc::mem_fun(*this, &RefWindow::onWorkOfflineToggled));
				
	Glib::RefPtr <Gtk::RadioAction>::cast_static(
			actiongroup_->get_action ("UseListView"))->set_active(
				docview_->getUseListView());
	Glib::RefPtr <Gtk::RadioAction>::cast_static(
			actiongroup_->get_action ("UseIconView"))->set_active(
				!docview_->getUseListView());

	_global_prefs->getUseListViewSignal ().connect (
		sigc::mem_fun (*this, &RefWindow::onUseListViewPrefChanged));

	Glib::RefPtr <Gtk::RadioAction>::cast_static(
		actiongroup_->get_action ("UseListView"))->signal_toggled ().connect (
			sigc::mem_fun(*this, &RefWindow::onUseListViewToggled));


	// Generate UI for plugins and connect to changes
	onEnabledPluginsPrefChanged ();
	_global_prefs->getPluginDisabledSignal ().connect (
		sigc::mem_fun (*this, &RefWindow::onEnabledPluginsPrefChanged));
}


void RefWindow::constructMenu ()
{
	actiongroup_ = Gtk::ActionGroup::create();

	actiongroup_->add ( Gtk::Action::create("LibraryMenu", _("_Library")) );
	actiongroup_->add( Gtk::Action::create("NewLibrary",
		Gtk::Stock::NEW),
  	sigc::mem_fun(*this, &RefWindow::onNewLibrary));
	actiongroup_->add( Gtk::Action::create("OpenLibrary",
		Gtk::Stock::OPEN, _("_Open...")),
  	sigc::mem_fun(*this, &RefWindow::onOpenLibrary));
	actiongroup_->add( Gtk::Action::create("SaveLibrary",
		Gtk::Stock::SAVE),
  	sigc::mem_fun(*this, &RefWindow::onSaveLibrary));
	actiongroup_->add( Gtk::Action::create("SaveAsLibrary",
		Gtk::Stock::SAVE_AS, _("Save _As...")), Gtk::AccelKey ("<control><shift>s"),
  	sigc::mem_fun(*this, &RefWindow::onSaveAsLibrary));
	actiongroup_->add( Gtk::Action::create("ExportBibtex",
		Gtk::Stock::CONVERT, _("E_xport as BibTeX...")), Gtk::AccelKey ("<control>b"),
  	sigc::mem_fun(*this, &RefWindow::onExportBibtex));
	actiongroup_->add( Gtk::Action::create("ManageBibtex",
		Gtk::Stock::CONVERT, _("_Manage BibTeX File...")), Gtk::AccelKey ("<control><shift>b"),
		sigc::mem_fun(*this, &RefWindow::onManageBibtex));
	actiongroup_->add( Gtk::Action::create("LibraryFolder",
		Gtk::Stock::OPEN, _("_Library Folder...")),
		sigc::mem_fun(*this, &RefWindow::onLibraryFolder));
	actiongroup_->add( Gtk::Action::create("Import",
		_("_Import...")),
  	sigc::mem_fun(*this, &RefWindow::onImport));
	actiongroup_->add( Gtk::ToggleAction::create("WorkOffline",
		_("_Work Offline")));
	actiongroup_->add( Gtk::Action::create("Quit", Gtk::Stock::QUIT),
  	sigc::mem_fun(*this, &RefWindow::onQuit));

	actiongroup_->add ( Gtk::Action::create("EditMenu", _("_Edit")) );

	actiongroup_->add ( Gtk::Action::create("ViewMenu", _("_View")) );
	/* Translation note: this begins the sentence completed by
	 * the "By %1" string, which is the child menu 
	 * item.  If this does not make sense in your locale, translate 
	 * "Arrange Items" as something like "Sort" and translate "By %1" 
	 * as just "%1" */
	actiongroup_->add ( Gtk::Action::create("ViewMenuSort", _("Arran_ge Items")) );
	actiongroup_->add( Gtk::Action::create(
		"PasteBibtex", Gtk::Stock::PASTE, _("_Paste BibTeX"),
		_("Import references from BibTeX on the clipboard")),
		sigc::bind (sigc::mem_fun(*this, &RefWindow::onPasteBibtex), GDK_SELECTION_CLIPBOARD));
	actiongroup_->add( Gtk::Action::create(
		"CopyCite", Gtk::Stock::COPY, _("_Copy LaTeX citation"),
		_("Copy currently selected keys to the clipboard as a LaTeX citation")),
  	sigc::mem_fun(*this, &RefWindow::onCopyCite));
	actiongroup_->add( Gtk::Action::create("Preferences",
		Gtk::Stock::PREFERENCES),
  	sigc::mem_fun(*this, &RefWindow::onPreferences));

	Gtk::RadioButtonGroup group;
	actiongroup_->add( Gtk::RadioAction::create(group, "UseListView",
		_("Use _List View")));
	actiongroup_->add( Gtk::RadioAction::create(group, "UseIconView",
		_("Use _Icon View")));
	actiongroup_->add( Gtk::ToggleAction::create("ShowTagPane",
		_("_Show Tag Pane")), Gtk::AccelKey ("<control><shift>t"));
	actiongroup_->add( Gtk::ToggleAction::create("ShowNotesPane",
		_("Show _Notes Pane")), Gtk::AccelKey ("<control><shift>n"));

	actiongroup_->add ( Gtk::Action::create("TagMenu", _("_Tags")) );
	actiongroup_->add( Gtk::Action::create(
		"CreateTag", Gtk::Stock::NEW, _("_Create Tag...")), Gtk::AccelKey ("<control>t"),
		sigc::mem_fun(*this, &RefWindow::onCreateTag));
	actiongroup_->add( Gtk::Action::create(
		"CreateAndAttachTag", Gtk::Stock::NEW, _("_Attach New Tag...")), Gtk::AccelKey ("<control>t"),
		sigc::mem_fun(*this, &RefWindow::onCreateAndAttachTag));
	actiongroup_->add( Gtk::Action::create(
		"DeleteTag", Gtk::Stock::DELETE, _("_Delete Tag")),
		sigc::mem_fun(*this, &RefWindow::onDeleteTag));
	actiongroup_->add( Gtk::Action::create(
		"RenameTag", Gtk::Stock::EDIT, _("_Rename Tag")),
		sigc::mem_fun(*this, &RefWindow::onRenameTag));

	actiongroup_->add ( Gtk::Action::create("DocMenu", _("_Documents")) );
	actiongroup_->add( Gtk::Action::create(
		"AddDocFile", Gtk::Stock::ADD, _("_Add File...")),
  	sigc::mem_fun(*this, &RefWindow::onAddDocFile));
	actiongroup_->add( Gtk::Action::create(
		"AddDocFolder", Gtk::Stock::ADD, _("Add _Folder...")),
  	sigc::mem_fun(*this, &RefWindow::onAddDocFolder));
 	actiongroup_->add( Gtk::Action::create(
		"AddDocUnnamed", Gtk::Stock::ADD, _("Add E_mpty Reference...")),
  	sigc::mem_fun(*this, &RefWindow::onAddDocUnnamed));
	actiongroup_->add( Gtk::Action::create(
		"AddDocDoi", Gtk::Stock::ADD, _("Add Refere_nce with ID...")),
  	sigc::mem_fun(*this, &RefWindow::onAddDocById));
	actiongroup_->add( Gtk::Action::create(
		"RemoveDoc", Gtk::Stock::REMOVE, _("_Remove")),
		Gtk::AccelKey ("<control>Delete"),
  	sigc::mem_fun(*this, &RefWindow::onRemoveDoc));
	actiongroup_->add( Gtk::Action::create(
		"OpenDoc", Gtk::Stock::OPEN, _("_Open...")), Gtk::AccelKey ("<control>a"),
  	sigc::mem_fun(*this, &RefWindow::onOpenDoc));
	actiongroup_->add( Gtk::Action::create(
		"DocProperties", Gtk::Stock::PROPERTIES), Gtk::AccelKey ("<control>e"),
  	sigc::mem_fun(*this, &RefWindow::onDocProperties));

	actiongroup_->add( Gtk::Action::create(
		    "Search", Gtk::Stock::FIND, _("Search...")), Gtk::AccelKey ("<control>e"),
		sigc::mem_fun(*this, &RefWindow::onSearch));

	actiongroup_->add ( Gtk::Action::create("TaggerMenuAction", _("_Tags")) );
	//actiongroup_->get_action("TaggerMenuAction")->set_property("hide_if_empty", false);

	actiongroup_->add( Gtk::Action::create(
		"GetMetadataDoc", Gtk::Stock::CONNECT, _("_Get Metadata")),
  	sigc::mem_fun(*this, &RefWindow::onGetMetadataDoc));
	actiongroup_->add( Gtk::Action::create(
		"DeleteDoc", Gtk::Stock::DELETE, _("_Delete File from Drive")),
		Gtk::AccelKey ("<control><shift>Delete"),
  	sigc::mem_fun(*this, &RefWindow::onDeleteDoc));
	actiongroup_->add( Gtk::Action::create(
		"RenameDoc", Gtk::Stock::EDIT, _("R_ename File from Key")),
  	sigc::mem_fun(*this, &RefWindow::onRenameDoc));

	actiongroup_->add ( Gtk::Action::create("ToolsMenu", _("_Tools")) );
	actiongroup_->add( Gtk::Action::create("ExportNotes",
		Gtk::Stock::CONVERT, _("Export Notes as HTML")),
 	sigc::mem_fun(*this, &RefWindow::onNotesExport));

	actiongroup_->add ( Gtk::Action::create("HelpMenu", _("_Help")) );
	actiongroup_->add( Gtk::Action::create(
		"Introduction", Gtk::Stock::HELP, _("Introduction")),
  	sigc::mem_fun(*this, &RefWindow::onIntroduction));
	actiongroup_->add( Gtk::Action::create(
		"About", Gtk::Stock::ABOUT),
  	sigc::mem_fun(*this, &RefWindow::onAbout));
  	
	// Just for the keyboard shortcut
	actiongroup_->add( Gtk::Action::create(
		"Find", Gtk::Stock::FIND),
		Gtk::AccelKey ("<control>f"),
		sigc::mem_fun(*this, &RefWindow::onFind));

	uimanager_ = Gtk::UIManager::create ();
	uimanager_->insert_action_group (actiongroup_);

	/* From referencer_ui.h */
	uimanager_->add_ui_from_string (referencer_ui);

	window_->add_accel_group (uimanager_->get_accel_group ());
}


void RefWindow::onEnabledPluginsPrefChanged ()
{
	std::list<Plugin*> plugins = _global_plugins->getPlugins();
	std::list<Plugin*>::iterator pit = plugins.begin();
	std::list<Plugin*>::iterator const pend = plugins.end();
	for (; pit != pend; pit++) {
		Gtk::UIManager::ui_merge_id mergeId = pluginUI_[(*pit)->getShortName()];
		if ((*pit)->isEnabled() && !mergeId) {
			Plugin::ActionList actions = (*pit)->getActions ();
			Plugin::ActionList::iterator it = actions.begin ();
			Plugin::ActionList::iterator const end = actions.end ();
			for (; it != end; ++it) {
				Glib::ustring callback = *((Glib::ustring*)(*it)->get_data("callback"));
				Glib::ustring accelerator = *((Glib::ustring*)(*it)->get_data("accelerator"));
				if (!callback.empty()) {
					/* Connect the callback and stash the connection cookie */
					sigc::connection *connection = new sigc::connection;
					*connection = (*it)->signal_activate().connect (
						sigc::bind<Glib::ustring const, Plugin*>(
						sigc::mem_fun(*this, &RefWindow::onPluginRun),
						*((Glib::ustring*)(*it)->get_data("callback")), (*pit)));
					(*it)->set_data("connection", (void*)connection);
				}
				actiongroup_->add (*it,	Gtk::AccelKey (accelerator));
			}

			Glib::ustring ui = (*pit)->getUI (); 
			try {
				pluginUI_[(*pit)->getShortName()] = uimanager_->add_ui_from_string (ui);
			} catch (Glib::MarkupError &ex) {
				DEBUG (String::ucompose("Error merging UI For plugin %1, exception %2", (*pit)->getShortName(), ex.what()));
			}
		} else if (!(*pit)->isEnabled() && mergeId) {
			/* Remove plugin actions and UI */
			uimanager_->remove_ui (mergeId);
			Plugin::ActionList actions = (*pit)->getActions ();
			Plugin::ActionList::iterator it = actions.begin ();
			Plugin::ActionList::iterator const end = actions.end ();
			for (; it != end; ++it) {
				sigc::connection *connection = (sigc::connection*)((*it)->get_data ("connection"));
				connection->disconnect ();
				actiongroup_->remove (*it);
			}
			pluginUI_.erase ((*pit)->getShortName());
		}
	}

	/* Set up sensitivities */
	docSelectionChanged ();
}

void RefWindow::clearTagList ()
{
	tagstore_->clear();
}


void RefWindow::updateTagSizes ()
{
	std::map <int, int> tagusecounts;

	DocumentList::Container &docrefs = library_->doclist_->getDocs ();
	int const doccount = docrefs.size ();
	DocumentList::Container::iterator docit = docrefs.begin();
	DocumentList::Container::iterator const docend = docrefs.end();
	for (; docit != docend; docit++) {
		std::vector<int>& tags = (*docit).getTags ();
		std::vector<int>::iterator tagit = tags.begin ();
		std::vector<int>::iterator const tagend = tags.end ();
		for (; tagit != tagend; ++tagit)
			tagusecounts[*tagit]++;
	}


	

	/* What was I smoking when I wrote TagList?  Mung it into a more
	 * useful data structure. */
	typedef std::map<Glib::ustring, std::pair<int, int> > SensibleMap;
	std::map<Glib::ustring, std::pair<int, int> > sensibleTags;
	TagList::TagMap allTags = library_->taglist_->getTags();
	TagList::TagMap::iterator sensibleIter = allTags.begin();
	TagList::TagMap::iterator const sensibleEnd = allTags.end();
	for (; sensibleIter != sensibleEnd; ++sensibleIter) {
		sensibleTags[(*sensibleIter).second.name_] =
			std::pair<int, int> (
				(*sensibleIter).second.uid_,
				tagusecounts[(*sensibleIter).second.uid_]);
	}

	Gtk::TreeModel::iterator tagIter = tagstore_->children().begin();
	Gtk::TreeModel::iterator const tagEnd = tagstore_->children().end();
	for (; tagIter != tagEnd; ++tagIter) {
		int uid = (*tagIter)[taguidcol_];
		if (tagusecounts.find(uid) == tagusecounts.end())
			continue;

		int useCount = tagusecounts[(*tagIter)[taguidcol_]];



		float factor = 0.0;
		if (useCount > 0 && doccount > 0) {
			//factor = 0.75 + (logf((float)useCount / (float)doccount + 0.1) - logf(0.1)) * 0.4;
			factor = (float)useCount / (float)doccount;
		}

		int size = (int)((float)(window_->get_style ()->get_font().get_size ()) * (0.95f + (factor * 0.1f)));


		Pango::FontDescription font;
		if (factor < 0.1) {
			font.set_weight (Pango::WEIGHT_NORMAL);
		} else if (factor < 0.5) {
			font.set_weight (Pango::WEIGHT_SEMIBOLD);
		} else {
			font.set_weight (Pango::WEIGHT_SEMIBOLD);
		}
		font.set_size(size);

		(*tagIter)[tagfontcol_] = font;
	}
}


void RefWindow::populateTagList ()
{
	Gtk::TreeSelection::ListHandle_Path paths =
			tagselection_->get_selected_rows ();
	Gtk::TreePath initialpath;
	if (paths.size() > 0)
		initialpath = *paths.begin ();

	bool const ignore = ignoreTagSelectionChanged_;
	ignoreTagSelectionChanged_ = true;
	tagstore_->clear();

	Pango::FontDescription font_special;
	//font_special.set_weight (Pango::WEIGHT_LIGHT);

	Gtk::TreeModel::iterator all = tagstore_->append();
	(*all)[taguidcol_] = ALL_TAGS_UID;
	(*all)[tagnamecol_] = String::ucompose ("%1", _("All"));
	(*all)[tagfontcol_] = font_special;

	Gtk::TreeModel::iterator none = tagstore_->append();
	(*none)[taguidcol_] = NO_TAGS_UID;
	(*none)[tagnamecol_] = String::ucompose ("%1", _("Untagged"));
	(*none)[tagfontcol_] = font_special;

	Gtk::TreeModel::iterator sep = tagstore_->append();
	(*sep)[taguidcol_] = SEPARATOR_UID;

	/* Clear out tag actions and UI */
	TaggerUIMap::iterator taggerIter = taggerUI_.begin ();
	TaggerUIMap::iterator const taggerEnd  = taggerUI_.end ();
	for (; taggerIter != taggerEnd; ++taggerIter) {
		actiongroup_->remove ((*taggerIter).second.action);
		uimanager_->remove_ui ((*taggerIter).second.merge);
	}
	taggerUI_.clear ();

	/* What was I smoking when I wrote TagList?  Mung it into a more
	 * useful data structure. */
	typedef std::map<Glib::ustring, int > SensibleMap;
	SensibleMap sensibleTags;
	TagList::TagMap allTags = library_->taglist_->getTags();
	TagList::TagMap::iterator sensibleIter = allTags.begin();
	TagList::TagMap::iterator const sensibleEnd = allTags.end();
	for (; sensibleIter != sensibleEnd; ++sensibleIter)
		sensibleTags[(*sensibleIter).second.name_] = (*sensibleIter).second.uid_;

	// Populate from the sensibleTags structure
	SensibleMap::iterator tagIter = sensibleTags.begin ();
	SensibleMap::iterator const tagEnd = sensibleTags.end ();
	for (; tagIter != tagEnd; ++tagIter) {
		Glib::ustring name = (*tagIter).first;
		int uid = (*tagIter).second;

		Gtk::TreeModel::iterator item = tagstore_->append();
		(*item)[taguidcol_] = uid;
		(*item)[tagnamecol_] = name;
		(*item)[tagfontcol_] = Pango::FontDescription();

		/* Create tag actions */
		TagUI t;
		Glib::ustring actionName = String::ucompose ("tagger_%1", uid);
		Glib::RefPtr<Gtk::ToggleAction> action = Gtk::ToggleAction::create (
				actionName, name);

		action->set_visible(true);
		action->signal_toggled().connect(
			sigc::bind(
				sigc::mem_fun (*this, &RefWindow::taggerActionToggled),
				action,
				uid));

		/* Add it to the action group */
		actiongroup_->add (action);
		t.action = action;
		/* Merge it into the UI */
		Glib::ustring ui = 
			"<ui>"
			"<popup name='DocPopup'>"
			"<menu action='TaggerMenuAction' name='TaggerMenu'>"
			"  <placeholder name='TaggerMenuTags'>"
			"    <menuitem action='";
		ui += actionName;
		ui += "'/>"
			"  </placeholder>"
			"</menu>"
			"</popup>"
			"</ui>";

		t.merge = uimanager_->add_ui_from_string (ui); 

		/* Stash the info */
		taggerUI_[uid] = t;
	}


	// Restore initial selection or selected first row
	ignoreTagSelectionChanged_ = ignore;
	if (!initialpath.empty())
		tagselection_->select (initialpath);

	// If that didn't get anything, select All
	if (tagselection_->get_selected_rows ().size () == 0)
		tagselection_->select (tagstore_->children().begin());


	/* Give them tagcloud-style sizing */
	updateTagSizes ();

	/* Update tagger action checked-ness */
	docSelectionChanged ();
}


void RefWindow::printUI ()
{
	DEBUG (uimanager_->get_ui());
	DEBUG ("Actions\n");
	std::vector<Glib::RefPtr<Gtk::Action> > actions = actiongroup_->get_actions ();
	std::vector<Glib::RefPtr<Gtk::Action> >::iterator actionIt = actions.begin ();
	std::vector<Glib::RefPtr<Gtk::Action> >::iterator const actionEnd = actions.end ();
	for (; actionIt != actionEnd; ++actionIt) {
		DEBUG ((*actionIt)->get_name ());
	}
}


void RefWindow::taggerActionToggled (Glib::RefPtr<Gtk::ToggleAction> action, int taguid)
{
	if (ignoreTaggerActionToggled_)
		return;

	setDirty (true);

	std::vector<Document*> selecteddocs = docview_->getSelectedDocs ();

	bool active = action->get_active ();

	ignoreDocSelectionChanged_ = true;
	std::vector<Document*>::iterator it = selecteddocs.begin ();
	std::vector<Document*>::iterator const end = selecteddocs.end ();
	for (; it != end; it++) {
		Document *doc = (*it);
		if (active)
			doc->setTag (taguid);
		else
			doc->clearTag (taguid);

		docview_->updateDoc (doc);
	}
	ignoreDocSelectionChanged_ = false;
	docSelectionChanged ();

	// If we've untagged something it might no longer be visible
	// Or if we've added a tag to something while viewing "untagged"
	updateStatusBar ();
	
	// All tag changes influence the fonts in the tag list
	updateTagSizes ();
}


void RefWindow::tagNameEditingStarted (
        Gtk::CellEditable *,
        Glib::ustring const &pathstr)
{
	Gtk::TreePath path(pathstr);
	Gtk::ListStore::iterator iter = tagstore_->get_iter (path);

	tagoldname_ = (*iter)[tagnamecol_];
}


void RefWindow::tagNameEdited (
	Glib::ustring const &text1,
	Glib::ustring const &text2)
{
	// Text1 is the row number, text2 is the new setting
	Gtk::TreeSelection::ListHandle_Path paths =
		tagselection_->get_selected_rows ();

	if (paths.empty ()) {
		DEBUG ("Warning: RefWindow::tagNameEdited: no tag selected");
		return;
	} else if (paths.size () > 1) {
		DEBUG ("Warning: RefWindow::tagNameEdited: too many tags selected");
		return;
	}

	Gtk::TreePath path = (*paths.begin ());
	Gtk::ListStore::iterator iter = tagstore_->get_iter (path);

	// Should escape this
	Glib::ustring newname = text2;

        if (newname == tagoldname_)
                return;

	setDirty (true);

	/* Update name in tag bar */
	(*iter)[tagnamecol_] = newname;
	/* Update name in tag store */
	library_->taglist_->renameTag ((*iter)[taguidcol_], newname);

	/* Update name in tag action */
	taggerUI_[(*iter)[taguidcol_]].action->property_label() = newname;

}


bool RefWindow::tagSeparator (
        const Glib::RefPtr<Gtk::TreeModel> &model,
        const Gtk::TreeModel::iterator &iter)
{
        return ((*iter)[taguidcol_] == SEPARATOR_UID);
}


void RefWindow::tagCellRenderer (
        Gtk::CellRenderer * cell,
        const Gtk::TreeModel::iterator &iter) const
{
        Gtk::CellRendererText *render = dynamic_cast<Gtk::CellRendererText*> (cell);

        if (!render)
                return;

        if ((*iter)[taguidcol_] == ALL_TAGS_UID || (*iter)[taguidcol_] == NO_TAGS_UID)
                render->property_editable () = false;
        else
                render->property_editable () = true;
}


void RefWindow::tagSelectionChanged ()
{
	if (ignoreTagSelectionChanged_)
		return;

	Gtk::TreeSelection::ListHandle_Path paths =
		tagselection_->get_selected_rows ();

	filtertags_.clear();

	bool specialselected = false;
	bool anythingselected = false;

	if (paths.empty()) {
		specialselected = true;
		anythingselected = true;
		filtertags_.push_back(ALL_TAGS_UID);
	} else {
		Gtk::TreeSelection::ListHandle_Path::iterator it = paths.begin ();
		Gtk::TreeSelection::ListHandle_Path::iterator const end = paths.end ();
		for (; it != end; it++) {
			anythingselected = true;
			Gtk::TreePath path = (*it);
			Gtk::ListStore::iterator iter = tagstore_->get_iter (path);
			filtertags_.push_back((*iter)[taguidcol_]);
			if ((*iter)[taguidcol_] == ALL_TAGS_UID
			    || (*iter)[taguidcol_] == NO_TAGS_UID) {
				specialselected = true;
			}
		}
	}

	actiongroup_->get_action("DeleteTag")->set_sensitive (
		anythingselected && !specialselected);
	actiongroup_->get_action("RenameTag")->set_sensitive (
		paths.size() == 1 && !specialselected);

	docview_->updateVisible ();
	updateStatusBar ();
}


void RefWindow::updateStatusBar ()
{
	int selectcount = docview_->getSelectedDocCount ();

	bool const somethingselected = selectcount > 0;

	// Update the statusbar text
	int visibledocs = docview_->getVisibleDocCount ();
	Glib::ustring statustext;
	if (somethingselected) {
		statustext = String::ucompose (
			_("%1 documents (%2 selected)"),
			visibledocs, selectcount);
	} else {
		statustext = String::ucompose (
			_("%1 documents"),
			visibledocs);
	}
	statusbar_->push (statustext, 0);

}


void RefWindow::onNotesClose () 
{
	_global_prefs->setShowNotesPane(false);
}


void RefWindow::updateNotesPane ()
{
	int selectcount = docview_->getSelectedDocCount ();

	// Update the notes for the old selected file first
	static Document *doc = NULL;
	if ( doc && notesbuffer_->get_modified () && notesview_->get_editable() ) {
		setDirty(true);
		doc->setNotes( notesbuffer_->get_text() );
	}
	
	bool enabled = false;

	if (selectcount == 1) {
		// Now show the notes for the new one
		doc = docview_->getSelectedDoc ();
		Glib::ustring labeltext;
		if (doc->hasField("title"))
			labeltext = doc->getField("title");
		else
			labeltext = doc->getKey ();

		noteslabel_->set_markup_with_mnemonic(
			String::ucompose (_("_Notes: <i>%1</i>"), labeltext));
		enabled = true;
	} else if (selectcount > 1) {
		noteslabel_->set_label(_("Multiple documents selected"));
	} else {
		noteslabel_->set_label(_("Select a document to view and edit notes"));
	}

	if (enabled) {
		notesview_->set_cursor_visible(true);
		notesview_->set_editable(true);
		notesbufferignore_ = true;
		notesbuffer_->set_text( doc->getNotes() );
		notesbufferignore_ = false;
		notesbuffer_->set_modified(false);
	} else {
		notesview_->set_cursor_visible(false);
		notesview_->set_editable(false);
		notesbufferignore_ = true;
		notesbuffer_->set_text("");
		notesbufferignore_ = false;
	}

}


void RefWindow::onNotesChanged ()
{
	if (!notesbufferignore_)
		setDirty (true);
}


void RefWindow::tagClicked (GdkEventButton* event)
{
  if((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
		Gtk::Menu *popupmenu =
			(Gtk::Menu*)uimanager_->get_widget("/TagPopup");
		popupmenu->popup (event->button, event->time);
	}
}


void RefWindow::onQuit ()
{
	if (ensureSaved ())
		Gnome::Main::quit ();
}


bool RefWindow::onDelete (GdkEventAny *ev)
{
	if (ensureSaved ())
		return false;
	else
		return true;
}


// Prompts the user to save if necessary, and returns whether
// it is save for the caller to proceed (false if the user
// says to cancel, or saving failed)
bool RefWindow::ensureSaved ()
{
	if (getDirty ()) {
		Gtk::MessageDialog dialog (
			String::ucompose ("<b><big>%1</big></b>"
			, _("Save changes to library before closing?")),
			true,
			Gtk::MESSAGE_WARNING,
			Gtk::BUTTONS_NONE,
			true);

		dialog.add_button (_("Close _without Saving"), Gtk::RESPONSE_CLOSE);
		dialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		dialog.add_button (Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);

		int const result = dialog.run ();

		if (result == Gtk::RESPONSE_CLOSE) {
			return true;
		} else if (result == Gtk::RESPONSE_ACCEPT) {
			if (openedlib_.empty ()) {
				onSaveAsLibrary ();
				if (openedlib_.empty ()) {
					// The user cancelled
					return false;
				}
			} else {
				if (!library_->save (openedlib_)) {
					// Don't lose data
					return false;
				}
			}
			return true;
		} else /*if (result == Gtk::RESPONSE_CANCEL)*/ {
			return false;
		}
	} else {
		return true;
	}
}


/**
 * Helper for onCreateTag and onCreateAndAttachTag
 */
int RefWindow::createTag ()
{
	Glib::ustring newname = (_("Type a tag"));

	Glib::ustring message = String::ucompose(
		"<b><big>%1</big></b>\n\n%2",
		_("Create tag"),
		_("Short memorable tag names help you organise your documents"));
	Gtk::MessageDialog dialog(message, true, Gtk::MESSAGE_QUESTION,
		Gtk::BUTTONS_NONE, true);
	
	dialog.add_button (Gtk::Stock::CANCEL, 0);
	dialog.add_button (Gtk::Stock::OK, 1);
	dialog.set_default_response (1);
	
	Gtk::Entry nameentry;
	Gtk::HBox hbox;
	hbox.set_spacing (6);
	Gtk::Label label (String::ucompose ("%1:", _("Name")));
	hbox.pack_start (label, false, false, 0);
	hbox.pack_start (nameentry, true, true, 0);
	nameentry.set_activates_default (true);
	dialog.get_vbox()->pack_start (hbox, false, false, 0);
	hbox.show_all ();
	
	bool invalid = true;
	int newtag = -1;
	
	while (invalid && dialog.run()) {
		Glib::ustring newname = nameentry.get_text ();
		if (newname.empty()) {
			invalid = true;
		} else {
			invalid = false;
			newtag = library_->taglist_->newTag (newname);
			populateTagList();
		}

	}

	return newtag;
}

void RefWindow::onCreateTag  ()
{
	createTag ();
	setDirty (true);
}


void RefWindow::onCreateAndAttachTag ()
{
	/* Create a tag */
	int newtag = createTag ();
	if (newtag < 0)
		return;

	/* Apply newtag to all selected documents */
	std::vector<Document*> docs = docview_->getSelectedDocs ();
	std::vector<Document*>::iterator docIter = docs.begin ();
	std::vector<Document*>::iterator const docEnd = docs.end ();
	for (; docIter != docEnd; ++docIter) {
		(*docIter)->setTag (newtag);
	}

	updateTagSizes ();

	setDirty (true);
}


void RefWindow::onDeleteTag ()
{
	Gtk::TreeSelection::ListHandle_Path paths =
		tagselection_->get_selected_rows ();

	if (paths.empty()) {
		DEBUG ("Warning: RefWindow::onDeleteTag: nothing selected");
		return;
	}

	std::vector <int> uidstodelete;

	Gtk::TreeSelection::ListHandle_Path::iterator it = paths.begin ();
	Gtk::TreeSelection::ListHandle_Path::iterator const end = paths.end ();
	for (; it != end; it++) {
		Gtk::TreePath path = (*it);
		Gtk::ListStore::iterator iter = tagstore_->get_iter (path);
		if ((*iter)[taguidcol_] == ALL_TAGS_UID) {
			DEBUG ("Warning: RefWindow::onDeleteTag:"
				" someone tried to delete 'All'\n");
			continue;
		}

		Glib::ustring message = String::ucompose (
			_("Are you sure you want to delete \"%1\"?"),
			(Glib::ustring)(*iter)[tagnamecol_]);

		Gtk::MessageDialog confirmdialog (
			message, false, Gtk::MESSAGE_QUESTION,
			Gtk::BUTTONS_NONE, true);

		confirmdialog.set_secondary_text (_("When a tag is deleted it is also "
			"permanently removed from all documents it is currently "
			"associated with."));

		confirmdialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		confirmdialog.add_button (Gtk::Stock::DELETE, Gtk::RESPONSE_ACCEPT);
		confirmdialog.set_default_response (Gtk::RESPONSE_CANCEL);

		if (confirmdialog.run () == Gtk::RESPONSE_ACCEPT) {
			DEBUG (String::ucompose ("going to delete %1", (*iter)[taguidcol_]));
			uidstodelete.push_back ((*iter)[taguidcol_]);
		}
	}

	std::vector<int>::iterator uidit = uidstodelete.begin ();
	std::vector<int>::iterator const uidend = uidstodelete.end ();
	for (; uidit != uidend; ++uidit) {
		DEBUG (String::ucompose ("really deleting %1", *uidit));
		// Take it off any docs
		library_->doclist_->clearTag(*uidit);
		// Remove it from the tag list
		library_->taglist_->deleteTag (*uidit);
	}

	if (uidstodelete.size() > 0) {
		setDirty (true);
		populateTagList ();
	}
}


void RefWindow::onRenameTag ()
{
	Gtk::TreeSelection::ListHandle_Path paths =
		tagselection_->get_selected_rows ();

	if (paths.empty ()) {
		DEBUG ("no tag selected");
		return;
	} else if (paths.size () > 1) {
		DEBUG ("too many tags selected");
		return;
	}

	Gtk::TreePath path = (*paths.begin ());

	tagview_->set_cursor (path, *tagview_->get_column (0), true);
}


void RefWindow::onExportBibtex ()
{
	Gtk::FileChooserDialog chooser (
		_("Export BibTeX"),
		Gtk::FILE_CHOOSER_ACTION_SAVE);
	chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_REJECT);
	chooser.add_button (Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	chooser.set_default_response (Gtk::RESPONSE_ACCEPT);
	chooser.set_do_overwrite_confirmation (true);

	Gtk::FileFilter bibtexfiles;
	bibtexfiles.add_pattern ("*.[bB][iI][bB]");
	bibtexfiles.set_name (_("BibTeX Files"));
	chooser.add_filter (bibtexfiles);

	Gtk::FileFilter allfiles;
	allfiles.add_pattern ("*");
	allfiles.set_name (_("All Files"));
	chooser.add_filter (allfiles);

	Gtk::VBox extrabox;
	extrabox.set_spacing (6);
	chooser.set_extra_widget (extrabox);

	Gtk::HBox selectionbox;
	selectionbox.set_spacing (6);
	extrabox.pack_start (selectionbox, false, false, 0);

	Gtk::Label label (_("Selection:"));
	selectionbox.pack_start (label, false, false, 0);
	Gtk::ComboBoxText combo;
	combo.append_text (_("All Documents"));
	combo.append_text (_("Selected Documents"));
	combo.set_active (0);
	selectionbox.pack_start (combo, true, true, 0);
	combo.set_sensitive (docview_->getSelectedDocCount ());

	// Any options here should be replicated in onManageBibtex
	Gtk::CheckButton bracescheck (_("Protect capitalization (surround values with {})"));
	extrabox.pack_start (bracescheck, false, false, 0);
	Gtk::CheckButton utf8check (_("Use Unicode (UTF-8) encoding"));
	extrabox.pack_start (utf8check, false, false, 0);

	extrabox.show_all ();

	// Browsing to remote hosts not working for some reason
	//chooser.set_local_only (false);

	if (!exportfolder_.empty())
		chooser.set_current_folder (exportfolder_);

	if (chooser.run() == Gtk::RESPONSE_ACCEPT) {
		bool const usebraces = bracescheck.get_active ();
		bool const useutf8 = utf8check.get_active ();
		bool const selectedonly = combo.get_active_row_number () == 1;
		exportfolder_ = Glib::path_get_dirname(chooser.get_filename());
		Glib::ustring bibfilename = chooser.get_uri();
		chooser.hide ();
		// Really we shouldn't add the extension if the user has chosen an
		// existing file rather than typing in a name themselves.
		bibfilename = Utility::ensureExtension (bibfilename, "bib");

		std::vector<Document*> docs;
		if (selectedonly) {
			docs = docview_->getSelectedDocs ();
		} else {
			DocumentList::Container &docrefs = library_->doclist_->getDocs ();
			DocumentList::Container::iterator it = docrefs.begin();
			DocumentList::Container::iterator const end = docrefs.end();
			for (; it != end; it++) {
				docs.push_back(&(*it));
			}
		}

		library_->writeBibtex (bibfilename, docs, usebraces, useutf8);
	}
}

void RefWindow::onNotesExport ()
{
	if ( notesbuffer_->get_modified() )
		updateNotesPane ();
		
	Gtk::FileChooserDialog chooser (
		_("Export Notes"),
		Gtk::FILE_CHOOSER_ACTION_SAVE);
	chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_REJECT);
	chooser.add_button (Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	chooser.set_default_response (Gtk::RESPONSE_ACCEPT);
	chooser.set_do_overwrite_confirmation (true);

	Gtk::FileFilter htmlfiles;
	htmlfiles.add_pattern ("*.[Hh][Tt][Mm][Ll*]");
	htmlfiles.set_name (_("HTML Files"));
	chooser.add_filter (htmlfiles);

	Gtk::FileFilter allfiles;
	allfiles.add_pattern ("*");
	allfiles.set_name (_("All Files"));
	chooser.add_filter (allfiles);

	Gtk::VBox extrabox;
	extrabox.set_spacing (6);
	chooser.set_extra_widget (extrabox);

	Gtk::HBox selectionbox;
	selectionbox.set_spacing (6);
	extrabox.pack_start (selectionbox, false, false, 0);

	Gtk::Label label (_("Selection:"));
	selectionbox.pack_start (label, false, false, 0);
	Gtk::ComboBoxText combo;
	combo.append_text (_("All Documents"));
	combo.append_text (_("Selected Documents"));
	combo.set_active (0);
	selectionbox.pack_start (combo, true, true, 0);
	combo.set_sensitive (docview_->getSelectedDocCount ());

	extrabox.show_all ();

	// Browsing to remote hosts not working for some reason
	//chooser.set_local_only (false);

	if (!exportfolder_.empty())
		chooser.set_current_folder (exportfolder_);

	if (chooser.run() == Gtk::RESPONSE_ACCEPT) {
		bool const selectedonly = combo.get_active_row_number () == 1;
		exportfolder_ = Glib::path_get_dirname(chooser.get_filename());
		chooser.hide ();

		std::vector<Document*> docs;
		if (selectedonly) {
			docs = docview_->getSelectedDocs ();
		} else {
			DocumentList::Container &docrefs = library_->doclist_->getDocs ();
			DocumentList::Container::iterator it = docrefs.begin();
			DocumentList::Container::iterator const end = docrefs.end();
			for (; it != end; it++) {
				// Don't export documents without notes
				if ( !(*it).getNotes().empty() )
					docs.push_back(&(*it));
			}
		}
		
		/* Extremely basic HTML writer, maybe move into a separate function
		   if it gets more complicated */
		std::ofstream notesfile(chooser.get_filename().c_str());
		if (!notesfile) {
			DEBUG (String::ucompose ("Error opening '%1'", chooser.get_filename()));
			notesfile.close ();
			return;
		}

		Glib::ustring libname;
		if (openedlib_.empty ()) {
			libname = _("Unnamed Library");
		} else {
			libname = Gnome::Vfs::unescape_string_for_display (Glib::path_get_basename (openedlib_));
			Glib::ustring::size_type pos = libname.find (".reflib");
			if (pos != Glib::ustring::npos) {
				libname = libname.substr (0, pos);
			}
		}

		// Just a plain HTML page
		// for now, styles and such could be added later
		notesfile << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\n";
		notesfile << "\"http://www.w3.org/TR/html4/strict.dtd\">" << std::endl;
		notesfile << "<html>\n<head>\n<meta http-equiv=\"Content-Type\"";
		notesfile << "content=\"text/html; charset=UTF-8\">" << std::endl;
		notesfile << "<title>Notes on "<< libname << "</title>\n</head>\n<body>" << std::endl;
		std::vector<Document*>::const_iterator it = docs.begin ();
		std::vector<Document*>::const_iterator const end = docs.end ();
		for (; it != end; ++it) {
			notesfile << "<h1>" << (*it)->getField("title") << "</h1>" << std::endl;
			notesfile << "<p>" << (*it)->getNotes() << "</p>" << std::endl;
		}
		notesfile << "</body>\n</html>";
		notesfile.close();
	}
}


void RefWindow::manageBrowseDialog (Gtk::Entry *entry)
{
	Glib::ustring filename = Glib::filename_from_utf8 (entry->get_text ());

	Gtk::FileChooserDialog dialog (_("Browse"), Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button (Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
	dialog.set_do_overwrite_confirmation ();

	Gtk::FileFilter bibtexfiles;
	bibtexfiles.add_pattern ("*.[bB][iI][bB]");
	bibtexfiles.set_name (_("BibTeX Files"));
	dialog.add_filter (bibtexfiles);

	Gtk::FileFilter allfiles;
	allfiles.add_pattern ("*");
	allfiles.set_name (_("All Files"));
	dialog.add_filter (allfiles);

	if (dialog.run() == Gtk::RESPONSE_OK) {
	//Gnome::Vfs::get_uri_from_local_path (urientry.get_text ()
		Glib::ustring relpath = Utility::relPath (openedlib_, dialog.get_uri ());
		DEBUG (String::ucompose ("manage path: %1", dialog.get_uri()));
		DEBUG (String::ucompose ("library path: %1", openedlib_));
		DEBUG (String::ucompose ("relative path: %1", relpath));
		// Effect is that we are always setting a UTF-8 filename
		// NOT a URI.
		if (!relpath.empty ()) {
			entry->set_text (Gnome::Vfs::unescape_string (relpath));
		} else {
			entry->set_text (Glib::filename_to_utf8 (dialog.get_filename()));
		}
	}
}


void RefWindow::onLibraryFolder ()
{
	library_->libraryFolderDialog ();
}


void RefWindow::onManageBibtex ()
{
	Gtk::Dialog dialog (_("Manage BibTeX File"), true, false);

	dialog.add_button (Gtk::Stock::CLOSE, Gtk::RESPONSE_CANCEL);

	Gtk::VBox *vbox = dialog.get_vbox ();
	Gtk::VBox mybox;
	vbox->pack_start (mybox);
	vbox = &mybox;
	vbox->set_border_width (6);
	vbox->set_spacing (6);

	Gtk::Label explanation;
	explanation.set_markup (
	String::ucompose (
		"<b><big>%1</big></b>\n\n%2",
		_("Manage BibTeX File"),
		_("If you choose a file here, it will be overwritten "
		"whenever this library is saved."))
		);
	vbox->pack_start (explanation);

	Gtk::HBox hbox;
	hbox.set_spacing (6);
	Gtk::Label label (_("BibTeX file:"));
	hbox.pack_start (label, false, false, 0);
	Gtk::Entry urientry;
	hbox.pack_start (urientry);

	DEBUG (String::ucompose ("Got bibtextarget = %1", library_->getBibtexTarget()));
	Glib::ustring bibfilename =
		Gnome::Vfs::get_local_path_from_uri (library_->getBibtexTarget ());
	DEBUG (String::ucompose ("Got absolute path = %1", bibfilename));
	// Did we fail?  (if so then it's a relative path)
	if (bibfilename.empty ()) {
		bibfilename = Gnome::Vfs::unescape_string (library_->getBibtexTarget ());
		DEBUG (String::ucompose ("Got relative path = %1", bibfilename));
	}
	urientry.set_text (bibfilename);

	vbox->pack_start (hbox);

	Gtk::HBox hbox2;
	hbox2.set_spacing (6);

	Gtk::Button browsebutton (_("_Browse..."), true);
	Gtk::Image *openicon = Gtk::manage (
	new Gtk::Image (Gtk::Stock::OPEN, Gtk::ICON_SIZE_BUTTON));
	browsebutton.set_image (*openicon);
	browsebutton.signal_clicked ().connect (
		sigc::bind(sigc::mem_fun (*this, &RefWindow::manageBrowseDialog), &urientry));

	browsebutton.set_use_underline (true);
	hbox2.pack_end (browsebutton, false, false, 0);
	Gtk::Button clearbutton (Gtk::Stock::CLEAR);
	hbox2.pack_end (clearbutton, false, false, 0);
	clearbutton.signal_clicked ().connect (
		sigc::bind (sigc::mem_fun (urientry, &Gtk::Entry::set_text), Glib::ustring()));
	vbox->pack_start (hbox2);

	// Any options here should be replicated in onExportBibtex
	Gtk::CheckButton bracescheck (_("Protect capitalization (surround values with {})"));
	vbox->pack_start (bracescheck);
	bracescheck.set_active (library_->getBibtexBraces ());
	Gtk::CheckButton utf8check (_("Use Unicode (UTF-8) encoding"));
	vbox->pack_start (utf8check, false, false, 0);
	utf8check.set_active (library_->getBibtexUTF8 ());

	vbox->show_all ();

	dialog.run ();

	// Take a UTF-8 filename (relative or abs) and convert it to URI form
	Glib::ustring newfilename = urientry.get_text ();

	Glib::ustring newtarget;
	if (Glib::path_is_absolute (newfilename)) {
		newtarget = Gnome::Vfs::get_uri_from_local_path (newfilename);
	} else {
		newtarget = Gnome::Vfs::escape_string (newfilename);
	}
	DEBUG (String::ucompose ("newtarget: %1", newtarget));


	bool const newbraces = bracescheck.get_active ();

	if (newtarget != library_->getBibtexTarget ()
	    || newbraces != library_->getBibtexBraces () ) {
		setDirty (true);
	}

	library_->manageBibtex (
		newtarget,
		bracescheck.get_active (),
		utf8check.get_active ());
}


void RefWindow::onNewLibrary ()
{
	if (ensureSaved ()) {
		setDirty (false);

		setOpenedLib ("");
		clearTagList ();
		docview_->clear ();

		library_->clear ();

		populateTagList ();
		docview_->populateDocStore ();
		updateStatusBar ();
	}
}


void RefWindow::onOpenLibrary ()
{
	Gtk::FileChooserDialog chooser(
		_("Open Library"),
		Gtk::FILE_CHOOSER_ACTION_OPEN);

	// remote not working?
	//chooser.set_local_only (false);

	if (!ensureSaved ())
		return;

	if (!libraryfolder_.empty())
		chooser.set_current_folder (libraryfolder_);
	chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_REJECT);
	chooser.add_button (Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);
	chooser.set_default_response (Gtk::RESPONSE_ACCEPT);

	Gtk::FileFilter reflibfiles;
	reflibfiles.add_pattern ("*.reflib");
	reflibfiles.set_name (_("Referencer Libraries"));
	chooser.add_filter (reflibfiles);

	Gtk::FileFilter allfiles;
	allfiles.add_pattern ("*");
	allfiles.set_name (_("All Files"));
	chooser.add_filter (allfiles);

	if (chooser.run () == Gtk::RESPONSE_ACCEPT) {
		libraryfolder_ = Glib::path_get_dirname(chooser.get_filename());
		Glib::ustring libfile = chooser.get_uri ();
		chooser.hide ();

		setDirty (false);

		DEBUG (String::ucompose("Calling library_->load on %1", libfile));
		if (library_->load (libfile)) {
			ignoreDocSelectionChanged_ = true;
			ignoreTagSelectionChanged_ = true;
			docview_->clear ();
			populateTagList ();
			docview_->populateDocStore ();
			ignoreDocSelectionChanged_ = false;
			ignoreTagSelectionChanged_ = false;

			updateStatusBar ();
			setOpenedLib (libfile);
		} else {
			//library_->load would have shown an exception error dialog
		}

	}
}


void RefWindow::onSaveLibrary ()
{
	if (openedlib_.empty()) {
		onSaveAsLibrary ();
	} else {
		bool saveReturn;
		updateNotesPane ();
		try {
			saveReturn = library_->save (openedlib_);
		} catch (const Glib::Exception &ex) {
			Utility::exceptionDialog (&ex, "Saving");
		}

		if (saveReturn)
			setDirty (false);
	}
}


void RefWindow::onSaveAsLibrary ()
{
	Gtk::FileChooserDialog chooser (
		_("Save Library As"),
		Gtk::FILE_CHOOSER_ACTION_SAVE);
	chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_REJECT);
	chooser.add_button (Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	chooser.set_default_response (Gtk::RESPONSE_ACCEPT);
	chooser.set_do_overwrite_confirmation (true);

	Gtk::FileFilter reflibfiles;
	reflibfiles.add_pattern ("*.reflib");
	reflibfiles.set_name (_("Referencer Libraries"));
	chooser.add_filter (reflibfiles);

	Gtk::FileFilter allfiles;
	allfiles.add_pattern ("*");
	allfiles.set_name (_("All Files"));
	chooser.add_filter (allfiles);

	// Browsing to remote hosts not working for some reason
	//chooser.set_local_only (false);

	if (!libraryfolder_.empty())
		chooser.set_current_folder (libraryfolder_);

	if (chooser.run() == Gtk::RESPONSE_ACCEPT) {
		libraryfolder_ = Glib::path_get_dirname(chooser.get_filename());
		Glib::ustring libfilename = chooser.get_uri();
		chooser.hide ();
		// Really we shouldn't add the extension if the user has chosen an
		// existing file rather than typing in a name themselves.
		libfilename = Utility::ensureExtension (libfilename, "reflib");

		bool saveReturn;
		try {
			saveReturn = library_->save (libfilename);
		} catch (const Glib::Exception &ex) {
			Utility::exceptionDialog (&ex, "Saving As");
		}

		if (saveReturn) {
			setDirty (false);
			setOpenedLib (libfilename);
		} else {
			// library_->save would have shown exception dialogs
		}
	}
}


void RefWindow::onAbout ()
{
	Gtk::AboutDialog dialog;
	std::vector<Glib::ustring> authors;
	Glib::ustring me = "John Spray";
	authors.push_back(me);
	dialog.set_authors (authors);
	dialog.set_name (DISPLAY_PROGRAM);
	dialog.set_version (VERSION);
	dialog.set_comments ("A document organiser and bibliography manager");
	dialog.set_copyright ("Copyright © 2008 John Spray");
	dialog.set_website ("http://icculus.org/referencer/");
	// Translators: your name here!
	dialog.set_translator_credits (_("translator-credits"));
	dialog.set_logo (
		Gdk::Pixbuf::create_from_file (
			Utility::findDataFile ("referencer.svg"),
			128, 128));
	dialog.run ();
}


void RefWindow::onIntroduction ()
{
	Glib::ustring filename = Utility::findDataFile ("introduction.html");

	if (filename.empty ())
		return;

	try {
		Gnome::Vfs::url_show (Gnome::Vfs::get_uri_from_local_path (filename));
	} catch (Gnome::Vfs::exception &ex) {
		Utility::exceptionDialog (&ex,
			String::ucompose(_("Showing '%1'"), filename));
	}
}


void RefWindow::onAddDocFilesCancel (Gtk::Button *button, Gtk::ProgressBar *progress)
{
	cancelAddDocFiles_ = true;

	progress->set_text (_("Cancelling"));
	button->set_sensitive (false);
}


void RefWindow::TaggerDialog::onCreateTag ()
{
	int uid = parent_->createTag ();

	selections_[uid] = true;

	populate ();
}


void RefWindow::TaggerDialog::populate ()
{
	model_->clear ();

	TagList::TagMap allTags = taglist_->getTags();
	TagList::TagMap::iterator tagIter = allTags.begin();
	TagList::TagMap::iterator const tagEnd = allTags.end();
	for (; tagIter != tagEnd; ++tagIter) {
		Gtk::ListStore::iterator row = model_->append();
		(*row)[nameColumn_] = tagIter->second.name_;
		(*row)[uidColumn_] = tagIter->second.uid_;
		(*row)[selectedColumn_] = selections_[tagIter->second.uid_];
	}
}


void RefWindow::TaggerDialog::toggled (Glib::ustring const &path)
{
	Gtk::ListStore::iterator row = model_->get_iter (path);
	int uid = (*row)[uidColumn_]; 
	bool selected = !(*row)[selectedColumn_];
	(*row)[selectedColumn_] = selected;
	selections_[uid] = selected;
}

RefWindow::TaggerDialog::TaggerDialog (RefWindow *window, TagList *taglist)
	: parent_(window), taglist_ (taglist)
{
	/* Show a dialog containing a treeview of checkboxes for
	 * tags, and a button for creating tags */
	set_title (_("Tag Added Files"));
	set_has_separator (false);

	Gtk::VBox *vbox = get_vbox ();
	vbox->set_spacing (12);

	/* Map of selected tags */
	std::map<int, bool> selections;

	/* Create the model */
	Gtk::TreeModelColumnRecord columns;
	columns.add (nameColumn_);
	columns.add (uidColumn_);
	columns.add (selectedColumn_);
	model_ = Gtk::ListStore::create(columns);
	model_->set_sort_column (nameColumn_, Gtk::SORT_ASCENDING);

	/* Create the view */
	view_ = Gtk::manage (new Gtk::TreeView (model_));
	view_->set_headers_visible (false);
	scroll_ = Gtk::manage (new Gtk::ScrolledWindow ());
	scroll_->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	scroll_->set_size_request (-1, 200);
	scroll_->set_shadow_type (Gtk::SHADOW_IN);
	scroll_->add (*view_);
	vbox->pack_start (*scroll_);

	toggle_ = Gtk::manage (new Gtk::CellRendererToggle);
	toggle_->property_activatable() = true;
	toggle_->signal_toggled().connect (
			sigc::mem_fun (*this, &RefWindow::TaggerDialog::toggled));
	Gtk::TreeViewColumn *selected = Gtk::manage (new Gtk::TreeViewColumn ("", *toggle_));
	selected->add_attribute (toggle_->property_active(), selectedColumn_);

	view_->append_column (*selected);
	Gtk::TreeViewColumn *name_column = (Gtk::manage (new Gtk::TreeViewColumn ("", nameColumn_)));
	((Gtk::CellRendererText*)(std::vector<Gtk::CellRenderer*>(name_column->get_cell_renderers())[0]))->property_ellipsize() = Pango::ELLIPSIZE_MIDDLE;
	view_->append_column (*name_column);

	/* "Create tag" button */
	Gtk::Button *button = Gtk::manage (new Gtk::Button (_("C_reate Tag..."), true));
	button->set_image (*(Gtk::manage(new Gtk::Image(Gtk::Stock::NEW, Gtk::ICON_SIZE_BUTTON))));
	button->signal_clicked().connect(
			sigc::mem_fun (*this, &RefWindow::TaggerDialog::onCreateTag));
	vbox->pack_start (*button, Gtk::PACK_SHRINK);


	/* Response buttons */
	add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	add_button (Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
}

std::vector<int> RefWindow::TaggerDialog::tagPrompt ()
{
	/* Populate the model */
	populate ();

	show_all ();
	get_vbox()->set_border_width(12);
	int response = run ();
	hide ();

	std::vector<int> tags;
	if (response == Gtk::RESPONSE_ACCEPT) {
		/* append to tags for each selected tag */
		std::map<int, bool>::iterator selIter = selections_.begin ();
		std::map<int, bool>::iterator const selEnd = selections_.end ();
		for (; selIter != selEnd; ++selIter) {
			if (selIter->second == true) {
				tags.push_back (selIter->first);
			}
		}
	}

	return tags;
}


void RefWindow::onAddDocFilesTag (std::vector<Document*> &docs)
{
	TaggerDialog dialog (this, library_->taglist_);
	std::vector<int> tags = dialog.tagPrompt ();

	std::vector<int>::iterator tagIter = tags.begin ();
	std::vector<int>::iterator const tagEnd = tags.end ();
	for (; tagIter != tagEnd; ++tagIter) {
		std::vector<Document *>::iterator docIter = docs.begin ();
		std::vector<Document *>::iterator const docEnd = docs.end ();
		for (; docIter != docEnd; ++docIter) {
			(*docIter)->setTag (*tagIter);
		}
	}

	/* He might have added or removed tags */
	updateTagSizes ();
}

void RefWindow::addDocFiles (std::vector<Glib::ustring> const &filenames)
{
	bool const singular = (filenames.size() == 1);
	/* List of documents we add */
	std::vector<Document*> addedDocs;

	Gtk::Dialog dialog (_("Add Document Files"), true, false);
	dialog.set_icon (window_->get_icon());

	Gtk::VBox *vbox = dialog.get_vbox ();
	vbox->set_spacing (12);

	Glib::ustring messagetext =
		String::ucompose ("<b><big>%1</big></b>\n\n", _("Adding document files")) +
		_("This process may take some time as the bibliographic "
		"information for each document is looked up.");

	Gtk::Label label ("", false);
	label.set_markup (messagetext);

	vbox->pack_start (label, true, true, 0);

	Gtk::ProgressBar progress;
	vbox->pack_start (progress, false, false, 0);

	Gtk::TreeModelColumn<Glib::ustring> keyColumn;
	Gtk::TreeModelColumn<Glib::ustring> textColumn;
	Gtk::TreeModelColumn<Glib::ustring> idColumn;
	Gtk::TreeModelColumn<Glib::ustring> metadataColumn;
	Gtk::TreeModelColumn<Glib::ustring> resultColumn;
	Gtk::TreeModelColumnRecord columns;
	columns.add (keyColumn);
	columns.add (textColumn);
	columns.add (idColumn);
	columns.add (metadataColumn);
	columns.add (resultColumn);
	Glib::RefPtr<Gtk::ListStore> reportModel = Gtk::ListStore::create(columns);

	Gtk::TreeView reportView (reportModel);
	reportView.insert_column (_("Document"), keyColumn, 0);
	reportView.insert_column (_("Got text"), textColumn, 1);
	reportView.insert_column (_("Got ID"), idColumn, 2);
	reportView.insert_column (_("Got metadata"), metadataColumn, 3);
	reportView.insert_column (_("Outcome"), resultColumn, 4);
	reportView.show_all ();

	Gtk::ScrolledWindow reportScroll;
	reportScroll.set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	reportScroll.add (reportView);
	reportScroll.set_size_request (-1, 200);

	Gtk::Frame reportFrame;
	reportFrame.add (reportScroll);
	if (!singular)
		vbox->pack_start (reportFrame, true, true, 0);

	/* Create buttons */
	Gtk::Button *cancelButton = dialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_ACCEPT);
	Gtk::Button *tagButton = dialog.add_button (_("_Attach tags..."), Gtk::RESPONSE_ACCEPT);
	Gtk::Button *closeButton = dialog.add_button (Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE);
	tagButton->set_sensitive    (false);
	closeButton->set_sensitive  (false);
	cancelButton->set_sensitive (true);
	cancelButton->signal_clicked().connect(
		sigc::bind (
			sigc::mem_fun (*this, &RefWindow::onAddDocFilesCancel),
			cancelButton,
			&progress));
	tagButton->signal_clicked().connect(
		sigc::bind (
			sigc::mem_fun (*this, &RefWindow::onAddDocFilesTag),
			sigc::ref(addedDocs)));


	dialog.show_all ();
	vbox->set_border_width (12);

	Glib::ustring progresstext;


	cancelAddDocFiles_ = false;
	int n = 0;
	std::vector<Glib::ustring>::const_iterator it = filenames.begin();
	std::vector<Glib::ustring>::const_iterator const end = filenames.end();
	for (; it != end && !cancelAddDocFiles_; ++it) {
		progress.set_fraction ((float)n / (float)filenames.size());
		progresstext = String::ucompose (_("%1 of %2 documents"), n, filenames.size ());
		progress.set_text (progresstext);
		while (Gnome::Main::events_pending())
			Gnome::Main::iteration ();

		Glib::RefPtr<Gnome::Vfs::Uri> uri = Gnome::Vfs::Uri::create (*it);
		if (!Utility::uriIsFast (uri)) {
			// Should prompt the user to download the file
			DEBUG ("Ooh, a remote uri\n");
		}

		Document *newdoc = library_->doclist_->newDocWithFile(*it);
		bool added = false;
		bool gotMetadata = false;
		bool gotText = false;
		bool gotId = false;
		Glib::ustring key = "";
		if (newdoc) {
			gotText = newdoc->readPDF ();

			while (Gnome::Main::events_pending())
				Gnome::Main::iteration ();

			// If we got a DOI or eprint field this will work
			gotMetadata = newdoc->getMetaData ();

			// Generate a Zoidberg99 type key
			newdoc->setKey (
				library_->doclist_->uniqueKey (
					newdoc->generateKey ()));
			
			// If we did not succeed in getting a title, use the filename
			if (newdoc->getBibData().getTitle().empty()) {
				Glib::ustring filename = Gnome::Vfs::unescape_string_for_display (
					Glib::path_get_basename (newdoc->getFileName()));

				Glib::ustring::size_type periodpos = filename.find_last_of (".");
				if (periodpos != std::string::npos) {
					filename = filename.substr (0, periodpos);
				}
				
				newdoc->getBibData().setTitle (filename);
			}
			
			/* Add the document to the view */
			docview_->addDoc (newdoc);

			/* Remember it for the end */
			addedDocs.push_back (newdoc);

			/* Add-dialog UI fields */
			added = true;
			gotId = newdoc->hasField ("doi") || newdoc->hasField ("eprint") || newdoc->hasField ("pmid");
			key = newdoc->getKey ();
				
		} else {
			DEBUG (String::ucompose ("RefWindow::addDocFiles: Warning: didn't succeed adding '%1'.  Duplicate file?\n", *it));
		}

	

		if (key.empty ()) {
			// We didn't add this guy so didn't work out his key
			Glib::ustring filename = (*it);
			Glib::ustring::size_type len = filename.size();
			key = (*it).substr(len - 14, len);
		}

		/*
		Glib::ustring yes = _("Yes");
		Glib::ustring no = _("No");
		*/
		/* A tick and a cross */
		Glib::ustring yes (1, (gunichar)0x2714);
		Glib::ustring no  (1, (gunichar)0x2718);

		Gtk::TreeModel::iterator newRow = reportModel->append();
		(*newRow)[keyColumn] = key;
		(*newRow)[resultColumn] = added ? "Added" : "Not added";
		(*newRow)[idColumn] = gotId ? yes : no;
		(*newRow)[textColumn] = gotText ? yes : no;
		(*newRow)[metadataColumn] = gotMetadata ? yes : no;

		reportView.scroll_to_row (reportModel->get_path(newRow));

		++n;
	}

	if (cancelAddDocFiles_) {
		progress.set_text (_("Cancelled"));
		Gtk::TreeModel::iterator newRow = reportModel->append();
		(*newRow)[keyColumn] = _("Cancelled");
		reportView.scroll_to_row (reportModel->get_path(newRow));
	} else {
		progress.set_fraction (1.0);
		progress.set_text (_("Finished"));
	}

	closeButton->set_sensitive (true);
	tagButton->set_sensitive (true);
	cancelButton->set_sensitive (false);
	dialog.set_urgency_hint (true);
	if (!singular) {
		int response = Gtk::RESPONSE_ACCEPT;
		while (response != Gtk::RESPONSE_CLOSE) {
			response = dialog.run ();
		}
	}

	if (!filenames.empty()) {
		// We added something
		// Should check if we actually added something in case a newDoc
		// failed, eg if the doc was already in there
		setDirty (true);
		updateStatusBar ();
	}
}


void RefWindow::onAddDocUnnamed ()
{
	setDirty (true);
	Document *newdoc = library_->doclist_->newDocUnnamed ();
	newdoc->setKey (library_->doclist_->uniqueKey (newdoc->generateKey ()));
	if (docpropertiesdialog_->show (newdoc)) {
		docview_->addDoc (newdoc);
		updateStatusBar ();
	} else {
		library_->doclist_->removeDoc (newdoc);
	}
}


void RefWindow::onAddDocById ()
{
	static Glib::ustring lastSelected;

	Gtk::Dialog dialog (_("Add Reference with ID"), true, false);

	Gtk::VBox *vbox = dialog.get_vbox ();


	/* Populate structures of ID type names, IDs */
	std::vector<PluginCapability::Identifier> capIds;
	std::vector<Glib::ustring> capNames;
	std::map<Glib::ustring, PluginCapability::Identifier> capNameToId;

	capIds = PluginCapability::getMetadataCapabilities ();
	std::vector<PluginCapability::Identifier>::iterator capIter = capIds.begin();
	std::vector<PluginCapability::Identifier>::iterator const capEnd = capIds.end();
	for (; capIter != capEnd; ++capIter) {
		PluginCapability::Identifier id = *capIter;
		Glib::ustring const name = PluginCapability::getFriendlyName(id);
		capNames.push_back(name);
		capNameToId.insert (std::pair<Glib::ustring, PluginCapability::Identifier>(name, id));
	}

	/*
	 * A combo to select between ID types
	 */

	Gtk::ComboBoxText combo;
	std::vector<Glib::ustring>::iterator nameIter = capNames.begin();
	std::vector<Glib::ustring>::iterator const nameEnd = capNames.end();
	for (; nameIter != nameEnd; ++nameIter) {
		combo.append_text (*nameIter);
	}

	if (!lastSelected.empty())
		combo.set_active_text (lastSelected);
	else
		combo.set_active_text (capNames[0]);

	Gtk::HBox hbox;
	hbox.set_spacing (6);
	vbox->pack_start (hbox, true, true, 0);

	hbox.pack_start (combo, false, false, 0);

	Gtk::Entry entry;
	hbox.pack_start (entry, true, true, 0);
	entry.set_activates_default (true);
	entry.grab_focus ();

	dialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button (Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
	dialog.set_default_response (Gtk::RESPONSE_ACCEPT);

	dialog.show_all ();
	vbox->set_border_width (12);

	if (dialog.run () == Gtk::RESPONSE_ACCEPT) {
		setDirty (true);
		Document *newdoc = library_->doclist_->newDocUnnamed ();

		/* Look up selection to capability ID */
		Glib::ustring displayField = combo.get_active_text ();
		std::map<Glib::ustring, PluginCapability::Identifier>::iterator idIter = capNameToId.find(displayField);
		if (idIter == capNameToId.end()) {
			/* Epic fail, user somehow selected a field that 
			 * we don't remember creating */
			DEBUG1 ("Bad displayField %1", displayField);
		}
		PluginCapability::Identifier capId = idIter->second;

		/* Look up capability ID to document field name */
		Glib::ustring const field = PluginCapability::getFieldName(capId);
		if (field == "") {
			/* A capability without a valid field name */
			DEBUG1 ("Bad capId %1", capId);
		}

		Glib::ustring id = entry.get_text ();
		id = Utility::trimWhiteSpace (id);

		newdoc->setField (field, id);

		newdoc->getMetaData ();
		newdoc->setKey (library_->doclist_->uniqueKey (newdoc->generateKey ()));

		docview_->addDoc (newdoc);
		updateStatusBar ();

		lastSelected = displayField;
	}
}


void RefWindow::onAddDocFile ()
{
	Gtk::FileChooserDialog chooser(
		_("Add Document"),
		Gtk::FILE_CHOOSER_ACTION_OPEN);

	chooser.set_select_multiple (true);
	chooser.set_local_only (false);
	if (!addfolder_.empty())
		chooser.set_current_folder (addfolder_);
	chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_REJECT);
	chooser.add_button (Gtk::Stock::ADD, Gtk::RESPONSE_ACCEPT);
	chooser.set_default_response (Gtk::RESPONSE_ACCEPT);


	if (chooser.run () == Gtk::RESPONSE_ACCEPT) {
		// Dirty is set in adddocfiles
		addfolder_ = Glib::path_get_dirname(chooser.get_filename());
		std::vector<Glib::ustring> newfiles;
		Glib::SListHandle<Glib::ustring> uris = chooser.get_uris ();
		chooser.hide ();
		Glib::SListHandle<Glib::ustring>::iterator iter = uris.begin();
		Glib::SListHandle<Glib::ustring>::iterator const end = uris.end();
		for (; iter != end; ++iter) {
			newfiles.push_back (*iter);
		}
		addDocFiles (newfiles);
	}
}


void RefWindow::onAddDocFolder ()
{
	Gtk::FileChooserDialog chooser(
		_("Add Folder"),
		Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

	// Want an option for following symlinks? (we don't do it)

	chooser.set_local_only (false);
	if (!addfolder_.empty())
		chooser.set_current_folder (addfolder_);
	chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_REJECT);
	chooser.add_button (Gtk::Stock::ADD, Gtk::RESPONSE_ACCEPT);
	chooser.set_default_response (Gtk::RESPONSE_ACCEPT);

	if (chooser.run () == Gtk::RESPONSE_ACCEPT) {
		// Dirty is set in adddocfiles
		addfolder_ = Glib::path_get_dirname(chooser.get_filename());
		Glib::ustring rootfoldername = chooser.get_uri();
		chooser.hide ();
		std::vector<Glib::ustring> files = Utility::recurseFolder (rootfoldername);
		addDocFiles (files);
	}
}



void RefWindow::onRemoveDoc ()
{
	std::vector<Document*> docs = docview_->getSelectedDocs ();
	if (docs.size() == 0)
		return;

	bool const multiple = docs.size() > 1;

	Glib::ustring message;
	Glib::ustring secondary;

	if (multiple) {
		message = String::ucompose (
			_("Are you sure you want to remove these %1 documents?"),
			docs.size ());
		secondary = _("All tag associations and metadata for these documents will be permanently lost.");
	} else {
		message = String::ucompose (
			_("Are you sure you want to remove '%1'?"),
			(*docs.begin ())->getKey ());
		secondary = _("All tag associations and metadata for the document will be permanently lost.");
	}
	Gtk::MessageDialog confirmdialog (
		message, false, Gtk::MESSAGE_QUESTION,
		Gtk::BUTTONS_NONE, true);

	confirmdialog.set_secondary_text (secondary);

	confirmdialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	confirmdialog.add_button (Gtk::Stock::REMOVE, Gtk::RESPONSE_ACCEPT);
	confirmdialog.set_default_response (Gtk::RESPONSE_CANCEL);

	if (confirmdialog.run () != Gtk::RESPONSE_ACCEPT) {
		return;
	}

	std::vector<Document*>::iterator it = docs.begin ();
	std::vector<Document*>::iterator const end = docs.end ();
	for (; it != end; it++) {
		DEBUG (String::ucompose ("RefWindow::onRemoveDoc: removeDoc on '%1'", *it));
		docview_->removeDoc (*it);
		library_->doclist_->removeDoc(*it);
	}

	setDirty (true);
}


void RefWindow::onSearch ()
{
	SearchDialog dialog;
	dialog.run();
}


void RefWindow::onGetMetadataDoc ()
{
	progress_->start (_("Fetching metadata"));

	bool doclistdirty = false;
	std::vector <Document*> docs = docview_->getSelectedDocs ();
	std::vector <Document*>::iterator it = docs.begin ();
	std::vector <Document*>::iterator const end = docs.end ();
	for (int i = 0; it != end; ++it, ++i) {
		Document* doc = *it;
		if (doc->canGetMetadata ()) {
			setDirty (true);
			doclistdirty = true;
			doc->getMetaData ();
			docview_->updateDoc(doc);
		}

		progress_->update ((float)i / (float)(docs.size()));
	}

	progress_->finish ();
}


void RefWindow::onRenameDoc ()
{
	std::vector <Document*> docs = docview_->getSelectedDocs ();
	if (docs.size () == 0)
		return;

	Glib::ustring message;
	if (docs.size () == 1) {
		message = String::ucompose (_("Really rename this file to '%1'?"),
			(*docs.begin())->getKey());
	} else if (docs.size () > 1) {
		message = String::ucompose (_("Really rename these %1 files to their keys?"),
			docs.size ());
	}

	Gtk::MessageDialog confirmdialog (
		message, false, Gtk::MESSAGE_QUESTION,
		Gtk::BUTTONS_NONE, true);

	confirmdialog.set_secondary_text (_("This action <b>cannot be undone</b>."), true);

	confirmdialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	confirmdialog.add_button (_("Rename from Key"), Gtk::RESPONSE_ACCEPT);
	confirmdialog.set_default_response (Gtk::RESPONSE_CANCEL);

	if (confirmdialog.run() != Gtk::RESPONSE_ACCEPT)
		return;

	std::vector <Document*>::iterator it = docs.begin ();
	std::vector <Document*>::iterator const end = docs.end ();
	for (; it != end; ++it) {
		Document* doc = *it;
		doc->renameFromKey ();
		docview_->updateDoc (doc);
	}

	updateStatusBar ();
	setDirty (true);
}


void RefWindow::onDeleteDoc ()
{
	std::vector<Document*> docs = docview_->getSelectedDocs ();
	if (docs.size() == 0)
		return;

	bool const multiple = docs.size() > 1;
	Glib::ustring message;
	Glib::ustring secondary;

	if (multiple) {
		message = String::ucompose (
			_("Are you sure you want to delete the files of these %1 documents?"),
			docs.size ());
		secondary = _("All tag associations and metadata for these documents "
			"will be permanently lost, and the files they refer to will be "
			"irretrievably deleted.");
	}
	else {
		message = String::ucompose (
			_("Are you sure you want to delete the file of '%1'?"),
			(*docs.begin ())->getKey ());
		secondary = _("All tag associations and metadata for the document "
			"will be permanently lost, and the file it refers to will be "
			"irretrievably deleted.");
	}

	Gtk::MessageDialog confirmdialog (
		message, false, Gtk::MESSAGE_QUESTION,
		Gtk::BUTTONS_NONE, true);

	confirmdialog.set_secondary_text (secondary);

	confirmdialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	confirmdialog.add_button (Gtk::Stock::DELETE, Gtk::RESPONSE_ACCEPT);
	confirmdialog.set_default_response (Gtk::RESPONSE_CANCEL);

	if (confirmdialog.run() != Gtk::RESPONSE_ACCEPT) {
		return;
	}

	std::vector<Document*>::iterator it = docs.begin ();
	std::vector<Document*>::iterator const end = docs.end ();
	for (; it != end; it++) {
		try {
			Utility::deleteFile ((*it)->getFileName ());
			docview_->removeDoc (*it);
			library_->doclist_->removeDoc(*it);
		} catch (Glib::Exception &ex) {
			Utility::exceptionDialog (&ex,
				String::ucompose (_("Deleting '%1'"), (*it)->getFileName ()));
		}

	}

	updateStatusBar ();
	setDirty (true);
}


void RefWindow::onOpenDoc ()
{
	std::vector<Document*> docs = docview_->getSelectedDocs ();
	std::vector<Document*>::iterator it = docs.begin ();
	std::vector<Document*>::iterator const end = docs.end ();
	for (; it != end ; ++it) {
		if (!(*it)->getFileName().empty()) {
			try {
				Gnome::Vfs::url_show ((*it)->getFileName());
			} catch (const Gnome::Vfs::exception ex) {
				Utility::exceptionDialog (&ex,
					String::ucompose (
						_("Trying to open file '%1'"),
						Gnome::Vfs::unescape_string ((*it)->getFileName())));
				return;
			}
		}
	}
}


void RefWindow::openProperties (Document *doc)
{
	if (doc) {
		if (docpropertiesdialog_->show (doc)) {

			/* Check for dupe keys */
			Glib::ustring key = doc->getKey ();
			Glib::ustring uniqueKey = library_->doclist_->uniqueKey (key, doc);
			if (key != uniqueKey)
				doc->setKey (Document::keyReplaceDialog (key, uniqueKey));

			setDirty (true);
			docview_->updateDoc (doc);
			updateStatusBar ();
		}
	}
}


void RefWindow::onDocProperties ()
{
	Document *doc = docview_->getSelectedDoc ();
	openProperties (doc);
}


void RefWindow::onPreferences ()
{
	_global_prefs->showDialog ();
}


void RefWindow::onShowTagPaneToggled ()
{
	_global_prefs->setShowTagPane (
		Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("ShowTagPane"))->get_active ());
}


void RefWindow::onShowTagPanePrefChanged ()
{
	bool const showtagpane = _global_prefs->getShowTagPane ();
	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
		actiongroup_->get_action ("ShowTagPane"))->set_active (showtagpane);
	if (showtagpane) {
		tagpane_->show ();
	} else {
		tagpane_->hide ();
	}
}

void RefWindow::onShowNotesPaneToggled ()
{
	_global_prefs->setShowNotesPane (
		Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("ShowNotesPane"))->get_active ());
}


void RefWindow::onShowNotesPanePrefChanged ()
{
	bool const shownotespane = _global_prefs->getShowNotesPane ();
	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
		actiongroup_->get_action ("ShowNotesPane"))->set_active (shownotespane);
	if (shownotespane) {
		notespane_->show ();
	} else {
		notespane_->hide ();
	}
}

void RefWindow::updateOfflineIcon ()
{
	bool const offline = _global_prefs->getWorkOffline ();
	
	Gtk::StockID icon = offline ?
		Gtk::Stock::DISCONNECT :
		Gtk::Stock::CONNECT;

	offlineicon_->set (icon, Gtk::IconSize(Gtk::ICON_SIZE_MENU));
	
	#if GTK_VERSION_GE(2,12)
	offlineicon_->set_tooltip_text ( offline ? 
		_("Working offline") :
		_("Working online"));
	#endif
}


void RefWindow::onWorkOfflineToggled ()
{
	_global_prefs->setWorkOffline (
		Glib::RefPtr <Gtk::ToggleAction>::cast_static(
		actiongroup_->get_action ("WorkOffline"))->get_active ());
	
	updateOfflineIcon ();
}





void RefWindow::onWorkOfflinePrefChanged ()
{
	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
		actiongroup_->get_action ("WorkOffline"))->set_active (
			_global_prefs->getWorkOffline ());

	// To pick up sensitivity changes in lookup metadata etc
	docSelectionChanged ();
	
	updateOfflineIcon ();
}


void RefWindow::updateTitle ()
{
	Glib::ustring filename;
	if (openedlib_.empty ()) {
		filename = _("Unnamed Library");
	} else {
		filename = Gnome::Vfs::unescape_string_for_display (Glib::path_get_basename (openedlib_));
		Glib::ustring::size_type pos = filename.find (".reflib");
		if (pos != Glib::ustring::npos) {
			filename = filename.substr (0, pos);
		}
	}
	window_->set_title (
		(getDirty () ? "*" : "")
		+ filename
		+ " - "
		+ DISPLAY_PROGRAM);
}


void RefWindow::setOpenedLib (Glib::ustring const &openedlib)
{
	openedlib_ = openedlib;
	updateTitle();
}

void RefWindow::setDirty (bool const &dirty)
{
	dirty_ = dirty;
	actiongroup_->get_action("SaveLibrary")
		->set_sensitive (dirty_);
	updateTitle ();
}


void RefWindow::onImport ()
{
	Gtk::FileChooserDialog chooser(
		_("Import References"),
		Gtk::FILE_CHOOSER_ACTION_OPEN);

	// remote not working?
	//chooser.set_local_only (false);

	if (!libraryfolder_.empty())
		chooser.set_current_folder (libraryfolder_);

	chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_REJECT);
	chooser.add_button (Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);
	chooser.set_default_response (Gtk::RESPONSE_ACCEPT);

	Gtk::FileFilter allfiles;
	allfiles.add_pattern ("*");
	allfiles.set_name (_("All Files"));
	chooser.add_filter (allfiles);

	Gtk::FileFilter allbibfiles;
	// Complete random guesses, what are the real extensions?
	allbibfiles.add_pattern ("*.ris");
	allbibfiles.add_pattern ("*.[bB][iI][bB]");
	allbibfiles.add_pattern ("*.ref");
	allbibfiles.set_name (_("Bibliography Files"));
	chooser.add_filter (allbibfiles);

	Gtk::FileFilter bibtexfiles;
	bibtexfiles.add_pattern ("*.[bB][iI][bB]");
	bibtexfiles.set_name (_("BibTeX Files"));
	chooser.add_filter (bibtexfiles);

	Gtk::HBox extrabox;
	extrabox.set_spacing (6);
	chooser.set_extra_widget (extrabox);
	Gtk::Label label (_("Format:"));
	extrabox.pack_start (label, false, false, 0);
	Gtk::ComboBoxText combo;
	combo.append_text ("BibTeX");
	combo.append_text ("EndNote");
	combo.append_text ("RIS");
	combo.append_text ("MODS");
	//combo.append_text (_("Auto Detect"));
	combo.set_active (0);
	extrabox.pack_start (combo, true, true, 0);
	extrabox.show_all ();


	if (chooser.run () == Gtk::RESPONSE_ACCEPT) {
		libraryfolder_ = Glib::path_get_dirname(chooser.get_filename());
		Glib::ustring filename = chooser.get_uri ();
		chooser.hide ();
		setDirty (true);

		BibUtils::Format format;
		switch (combo.get_active_row_number ()) {
			case 0:
				format = BibUtils::FORMAT_BIBTEX;
				break;
			case 1:
				format = BibUtils::FORMAT_ENDNOTE;
				break;
			case 2:
				format = BibUtils::FORMAT_RIS;
				break;
			case 3:
				format = BibUtils::FORMAT_MODS;
				break;
			default:
				// Users selected "Auto detect"
				// DocumentList::import will try to guess once
				// it has the text
				format = BibUtils::FORMAT_UNKNOWN;
		}

		library_->doclist_->importFromFile (filename, format);


		populateTagList ();
		/*
		 * Should iterate over added docs with addDoc
		 * but this performance hit is acceptable since importing
		 * is a super-rare operation
		 */
		docview_->populateDocStore ();
		updateStatusBar ();
	}
}


// Selection should be GDK_SELECTION_CLIPBOARD for the windows style
// clipboard and GDK_SELECTION_PRIMARY for middle-click style
void RefWindow::onPasteBibtex (GdkAtom selection)
{
	// Should have sensitivity changing for this
	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get (selection);

/*
	Gtk::Clipboard reference:
	If you don't want to deal with providing a separate callback,
	you can also use wait_for_contents(). This runs the GLib main
	loop recursively waiting for the contents. This can simplify
	the code flow, but you still have to be aware that other
	callbacks in your program can be called while this recursive
	mainloop is running.
*/

	Glib::ustring clipboardtext = clipboard->wait_for_text ();

// Uncomment this for correct behaviour with primary clipboard
// only once sensitivity of explicity copy is set correctly
/*
	if (clibboardtext.empty ())
		return;
*/

	int imported =
		library_->doclist_->import (clipboardtext, BibUtils::FORMAT_BIBTEX);

	DEBUG (String::ucompose ("Imported %1 references", imported));

	if (imported) {

		populateTagList ();
		/*
		 * FIXME  should get the Document* from the import
		 * call and iterate over them with addDoc to be 
		 * more efficient and not risk losing selection
		 */
		docview_->populateDocStore ();
		updateStatusBar ();
		statusbar_->push (String::ucompose
			(_("Imported %1 BibTeX references"), imported), 0);

		setDirty (true);
	} else {
		Glib::ustring message = String::ucompose (
			"<b><big>%1</big></b>",
			_("No references found on clipboard.\n"));

		Gtk::MessageDialog dialog (
			message, true,
			Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE, true);

		dialog.run ();
	}
}




void RefWindow::onCopyCite ()
{
	std::vector<Document*> docs = docview_->getSelectedDocs ();
	std::vector<Document*>::iterator it = docs.begin ();
	std::vector<Document*>::iterator const end = docs.end ();

	Glib::ustring citation = "\\cite{";

	for (; it != end; ++it) {
		Glib::ustring const key = (*it)->getKey ();
		if (it != docs.begin ()) {
			citation += ",";
		}
		citation += key;
	}

	citation += "}";

	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get ();

	clipboard->set_text (citation);
}


void RefWindow::setSensitive (bool const sensitive)
{
	window_->set_sensitive (sensitive);
}


int RefWindow::sortTags (
	const Gtk::TreeModel::iterator& a,
	const Gtk::TreeModel::iterator& b)
{
	// This callback should return
	//  * -1 if a compares before b
	//  *  0 if they compare equal
	//  *  1 if a compares after b

	int const & a_uid = (*a)[taguidcol_];
	int const & b_uid = (*b)[taguidcol_];

	bool const a_is_special = a_uid < 0;
	bool const b_is_special = b_uid < 0;

	Glib::ustring const & a_name = (*a)[tagnamecol_];
	Glib::ustring const & b_name = (*b)[tagnamecol_];

	if (a_is_special && b_is_special) {
		if (a_uid > b_uid) {
			return -1;
		} else if (a_uid < b_uid) {
			return 1;
		} else {
			return 0;
		}
	} else if (a_is_special) {
		return -1;
	} else if (b_is_special) {
		return 1;
	} else {
		return a_name.compare (b_name);
	}
	
	// Shut up the compiler.
	return 0;
}


void RefWindow::onResize (GdkEventConfigure *event)
{
	_global_prefs->setWindowSize (
		std::pair<int,int> (
			event->width,
			event->height));
}


void RefWindow::onNotesPaneResize (Gdk::Rectangle &allocation)
{
	_global_prefs->setNotesPaneHeight (allocation.get_height());
}


void RefWindow::onFind ()
{
	docview_->getSearchEntry().grab_focus ();
}


void RefWindow::docSelectionChanged ()
{
	if (ignoreDocSelectionChanged_)
		return;

	int selectcount = docview_->getSelectedDocCount ();

	bool const somethingselected = selectcount > 0;
	bool const onlyoneselected = selectcount == 1;

	actiongroup_->get_action("CopyCite")->set_sensitive (somethingselected);

	actiongroup_->get_action("RemoveDoc")->set_sensitive (somethingselected);
	actiongroup_->get_action("DeleteDoc")->set_sensitive (somethingselected);
	actiongroup_->get_action("RenameDoc")->set_sensitive (somethingselected);
	actiongroup_->get_action("TaggerMenuAction")->set_sensitive (somethingselected);
	actiongroup_->get_action("DocProperties")->set_sensitive (onlyoneselected);

	// plugin's action
	std::list<Plugin*> plugins = _global_plugins->getPlugins();
	std::list<Plugin*>::iterator pit = plugins.begin();
	std::list<Plugin*>::iterator const pend = plugins.end();
	for (; pit != pend; pit++) {
		if ((*pit)->isEnabled())
		{
			Plugin::ActionList actions = (*pit)->getActions ();
			Plugin::ActionList::iterator it = actions.begin ();
			Plugin::ActionList::iterator const end = actions.end ();
			for (; it != end; ++it) {
				Glib::ustring *callback = (Glib::ustring*) (*it)->get_data("sensitivity");
				bool enable = (*pit)->updateSensitivity (*callback, docview_->getSelectedDocs());
				(*it)->set_sensitive (enable);
			}

		}
	}

	DocumentView::Capabilities cap
		= docview_->getDocSelectionCapabilities ();
	actiongroup_->get_action("OpenDoc")->set_sensitive (cap.open);
	actiongroup_->get_action("GetMetadataDoc")->set_sensitive (cap.getmetadata);

	/* Update tagger Actions */
	ignoreTaggerActionToggled_ = true;
	for (TagList::TagMap::iterator tagit = library_->taglist_->getTags().begin();
	     tagit != library_->taglist_->getTags().end(); ++tagit) {
		Glib::RefPtr<Gtk::ToggleAction> action = taggerUI_[(*tagit).second.uid_].action;
		DocumentView::SubSet state = docview_->selectedDocsHaveTag ((*tagit).second.uid_);

		/* God fucking damn it, why doesn't Gtk::ToggleAction
		 * support inconsistent state?  That is so inconsistent!  My eyes! */
		if (state == DocumentView::ALL) {
			action->set_active (true);
			//action->set_inconsistent (false);
		} else if (state == DocumentView::SOME) {
			action->set_active (false);
			//action->set_inconsistent (true);
		} else {
			action->set_active (false);
			//action->set_inconsistent (false);
		}
	}
	ignoreTaggerActionToggled_ = false;
	
	/* Update notes frame text */
	updateNotesPane ();

	updateStatusBar ();
}

void RefWindow::onUseListViewToggled ()
{
	_global_prefs->setUseListView (
		Glib::RefPtr <Gtk::RadioAction>::cast_static(
			actiongroup_->get_action ("UseListView"))->get_active ());
}


void RefWindow::onUseListViewPrefChanged ()
{
	docview_->setUseListView(_global_prefs->getUseListView ());

	if (_global_prefs->getUseListView ()) {
		Glib::RefPtr <Gtk::RadioAction>::cast_static(
			actiongroup_->get_action ("UseListView"))->set_active (true);
	} else {
		Glib::RefPtr <Gtk::RadioAction>::cast_static(
			actiongroup_->get_action ("UseIconView"))->set_active (true);
	}
	
	updateStatusBar ();
}


void RefWindow::onPluginRun (Glib::ustring const function, Plugin* plugin)
{
	std::vector<Document*> docs = docview_->getSelectedDocs();
	if (plugin->doAction(function, docs)) {;
	    /*
	     * Update the docs in the view since the plugin could 
	     * have written to them
	     */
	    std::vector<Document*>::iterator it = docs.begin ();
	    std::vector<Document*>::iterator const end = docs.end ();
	    for (; it != end; ++it) {
		    docview_->updateDoc (*it);
	    }

	    /*
	     * Mark the library as dirty since the plugin might 
	     * have modified any of the documents
	     */
	    setDirty (true);
	}
}


RefWindow::SearchDialog::SearchDialog ()
{
	xml_ = Gnome::Glade::Xml::create (
		Utility::findDataFile ("search.glade"));
	
	xml_->get_widget ("SearchDialog", dialog_);
	xml_->get_widget ("Find", searchButton_);
	xml_->get_widget ("SearchText", searchEntry_);
	xml_->get_widget ("Plugin", pluginCombo_);
	xml_->get_widget ("SearchResults", resultView_);
	xml_->get_widget ("Cancel", cancelButton_);
	xml_->get_widget ("Progress", progressbar_);

	searchButton_->signal_clicked().connect (
		sigc::mem_fun (*this, &RefWindow::SearchDialog::search));

	Gtk::TreeModel::ColumnRecord resultCols;
	resultCols.add (resultTokenColumn_);
	resultCols.add (resultTitleColumn_);
	resultCols.add (resultAuthorColumn_);
	resultModel_ = Gtk::ListStore::create (resultCols);

	resultView_->set_model (resultModel_);
	resultView_->append_column ("Author", resultAuthorColumn_);
	resultView_->append_column ("Title", resultTitleColumn_);

	Gtk::TreeModel::ColumnRecord pluginCols;
	pluginCols.add (pluginPtrColumn_);
	pluginCols.add (pluginNameColumn_);
	pluginModel_ = Gtk::ListStore::create (pluginCols);
	pluginCombo_->set_model (pluginModel_);
	Gtk::CellRendererText *pluginCell = Gtk::manage (new Gtk::CellRendererText());
	pluginCombo_->pack_start (*pluginCell, true);
	pluginCombo_->add_attribute (pluginCell->property_text(), pluginNameColumn_);
}


void RefWindow::SearchDialog::run ()
{
	/* FIXME: choose plugin from last time? */
	Glib::ustring const defaultPlugin = "blahblahlbah";

	/* Retrieve list of usable search plugins */
	PluginManager::PluginList plugins = _global_plugins->getEnabledPlugins ();
	PluginManager::PluginList::const_iterator pluginIter = plugins.begin();
	PluginManager::PluginList::const_iterator const pluginEnd = plugins.end();
	for (; pluginIter != pluginEnd; pluginIter++) {
		if ((*pluginIter)->canSearch()) {
			Gtk::ListStore::iterator iter = pluginModel_->append();
			Plugin *plugin = *pluginIter;

			(*iter)[pluginPtrColumn_] = plugin;
			(*iter)[pluginNameColumn_] = plugin->getShortName();

			/* Make this active if it's the default */
			if (plugin->getShortName() == defaultPlugin) {
				pluginCombo_->set_active (iter);
			}
		}
	}

	/* If the default wasn't found then make then
	 * activate the first plugin in the list */
	if (pluginModel_->children().size()) {
		pluginCombo_->set_active (pluginModel_->children().begin());
	}

	searchButton_->set_sensitive (pluginModel_->children().size());	

	cancelButton_->hide ();
	progressbar_->hide ();

	dialog_->run ();

	dialog_->hide ();
}


bool RefWindow::SearchDialog::progressCallback (void *ptr)
{
	RefWindow::SearchDialog *dialog = (RefWindow::SearchDialog*)ptr;

	return dialog->progress ();
}


bool RefWindow::SearchDialog::progress ()
{
	progressbar_->pulse ();
}


void RefWindow::SearchDialog::search ()
{
	/* UI to initial state */
	cancelButton_->show ();
	cancelButton_->set_sensitive (true);
	progressbar_->show ();
	progressbar_->set_fraction (0.0);
	progressbar_->set_text (_("Searching..."));
	resultModel_->clear();

	/* Retrieve search text */
	Glib::ustring const searchTerms = searchEntry_->get_text ();
	DEBUG1 ("Searching for '%1'", searchTerms);

	/* Retrieve choice of plugin */
	Plugin *plugin = (*(pluginCombo_->get_active()))[pluginPtrColumn_];

	/* Set up progress callback */
	_global_plugins->progressCallback_ = &(RefWindow::SearchDialog::progressCallback);
	_global_plugins->progressObject_ = this;

	/* Invoke plugin's search function */
	Plugin::SearchResults results = plugin->doSearch(searchTerms);
	DEBUG1 ("Searching with plugin '%1'", plugin->getShortName());

	/* Revoke callback */
	_global_plugins->progressCallback_ = NULL;
	_global_plugins->progressObject_ = NULL;

	/* Progress UI to complete state */
	cancelButton_->set_sensitive (false);
	progressbar_->set_fraction (1.0);
	progressbar_->set_text (_("Complete"));

	Plugin::SearchResults::const_iterator resultIter = results.begin ();
	Plugin::SearchResults::const_iterator const resultEnd = results.end ();
	for (; resultIter != resultEnd; ++resultIter) {
		Glib::ustring title;
		Glib::ustring author;

		/* FIXME: unnecessary copy */
		Plugin::SearchResult result = *resultIter;

		Plugin::SearchResult::iterator found;
		found = result.find("title");
		if (found != result.end())
			title = found->second;
		found = result.find("author");
		if (found != result.end())
			author = found->second;

		Gtk::ListStore::iterator newRow = resultModel_->append();
		(*newRow)[resultTitleColumn_] = title;
		(*newRow)[resultAuthorColumn_] = author;
	}
}





