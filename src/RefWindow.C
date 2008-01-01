
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
#include "Preferences.h"
#include "Progress.h"
#include "TagList.h"

#include "config.h"
#include "RefWindow.h"


RefWindow::RefWindow ()
{
	ignoreTagSelectionChanged_ = false;
	ignoretaggerchecktoggled_ = false;
	ignoreDocSelectionChanged_ = false;

	dirty_ = false;

	gdk_threads_enter ();

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

	gdk_threads_leave ();
}


RefWindow::~RefWindow ()
{
	_global_prefs->setLibraryFilename (openedlib_);

	delete progress_;
	delete docpropertiesdialog_;
	delete library_;	
}


void RefWindow::run ()
{
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

	window_->set_icon (
		Gdk::Pixbuf::create_from_file(
			Utility::findDataFile("referencer.svg")));

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

	/* Contains tags and document view */
	Gtk::HPaned *hpaned = Gtk::manage(new Gtk::HPaned());
	vbox->pack_start (*hpaned, true, true, 0);

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
	hpaned->pack2(*docview_, Gtk::EXPAND);

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
	render->property_xalign () = 0.5;
	Gtk::TreeView::Column *namecol = Gtk::manage(
		new Gtk::TreeView::Column (_("Tags"), *render));
	namecol->add_attribute (render->property_markup (), tagnamecol_);
	namecol->add_attribute (render->property_font_desc (), tagfontcol_);
	tags->append_column (*namecol);
	tags->signal_button_press_event().connect_notify(
		sigc::mem_fun (*this, &RefWindow::tagClicked));
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
	tagbar.set_toolbar_style (Gtk::TOOLBAR_ICONS);
	tagbar.set_show_arrow (false);
	filtervbox->pack_start (tagbar, false, false, 0);

	Gtk::Label *taggerlabel = Gtk::manage (new Gtk::Label (""));
	taggerlabel->set_markup (Glib::ustring("<b> ") + _("This Document") + " </b>");
	taggerlabel->set_alignment (0.0, 0.0);
	vbox->pack_start (*taggerlabel, false, true, 3);

	// The tagger box
	Gtk::VBox *taggervbox = Gtk::manage (new Gtk::VBox);
	taggervbox->set_border_width(6);

	Gtk::ScrolledWindow *taggerscroll = Gtk::manage(new Gtk::ScrolledWindow());
	taggerscroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	taggerscroll->set_shadow_type (Gtk::SHADOW_NONE);
	taggerscroll->add (*taggervbox);
	((Gtk::Viewport *)(taggerscroll->get_child()))->set_shadow_type (Gtk::SHADOW_NONE);
	vbox->pack_start (*taggerscroll, true, true, 0);

	taggerbox_ = taggervbox;
	
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
	actiongroup_->add( Gtk::Action::create("Import",
		_("_Import...")),
  	sigc::mem_fun(*this, &RefWindow::onImport));
	actiongroup_->add( Gtk::ToggleAction::create("WorkOffline",
		_("_Work Offline")));
	actiongroup_->add( Gtk::Action::create("Quit", Gtk::Stock::QUIT),
  	sigc::mem_fun(*this, &RefWindow::onQuit));

	actiongroup_->add ( Gtk::Action::create("EditMenu", _("_Edit")) );

	actiongroup_->add ( Gtk::Action::create("ViewMenu", _("_View")) );
	actiongroup_->add( Gtk::Action::create(
		"PasteBibtex", Gtk::Stock::PASTE, _("_Paste BibTeX"),
		_("Import references from BibTeX on the clipboard")),
  	sigc::bind (sigc::mem_fun(*this, &RefWindow::onPasteBibtex), GDK_SELECTION_PRIMARY));
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

	actiongroup_->add ( Gtk::Action::create("TagMenu", _("_Tags")) );
	actiongroup_->add( Gtk::Action::create(
		"CreateTag", Gtk::Stock::NEW, _("_Create Tag...")), Gtk::AccelKey ("<control>t"),
  	sigc::mem_fun(*this, &RefWindow::onCreateTag));
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
		"AddDocDoi", Gtk::Stock::ADD, _("Add Refere_nce with DOI...")),
  	sigc::mem_fun(*this, &RefWindow::onAddDocByDoi));
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
		"GetMetadataDoc", Gtk::Stock::CONNECT, _("_Get Metadata")),
  	sigc::mem_fun(*this, &RefWindow::onGetMetadataDoc));
	actiongroup_->add( Gtk::Action::create(
		"DeleteDoc", Gtk::Stock::DELETE, _("_Delete File from drive")),
		Gtk::AccelKey ("<control><shift>Delete"),
  	sigc::mem_fun(*this, &RefWindow::onDeleteDoc));
	actiongroup_->add( Gtk::Action::create(
		"RenameDoc", Gtk::Stock::EDIT, _("R_ename File from Key")),
  	sigc::mem_fun(*this, &RefWindow::onRenameDoc));

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
	Glib::ustring ui =
		"<ui>"
		"  <menubar name='MenuBar'>"
		"    <menu action='LibraryMenu'>"
		"      <menuitem action='NewLibrary'/>"
		"      <menuitem action='OpenLibrary'/>"
		"      <separator/>"
		"      <menuitem action='SaveLibrary'/>"
		"      <menuitem action='SaveAsLibrary'/>"
		"      <separator/>"
		"      <menuitem action='ExportBibtex'/>"
		"      <menuitem action='ManageBibtex'/>"
		"      <menuitem action='Import'/>"
		"      <separator/>"
		"      <menuitem action='WorkOffline'/>"
		"      <separator/>"
		"      <menuitem action='Quit'/>"
		"    </menu>"
		"    <menu action='EditMenu'>"
		"      <menuitem action='PasteBibtex'/>"
		"      <menuitem action='CopyCite'/>"
		"      <separator/>"
		"      <menuitem action='Preferences'/>"
		"    </menu>"
		"    <menu action='ViewMenu'>"
		"      <menuitem action='UseIconView'/>"
		"      <menuitem action='UseListView'/>"
		"      <separator/>"
		"      <menuitem action='ShowTagPane'/>"
		"    </menu>"
		"    <menu action='TagMenu'>"
		"      <menuitem action='CreateTag'/>"
		"      <separator/>"
		"      <menuitem action='DeleteTag'/>"
		"      <menuitem action='RenameTag'/>"
		"    </menu>"
		"    <menu action='DocMenu'>"
		"      <menuitem action='AddDocFile'/>"
		"      <menuitem action='AddDocFolder'/>"
		"      <menuitem action='AddDocDoi'/>"
		"      <menuitem action='AddDocUnnamed'/>"
		"      <separator/>"
		"      <menuitem action='OpenDoc'/>"
		"      <menuitem action='DocProperties'/>"
		"      <separator/>"
		"      <menuitem action='RemoveDoc'/>"
		"      <menuitem action='GetMetadataDoc'/>"
		"      <menuitem action='RenameDoc'/>"
		"      <menuitem action='DeleteDoc'/>"
		"    </menu>"
		"    <menu action='HelpMenu'>"
		"      <menuitem action='Introduction'/>"
		"      <menuitem action='About'/>"
		"    </menu>"
		"  </menubar>"
		"  <toolbar name='ToolBar'>"
		"    <toolitem action='NewLibrary'/>"
		"    <toolitem action='OpenLibrary'/>"
		"    <toolitem action='SaveLibrary'/>"
		"    <separator/>"
		"    <toolitem action='CopyCite'/>"
		"    <toolitem action='PasteBibtex'/>"
		"  </toolbar>"
		"  <toolbar name='TagBar'>"
		"    <toolitem action='CreateTag'/>"
		"    <toolitem action='DeleteTag'/>"
		"  </toolbar>"
		"  <toolbar name='DocBar'>"
		"    <toolitem action='AddDocFile'/>"
		"    <toolitem action='RemoveDoc'/>"
		"  </toolbar>"
		"  <popup name='DocPopup'>"
		"    <menuitem action='AddDocFile'/>"
		"    <menuitem action='AddDocFolder'/>"
		"    <separator/>"
		"    <menuitem action='RemoveDoc'/>"
		"    <menuitem action='GetMetadataDoc'/>"
		"    <menuitem action='OpenDoc'/>"
		"    <menuitem action='DocProperties'/>"
		"  </popup>"
		"  <popup name='TagPopup'>"
		"    <menuitem action='CreateTag'/>"
		"    <separator/>"
		"    <menuitem action='DeleteTag'/>"
		"    <menuitem action='RenameTag'/>"
		"  </popup>"
		"  <accelerator action='Find'/>"
		"</ui>";

	uimanager_->add_ui_from_string (ui);

	window_->add_accel_group (uimanager_->get_accel_group ());

}


void RefWindow::clearTagList ()
{
	tagstore_->clear();
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

	taggerbox_->children().clear();

	taggerchecks_.clear();

	std::map <int, int> tagusecounts;

	DocumentList::Container &docrefs = library_->doclist_->getDocs ();
	int const doccount = docrefs.size ();
	DocumentList::Container::iterator docit = docrefs.begin();
	DocumentList::Container::iterator const docend = docrefs.end();
	for (; docit != docend; docit++) {
		std::vector<int>& tags = (*docit).getTags ();
		std::vector<int>::iterator tagit = tags.begin ();
		std::vector<int>::iterator const tagend = tags.end ();
		for (; tagit != tagend; ++tagit) {
			tagusecounts[*tagit]++;
		}
	}

	// Populate from library_->taglist_
	std::vector<Tag> tagvec = library_->taglist_->getTags();
	std::vector<Tag>::iterator it = tagvec.begin();
	std::vector<Tag>::iterator const end = tagvec.end();
	for (; it != end; ++it) {
		Gtk::TreeModel::iterator item = tagstore_->append();
		(*item)[taguidcol_] = (*it).uid_;
		(*item)[tagnamecol_] = (*it).name_;

		int timesused = tagusecounts[(*it).uid_];
		float factor;
		float maxfactor = 1.55;
		if (doccount > 0)
			factor = 0.75 + (logf((float)timesused / (float)doccount + 0.1) - logf(0.1)) * 0.4;
		else
			factor = 1.5;
		if (factor > maxfactor)
			factor = maxfactor;
		//std::cerr << "factor for " << (*it).name_ << " = " << factor << "\n";

		int basesize = window_->get_style ()->get_font().get_size ();
		int size = (int) (factor * (float)basesize);
		Pango::FontDescription font;
		font.set_size (size);
		font.set_weight (Pango::WEIGHT_SEMIBOLD);
		(*item)[tagfontcol_] = font;

		Gtk::ToggleButton *check =
			Gtk::manage (new Gtk::CheckButton ((*it).name_));
		check->signal_toggled().connect(
			sigc::bind(
				sigc::mem_fun (*this, &RefWindow::taggerCheckToggled),
				check,
				(*it).uid_));
		taggerbox_->pack_start (*check, false, false, 1);
		taggerchecks_[(*it).uid_] = check;
	}

	taggerbox_->show_all ();

	// Restore initial selection or selected first row
	ignoreTagSelectionChanged_ = ignore;
	if (!initialpath.empty())
		tagselection_->select (initialpath);

	// If that didn't get anything, select All
	if (tagselection_->get_selected_rows ().size () == 0)
		tagselection_->select (tagstore_->children().begin());


	// To update taggerbox
	docSelectionChanged ();
}


void RefWindow::taggerCheckToggled (Gtk::ToggleButton *check, int taguid)
{
	if (ignoretaggerchecktoggled_)
		return;

	setDirty (true);

	std::vector<Document*> selecteddocs = docview_->getSelectedDocs ();

	bool active = check->get_active ();
	check->set_inconsistent (false);

	bool tagsremoved = false;
	bool tagsadded = false;

	std::vector<Document*>::iterator it = selecteddocs.begin ();
	std::vector<Document*>::iterator const end = selecteddocs.end ();
	for (; it != end; it++) {
		Document *doc = (*it);
		if (active) {
			doc->setTag (taguid);
			tagsadded = true;
		} else {
			doc->clearTag (taguid);
			tagsremoved = true;
		}
	}

	bool allselected = false;
	bool taglessselected = false;

	for (std::vector<int>::iterator tagit = filtertags_.begin();
	     tagit != filtertags_.end ();
	     ++tagit) {
		if (*tagit == ALL_TAGS_UID)
			allselected = true;
		else if (*tagit == NO_TAGS_UID)
			taglessselected = true;
	}

	if (
	    // If we've untagged something it might no longer be visible
	    (tagsremoved && !allselected)
	    // Or if we've added a tag to something while viewing "untagged"
	    || (tagsadded && taglessselected)
	    ) {
		docview_->updateVisible ();
		updateStatusBar ();
	}
	
	// All tag changes influence the fonts in the tag list
	populateTagList ();
}


void RefWindow::tagNameEdited (
	Glib::ustring const &text1,
	Glib::ustring const &text2)
{
	// Text1 is the row number, text2 is the new setting
	Gtk::TreeSelection::ListHandle_Path paths =
		tagselection_->get_selected_rows ();

	if (paths.empty ()) {
		std::cerr << "Warning: RefWindow::tagNameEdited: no tag selected\n";
		return;
	} else if (paths.size () > 1) {
		std::cerr << "Warning: RefWindow::tagNameEdited: too many tags selected\n";
		return;
	}

	Gtk::TreePath path = (*paths.begin ());
	Gtk::ListStore::iterator iter = tagstore_->get_iter (path);

	if ((*iter)[taguidcol_] == ALL_TAGS_UID
	    || (*iter)[taguidcol_] == NO_TAGS_UID)
		return;

	setDirty (true);

	// Should escape this
	Glib::ustring newname = text2;
	(*iter)[tagnamecol_] = newname;
	library_->taglist_->renameTag ((*iter)[taguidcol_], newname);
	taggerchecks_[(*iter)[taguidcol_]]->set_label (newname);
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


void RefWindow::onCreateTag  ()
{
	Glib::ustring newname = (_("Type a tag"));

	int newuid = library_->taglist_->newTag (newname, Tag::ATTACH);

	populateTagList();

	Gtk::TreeModel::iterator it = tagstore_->children().begin();
	Gtk::TreeModel::iterator const end = tagstore_->children().end();
	for (; it != end; ++it) {
		if ((*it)[taguidcol_] == newuid) {
			// Assume tagview's first column is the name field
			tagview_->set_cursor (Gtk::TreePath(it), *tagview_->get_column (0), true);
			break;
		}
	}

	setDirty (true);
}


void RefWindow::onDeleteTag ()
{
	Gtk::TreeSelection::ListHandle_Path paths =
		tagselection_->get_selected_rows ();

	if (paths.empty()) {
		std::cerr << "Warning: RefWindow::onDeleteTag: nothing selected\n";
		return;
	}

	std::vector <int> uidstodelete;

	Gtk::TreeSelection::ListHandle_Path::iterator it = paths.begin ();
	Gtk::TreeSelection::ListHandle_Path::iterator const end = paths.end ();
	for (; it != end; it++) {
		Gtk::TreePath path = (*it);
		Gtk::ListStore::iterator iter = tagstore_->get_iter (path);
		std::cerr << (*iter)[taguidcol_] << std::endl;
		if ((*iter)[taguidcol_] == ALL_TAGS_UID) {
			std::cerr << "Warning: RefWindow::onDeleteTag:"
				" someone tried to delete 'All'\n";
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
			std::cerr << "going to delete " << (*iter)[taguidcol_] << "\n";
			uidstodelete.push_back ((*iter)[taguidcol_]);
		}
	}

	std::vector<int>::iterator uidit = uidstodelete.begin ();
	std::vector<int>::iterator const uidend = uidstodelete.end ();
	for (; uidit != uidend; ++uidit) {
		std::cerr << "Really deleting " << *uidit << "\n";
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
		std::cerr << "Warning: RefWindow::onRenameTag: no tag selected\n";
		return;
	} else if (paths.size () > 1) {
		std::cerr << "Warning: RefWindow::onRenameTag: too many tags selected\n";
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
		std::cerr << "Manage path is " << dialog.get_uri () << "\n";
		std::cerr << "Library path is " << openedlib_ << "\n";
		Glib::ustring relpath = Utility::relPath (openedlib_, dialog.get_uri ());
		std::cerr << "Relative path is " << relpath << "\n";
		// Effect is that we are always setting a UTF-8 filename
		// NOT a URI.
		if (!relpath.empty ()) {
			entry->set_text (Gnome::Vfs::unescape_string (relpath));
		} else {
			entry->set_text (Glib::filename_to_utf8 (dialog.get_filename()));
		}
	}
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

	std::cerr << "Got bibtextarget = " << library_->getBibtexTarget () << "\n";
	Glib::ustring bibfilename =
		Gnome::Vfs::get_local_path_from_uri (library_->getBibtexTarget ());
	std::cerr << "Got absolute path " << bibfilename << "\n";
	// Did we fail?  (if so then it's a relative path)
	if (bibfilename.empty ()) {
		bibfilename = Gnome::Vfs::unescape_string (library_->getBibtexTarget ());
		std::cerr << "Got relative path " << bibfilename << "\n";
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
	std::cerr << "newtarget = " << newtarget << "\n";


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

		std::cerr << "Calling library_->load on " << libfile << "\n";
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
		if (library_->save (openedlib_))
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

		if (library_->save (libfilename)) {
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
	dialog.set_copyright ("Copyright © 2007 John Spray");
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


void RefWindow::addDocFiles (std::vector<Glib::ustring> const &filenames)
{
	Gtk::Dialog dialog (_("Add Document Files"), true, false);

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

	dialog.show_all ();
	vbox->set_border_width (12);

	Glib::ustring progresstext;

	int n = 0;
	std::vector<Glib::ustring>::const_iterator it = filenames.begin();
	std::vector<Glib::ustring>::const_iterator const end = filenames.end();
	for (; it != end; ++it) {
		progress.set_fraction ((float)n / (float)filenames.size());
		progresstext = String::ucompose (_("%1 of %2 documents"), n, filenames.size ());
		progress.set_text (progresstext);
		while (Gnome::Main::events_pending())
			Gnome::Main::iteration ();

		Glib::RefPtr<Gnome::Vfs::Uri> uri = Gnome::Vfs::Uri::create (*it);
		if (!Utility::uriIsFast (uri)) {
			// Should prompt the user to download the file
			std::cerr << "Ooh, a remote uri\n";
		}

		Document *newdoc = library_->doclist_->newDocWithFile(*it);
		if (newdoc) {
			newdoc->readPDF ();

			while (Gnome::Main::events_pending())
				Gnome::Main::iteration ();

			// If we got a DOI or eprint field this will work
			newdoc->getMetaData ();

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
			
			docview_->addDoc (newdoc);
				
		} else {
			std::cerr << "RefWindow::addDocFiles: Warning: didn't succeed adding '" << *it << "'.  Duplicate file?\n";
		}
		++n;
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


void RefWindow::onAddDocByDoi ()
{

	Gtk::Dialog dialog (_("Add Reference with DOI"), true, false);

	Gtk::VBox *vbox = dialog.get_vbox ();

	Gtk::HBox hbox;
	hbox.set_spacing (12);
	vbox->pack_start (hbox, true, true, 0);

	Gtk::Label label (_("DOI:"), false);
	hbox.pack_start (label, false, false, 0);

	Gtk::Entry entry;
	hbox.pack_start (entry, true, true, 0);

	dialog.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dialog.add_button (Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
	dialog.set_default_response (Gtk::RESPONSE_ACCEPT);

	dialog.show_all ();
	vbox->set_border_width (12);

	if (dialog.run () == Gtk::RESPONSE_ACCEPT) {
		setDirty (true);
		Document *newdoc = library_->doclist_->newDocWithDoi (entry.get_text ());

		newdoc->getMetaData ();
		newdoc->setKey (library_->doclist_->uniqueKey (newdoc->generateKey ()));

		docview_->addDoc (newdoc);
		updateStatusBar ();
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
		secondary = _("All tag associations and metadata for these documents "
			"will be permanently lost.");
	}
	else {
		message = String::ucompose (
			_("Are you sure you want to remove '%1'?"),
			(*docs.begin ())->getKey ());
		secondary = _("All tag associations and metadata for the document "
			"will be permanently lost.");
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
		std::cerr << "RefWindow::onRemoveDoc: removeDoc on '" << *it << "'\n";
		docview_->removeDoc (*it);
		library_->doclist_->removeDoc(*it);
	}

	setDirty (true);
}


void RefWindow::onGetMetadataDoc ()
{
	bool doclistdirty = false;
	std::vector <Document*> docs = docview_->getSelectedDocs ();
	std::vector <Document*>::iterator it = docs.begin ();
	std::vector <Document*>::iterator const end = docs.end ();
	for (; it != end; ++it) {
		Document* doc = *it;
		if (doc->canGetMetadata ()) {
			setDirty (true);
			doclistdirty = true;
			doc->getMetaData ();
			docview_->updateDoc(doc);
		}
	}

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


void RefWindow::onDocProperties ()
{
	Document *doc = docview_->getSelectedDoc ();
	if (doc) {
		if (docpropertiesdialog_->show (doc)) {
			setDirty (true);
			docview_->updateDoc (doc);
			updateStatusBar ();
		}
	}
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

	std::string latintext;
	try {
		latintext = Glib::convert (clipboardtext, "iso-8859-1", "UTF8");
	} catch (Glib::ConvertError &ex) {
		Utility::exceptionDialog (&ex, _("Converting clipboard text to latin1"));
		// On conversion failure, try passing UTF-8 straight through
		latintext = clipboardtext;
	}

	int imported =
		library_->doclist_->import (latintext, BibUtils::FORMAT_BIBTEX);

	std::cerr << "Imported " << imported << " references\n";

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
	actiongroup_->get_action("DocProperties")->set_sensitive (onlyoneselected);
	taggerbox_->set_sensitive (somethingselected);

	DocumentView::Capabilities cap
		= docview_->getDocSelectionCapabilities ();
	actiongroup_->get_action("OpenDoc")->set_sensitive (cap.open);
	actiongroup_->get_action("GetMetadataDoc")->set_sensitive (cap.getmetadata);

	ignoretaggerchecktoggled_ = true;
	for (std::vector<Tag>::iterator tagit = library_->taglist_->getTags().begin();
	     tagit != library_->taglist_->getTags().end(); ++tagit) {
		Gtk::ToggleButton *check = taggerchecks_[(*tagit).uid_];
		DocumentView::SubSet state = docview_->selectedDocsHaveTag ((*tagit).uid_);
		if (state == DocumentView::ALL) {
			check->set_active (true);
			check->set_inconsistent (false);
		} else if (state == DocumentView::SOME) {
			check->set_active (false);
			check->set_inconsistent (true);
		} else {
			check->set_active (false);
			check->set_inconsistent (false);
		}
	}
	ignoretaggerchecktoggled_ = false;

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

