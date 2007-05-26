
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */


#include <map>

#include <gtkmm.h>
#include <libgnomeuimm.h>
#include <libgnomevfsmm.h>
#include <glibmm/i18n.h>
#include "ucompose.hpp"

#include "Library.h"
#include "DocumentList.h"
#include "TagList.h"
#include "Document.h"
#include "DocumentProperties.h"
#include "Preferences.h"
#include "LibraryParser.h"
#include "ev-tooltip.h"
#include "icon-entry.h"

#include "config.h"
#include "TagWindow.h"


int main (int argc, char **argv)
{
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	Gnome::Main gui (PROGRAM_NAME, VERSION,
		Gnome::UI::module_info_get(), argc, argv);

	Gnome::Vfs::init ();

	_global_prefs = new Preferences();

	if (argc > 1) {
		Glib::ustring libfile = argv[1];
		if (!Glib::path_is_absolute (libfile)) {
			libfile = Glib::build_filename (Glib::get_current_dir (), libfile);
		}

		libfile = Gnome::Vfs::get_uri_from_local_path (libfile);

		_global_prefs->setLibraryFilename (libfile);
	}

	try {
		TagWindow window;
		window.run();
	} catch (Glib::Error ex) {
		Utility::exceptionDialog (&ex, _("Terminating due to unhandled exception"));
	}

	return 0;
}


TagWindow::TagWindow ()
{
	tagselectionignore_ = false;
	ignoretaggerchecktoggled_ = false;
	docselectionignore_ = false;
	dirty_ = false;

	library_ = new Library (*this);

	usinglistview_ = _global_prefs->getUseListView ();

	docpropertiesdialog_ = new DocumentProperties ();

	constructUI ();

	Glib::ustring const libfile = _global_prefs->getLibraryFilename ();

	if (!libfile.empty() && library_->load (libfile)) {
		setOpenedLib (libfile);
	} else {
		onNewLibrary ();
	}

	hoverdoc_ = NULL;

	setDirty (false);

	populateDocStore ();
	populateTagList ();
}


TagWindow::~TagWindow ()
{
	_global_prefs->setLibraryFilename (openedlib_);

	gtk_widget_destroy (doctooltip_);

	delete library_;
	delete docpropertiesdialog_;

	delete _global_prefs;
}


void TagWindow::run ()
{
	Gnome::Main::run (*window_);
}


void TagWindow::constructUI ()
{
	window_ = new Gtk::Window(Gtk::WINDOW_TOPLEVEL);
	std::pair<int, int> size = _global_prefs->getWindowSize ();
	window_->set_default_size (size.first, size.second);
	window_->signal_delete_event().connect (
		sigc::mem_fun (*this, &TagWindow::onDelete));
	window_->signal_configure_event().connect_notify (
		sigc::mem_fun (*this, &TagWindow::onResize));

	window_->set_icon (
		Gdk::Pixbuf::create_from_file(
			Utility::findDataFile("referencer.svg")));

	constructMenu ();

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);
	window_->add (*vbox);

	vbox->pack_start (*uimanager_->get_widget("/MenuBar"), false, false, 0);
	vbox->pack_start (*uimanager_->get_widget("/ToolBar"), false, false, 0);
	Gtk::HPaned *hpaned = Gtk::manage(new Gtk::HPaned());
	vbox->pack_start (*hpaned, true, true, 0);


	Gtk::Toolbar *toolbar = (Gtk::Toolbar *) uimanager_->get_widget("/ToolBar");

	Gtk::ToolItem *paditem = Gtk::manage (new Gtk::ToolItem);
	paditem->set_expand (true);
	toolbar->append (*paditem);

	Gtk::ToolItem *searchitem = Gtk::manage (new Gtk::ToolItem);
	Gtk::HBox *search = Gtk::manage (new Gtk::HBox);
	Gtk::Label *searchlabel = Gtk::manage (new Gtk::Label (_("_Search:"), true));
	//Gtk::Entry *searchentry = Gtk::manage (new Gtk::Entry);
	Sexy::IconEntry *searchentry = Gtk::manage (new Sexy::IconEntry ());
	searchentry->add_clear_button ();
	searchlabel->set_mnemonic_widget (*searchentry);
	search->set_spacing (6);
	search->pack_start (*searchlabel, false, false, 0);
	search->pack_start (*searchentry, false, false, 0);
	search->show_all ();
	searchitem->add (*search);

	toolbar->append (*searchitem);
	// In order to prevent search box falling off the edge.
	toolbar->set_show_arrow ( false);

	searchentry_ = searchentry;
	searchentry_->signal_changed ().connect (
		sigc::mem_fun (*this, &TagWindow::onSearchChanged));


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

	tagstore_->set_sort_func (tagnamecol_, sigc::mem_fun (*this, &TagWindow::sortTags));
	tagstore_->set_sort_column (tagnamecol_, Gtk::SORT_ASCENDING);

	// Create the treeview for the tag list
	Gtk::TreeView *tags = Gtk::manage(new Gtk::TreeView(tagstore_));
	//tags->append_column("UID", taguidcol_);
	Gtk::CellRendererText *render = Gtk::manage(new Gtk::CellRendererText());
	render->property_editable() = true;
	render->signal_edited().connect (
		sigc::mem_fun (*this, &TagWindow::tagNameEdited));
	render->property_xalign () = 0.5;
	Gtk::TreeView::Column *namecol = Gtk::manage(
		new Gtk::TreeView::Column (_("Tags"), *render));
	namecol->add_attribute (render->property_markup (), tagnamecol_);
	namecol->add_attribute (render->property_font_desc (), tagfontcol_);
	tags->append_column (*namecol);
	tags->signal_button_press_event().connect_notify(
		sigc::mem_fun (*this, &TagWindow::tagClicked));
	tags->set_headers_visible (false);
	tags->set_search_column (tagnamecol_);

	Gtk::ScrolledWindow *tagsscroll = Gtk::manage(new Gtk::ScrolledWindow());
	tagsscroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	tagsscroll->set_shadow_type (Gtk::SHADOW_NONE);
	tagsscroll->add (*tags);
	filtervbox->pack_start(*tagsscroll, true, true, 0);
	
	filtervbox->pack_start (*tags);

	tagview_ = tags;

	tagselection_ = tags->get_selection();
	tagselection_->signal_changed().connect_notify (
		sigc::mem_fun (*this, &TagWindow::tagSelectionChanged));
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

	// The iconview side
	vbox = Gtk::manage(new Gtk::VBox);
	Gtk::Frame *iconsframe = new Gtk::Frame ();
	iconsframe->add (*vbox);
	hpaned->pack2(*iconsframe, Gtk::EXPAND);

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
		sigc::mem_fun (*this, &TagWindow::docActivated));

	icons->signal_button_press_event().connect(
		sigc::mem_fun (*this, &TagWindow::docClicked), false);

	icons->signal_selection_changed().connect(
		sigc::mem_fun (*this, &TagWindow::docSelectionChanged));

	icons->set_selection_mode (Gtk::SELECTION_MULTIPLE);
	/*icons->set_column_spacing (50);
	icons->set_icon_width (10);*/
	icons->set_columns (-1);

	std::vector<Gtk::TargetEntry> dragtypes;
	Gtk::TargetEntry urilist;
	urilist.set_info (0);
	urilist.set_target ("text/uri-list");
	dragtypes.push_back (urilist);
	urilist.set_target ("text/x-moz-url-data");
	dragtypes.push_back (urilist);

	icons->drag_dest_set (
		dragtypes,
		Gtk::DEST_DEFAULT_ALL,
		Gdk::ACTION_COPY | Gdk::ACTION_MOVE | Gdk::ACTION_LINK);

	icons->signal_drag_data_received ().connect (
		sigc::mem_fun (*this, &TagWindow::onIconsDragData));
	icons->set_events (Gdk::POINTER_MOTION_MASK | Gdk::LEAVE_NOTIFY_MASK);
	icons->signal_motion_notify_event ().connect_notify (
		sigc::mem_fun (*this, &TagWindow::onDocMouseMotion));
	icons->signal_leave_notify_event ().connect_notify (
		sigc::mem_fun (*this, &TagWindow::onDocMouseLeave));

	docsiconview_ = icons;

	Gtk::ScrolledWindow *iconsscroll = Gtk::manage(new Gtk::ScrolledWindow());
	iconsscroll->add(*icons);
	iconsscroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	vbox->pack_start(*iconsscroll, true, true, 0);

	docsiconscroll_ = iconsscroll;


	doctooltip_ = ev_tooltip_new (GTK_WIDGET(docsiconview_->gobj()));

	/*Gtk::Toolbar& docbar = (Gtk::Toolbar&) *uimanager_->get_widget("/DocBar");
	vbox->pack_start (docbar, false, false, 0);
	docbar.set_toolbar_style (Gtk::TOOLBAR_ICONS);
	docbar.set_show_arrow (false);*/

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
		sigc::mem_fun (*this, &TagWindow::onIconsDragData));

	table->signal_row_activated ().connect (
		sigc::mem_fun (*this, &TagWindow::docListActivated));

	table->signal_button_press_event().connect(
		sigc::mem_fun (*this, &TagWindow::docClicked), false);

	docslistselection_ = table->get_selection ();
	docslistselection_->set_mode (Gtk::SELECTION_MULTIPLE);
	docslistselection_->signal_changed ().connect (
		sigc::mem_fun (*this, &TagWindow::docSelectionChanged));

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

	// The statusbar
	statusbar_ = Gtk::manage (new Gtk::Statusbar ());
	vbox->pack_start (*statusbar_, false, false, 0);

	progressbar_ = Gtk::manage (new Gtk::ProgressBar ());
	statusbar_->pack_start (*progressbar_, false, false, 0);

	window_->show_all ();


	// usinglistview_ was initialised immediately after new Preferences
	// Initialise widgets and listen for prefs change or user input
	Glib::RefPtr <Gtk::RadioAction>::cast_static(
			actiongroup_->get_action ("UseListView"))->set_active(
				usinglistview_);
	Glib::RefPtr <Gtk::RadioAction>::cast_static(
			actiongroup_->get_action ("UseIconView"))->set_active(
				!usinglistview_);
	onUseListViewPrefChanged ();
	_global_prefs->getUseListViewSignal ().connect (
		sigc::mem_fun (*this, &TagWindow::onUseListViewPrefChanged));

	Glib::RefPtr <Gtk::RadioAction>::cast_static(
		actiongroup_->get_action ("UseListView"))->signal_toggled ().connect (
			sigc::mem_fun(*this, &TagWindow::onUseListViewToggled));

	// Initialise and listen for prefs change or user input
	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("ShowTagPane"))->set_active(
				_global_prefs->getShowTagPane ());
	onShowTagPanePrefChanged ();
	_global_prefs->getShowTagPaneSignal ().connect (
		sigc::mem_fun (*this, &TagWindow::onShowTagPanePrefChanged));

	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("ShowTagPane"))->signal_toggled ().connect (
				sigc::mem_fun(*this, &TagWindow::onShowTagPaneToggled));

	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("WorkOffline"))->set_active(
				_global_prefs->getWorkOffline());
	_global_prefs->getWorkOfflineSignal ().connect (
		sigc::mem_fun (*this, &TagWindow::onWorkOfflinePrefChanged));

	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("WorkOffline"))->signal_toggled ().connect (
				sigc::mem_fun(*this, &TagWindow::onWorkOfflineToggled));
}


void TagWindow::constructMenu ()
{
	actiongroup_ = Gtk::ActionGroup::create();

	actiongroup_->add ( Gtk::Action::create("LibraryMenu", _("_Library")) );
	actiongroup_->add( Gtk::Action::create("NewLibrary",
		Gtk::Stock::NEW),
  	sigc::mem_fun(*this, &TagWindow::onNewLibrary));
	actiongroup_->add( Gtk::Action::create("OpenLibrary",
		Gtk::Stock::OPEN),
  	sigc::mem_fun(*this, &TagWindow::onOpenLibrary));
	actiongroup_->add( Gtk::Action::create("SaveLibrary",
		Gtk::Stock::SAVE),
  	sigc::mem_fun(*this, &TagWindow::onSaveLibrary));
	actiongroup_->add( Gtk::Action::create("SaveAsLibrary",
		Gtk::Stock::SAVE_AS), Gtk::AccelKey ("<control><shift>s"),
  	sigc::mem_fun(*this, &TagWindow::onSaveAsLibrary));
	actiongroup_->add( Gtk::Action::create("ExportBibtex",
		Gtk::Stock::CONVERT, _("E_xport as BibTeX...")), Gtk::AccelKey ("<control>b"),
  	sigc::mem_fun(*this, &TagWindow::onExportBibtex));
	actiongroup_->add( Gtk::Action::create("ManageBibtex",
		Gtk::Stock::CONVERT, _("_Manage BibTeX File...")), Gtk::AccelKey ("<control><shift>b"),
  	sigc::mem_fun(*this, &TagWindow::onManageBibtex));
	actiongroup_->add( Gtk::Action::create("Import",
		_("_Import...")),
  	sigc::mem_fun(*this, &TagWindow::onImport));
	actiongroup_->add( Gtk::ToggleAction::create("WorkOffline",
		_("_Work Offline")));
	actiongroup_->add( Gtk::Action::create("Quit", Gtk::Stock::QUIT),
  	sigc::mem_fun(*this, &TagWindow::onQuit));

	actiongroup_->add ( Gtk::Action::create("EditMenu", _("_Edit")) );

	actiongroup_->add ( Gtk::Action::create("ViewMenu", _("_View")) );
	actiongroup_->add( Gtk::Action::create(
		"PasteBibtex", Gtk::Stock::PASTE, _("_Paste BibTeX"),
		_("Import references from BibTeX on the clipboard")),
  	sigc::bind (sigc::mem_fun(*this, &TagWindow::onPasteBibtex), GDK_SELECTION_PRIMARY));
	actiongroup_->add( Gtk::Action::create(
		"CopyCite", Gtk::Stock::COPY, _("_Copy LaTeX citation"),
		_("Copy currently selected keys to the clipboard as a LaTeX citation")),
  	sigc::mem_fun(*this, &TagWindow::onCopyCite));
	actiongroup_->add( Gtk::Action::create("Preferences",
		Gtk::Stock::PREFERENCES),
  	sigc::mem_fun(*this, &TagWindow::onPreferences));

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
  	sigc::mem_fun(*this, &TagWindow::onCreateTag));
	actiongroup_->add( Gtk::Action::create(
		"DeleteTag", Gtk::Stock::DELETE, _("_Delete Tag")),
  	sigc::mem_fun(*this, &TagWindow::onDeleteTag));
	actiongroup_->add( Gtk::Action::create(
		"RenameTag", Gtk::Stock::EDIT, _("_Rename Tag")),
  	sigc::mem_fun(*this, &TagWindow::onRenameTag));

	actiongroup_->add ( Gtk::Action::create("DocMenu", _("_Documents")) );
	actiongroup_->add( Gtk::Action::create(
		"AddDocFile", Gtk::Stock::ADD, _("_Add File...")),
  	sigc::mem_fun(*this, &TagWindow::onAddDocFile));
	actiongroup_->add( Gtk::Action::create(
		"AddDocFolder", Gtk::Stock::ADD, _("_Add Folder...")),
  	sigc::mem_fun(*this, &TagWindow::onAddDocFolder));
 	actiongroup_->add( Gtk::Action::create(
		"AddDocUnnamed", Gtk::Stock::ADD, _("_Add Empty Reference...")),
  	sigc::mem_fun(*this, &TagWindow::onAddDocUnnamed));
	actiongroup_->add( Gtk::Action::create(
		"AddDocDoi", Gtk::Stock::ADD, _("_Add Reference with DOI...")),
  	sigc::mem_fun(*this, &TagWindow::onAddDocByDoi));
	actiongroup_->add( Gtk::Action::create(
		"RemoveDoc", Gtk::Stock::REMOVE, _("_Remove")),
		Gtk::AccelKey ("<control>Delete"),
  	sigc::mem_fun(*this, &TagWindow::onRemoveDoc));
	actiongroup_->add( Gtk::Action::create(
		"WebLinkDoc", Gtk::Stock::CONNECT, _("_Web Link...")), Gtk::AccelKey ("<control><shift>a"),
  	sigc::mem_fun(*this, &TagWindow::onWebLinkDoc));
	actiongroup_->add( Gtk::Action::create(
		"OpenDoc", Gtk::Stock::OPEN, _("_Open...")), Gtk::AccelKey ("<control>a"),
  	sigc::mem_fun(*this, &TagWindow::onOpenDoc));
	actiongroup_->add( Gtk::Action::create(
		"DocProperties", Gtk::Stock::PROPERTIES), Gtk::AccelKey ("<control>e"),
  	sigc::mem_fun(*this, &TagWindow::onDocProperties));

	actiongroup_->add( Gtk::Action::create(
		"GetMetadataDoc", Gtk::Stock::CONNECT, _("_Get Metadata")),
  	sigc::mem_fun(*this, &TagWindow::onGetMetadataDoc));
	actiongroup_->add( Gtk::Action::create(
		"DeleteDoc", Gtk::Stock::DELETE, _("_Delete File from drive")),
		Gtk::AccelKey ("<control><shift>Delete"),
  	sigc::mem_fun(*this, &TagWindow::onDeleteDoc));
	actiongroup_->add( Gtk::Action::create(
		"RenameDoc", Gtk::Stock::EDIT, _("_Rename File from Key")),
  	sigc::mem_fun(*this, &TagWindow::onRenameDoc));

	actiongroup_->add ( Gtk::Action::create("HelpMenu", _("_Help")) );
	actiongroup_->add( Gtk::Action::create(
		"Introduction", Gtk::Stock::HELP, _("Introduction")),
  	sigc::mem_fun(*this, &TagWindow::onIntroduction));
	actiongroup_->add( Gtk::Action::create(
		"About", Gtk::Stock::ABOUT),
  	sigc::mem_fun(*this, &TagWindow::onAbout));

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
		"      <menuitem action='WebLinkDoc'/>"
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
		"    <toolitem action='ExportBibtex'/>"
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
		"    <menuitem action='WebLinkDoc'/>"
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
		"</ui>";

	uimanager_->add_ui_from_string (ui);

	window_->add_accel_group (uimanager_->get_accel_group ());

}


void TagWindow::populateDocStore ()
{
	//std::cerr << "TagWindow::populateDocStore >>\n";

	// This is our notification that something about the documentlist
	// has changed, including its length, so update dependent sensitivities:
	actiongroup_->get_action("ExportBibtex")
		->set_sensitive (library_->doclist_->size() > 0);
	// Save initial selection
	Gtk::TreePath initialpath;
	if (usinglistview_) {
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
	DocumentList::Container& docvec = library_->doclist_->getDocs();
	DocumentList::Container::iterator docit = docvec.begin();
	DocumentList::Container::iterator const docend = docvec.end();
	for (; docit != docend; ++docit) {
		bool filtered = true;
		for (std::vector<int>::iterator tagit = filtertags_.begin();
		     tagit != filtertags_.end(); ++tagit) {
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
	if (usinglistview_) {
		docslistselection_->select (initialpath);
	} else {
		docsiconview_->select_path (initialpath);
	}

	// If we set the selection in a valid way
	// this probably already got called, although not for
	// opening library etc.
	updateStatusBar ();
}


void TagWindow::populateTagList ()
{
	Gtk::TreeSelection::ListHandle_Path paths =
			tagselection_->get_selected_rows ();
	Gtk::TreePath initialpath;
	if (paths.size() > 0)
		initialpath = *paths.begin ();

	tagselectionignore_ = true;
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
				sigc::mem_fun (*this, &TagWindow::taggerCheckToggled),
				check,
				(*it).uid_));
		taggerbox_->pack_start (*check, false, false, 1);
		taggerchecks_[(*it).uid_] = check;
	}

	taggerbox_->show_all ();

	// Restore initial selection or selected first row
	tagselectionignore_ = false;
	if (!initialpath.empty())
		tagselection_->select (initialpath);

	// If that didn't get anything, select All
	if (tagselection_->get_selected_rows ().size () == 0)
		tagselection_->select (tagstore_->children().begin());


	// To update taggerbox
	docSelectionChanged ();
}


void TagWindow::taggerCheckToggled (Gtk::ToggleButton *check, int taguid)
{
	if (ignoretaggerchecktoggled_)
		return;

	setDirty (true);

	std::vector<Document*> selecteddocs = getSelectedDocs ();

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
		populateDocStore ();
	}
	
	// All tag changes influence the fonts in the tag list
	populateTagList ();


}


void TagWindow::tagNameEdited (
	Glib::ustring const &text1,
	Glib::ustring const &text2)
{
	// Text1 is the row number, text2 is the new setting
	Gtk::TreeSelection::ListHandle_Path paths =
		tagselection_->get_selected_rows ();

	if (paths.empty ()) {
		std::cerr << "Warning: TagWindow::tagNameEdited: no tag selected\n";
		return;
	} else if (paths.size () > 1) {
		std::cerr << "Warning: TagWindow::tagNameEdited: too many tags selected\n";
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


void TagWindow::docActivated (const Gtk::TreeModel::Path& path)
{
	Gtk::ListStore::iterator it = docstore_->get_iter (path);
	Document *doc = (*it)[docpointercol_];
	// The methods we're calling should fail out safely and quietly
	// if the number of docs selected != 1
	if (!doc->getFileName ().empty ()) {
		onOpenDoc ();
	} else if (doc->canWebLink ()) {
		onWebLinkDoc ();
	} else {
		onDocProperties ();
	}
}


void TagWindow::tagSelectionChanged ()
{
	if (tagselectionignore_)
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
				break;
			}
		}
	}

	actiongroup_->get_action("DeleteTag")->set_sensitive (
		anythingselected && !specialselected);
	actiongroup_->get_action("RenameTag")->set_sensitive (
		paths.size() == 1 && !specialselected);

	populateDocStore ();
}


void TagWindow::docSelectionChanged ()
{
	if (docselectionignore_)
		return;

	std::cerr << "TagWindow::docSelectionChanged >>\n";

	int selectcount = getSelectedDocCount ();

	bool const somethingselected = selectcount > 0;
	bool const onlyoneselected = selectcount == 1;

	actiongroup_->get_action("CopyCite")->set_sensitive (somethingselected);

	actiongroup_->get_action("RemoveDoc")->set_sensitive (somethingselected);
	actiongroup_->get_action("DeleteDoc")->set_sensitive (somethingselected);
	actiongroup_->get_action("RenameDoc")->set_sensitive (somethingselected);
	actiongroup_->get_action("DocProperties")->set_sensitive (onlyoneselected);
	taggerbox_->set_sensitive (somethingselected);

	Capabilities cap = getDocSelectionCapabilities ();
	actiongroup_->get_action("WebLinkDoc")->set_sensitive (cap.weblink);
	actiongroup_->get_action("OpenDoc")->set_sensitive (cap.open);
	actiongroup_->get_action("GetMetadataDoc")->set_sensitive (cap.getmetadata);

	ignoretaggerchecktoggled_ = true;
	for (std::vector<Tag>::iterator tagit = library_->taglist_->getTags().begin();
	     tagit != library_->taglist_->getTags().end(); ++tagit) {
		Gtk::ToggleButton *check = taggerchecks_[(*tagit).uid_];
		SubSet state = selectedDocsHaveTag ((*tagit).uid_);
		if (state == ALL) {
			check->set_active (true);
			check->set_inconsistent (false);
		} else if (state == SOME) {
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


void TagWindow::updateStatusBar ()
{
	int selectcount = getSelectedDocCount ();

	bool const somethingselected = selectcount > 0;

	// Update the statusbar text
	int visibledocs = docstore_->children().size();
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


bool TagWindow::docClicked (GdkEventButton* event)
{
  if((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
  	if (usinglistview_) {
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
			(Gtk::Menu*)uimanager_->get_widget("/DocPopup");
		popupmenu->popup (event->button, event->time);

		return true;
  } else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 2)) {
  	// Epic middle click pasting
  	onPasteBibtex (GDK_SELECTION_PRIMARY);

  } else {
  	return false;
  }
}


TagWindow::SubSet TagWindow::selectedDocsHaveTag (int uid)
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


TagWindow::Capabilities TagWindow::getDocSelectionCapabilities ()
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


void TagWindow::tagClicked (GdkEventButton* event)
{
  if((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
		Gtk::Menu *popupmenu =
			(Gtk::Menu*)uimanager_->get_widget("/TagPopup");
		popupmenu->popup (event->button, event->time);
	}
}


void TagWindow::onQuit ()
{
	if (ensureSaved ())
		Gnome::Main::quit ();
}


bool TagWindow::onDelete (GdkEventAny *ev)
{
	if (ensureSaved ())
		return false;
	else
		return true;
}


// Prompts the user to save if necessary, and returns whether
// it is save for the caller to proceed (false if the user
// says to cancel, or saving failed)
bool TagWindow::ensureSaved ()
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


void TagWindow::onCreateTag  ()
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


void TagWindow::onDeleteTag ()
{
	Gtk::TreeSelection::ListHandle_Path paths =
		tagselection_->get_selected_rows ();

	if (paths.empty()) {
		std::cerr << "Warning: TagWindow::onDeleteTag: nothing selected\n";
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
			std::cerr << "Warning: TagWindow::onDeleteTag:"
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


void TagWindow::onRenameTag ()
{
	Gtk::TreeSelection::ListHandle_Path paths =
		tagselection_->get_selected_rows ();

	if (paths.empty ()) {
		std::cerr << "Warning: TagWindow::onRenameTag: no tag selected\n";
		return;
	} else if (paths.size () > 1) {
		std::cerr << "Warning: TagWindow::onRenameTag: too many tags selected\n";
		return;
	}

	Gtk::TreePath path = (*paths.begin ());

	tagview_->set_cursor (path, *tagview_->get_column (0), true);
}


void TagWindow::onExportBibtex ()
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
	combo.set_sensitive (getSelectedDocCount ());

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
			docs = getSelectedDocs ();
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


void TagWindow::manageBrowseDialog (Gtk::Entry *entry)
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


void TagWindow::onManageBibtex ()
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
		sigc::bind(sigc::mem_fun (*this, &TagWindow::manageBrowseDialog), &urientry));

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


void TagWindow::onNewLibrary ()
{
	if (ensureSaved ()) {
		setDirty (false);

		setOpenedLib ("");
		library_->clear ();

		populateDocStore ();
		populateTagList ();
	}
}


void TagWindow::onOpenLibrary ()
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
		std::cerr << "Calling library_->load on " << libfile << "\n";
		if (library_->load (libfile)) {
			setDirty (false);
			populateDocStore ();
			populateTagList ();
			setOpenedLib (libfile);
		} else {
			//library_->load would have shown an exception error dialog
		}

	}
}


void TagWindow::onSaveLibrary ()
{
	if (openedlib_.empty()) {
		onSaveAsLibrary ();
	} else {
		if (library_->save (openedlib_))
			setDirty (false);
	}
}


void TagWindow::onSaveAsLibrary ()
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


void TagWindow::onAbout ()
{
	Gtk::AboutDialog dialog;
	std::vector<Glib::ustring> authors;
	Glib::ustring me = "John Spray";
	authors.push_back(me);
	dialog.set_authors (authors);
	dialog.set_name (PROGRAM_NAME);
	dialog.set_version (VERSION);
	dialog.set_comments ("A document organiser and bibliography manager");
	dialog.set_copyright ("Copyright © 2007 John Spray");
	dialog.set_website ("http://icculus.org/referencer/");
	dialog.set_translator_credits (_("translator-credits"));
	dialog.set_logo (
		Gdk::Pixbuf::create_from_file (
			Utility::findDataFile ("referencer.svg"),
			128, 128));
	dialog.run ();
}


void TagWindow::onIntroduction ()
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


void TagWindow::addDocFiles (std::vector<Glib::ustring> const &filenames)
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

			newdoc->setKey (library_->doclist_->uniqueKey (newdoc->generateKey ()));
		} else {
			std::cerr << "TagWindow::addDocFiles: Warning: didn't succeed adding '" << *it << "'.  Duplicate file?\n";
		}
		++n;
	}

	if (!filenames.empty()) {
		// We added something
		// Should check if we actually added something in case a newDoc
		// failed, eg if the doc was already in there
		setDirty (true);
		populateDocStore ();
	}

}


void TagWindow::onAddDocUnnamed ()
{
	setDirty (true);
	Document *newdoc = library_->doclist_->newDocUnnamed ();
	newdoc->setKey (library_->doclist_->uniqueKey (newdoc->generateKey ()));
	docpropertiesdialog_->show (newdoc);
	populateDocStore ();
}


void TagWindow::onAddDocByDoi ()
{

	Gtk::Dialog dialog (_("Add Document with DOI"), true, false);

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

		populateDocStore ();
	}
}


void TagWindow::onAddDocFile ()
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


void TagWindow::onAddDocFolder ()
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


std::vector<Document*> TagWindow::getSelectedDocs ()
{
	std::vector<Document*> docpointers;

	if (usinglistview_) {
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


Document *TagWindow::getSelectedDoc ()
{
	if (usinglistview_) {
		Gtk::TreeSelection::ListHandle_Path paths =
			docslistselection_->get_selected_rows ();

		if (paths.size() != 1) {
			std::cerr << "Warning: TagWindow::getSelectedDoc: size != 1\n";
			return false;
		}

		Gtk::TreePath path = (*paths.begin ());
		Gtk::ListStore::iterator iter = docstore_->get_iter (path);
		return (*iter)[docpointercol_];

	} else {
		Gtk::IconView::ArrayHandle_TreePaths paths =
			docsiconview_->get_selected_items ();

		if (paths.size() != 1) {
			std::cerr << "Warning: TagWindow::getSelectedDoc: size != 1\n";
			return false;
		}

		Gtk::TreePath path = (*paths.begin ());
		Gtk::ListStore::iterator iter = docstore_->get_iter (path);
		return (*iter)[docpointercol_];
	}
}


int TagWindow::getSelectedDocCount ()
{
	if (usinglistview_) {
		return docslistselection_->get_selected_rows().size ();
	} else {
		return docsiconview_->get_selected_items().size ();
	}
}


void TagWindow::onRemoveDoc ()
{
	std::vector<Document*> docs = getSelectedDocs ();

	bool const multiple = docs.size() > 1;

	bool doclistdirty = false;

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
		std::cerr << "TagWindow::onRemoveDoc: removeDoc on '" << *it << "'\n";
		library_->doclist_->removeDoc(*it);
		doclistdirty = true;
	}

	if (doclistdirty) {
		setDirty (true);
		std::cerr << "TagWindow::onRemoveDoc: dirty, calling populateDocStore\n";
		// We disable docSelectionChanged because otherwise it gets called N
		// times for deleting N items
		docselectionignore_ = true;
		populateDocStore ();
		docselectionignore_ = false;
		docSelectionChanged ();
	} else {
		docselectionignore_ = false;
	}
}


void TagWindow::onWebLinkDoc ()
{
	std::vector <Document*> docs = getSelectedDocs ();
	std::vector <Document*>::iterator it = docs.begin ();
	std::vector <Document*>::iterator const end = docs.end ();
	for (; it != end; ++it) {
		Document* doc = *it;
		if (!doc->getBibData().getDoi().empty()) {
			Utility::StringPair ends = _global_prefs->getDoiLaunch ();
			Glib::ustring doi = doc->getBibData().getDoi();
			Gnome::Vfs::url_show (ends.first + doi + ends.second);
		} else if (!doc->getBibData().getExtras()["eprint"].empty()) {
			Glib::ustring eprint = doc->getBibData().getExtras()["eprint"];
			Gnome::Vfs::url_show ("http://arxiv.org/abs/" + eprint);
		} else {
			std::cerr << "Warning: TagWindow::onWebLinkDoc: nothing to link on\n";
		}
	}
}


void TagWindow::onGetMetadataDoc ()
{
	bool doclistdirty = false;
	std::vector <Document*> docs = getSelectedDocs ();
	std::vector <Document*>::iterator it = docs.begin ();
	std::vector <Document*>::iterator const end = docs.end ();
	for (; it != end; ++it) {
		Document* doc = *it;
		if (doc->canGetMetadata ()) {
			setDirty (true);
			doclistdirty = true;
			doc->getMetaData ();
		}
	}

	if (doclistdirty)
		populateDocStore ();
}


void TagWindow::onRenameDoc ()
{
	bool doclistdirty = false;
	std::vector <Document*> docs = getSelectedDocs ();

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
		doclistdirty = true;
	}

	if (doclistdirty) {
		setDirty (true);
		populateDocStore ();
	}
}


void TagWindow::onDeleteDoc ()
{
	std::vector<Document*> docs = getSelectedDocs ();
	bool const multiple = docs.size() > 1;
	bool doclistdirty = false;
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
			library_->doclist_->removeDoc(*it);
			doclistdirty = true;
		} catch (Glib::Exception &ex) {
			Utility::exceptionDialog (&ex,
				String::ucompose (_("Deleting '%1'"), (*it)->getFileName ()));
		}

	}

	if (doclistdirty) {
		setDirty (true);
		std::cerr << "TagWindow::onDeleteDoc: dirty, calling populateDocStore\n";
		// We disable docSelectionChanged because otherwise it gets called N
		// times for deleting N items
		docselectionignore_ = true;
		populateDocStore ();
		docselectionignore_ = false;
		docSelectionChanged ();
	} else {
		docselectionignore_ = false;
	}
}


void TagWindow::onOpenDoc ()
{
	std::vector<Document*> docs = getSelectedDocs ();
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


void TagWindow::onDocProperties ()
{
	Document *doc = getSelectedDoc ();
	if (doc) {
		if (docpropertiesdialog_->show (doc)) {
			setDirty (true);
			populateDocStore ();
		}
	}
}


void TagWindow::onPreferences ()
{
	_global_prefs->showDialog ();
}


void TagWindow::onIconsDragData (
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

		gchar *utf8;

		utf8 = g_utf16_to_utf8((gunichar2 *) sel.get_data(),
				(glong) sel.get_length(),
				NULL, NULL, NULL);

		uris.push_back (utf8);
		g_free(utf8);
	}

	urilist::iterator it = uris.begin ();
	urilist::iterator const end = uris.end ();
	for (; it != end; ++it) {
		bool is_dir = false;
		Glib::RefPtr<Gnome::Vfs::Uri> uri = Gnome::Vfs::Uri::create (*it);

		//if (uri->is_local()) {
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
		if (info->get_type() == Gnome::Vfs::FILE_TYPE_DIRECTORY)
			is_dir = true;

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

	addDocFiles (files);
}


void TagWindow::onUseListViewToggled ()
{
	_global_prefs->setUseListView (
		Glib::RefPtr <Gtk::RadioAction>::cast_static(
			actiongroup_->get_action ("UseListView"))->get_active ());
}


void TagWindow::onUseListViewPrefChanged ()
{
	usinglistview_ = _global_prefs->getUseListView ();
	if (usinglistview_) {
		Glib::RefPtr <Gtk::RadioAction>::cast_static(
			actiongroup_->get_action ("UseListView"))->set_active (true);
		docsiconscroll_->hide ();
		docslistscroll_->show ();
		docslistview_->grab_focus ();
	} else {
		Glib::RefPtr <Gtk::RadioAction>::cast_static(
			actiongroup_->get_action ("UseIconView"))->set_active (true);
		docslistscroll_->hide ();
		docsiconscroll_->show ();
		docsiconview_->grab_focus ();
	}

	docSelectionChanged ();
}


void TagWindow::onShowTagPaneToggled ()
{
	_global_prefs->setShowTagPane (
		Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("ShowTagPane"))->get_active ());
}


void TagWindow::onShowTagPanePrefChanged ()
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


void TagWindow::onWorkOfflineToggled ()
{
	_global_prefs->setWorkOffline (
		Glib::RefPtr <Gtk::ToggleAction>::cast_static(
			actiongroup_->get_action ("WorkOffline"))->get_active ());
}


void TagWindow::onWorkOfflinePrefChanged ()
{
	Glib::RefPtr <Gtk::ToggleAction>::cast_static(
		actiongroup_->get_action ("WorkOffline"))->set_active (
			_global_prefs->getWorkOffline ());

	// To pick up sensitivity changes
	populateDocStore ();
}


void TagWindow::updateTitle ()
{
	Glib::ustring filename;
	if (openedlib_.empty ()) {
		filename = _("Unnamed Library");
	} else {
		filename = Gnome::Vfs::unescape_string_for_display (Glib::path_get_basename (openedlib_));
		int pos = filename.find (".reflib");
		if (pos != Glib::ustring::npos) {
			filename = filename.substr (0, pos);
		}
	}
	window_->set_title (
		(getDirty () ? "*" : "")
		+ filename
		+ " - "
		+ PROGRAM_NAME);
}


void TagWindow::setOpenedLib (Glib::ustring const &openedlib)
{
	openedlib_ = openedlib;
	updateTitle();
}

void TagWindow::setDirty (bool const &dirty)
{
	dirty_ = dirty;
	actiongroup_->get_action("SaveLibrary")
		->set_sensitive (dirty_);
	updateTitle ();
}


void TagWindow::onImport ()
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

		populateDocStore ();
		populateTagList ();
	}
}


// Selection should be GDK_SELECTION_CLIPBOARD for the windows style
// clipboard and GDK_SELECTION_PRIMARY for middle-click style
void TagWindow::onPasteBibtex (GdkAtom selection)
{
	// Should have sensitivity changing for this
	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get (selection);

/*
Gtk::Clipboard reference:
If you don't want to deal with providing a separate callbac		"    <toolitem action='ExportBibtex'/>"k, you can also use wait_for_contents(). This runs the GLib main loop recursively waiting for the contents. This can simplify the code flow, but you still have to be aware that other callbacks in your program can be called while this recursive mainloop is running.
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
		// Should push to the statusbar how many we got
		populateDocStore ();
		populateTagList ();
		statusbar_->push (String::ucompose
			(_("Imported %1 BibTeX references"), imported), 0);
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


void TagWindow::onSearchChanged ()
{
	populateDocStore ();
}


void TagWindow::onCopyCite ()
{
	std::vector<Document*> docs = getSelectedDocs ();
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


void TagWindow::setSensitive (bool const sensitive)
{
	window_->set_sensitive (sensitive);
}


void TagWindow::onDocMouseMotion (GdkEventMotion* event)
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


void TagWindow::onDocMouseLeave (GdkEventCrossing *event)
{
	ev_tooltip_deactivate (EV_TOOLTIP (doctooltip_));
}


int TagWindow::sortTags (
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
}


void TagWindow::onResize (GdkEventConfigure *event)
{
	_global_prefs->setWindowSize (
		std::pair<int,int> (
			event->width,
			event->height));
}

