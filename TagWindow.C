
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

	TagWindow window;

	window.run();

	return 0;
}


TagWindow::TagWindow ()
{
	tagselectionignore_ = false;
	ignoretaggerchecktoggled_ = false;
	docselectionignore_ = false;

	docpropertiesdialog_ = NULL;

	taglist_ = new TagList();
	doclist_ = new DocumentList();
	loadLibrary ();

	_global_prefs = new Preferences();

	constructUI ();
	populateDocIcons ();
	populateTagList ();
	window_->show_all ();
}


TagWindow::~TagWindow ()
{
	saveLibrary ();

	delete taglist_;
	delete doclist_;
	if (docpropertiesdialog_)
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
	/*window_->signal_delete_event().connect (
		sigc::mem_fun (*this, &TagWindow::onQuit));*/

	constructMenu ();

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);
	window_->add (*vbox);

	vbox->pack_start (*uimanager_->get_widget("/MenuBar"), false, false, 0);
	Gtk::HPaned *hpaned = Gtk::manage(new Gtk::HPaned());
	vbox->pack_start (*hpaned, true, true, 6);

	vbox = Gtk::manage(new Gtk::VBox);
	Gtk::Frame *tagsframe = new Gtk::Frame ();
	tagsframe->add(*vbox);
	hpaned->pack1(*tagsframe, Gtk::FILL);

	Gtk::VBox *filtervbox = Gtk::manage (new Gtk::VBox);
	Gtk::Expander *filterexpander =
		Gtk::manage (new Gtk::Expander ("Tag _filter", true));
	filterexpander->set_expanded (true);
	filterexpander->add (*filtervbox);
	vbox->pack_start (*filterexpander, true, true, 0);

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

	// The tagger box
	Gtk::VBox *taggervbox = Gtk::manage (new Gtk::VBox);
	taggervbox->set_border_width(6);

	Gtk::ScrolledWindow *taggerscroll = Gtk::manage(new Gtk::ScrolledWindow());

	taggerscroll->set_shadow_type (Gtk::SHADOW_NONE);
	taggerscroll->add (*taggervbox);
	taggerscroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

	Gtk::Expander *taggerexpander =
		Gtk::manage (new Gtk::Expander ("_Tagger", true));
	taggerexpander->set_expanded (true);
	taggerexpander->add (*taggerscroll);
	vbox->pack_start (*taggerexpander, true, true, 0);

	taggerbox_ = taggervbox;


	// The iconview side
	vbox = Gtk::manage(new Gtk::VBox);
	Gtk::Frame *iconsframe = new Gtk::Frame ();
	iconsframe->add (*vbox);
	hpaned->pack2(*iconsframe, Gtk::EXPAND);

	// Create the store for the document icons
	Gtk::TreeModel::ColumnRecord iconcols;
	iconcols.add(docpointercol_);
	iconcols.add(docnamecol_);
	iconcols.add(docthumbnailcol_);
	iconstore_ = Gtk::ListStore::create(iconcols);

	// Create the IconView for the document icons
	Gtk::IconView *icons = Gtk::manage(new Gtk::IconView(iconstore_));
	icons->set_text_column(docnamecol_);
	icons->set_pixbuf_column(docthumbnailcol_);
	icons->signal_item_activated().connect (
		sigc::mem_fun (*this, &TagWindow::docActivated));

	icons->signal_button_press_event().connect(
		sigc::mem_fun (*this, &TagWindow::docClicked));

	icons->signal_selection_changed().connect(
		sigc::mem_fun (*this, &TagWindow::docSelectionChanged));

	icons->set_selection_mode (Gtk::SELECTION_MULTIPLE);
	/*icons->set_column_spacing (50);
	icons->set_icon_width (10);*/
	icons->set_columns (-1);

	docsview_ = icons;

	Gtk::ScrolledWindow *iconsscroll = Gtk::manage(new Gtk::ScrolledWindow());
	iconsscroll->add(*icons);
	iconsscroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	vbox->pack_start(*iconsscroll, true, true, 0);

	/*Gtk::Toolbar& docbar = (Gtk::Toolbar&) *uimanager_->get_widget("/DocBar");
	vbox->pack_start (docbar, false, false, 0);
	docbar.set_toolbar_style (Gtk::TOOLBAR_ICONS);
	docbar.set_show_arrow (false);*/
}


void TagWindow::constructMenu ()
{
	actiongroup_ = Gtk::ActionGroup::create();

	actiongroup_->add ( Gtk::Action::create("LibraryMenu", "_Library") );
	actiongroup_->add( Gtk::Action::create("ExportBibtex",
		Gtk::Stock::CONVERT, "E_xport to BibTeX"),
  	sigc::mem_fun(*this, &TagWindow::onExportBibtex));
	actiongroup_->add( Gtk::Action::create("Preferences",
		Gtk::Stock::PREFERENCES),
  	sigc::mem_fun(*this, &TagWindow::onPreferences));
	actiongroup_->add( Gtk::Action::create("Quit", Gtk::Stock::QUIT),
  	sigc::mem_fun(*this, &TagWindow::onQuit));

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
		"AddDocUnnamed", Gtk::Stock::ADD, "_Add Empty Reference"),
  	sigc::mem_fun(*this, &TagWindow::onAddDocUnnamed));
	actiongroup_->add( Gtk::Action::create(
		"AddDocDoi", Gtk::Stock::ADD, "_Add Reference with DOI..."),
  	sigc::mem_fun(*this, &TagWindow::onAddDocByDoi));
	actiongroup_->add( Gtk::Action::create(
		"RemoveDoc", Gtk::Stock::REMOVE, "_Remove"),
  	sigc::mem_fun(*this, &TagWindow::onRemoveDoc));
	actiongroup_->add( Gtk::Action::create(
		"DoiLookupDoc", Gtk::Stock::CONNECT, "_Web Link (DOI)..."),
  	sigc::mem_fun(*this, &TagWindow::onDoiLookupDoc));
	actiongroup_->add( Gtk::Action::create(
		"OpenDoc", Gtk::Stock::OPEN, "_Open..."),
  	sigc::mem_fun(*this, &TagWindow::onOpenDoc));
	actiongroup_->add( Gtk::Action::create(
		"DocProperties", Gtk::Stock::PROPERTIES),
  	sigc::mem_fun(*this, &TagWindow::onDocProperties));
	actiongroup_->add( Gtk::Action::create(
		"Divine", "_Divine..."),
  	sigc::mem_fun(*this, &TagWindow::onDivine));

	actiongroup_->add ( Gtk::Action::create("HelpMenu", "_Help") );
	actiongroup_->add( Gtk::Action::create(
		"About", Gtk::Stock::ABOUT),
  	sigc::mem_fun(*this, &TagWindow::onAbout));

	uimanager_ = Gtk::UIManager::create ();
	uimanager_->insert_action_group (actiongroup_);
	Glib::ustring ui =
		"<ui>"
		"  <menubar name='MenuBar'>"
		"    <menu action='LibraryMenu'>"
		"      <menuitem action='ExportBibtex'/>"
		"      <separator/>"
		"      <menuitem action='Preferences'/>"
		"      <separator/>"
		"      <menuitem action='Quit'/>"
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
		"      <menuitem action='DoiLookupDoc'/>"
		"      <menuitem action='OpenDoc'/>"
		"      <menuitem action='DocProperties'/>"
		"    </menu>"
		"    <menu action='HelpMenu'>"
		"      <menuitem action='About'/>"
		"    </menu>"
		"  </menubar>"
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
		"    <menuitem action='DoiLookupDoc'/>"
		"    <menuitem action='OpenDoc'/>"
		//"    <menuitem action='Divine'/>"
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


void TagWindow::populateDocIcons ()
{
	std::cerr << "TagWindow::populateDocIcons >>\n";

	// Save initial selection
	Gtk::TreePath initialpath;
	Gtk::IconView::ArrayHandle_TreePaths paths = docsview_->get_selected_items ();
	if (paths.size () > 0)
		initialpath = (*paths.begin());

	iconstore_->clear ();

	// Populate from doclist_
	std::vector<Document>& docvec = doclist_->getDocs();
	std::vector<Document>::iterator docit = docvec.begin();
	std::vector<Document>::iterator const docend = docvec.end();
	for (; docit != docend; ++docit) {
		bool filtered = true;
		for (std::vector<int>::iterator tagit = filtertags_.begin();
		     tagit != filtertags_.end(); ++tagit) {
			if (*tagit == ALL_TAGS_UID || (*docit).hasTag(*tagit)) {
				filtered = false;
				break;
			}
		}
		if (filtered)
			continue;

		Gtk::TreeModel::iterator item = iconstore_->append();
		(*item)[docnamecol_] = (*docit).getDisplayName();
		// PROGRAM CRASHING THIS HOLIDAY SEASON?
		// THIS LINE DID IT!
		// WHEE!  LOOK AT THIS LINE OF CODE!
		(*item)[docpointercol_] = &(*docit);
		(*item)[docthumbnailcol_] = (*docit).getThumbnail();
	}

	// Restore initial selection
	docsview_->select_path (initialpath);
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

	std::vector<Document*> selecteddocs = getSelectedDocs ();

	bool active = check->get_active ();
	check->set_inconsistent (false);

	bool tagsremoved = false;

	std::vector<Document*>::iterator it = selecteddocs.begin ();
	std::vector<Document*>::iterator const end = selecteddocs.end ();
	for (; it != end; it++) {
		Document *doc = (*it);
		if (active) {
			doc->setTag (taguid);
		} else {
			doc->clearTag (taguid);
			tagsremoved = true;
		}
	}

	// If we've untagged something it might no longer be visible
	if (tagsremoved
	    && (*filtertags_.begin()) != ALL_TAGS_UID) {
	  // Should check if the tag we removed is in filtertags ideally
	  // This is quite annoying when we don't save which document is selected
		populateDocIcons ();
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

	if ((*iter)[taguidcol_] == ALL_TAGS_UID)
		return;


	// Should escape this
	Glib::ustring newname = text2;
	(*iter)[tagnamecol_] = newname;
	taglist_->renameTag ((*iter)[taguidcol_], newname);
	taggerchecks_[(*iter)[taguidcol_]]->set_label (newname);
}


void TagWindow::docActivated (const Gtk::TreeModel::Path& path)
{
	Gtk::ListStore::iterator it = iconstore_->get_iter (path);
	Document *doc = (*it)[docpointercol_];
	// The methods we're calling should fail out safely and quietly
	// if the number of docs selected != 1
	if (!doc->getFileName ().empty ()) {
		onOpenDoc ();
	} else if (!doc->getBibData ().getDoi ().empty ()) {
		onDoiLookupDoc ();
	}
}


void TagWindow::tagSelectionChanged ()
{
	if (tagselectionignore_)
		return;

	Gtk::TreeSelection::ListHandle_Path paths =
		tagselection_->get_selected_rows ();

	filtertags_.clear();

	bool allselected = false;
	bool anythingselected = false;

	if (paths.empty()) {
		allselected = true;
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
			if ((*iter)[taguidcol_] == ALL_TAGS_UID) {
				allselected = true;
				break;
			}
		}
	}

	actiongroup_->get_action("DeleteTag")->set_sensitive (
		anythingselected && !allselected);
	actiongroup_->get_action("RenameTag")->set_sensitive (
		paths.size() == 1 && !allselected);

	populateDocIcons ();
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


	actiongroup_->get_action("DoiLookupDoc")->set_sensitive (
		onlyoneselected
		&& !getSelectedDoc()->getBibData().getDoi().empty());
	actiongroup_->get_action("OpenDoc")->set_sensitive (
		onlyoneselected
		&& !getSelectedDoc()->getFileName().empty());




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
		Gtk::TreeModel::Path clickedpath =
			docsview_->get_path_at_pos ((int)event->x, (int)event->y);

		if (!clickedpath.empty()) {
			if (!docsview_->path_is_selected (clickedpath)) {
				docsview_->unselect_all ();
				docsview_->select_path (clickedpath);
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

	Gtk::IconView::ArrayHandle_TreePaths paths = docsview_->get_selected_items ();

	Gtk::IconView::ArrayHandle_TreePaths::iterator it = paths.begin ();
	Gtk::IconView::ArrayHandle_TreePaths::iterator const end = paths.end ();
	for (; it != end; it++) {
		Gtk::TreePath path = (*it);
		Gtk::ListStore::iterator iter = iconstore_->get_iter (path);
		Document *doc = (*iter)[docpointercol_];
		if (doc->hasTag(uid)) {
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


Document *TagWindow::getSelectedDoc ()
{
	if (docsview_->get_selected_items ().size() != 1) {
		std::cerr << "Warning: TagWindow::getSelectedDoc: size != 1\n";
		return false;
	}

	Gtk::IconView::ArrayHandle_TreePaths paths = docsview_->get_selected_items ();

	Gtk::TreePath path = (*paths.begin ());
	Gtk::ListStore::iterator iter = iconstore_->get_iter (path);
	return (*iter)[docpointercol_];
}


void TagWindow::tagClicked (GdkEventButton* event)
{
  if((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
		Gtk::Menu *popupmenu =
			(Gtk::Menu*)uimanager_->get_widget("/TagPopup");
		popupmenu->popup (event->button, event->time);
	}
}


void TagWindow::onQuit (/*GdkEventAny *ev*/)
{
	Gnome::Main::quit ();
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
		if (newname.empty()) {
			invalid = true;
		} else {
			invalid = false;
			taglist_->newTag (newname, Tag::ATTACH);
			populateTagList();
		}

	}

	// When we add a tag we need to escape it to avoid markup
}


void TagWindow::onDeleteTag ()
{
	Gtk::TreeSelection::ListHandle_Path paths =
		tagselection_->get_selected_rows ();

	if (paths.empty()) {
		std::cerr << "Warning: TagWindow::onDeleteTag: nothing selected\n";
		return;
	}

	bool somethingdeleted = false;

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
			// Remove tag from tagged documents
			doclist_->clearTag ((*iter)[taguidcol_]);
			// Delete tag
			taglist_->deleteTag ((*iter)[taguidcol_]);
			somethingdeleted = true;
		}
	}

	if (somethingdeleted)
		populateTagList ();
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

	if (!exportfolder_.empty())
		chooser.set_current_folder (exportfolder_);

	if (chooser.run() == Gtk::RESPONSE_ACCEPT) {
		exportfolder_ = Glib::path_get_dirname(chooser.get_filename());
		Glib::ustring bibfilename = chooser.get_uri();

		std::ostringstream bibtext;
		doclist_->writeBibtex (bibtext);

		Gnome::Vfs::Handle bibfile;

		try {
			bibfile.create (bibfilename, Gnome::Vfs::OPEN_WRITE,
				false, Gnome::Vfs::PERM_USER_READ | Gnome::Vfs::PERM_USER_WRITE);
		} catch (const Gnome::Vfs::exception ex) {
			std::cerr << "TagWindow::onExportBibtex: "
				"exception in create '" << ex.what() << "'\n";
			return;
		}

		bibfile.write (bibtext.str().c_str(), strlen(bibtext.str().c_str()));

		bibfile.close ();
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
	dialog.run ();
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
		Document *newdoc = doclist_->newDocWithFile(*it);
		newdoc->readPDF ();
		if (!newdoc->getBibData().getDoi().empty())
			newdoc->getBibData().getCrossRef ();

		newdoc->setDisplayName (doclist_->uniqueDisplayName (newdoc->generateKey ()));

		++n;
	}

	if (!filenames.empty()) {
		// We added something
		// Should check if we actually added something in case a newDoc
		// failed, eg if the doc was already in there
		populateDocIcons ();
	}

}


void TagWindow::onAddDocUnnamed ()
{
	Document *newdoc = doclist_->newDocUnnamed ();
	newdoc->setDisplayName (doclist_->uniqueDisplayName (newdoc->generateKey ()));
	docpropertiesdialog_->show (newdoc);
	populateDocIcons ();
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
		Document *newdoc = doclist_->newDocWithDoi (entry.get_text ());

		newdoc->getBibData().getCrossRef ();
		newdoc->setDisplayName (doclist_->uniqueDisplayName (newdoc->generateKey ()));

		populateDocIcons ();
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



// For recursing through directories to add all documents from a folder

static Glib::ustring _basepath;
std::vector<Glib::ustring> _filestoadd;

bool onAddDocFolderRecurse (const Glib::ustring& rel_path, const Glib::RefPtr<const Gnome::Vfs::FileInfo>& info, bool recursing_will_loop, bool& recurse)
{
	// We escape to be consistent with the escaped URI that FileChooser
	// gave us for basepath
	Glib::ustring fullname =
		Glib::build_filename (
			_basepath, Gnome::Vfs::escape_string(info->get_name()));
	std::cerr << "onAddDocFolderRecurse: fullpath = '" << fullname << "'\n";

	if (info->get_type () == Gnome::Vfs::FILE_TYPE_DIRECTORY) {
		Gnome::Vfs::DirectoryHandle dir;
		Glib::ustring tmp = _basepath;
		_basepath = fullname;
		dir.visit (
			fullname,
			Gnome::Vfs::FILE_INFO_DEFAULT,
			Gnome::Vfs::DIRECTORY_VISIT_DEFAULT,
			&onAddDocFolderRecurse);
		_basepath = tmp;
	} else {
		_filestoadd.push_back (fullname);
	}

	// What does this retval do?
	return true;
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
		addfolder_ = Glib::path_get_dirname(chooser.get_filename());
		Glib::ustring rootfoldername = chooser.get_uri();
		std::cerr << "Adding folder '" << rootfoldername << "'\n";
		Gnome::Vfs::DirectoryHandle dir;
		_basepath = rootfoldername;
		_filestoadd.clear();
		dir.visit (
			rootfoldername,
			Gnome::Vfs::FILE_INFO_DEFAULT,
			Gnome::Vfs::DIRECTORY_VISIT_DEFAULT,
			&onAddDocFolderRecurse);


		addDocFiles (_filestoadd);
	}
}


std::vector<Document*> TagWindow::getSelectedDocs ()
{
	std::vector<Document*> docpointers;

	Gtk::IconView::ArrayHandle_TreePaths paths =
		docsview_->get_selected_items ();

	Gtk::IconView::ArrayHandle_TreePaths::iterator it = paths.begin ();
	Gtk::IconView::ArrayHandle_TreePaths::iterator const end = paths.end ();
	for (; it != end; it++) {
		Gtk::TreePath path = (*it);
		Gtk::ListStore::iterator iter = iconstore_->get_iter (path);
		docpointers.push_back((*iter)[docpointercol_]);
	}

	return docpointers;
}


int TagWindow::getSelectedDocCount ()
{
	return docsview_->get_selected_items().size();
}


void TagWindow::onRemoveDoc ()
{
	Gtk::IconView::ArrayHandle_TreePaths paths = docsview_->get_selected_items ();

	bool const multiple = paths.size() > 1;

	bool doclistdirty = false;

	if (multiple) {
		// Do you really want to remove N documents?
		std::ostringstream num;
		num << paths.size ();
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

	Gtk::IconView::ArrayHandle_TreePaths::iterator it = paths.begin ();
	Gtk::IconView::ArrayHandle_TreePaths::iterator const end = paths.end ();
	for (; it != end; it++) {
		Gtk::TreePath path = (*it);
		Gtk::ListStore::iterator iter = iconstore_->get_iter (path);
		if (!multiple) {
			Glib::ustring message = "<b><big>Are you sure you want to remove '" +
				(*iter)[docnamecol_] + "'?</big></b>\n\nAll tag "
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

		std::cerr << "TagWindow::onRemoveDoc: removeDoc on '" << (*iter)[docnamecol_] << "'\n";
		doclist_->removeDoc((*iter)[docnamecol_]);
		doclistdirty = true;
	}

	if (doclistdirty) {
		std::cerr << "TagWindow::onRemoveDoc: dirty, calling populateDocIcons\n";
		// We disable docSelectionChanged because otherwise it gets called N
		// times for deleting N items
		docselectionignore_ = true;
		populateDocIcons ();
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


void TagWindow::readXML (Glib::ustring XMLtext)
{
	taglist_->clear();
	doclist_->clear();
	LibraryParser parser (*taglist_, *doclist_);
	Glib::Markup::ParseContext context (parser);
	try {
		context.parse (XMLtext);
	} catch (Glib::MarkupError const ex) {
		std::cerr << "Exception on line " << context.get_line_number () << ", character " << context.get_char_number () << ", code ";
		switch (ex.code()) {
			case Glib::MarkupError::BAD_UTF8:
				std::cerr << "Bad UTF8\n";
				break;
			case Glib::MarkupError::EMPTY:
				std::cerr << "Empty\n";
				break;
			case Glib::MarkupError::PARSE:
				std::cerr << "Parse error\n";
				break;
			default:
				std::cerr << (int)ex.code() << "\n";
		}
	}
	context.end_parse ();
}


void TagWindow::loadLibrary ()
{
	Gnome::Vfs::Handle libfile;

	Glib::ustring libfilename = Glib::get_home_dir () + "/.referencer.lib";

	Glib::RefPtr<Gnome::Vfs::Uri> liburi = Gnome::Vfs::Uri::create (libfilename);

	bool exists = liburi->uri_exists ();
	if (!exists) {
		std::cerr << "TagWindow::loadLibrary: Library file '" <<
			libfilename << "' not found\n";
		return;
	}

	libfile.open (libfilename, Gnome::Vfs::OPEN_READ);

	Glib::RefPtr<Gnome::Vfs::FileInfo> fileinfo;
	fileinfo = libfile.get_file_info ();

	char *buffer = (char *) malloc (sizeof(char) * (fileinfo->get_size() + 1));
	libfile.read (buffer, fileinfo->get_size());
	buffer[fileinfo->get_size()] = 0;

	Glib::ustring rawtext = buffer;
	free (buffer);
	libfile.close ();
	readXML (rawtext);
}


void TagWindow::saveLibrary ()
{
	Gnome::Vfs::Handle libfile;

	/*Glib::ustring libfilename =
		Gnome::Vfs::expand_initial_tilde("~/.pdfdivine.lib");*/
	Glib::ustring libfilename = "/home/jcspray/.pdfdivine.lib";

	try {
		libfile.create (libfilename, Gnome::Vfs::OPEN_WRITE,
			false, Gnome::Vfs::PERM_USER_READ | Gnome::Vfs::PERM_USER_WRITE);
	} catch (const Gnome::Vfs::exception ex) {
		std::cerr << "TagWindow::saveLibrary: "
			"exception in create '" << ex.what() << "'\n";
		return;
	}

	Glib::ustring rawtext = writeXML ();

	libfile.write (rawtext.c_str(), strlen(rawtext.c_str()));

	libfile.close ();
}


void TagWindow::onDoiLookupDoc ()
{
	Document *doc = getSelectedDoc ();
	if (doc) {
		Utility::StringPair ends = _global_prefs->getDoiLaunch ();
		Glib::ustring doi = doc->getBibData().getDoi();

		Gnome::Vfs::url_show (ends.first + doi + ends.second);
	} else {
		std::cerr << "Warning: TagWindow::onDoiLookupDoc: can't get doc\n";
	}
}


void TagWindow::onDivine ()
{
	std::vector<Document*> docpointers = getSelectedDocs ();

	std::vector<Document*>::iterator it = docpointers.begin ();
	std::vector<Document*>::iterator const end = docpointers.end ();
	for (; it != end; ++it) {
		//(*it)->readPDF ();
		(*it)->getBibData().getCrossRef ();
		(*it)->getBibData().print();
	}
}


void TagWindow::onOpenDoc ()
{
	Document *doc = getSelectedDoc ();
	if (doc && !doc->getFileName().empty()) {
			Gnome::Vfs::url_show (doc->getFileName());
	}
}


void TagWindow::onDocProperties ()
{
	Document *doc = getSelectedDoc ();
	if (doc) {
		if (!docpropertiesdialog_)
			docpropertiesdialog_ = new DocumentProperties;
		if (docpropertiesdialog_->show (doc))
			populateDocIcons ();
	}
}


void TagWindow::onPreferences ()
{
	_global_prefs->showDialog ();
}
