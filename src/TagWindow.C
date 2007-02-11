
#include <gtkmm.h>
#include <libgnomeuimm.h>
#include <libgnomevfsmm.h>
#include <poppler/GlobalParams.h>

// for ostringstream
#include <sstream>

#include "TagWindow.h"
#include "TagList.h"
#include "DocumentList.h"
#include "Document.h"
#include "DocumentProperties.h"
#include "Preferences.h"

#include "LibraryParser.h"

int main (int argc, char **argv)
{
	Gnome::Main gui (PROGRAM_NAME, VERSION,
		Gnome::UI::module_info_get(), argc, argv);

	Gnome::Vfs::init ();

	// Initialise libpoppler
	globalParams = new GlobalParams (NULL);

	_global_prefs = new Preferences();

	if (argc > 1) {
		Glib::ustring libfile = argv[1];
		if (!Glib::path_is_absolute (libfile)) {
			libfile = Glib::build_filename (Glib::get_current_dir (), libfile);
		}
		// Naughty, should support other protocols too
		if (libfile.find ("file://") != 0) {
			libfile = "file://" + libfile;
		}
		Gnome::Vfs::escape_string (libfile);

		_global_prefs->setLibraryFilename (libfile);
	}

	try {
		TagWindow window;
		window.run();
	} catch (Glib::Error ex) {
		Utility::exceptionDialog (&ex, "failing fatally");
	}

	return 0;
}


TagWindow::TagWindow ()
{
	tagselectionignore_ = false;
	ignoretaggerchecktoggled_ = false;
	docselectionignore_ = false;
	dirty_ = false;

	taglist_ = new TagList();
	doclist_ = new DocumentList();

	usinglistview_ = _global_prefs->getUseListView ();

	docpropertiesdialog_ = new DocumentProperties ();

	constructUI ();

	Glib::ustring const libfile = _global_prefs->getLibraryFilename ();

	if (!libfile.empty() && loadLibrary (libfile)) {
		setOpenedLib (libfile);
	} else {
		onNewLibrary ();
	}
	
	setDirty (false);

	populateDocStore ();
	populateTagList ();
}


TagWindow::~TagWindow ()
{
	_global_prefs->setLibraryFilename (openedlib_);

	delete taglist_;
	delete doclist_;
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
	window_->set_default_size (700, 500);
	window_->signal_delete_event().connect (
		sigc::mem_fun (*this, &TagWindow::onDelete));

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
	Gtk::Label *searchlabel = Gtk::manage (new Gtk::Label ("Search:"));
	Gtk::Entry *searchentry = Gtk::manage (new Gtk::Entry);
	search->set_spacing (6);
	search->pack_start (*searchlabel, false, false, 0);
	search->pack_start (*searchentry, false, false, 0);
	search->show_all ();
	searchitem->add (*search);
	toolbar->append (*searchitem);
	
	searchentry_ = searchentry;
	searchentry_->signal_changed ().connect (
		sigc::mem_fun (*this, &TagWindow::onSearchChanged));
	

	vbox = Gtk::manage(new Gtk::VBox);
	Gtk::Frame *tagsframe = new Gtk::Frame ();
	tagsframe->add(*vbox);
	hpaned->pack1(*tagsframe, Gtk::FILL);

	tagpane_ = tagsframe;

	Gtk::Label *tagslabel = Gtk::manage (new Gtk::Label (""));
	tagslabel->set_markup ("<b> Tags </b>");
	tagslabel->set_alignment (0.0, 0.0);
	vbox->pack_start (*tagslabel, false, true, 3);

	Gtk::VBox *filtervbox = Gtk::manage (new Gtk::VBox);
	vbox->pack_start (*filtervbox, true, true, 0);

	// Create the store for the tag list
	Gtk::TreeModel::ColumnRecord tagcols;
	tagcols.add(taguidcol_);
	tagcols.add(tagnamecol_);
	tagstore_ = Gtk::ListStore::create(tagcols);

	// Create the treeview for the tag list
	Gtk::TreeView *tags = Gtk::manage(new Gtk::TreeView(tagstore_));
	//tags->append_column("UID", taguidcol_);
	Gtk::CellRendererText *render = Gtk::manage(new Gtk::CellRendererText());
	render->property_editable() = true;
	render->signal_edited().connect (
		sigc::mem_fun (*this, &TagWindow::tagNameEdited));
	Gtk::TreeView::Column *namecol = Gtk::manage(
		new Gtk::TreeView::Column ("Tags", *render));
	namecol->add_attribute (render->property_markup (), tagnamecol_);
	tags->append_column (*namecol);
	tags->signal_button_press_event().connect_notify(
		sigc::mem_fun (*this, &TagWindow::tagClicked));
	tags->set_headers_visible (false);

	Gtk::ScrolledWindow *tagsscroll = Gtk::manage(new Gtk::ScrolledWindow());
	tagsscroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	tagsscroll->set_shadow_type (Gtk::SHADOW_NONE);
	tagsscroll->add (*tags);
	filtervbox->pack_start(*tagsscroll, true, true, 0);

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
	taggerlabel->set_markup ("<b> This Document </b>");
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

	docsiconview_ = icons;

	Gtk::ScrolledWindow *iconsscroll = Gtk::manage(new Gtk::ScrolledWindow());
	iconsscroll->add(*icons);
	iconsscroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	vbox->pack_start(*iconsscroll, true, true, 0);

	docsiconscroll_ = iconsscroll;

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
	col = Gtk::manage (new Gtk::TreeViewColumn ("Key", dockeycol_));
	col->set_resizable (true);
	col->set_sort_column (dockeycol_);
	table->append_column (*col);
	col = Gtk::manage (new Gtk::TreeViewColumn ("Title", doctitlecol_));
	col->set_resizable (true);
	col->set_expand (true);
	col->set_sort_column (doctitlecol_);
	cell = (Gtk::CellRendererText *) col->get_first_cell_renderer ();
	cell->property_ellipsize () = Pango::ELLIPSIZE_END;
	table->append_column (*col);
	col = Gtk::manage (new Gtk::TreeViewColumn ("Authors", docauthorscol_));
	col->set_resizable (true);
	//col->set_expand (true);
	col->set_sort_column (docauthorscol_);
	cell = (Gtk::CellRendererText *) col->get_first_cell_renderer ();
	cell->property_ellipsize () = Pango::ELLIPSIZE_END;
	table->append_column (*col);
	col = Gtk::manage (new Gtk::TreeViewColumn ("Year  ", docyearcol_));
	col->set_resizable (true);
	col->set_sort_column (docyearcol_);
	table->append_column (*col);

	docslistview_ = table;

	Gtk::ScrolledWindow *tablescroll = Gtk::manage(new Gtk::ScrolledWindow());
	tablescroll->add(*table);
	tablescroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	vbox->pack_start(*tablescroll, true, true, 0);

	docslistscroll_ = tablescroll;

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

	actiongroup_->add ( Gtk::Action::create("LibraryMenu", "_Library") );
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
		Gtk::Stock::SAVE_AS),
  	sigc::mem_fun(*this, &TagWindow::onSaveAsLibrary));
	actiongroup_->add( Gtk::Action::create("ExportBibtex",
		Gtk::Stock::CONVERT, "E_xport as BibTeX..."),
  	sigc::mem_fun(*this, &TagWindow::onExportBibtex));
	actiongroup_->add( Gtk::Action::create("Import",
		"_Import..."),
  	sigc::mem_fun(*this, &TagWindow::onImport));
	actiongroup_->add( Gtk::ToggleAction::create("WorkOffline",
		"_Work Offline"));
	actiongroup_->add( Gtk::Action::create("Preferences",
		Gtk::Stock::PREFERENCES),
  	sigc::mem_fun(*this, &TagWindow::onPreferences));
	actiongroup_->add( Gtk::Action::create("Quit", Gtk::Stock::QUIT),
  	sigc::mem_fun(*this, &TagWindow::onQuit));

	actiongroup_->add ( Gtk::Action::create("ViewMenu", "_View") );
	Gtk::RadioButtonGroup group;
	actiongroup_->add( Gtk::RadioAction::create(group, "UseListView",
		"Use _List View"));
	actiongroup_->add( Gtk::RadioAction::create(group, "UseIconView",
		"Use _Icon View"));
	actiongroup_->add( Gtk::ToggleAction::create("ShowTagPane",
		"_Show Tag Pane"));

	actiongroup_->add ( Gtk::Action::create("TagMenu", "_Tags") );
	actiongroup_->add( Gtk::Action::create(
		"CreateTag", Gtk::Stock::NEW, "_Create Tag..."),
  	sigc::mem_fun(*this, &TagWindow::onCreateTag));
	actiongroup_->add( Gtk::Action::create(
		"DeleteTag", Gtk::Stock::DELETE, "_Delete Tag"),
  	sigc::mem_fun(*this, &TagWindow::onDeleteTag));
	actiongroup_->add( Gtk::Action::create(
		"RenameTag", Gtk::Stock::EDIT, "_Rename Tag"),
  	sigc::mem_fun(*this, &TagWindow::onRenameTag));

	actiongroup_->add ( Gtk::Action::create("DocMenu", "_Documents") );
	actiongroup_->add( Gtk::Action::create(
		"AddDocFile", Gtk::Stock::ADD, "_Add File..."),
  	sigc::mem_fun(*this, &TagWindow::onAddDocFile));
	actiongroup_->add( Gtk::Action::create(
		"AddDocFolder", Gtk::Stock::ADD, "_Add Folder..."),
  	sigc::mem_fun(*this, &TagWindow::onAddDocFolder));
 	actiongroup_->add( Gtk::Action::create(
		"AddDocUnnamed", Gtk::Stock::ADD, "_Add Empty Reference..."),
  	sigc::mem_fun(*this, &TagWindow::onAddDocUnnamed));
	actiongroup_->add( Gtk::Action::create(
		"AddDocDoi", Gtk::Stock::ADD, "_Add Reference with DOI..."),
  	sigc::mem_fun(*this, &TagWindow::onAddDocByDoi));
	actiongroup_->add( Gtk::Action::create(
		"RemoveDoc", Gtk::Stock::REMOVE, "_Remove"),
  	sigc::mem_fun(*this, &TagWindow::onRemoveDoc));
	actiongroup_->add( Gtk::Action::create(
		"WebLinkDoc", Gtk::Stock::CONNECT, "_Web Link..."),
  	sigc::mem_fun(*this, &TagWindow::onWebLinkDoc));
	actiongroup_->add( Gtk::Action::create(
		"GetMetadataDoc", Gtk::Stock::CONNECT, "_Get Metadata"),
  	sigc::mem_fun(*this, &TagWindow::onGetMetadataDoc));
	actiongroup_->add( Gtk::Action::create(
		"OpenDoc", Gtk::Stock::OPEN, "_Open..."),
  	sigc::mem_fun(*this, &TagWindow::onOpenDoc));
	actiongroup_->add( Gtk::Action::create(
		"DocProperties", Gtk::Stock::PROPERTIES),
  	sigc::mem_fun(*this, &TagWindow::onDocProperties));

	actiongroup_->add ( Gtk::Action::create("HelpMenu", "_Help") );
	actiongroup_->add( Gtk::Action::create(
		"Introduction", Gtk::Stock::HELP, "Introduction"),
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
		"      <menuitem action='Import'/>"
		"      <separator/>"
		"      <menuitem action='WorkOffline'/>"
		"      <menuitem action='Preferences'/>"
		"      <separator/>"
		"      <menuitem action='Quit'/>"
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
		"      <menuitem action='RemoveDoc'/>"
		"      <menuitem action='WebLinkDoc'/>"
		"      <menuitem action='GetMetadataDoc'/>"
		"      <menuitem action='OpenDoc'/>"
		"      <menuitem action='DocProperties'/>"
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
		"    <toolitem action='ExportBibtex'/>"
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
		->set_sensitive (doclist_->size() > 0);
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

	// Populate from doclist_
	std::vector<Document>& docvec = doclist_->getDocs();
	std::vector<Document>::iterator docit = docvec.begin();
	std::vector<Document>::iterator const docend = docvec.end();
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

	Gtk::TreeModel::iterator all = tagstore_->append();
	(*all)[taguidcol_] = ALL_TAGS_UID;
	(*all)[tagnamecol_] = "<b>All</b>";
	
	Gtk::TreeModel::iterator none = tagstore_->append();
	(*none)[taguidcol_] = NO_TAGS_UID;
	(*none)[tagnamecol_] = "<b>Untagged</b>";

	taggerbox_->children().clear();

	taggerchecks_.clear();

	// Populate from taglist_
	std::vector<Tag> tagvec = taglist_->getTags();
	std::vector<Tag>::iterator it = tagvec.begin();
	std::vector<Tag>::iterator const end = tagvec.end();
	for (; it != end; ++it) {
		Gtk::TreeModel::iterator item = tagstore_->append();
		(*item)[taguidcol_] = (*it).uid_;
		(*item)[tagnamecol_] = (*it).name_;

		std::cerr << "Creating check for '" << (*it).name_ << "'\n";
		Gtk::CheckButton *check =
			Gtk::manage (new Gtk::CheckButton ((*it).name_));
		check->signal_toggled().connect(
			sigc::bind(
				sigc::mem_fun (*this, &TagWindow::taggerCheckToggled),
				check,
				(*it).uid_));
		taggerbox_->pack_start (*check, false, false, 6);
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


void TagWindow::taggerCheckToggled (Gtk::CheckButton *check, int taguid)
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
	taglist_->renameTag ((*iter)[taguidcol_], newname);
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

	actiongroup_->get_action("RemoveDoc")->set_sensitive (somethingselected);
	actiongroup_->get_action("DocProperties")->set_sensitive (onlyoneselected);
	taggerbox_->set_sensitive (somethingselected);

	Capabilities cap = getDocSelectionCapabilities ();
	actiongroup_->get_action("WebLinkDoc")->set_sensitive (cap.weblink);
	actiongroup_->get_action("OpenDoc")->set_sensitive (cap.open);
	actiongroup_->get_action("GetMetadataDoc")->set_sensitive (cap.getmetadata);

	ignoretaggerchecktoggled_ = true;
	for (std::vector<Tag>::iterator tagit = taglist_->getTags().begin();
	     tagit != taglist_->getTags().end(); ++tagit) {
		Gtk::CheckButton *check = taggerchecks_[(*tagit).uid_];
		YesNoMaybe state = selectedDocsHaveTag ((*tagit).uid_);
		if (state == YES) {
			check->set_active (true);
			check->set_inconsistent (false);
		} else if (state == MAYBE) {
			check->set_active (false);
			check->set_inconsistent (true);
		} else {
			check->set_active (false);
			check->set_inconsistent (false);
		}
	}
	ignoretaggerchecktoggled_ = false;
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

			if (!clickedpath.empty()) {
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
  } else {
  	return false;
  }
}


TagWindow::YesNoMaybe TagWindow::selectedDocsHaveTag (int uid)
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
		return NO;
	else if (alltrue == true)
		return YES;
	else if (alltrue == false && allfalse == false)
		return MAYBE;
	else
		return NO;
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
	if (ensureSaved ("closing"))
		Gnome::Main::quit ();
}


bool TagWindow::onDelete (GdkEventAny *ev)
{
	if (ensureSaved ("closing"))
		return false;
	else
		return true;
}


// Prompts the user to save if necessary, and returns whether
// it is save for the caller to proceed (false if the user
// says to cancel, or saving failed)
bool TagWindow::ensureSaved (Glib::ustring const & action)
{
	if (getDirty ()) {
		Gtk::MessageDialog dialog (
			"<b><big>Save changes to library before " + action + "?</big></b>",
			true,
			Gtk::MESSAGE_WARNING,
			Gtk::BUTTONS_NONE,
			true);

		dialog.add_button ("Close _without Saving", 0);
		dialog.add_button (Gtk::Stock::CANCEL, 1);
		dialog.add_button (Gtk::Stock::SAVE, 2);

		int const result = dialog.run ();

		if (result == 0) {
			return true;
		} else if (result == 1) {
			return false;
		} else /*if (result == 2)*/ {
			if (openedlib_.empty ()) {
				onSaveAsLibrary ();
				if (openedlib_.empty ()) {
					// The user cancelled
					return false;
				}
			} else {
				if (!saveLibrary (openedlib_)) {
					// Don't lose data
					return false;
				}
			}

			return true;
		}
	} else {
		return true;
	}
}


void TagWindow::onCreateTag  ()
{
	// For intelligent tags we'll need a dialog here.
	Glib::ustring message = "<b><big>New tag</big></b>\n\n";
	Gtk::MessageDialog dialog(message, true, Gtk::MESSAGE_QUESTION,
		Gtk::BUTTONS_NONE, true);

	dialog.add_button (Gtk::Stock::CANCEL, 0);
	dialog.add_button (Gtk::Stock::OK, 1);
	dialog.set_default_response (1);

	Gtk::Entry nameentry;
	nameentry.set_activates_default (true);
	Gtk::HBox hbox;
	hbox.set_spacing (6);
	Gtk::Label label ("Name:");
	hbox.pack_start (label, false, false, 0);
	hbox.pack_start (nameentry, true, true, 0);
	dialog.get_vbox()->pack_start (hbox, false, false, 0);
	hbox.show_all ();

	bool invalid = true;

	while (invalid && dialog.run()) {
		Glib::ustring newname = nameentry.get_text ();
		// Later displayed in markup'd cellrenderer
		newname = Glib::Markup::escape_text (newname);
		if (newname.empty()) {
			invalid = true;
		} else {
			invalid = false;
			setDirty (true);
			taglist_->newTag (newname, Tag::ATTACH);
			populateTagList();
		}

	}
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

		Glib::ustring message =
			"<b><big>Are you sure you want to delete \""
			+ (*iter)[tagnamecol_]
			+"\"?</big></b>\n\n"
			+ "When a tag is deleted it is also permanently removed "
			+ "from all documents it is currently associated with";

		Gtk::MessageDialog confirmdialog (
			message, true, Gtk::MESSAGE_QUESTION,
			Gtk::BUTTONS_NONE, true);

		confirmdialog.add_button (Gtk::Stock::CANCEL, 0);
		confirmdialog.add_button (Gtk::Stock::DELETE, 1);
		confirmdialog.set_default_response (0);

		if (confirmdialog.run ()) {
			std::cerr << "going to delete " << (*iter)[taguidcol_] << "\n";
			uidstodelete.push_back ((*iter)[taguidcol_]);
		}
	}

	std::vector<int>::iterator uidit = uidstodelete.begin ();
	std::vector<int>::iterator const uidend = uidstodelete.end ();
	for (; uidit != uidend; ++uidit) {
		std::cerr << "Really deleting " << *uidit << "\n";
		// Take it off any docs
		doclist_->clearTag(*uidit);
		// Remove it from the tag list
		taglist_->deleteTag (*uidit);
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
		"Export BibTeX",
		Gtk::FILE_CHOOSER_ACTION_SAVE);
	chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_REJECT);
	chooser.add_button (Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	chooser.set_default_response (Gtk::RESPONSE_ACCEPT);
	chooser.set_do_overwrite_confirmation (true);

	Gtk::FileFilter bibtexfiles;
	bibtexfiles.add_pattern ("*.[bB][iI][bB]");
	bibtexfiles.set_name ("BibTeX Files");
	chooser.add_filter (bibtexfiles);

	Gtk::FileFilter allfiles;
	allfiles.add_pattern ("*.*");
	allfiles.set_name ("All Files");
	chooser.add_filter (allfiles);
	
	Gtk::HBox extrabox;
	extrabox.set_spacing (6);
	chooser.set_extra_widget (extrabox);
	Gtk::Label label ("Selection:");
	extrabox.pack_start (label, false, false, 0);
	Gtk::ComboBoxText combo;
	combo.append_text ("All Documents");
	combo.append_text ("Selected Documents");
	combo.set_active (0);
	extrabox.pack_start (combo, true, true, 0);
	extrabox.show_all ();
	extrabox.set_sensitive (getSelectedDocCount ());

	// Browsing to remote hosts not working for some reason
	//chooser.set_local_only (false);

	if (!exportfolder_.empty())
		chooser.set_current_folder (exportfolder_);

	if (chooser.run() == Gtk::RESPONSE_ACCEPT) {
		bool const selectedonly = combo.get_active_row_number () == 1;
		exportfolder_ = Glib::path_get_dirname(chooser.get_filename());
		Glib::ustring bibfilename = chooser.get_uri();
		// Really we shouldn't add the extension if the user has chosen an
		// existing file rather than typing in a name themselves.
		bibfilename = Utility::ensureExtension (bibfilename, "bib");
		Glib::ustring tmpbibfilename = bibfilename + ".tmp";

		Gnome::Vfs::Handle bibfile;

		try {
			bibfile.create (tmpbibfilename, Gnome::Vfs::OPEN_WRITE,
				false, Gnome::Vfs::PERM_USER_READ | Gnome::Vfs::PERM_USER_WRITE);
		} catch (const Gnome::Vfs::exception ex) {
			std::cerr << "TagWindow::onExportBibtex: "
				"exception in create '" << ex.what() << "'\n";
			Utility::exceptionDialog (&ex, "opening BibTex file");
			return;
		}

		std::ostringstream bibtext;

		if (selectedonly) {
			std::vector<Document*> docs = getSelectedDocs ();
			std::vector<Document*>::iterator it = docs.begin ();
			std::vector<Document*>::iterator const end = docs.end ();
			for (; it != end; ++it) {
				(*it)->writeBibtex (bibtext);
			}
		} else {
			std::vector<Document> &docs = doclist_->getDocs ();
			std::vector<Document>::iterator it = docs.begin();
			std::vector<Document>::iterator const end = docs.end();
			for (; it != end; it++) {
				(*it).writeBibtex (bibtext);
			}
		}


		try {
			bibfile.write (bibtext.str().c_str(), strlen(bibtext.str().c_str()));
		} catch (const Gnome::Vfs::exception ex) {
			Utility::exceptionDialog (&ex, "writing to BibTex file");
			bibfile.close ();
			return;
		}

		bibfile.close ();

		// Forcefully move our tmp file into its real position
		Gnome::Vfs::Handle::move (tmpbibfilename, bibfilename, true);
	}
}


void TagWindow::onNewLibrary ()
{
	if (ensureSaved ("creating a new library")) {
		setDirty (false);

		setOpenedLib ("");
		taglist_->clear();
		doclist_->clear();

		populateDocStore ();
		populateTagList ();
	}
}


void TagWindow::onOpenLibrary ()
{
	Gtk::FileChooserDialog chooser(
		"Open Library",
		Gtk::FILE_CHOOSER_ACTION_OPEN);

	// remote not working?
	//chooser.set_local_only (false);

	if (!ensureSaved ("opening another library"))
		return;

	if (!libraryfolder_.empty())
		chooser.set_current_folder (libraryfolder_);
	chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_REJECT);
	chooser.add_button (Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);
	chooser.set_default_response (Gtk::RESPONSE_ACCEPT);

	Gtk::FileFilter reflibfiles;
	reflibfiles.add_pattern ("*.reflib");
	reflibfiles.set_name ("Referencer Libraries");
	chooser.add_filter (reflibfiles);

	Gtk::FileFilter allfiles;
	allfiles.add_pattern ("*.*");
	allfiles.set_name ("All Files");
	chooser.add_filter (allfiles);

	if (chooser.run () == Gtk::RESPONSE_ACCEPT) {
		libraryfolder_ = Glib::path_get_dirname(chooser.get_filename());
		Glib::ustring libfile = chooser.get_uri ();
		std::cerr << "Calling loadLibrary on " << libfile << "\n";
		if (loadLibrary (libfile)) {
			setDirty (false);
			populateDocStore ();
			populateTagList ();
			setOpenedLib (libfile);
		} else {
			//loadLibrary would have shown an exception error dialog
		}

	}
}


void TagWindow::onSaveLibrary ()
{
	if (openedlib_.empty()) {
		onSaveAsLibrary ();
	} else {
		if (saveLibrary (openedlib_))
			setDirty (false);
	}
}


void TagWindow::onSaveAsLibrary ()
{
	Gtk::FileChooserDialog chooser (
		"Save Library As",
		Gtk::FILE_CHOOSER_ACTION_SAVE);
	chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_REJECT);
	chooser.add_button (Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	chooser.set_default_response (Gtk::RESPONSE_ACCEPT);
	chooser.set_do_overwrite_confirmation (true);

	Gtk::FileFilter reflibfiles;
	reflibfiles.add_pattern ("*.reflib");
	reflibfiles.set_name ("Referencer Libraries");
	chooser.add_filter (reflibfiles);

	Gtk::FileFilter allfiles;
	allfiles.add_pattern ("*.*");
	allfiles.set_name ("All Files");
	chooser.add_filter (allfiles);

	// Browsing to remote hosts not working for some reason
	//chooser.set_local_only (false);

	if (!libraryfolder_.empty())
		chooser.set_current_folder (libraryfolder_);

	if (chooser.run() == Gtk::RESPONSE_ACCEPT) {
		libraryfolder_ = Glib::path_get_dirname(chooser.get_filename());
		Glib::ustring libfilename = chooser.get_uri();
		// Really we shouldn't add the extension if the user has chosen an
		// existing file rather than typing in a name themselves.
		libfilename = Utility::ensureExtension (libfilename, "reflib");

		if (saveLibrary (libfilename)) {
			setDirty (false);
			setOpenedLib (libfilename);
		} else {
			// saveLibrary would have shown exception dialogs
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
	dialog.set_logo (
		Gdk::Pixbuf::create_from_file (
			Utility::findDataFile ("referencer.svg"),
			128, 128));
	dialog.run ();
}


void TagWindow::onIntroduction ()
{
	Glib::ustring uri = "file://" + Utility::findDataFile ("introduction.html");
	std::cerr << uri << "\n";
	Gnome::Vfs::url_show (uri);
}


void TagWindow::addDocFiles (std::vector<Glib::ustring> const &filenames)
{
	Gtk::Dialog dialog ("Add Document Files", true, false);

	Gtk::VBox *vbox = dialog.get_vbox ();
	vbox->set_spacing (12);

	Glib::ustring messagetext =
		"<b><big>Adding document files</big></b>\n\n"
		"This process may take some time as the bibliographic \n"
		"information for each document is looked up.";

	Gtk::Label label ("", false);
	label.set_markup (messagetext);

	vbox->pack_start (label, true, true, 0);

	Gtk::ProgressBar progress;

	vbox->pack_start (progress, false, false, 0);

	dialog.show_all ();
	vbox->set_border_width (12);

	std::ostringstream progresstext;

	int n = 0;
	std::vector<Glib::ustring>::const_iterator it = filenames.begin();
	std::vector<Glib::ustring>::const_iterator const end = filenames.end();
	for (; it != end; ++it) {
		progress.set_fraction ((float)n / (float)filenames.size());
		progresstext.str ("");
		progresstext << n << " of " << filenames.size() << " documents";
		progress.set_text (progresstext.str());
		while (Gnome::Main::events_pending())
			Gnome::Main::iteration ();

		Glib::RefPtr<Gnome::Vfs::Uri> uri = Gnome::Vfs::Uri::create (*it);
		if (!uri->is_local ()) {
			// Should prompt the user to download the file
			std::cerr << "Ooh, a remote uri\n";
		}

		Document *newdoc = doclist_->newDocWithFile(*it);
		if (newdoc) {
			newdoc->readPDF ();

			while (Gnome::Main::events_pending())
				Gnome::Main::iteration ();

			// If we got a DOI or eprint field this will work
			newdoc->getMetaData ();

			newdoc->setKey (doclist_->uniqueKey (newdoc->generateKey ()));
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
	Document *newdoc = doclist_->newDocUnnamed ();
	newdoc->setKey (doclist_->uniqueKey (newdoc->generateKey ()));
	docpropertiesdialog_->show (newdoc);
	populateDocStore ();
}


void TagWindow::onAddDocByDoi ()
{

	Gtk::Dialog dialog ("Add Document with DOI", true, false);

	Gtk::VBox *vbox = dialog.get_vbox ();

	Gtk::HBox hbox;
	hbox.set_spacing (12);
	vbox->pack_start (hbox, true, true, 0);

	Gtk::Label label ("DOI:", false);
	hbox.pack_start (label, false, false, 0);

	Gtk::Entry entry;
	hbox.pack_start (entry, true, true, 0);

	dialog.add_button (Gtk::Stock::CANCEL, 0);
	dialog.add_button (Gtk::Stock::OK, 1);

	dialog.show_all ();
	vbox->set_border_width (12);

	if (dialog.run ()) {
		setDirty (true);
		Document *newdoc = doclist_->newDocWithDoi (entry.get_text ());

		newdoc->getMetaData ();
		newdoc->setKey (doclist_->uniqueKey (newdoc->generateKey ()));

		populateDocStore ();
	}
}


void TagWindow::onAddDocFile ()
{
	Gtk::FileChooserDialog chooser(
		"Add Document",
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
		"Add Folder",
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


std::vector<Glib::ustring> TagWindow::getSelectedDocKeys ()
{
	std::vector<Glib::ustring> dockeys;

	if (usinglistview_) {
		Gtk::TreeSelection::ListHandle_Path paths =
			docslistselection_->get_selected_rows ();

		Gtk::TreeSelection::ListHandle_Path::iterator it = paths.begin ();
		Gtk::TreeSelection::ListHandle_Path::iterator const end = paths.end ();
		for (; it != end; it++) {
			Gtk::TreePath path = (*it);
			Gtk::ListStore::iterator iter = docstore_->get_iter (path);
			dockeys.push_back((*iter)[dockeycol_]);
		}
	} else {
		Gtk::IconView::ArrayHandle_TreePaths paths =
			docsiconview_->get_selected_items ();

		Gtk::IconView::ArrayHandle_TreePaths::iterator it = paths.begin ();
		Gtk::IconView::ArrayHandle_TreePaths::iterator const end = paths.end ();
		for (; it != end; it++) {
			Gtk::TreePath path = (*it);
			Gtk::ListStore::iterator iter = docstore_->get_iter (path);
			dockeys.push_back((*iter)[dockeycol_]);
		}
	}

	return dockeys;
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
	std::vector<Glib::ustring> keys = getSelectedDocKeys ();

	bool const multiple = keys.size() > 1;

	bool doclistdirty = false;

	if (multiple) {
		// Do you really want to remove N documents?
		std::ostringstream num;
		num << keys.size ();
		Glib::ustring message = "<b><big>Are you sure you want to remove these "
			+ num.str() + " documents?</big></b>\n\nAll tag "
			"associations and metadata for these documents will be permanently lost.";
		Gtk::MessageDialog confirmdialog (
			message, true, Gtk::MESSAGE_QUESTION,
			Gtk::BUTTONS_NONE, true);

		confirmdialog.add_button (Gtk::Stock::CANCEL, 0);
		confirmdialog.add_button (Gtk::Stock::REMOVE, 1);
		confirmdialog.set_default_response (0);

		if (!confirmdialog.run()) {
			return;
		}
	}

	std::vector<Glib::ustring>::iterator it = keys.begin ();
	std::vector<Glib::ustring>::iterator const end = keys.end ();
	for (; it != end; it++) {
		if (!multiple) {
			Glib::ustring message = "<b><big>Are you sure you want to remove '" +
				*it + "'?</big></b>\n\nAll tag "
				"associations and metadata for the document will be permanently lost.";
			Gtk::MessageDialog confirmdialog (
				message, true, Gtk::MESSAGE_QUESTION,
				Gtk::BUTTONS_NONE, true);

			confirmdialog.add_button (Gtk::Stock::CANCEL, 0);
			confirmdialog.add_button (Gtk::Stock::REMOVE, 1);
			confirmdialog.set_default_response (0);

			if (!confirmdialog.run()) {
				continue;
			}
		}

		std::cerr << "TagWindow::onRemoveDoc: removeDoc on '" << *it << "'\n";
		doclist_->removeDoc(*it);
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


Glib::ustring TagWindow::writeXML ()
{
	std::ostringstream out;
	out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	out << "<library>\n";
	taglist_->writeXML (out);
	doclist_->writeXML (out);
	out << "</library>\n";
	return out.str ();
}


bool TagWindow::readXML (Glib::ustring XMLtext)
{
	TagList *newtags = new TagList ();
	DocumentList *newdocs = new DocumentList ();

	LibraryParser parser (*newtags, *newdocs);
	Glib::Markup::ParseContext context (parser);
	try {
		context.parse (XMLtext);
	} catch (Glib::MarkupError const ex) {
		std::cerr << "Exception on line " << context.get_line_number () << ", character " << context.get_char_number () << ": '" << ex.what () << "'\n";
		Utility::exceptionDialog (&ex, "parsing Library XML.");

		delete newtags;
		delete newdocs;
		return false;
	}

	context.end_parse ();

	delete taglist_;
	taglist_ = newtags;

	delete doclist_;
	doclist_ = newdocs;

	return true;
}


// True on success
bool TagWindow::loadLibrary (Glib::ustring const &libfilename)
{
	Gnome::Vfs::Handle libfile;

	Glib::RefPtr<Gnome::Vfs::Uri> liburi = Gnome::Vfs::Uri::create (libfilename);

	try {
		libfile.open (libfilename, Gnome::Vfs::OPEN_READ);
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "opening library '" + libfilename + "'");
		return false;
	}

	Glib::RefPtr<Gnome::Vfs::FileInfo> fileinfo;
	fileinfo = libfile.get_file_info ();

	char *buffer = (char *) malloc (sizeof(char) * (fileinfo->get_size() + 1));
	if (!buffer) {
		std::cerr << "Warning: TagWindow::loadLibrary: couldn't allocate buffer\n";
		return false;
	}

	try {
		libfile.read (buffer, fileinfo->get_size());
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "reading library '" + libfilename + "'");
		free (buffer);
		return false;
	}
	buffer[fileinfo->get_size()] = 0;

	Glib::ustring rawtext = buffer;
	free (buffer);
	libfile.close ();

	std::cerr << "Reading XML...\n";
	if (!readXML (rawtext))
		return false;
	std::cerr << "Done, got " << doclist_->getDocs ().size() << " docs\n";
	
	std::vector<Document> &docs = doclist_->getDocs ();
	std::vector<Document>::iterator docit = docs.begin ();
	std::vector<Document>::iterator const docend = docs.end ();
	for (; docit != docend; ++docit) {
		if (Utility::fileExists (docit->getFileName())) {
			// Do nothing, all is well, the file is still there
			std::cerr << "Filename still good: " << docit->getFileName() << "\n";
		} else if (!docit->getRelFileName().empty()) {
			// Oh no!  We lost the file!  But we've got a relfilename!  Is relfilename still valid?
			Glib::ustring filename = Glib::build_filename (
				Glib::path_get_dirname (libfilename),
				docit->getRelFileName());
			filename = "file://" + filename;
			std::cerr << "Derived from relative: " << filename << "\n";
			if (Utility::fileExists (filename)) {
				std::cerr << "\tValid\n";
				docit->setFileName (filename);
			}
		}
	}

	return true;
}


// True on success
bool TagWindow::saveLibrary (Glib::ustring const &libfilename)
{
	Gnome::Vfs::Handle libfile;

	Glib::ustring tmplibfilename = Glib::get_home_dir () + "/.referencer.lib" + ".tmp";

	try {
		libfile.create (tmplibfilename, Gnome::Vfs::OPEN_WRITE,
			false, Gnome::Vfs::PERM_USER_READ | Gnome::Vfs::PERM_USER_WRITE);
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "opening file '" + tmplibfilename + "'");
		return false;
	}

	std::vector<Document> &docs = doclist_->getDocs ();
	std::vector<Document>::iterator docit = docs.begin ();
	std::vector<Document>::iterator const docend = docs.end ();
	for (; docit != docend; ++docit) {
		if (Utility::fileExists (docit->getFileName())) {
			docit->updateRelFileName (libfilename);
		}
	}

	Glib::ustring rawtext = writeXML ();

	try {
		libfile.write (rawtext.c_str(), strlen(rawtext.c_str()));
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "writing to file '" + tmplibfilename + "'");
		return false;
	}

	libfile.close ();

	// Forcefully move our tmp file into its real position
	try {
		Gnome::Vfs::Handle::move (tmplibfilename, libfilename, true);
	} catch (const Gnome::Vfs::exception ex) {
		Utility::exceptionDialog (&ex, "moving '"
			+ tmplibfilename + "' to '" + libfilename + "'");
		return false;
	}

	return true;
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


void TagWindow::onOpenDoc ()
{
	std::vector<Document*> docs = getSelectedDocs ();
	std::vector<Document*>::iterator it = docs.begin ();
	std::vector<Document*>::iterator const end = docs.end ();
	for (; it != end ; ++it) {
		if (!(*it)->getFileName().empty()) {
			Gnome::Vfs::url_show ((*it)->getFileName());
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
				"getting info for file '" + uri->to_string () + "'");
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
	} else {
		Glib::RefPtr <Gtk::RadioAction>::cast_static(
			actiongroup_->get_action ("UseIconView"))->set_active (true);
		docslistscroll_->hide ();
		docsiconscroll_->show ();
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
		filename = "Unnamed Library";
	} else {
		filename = Glib::path_get_basename (openedlib_);
		unsigned int pos = filename.find (".reflib");
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
		"Import References",
		Gtk::FILE_CHOOSER_ACTION_OPEN);

	// remote not working?
	//chooser.set_local_only (false);

	if (!libraryfolder_.empty())
		chooser.set_current_folder (libraryfolder_);

	chooser.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_REJECT);
	chooser.add_button (Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);
	chooser.set_default_response (Gtk::RESPONSE_ACCEPT);

	Gtk::FileFilter allfiles;
	allfiles.add_pattern ("*.*");
	allfiles.set_name ("All Files");
	chooser.add_filter (allfiles);
	
	Gtk::FileFilter allbibfiles;
	// Complete random guesses, what are the real extensions?
	allbibfiles.add_pattern ("*.ris");
	allbibfiles.add_pattern ("*.[bB][iI][bB]");
	allbibfiles.add_pattern ("*.ref");
	allbibfiles.set_name ("Bibliography Files");
	chooser.add_filter (allbibfiles);
	
	Gtk::FileFilter bibtexfiles;
	bibtexfiles.add_pattern ("*.[bB][iI][bB]");
	bibtexfiles.set_name ("BibTeX Files");
	chooser.add_filter (bibtexfiles);

	Gtk::HBox extrabox;
	extrabox.set_spacing (6);
	chooser.set_extra_widget (extrabox);
	Gtk::Label label ("Format:");
	extrabox.pack_start (label, false, false, 0);
	Gtk::ComboBoxText combo;
	combo.append_text ("BibTeX");
	combo.append_text ("EndNote");
	combo.append_text ("RIS");
	combo.append_text ("MODS");
	//combo.append_text ("Auto Detected");
	combo.set_active (0);
	extrabox.pack_start (combo, true, true, 0);
	extrabox.show_all ();


	if (chooser.run () == Gtk::RESPONSE_ACCEPT) {
		libraryfolder_ = Glib::path_get_dirname(chooser.get_filename());
		Glib::ustring filename = chooser.get_uri ();
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

		doclist_->import (filename, format);

		populateDocStore ();
		populateTagList ();
	}
}


void TagWindow::onSearchChanged ()
{
	populateDocStore ();
}

