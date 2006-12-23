
#include <gtkmm.h>
#include <libgnomeuimm.h>
#include <libgnomevfsmm.h>

// for ostringstream
#include <sstream>

#include "TagWindow.h"
#include "TagList.h"
#include "DocumentList.h"

#include "LibraryParser.h"

int main (int argc, char **argv)
{
	Gnome::Main gui ("TagWindow", "0.0.0",
		Gnome::UI::module_info_get(), argc, argv);

	Gnome::Vfs::init ();

	TagWindow window;
	
	window.run();
	
	return 0;
}


void TagWindow::populateDocIcons ()
{
	Glib::RefPtr<Gnome::UI::ThumbnailFactory> thumbfac = 
		Gnome::UI::ThumbnailFactory::create (Gnome::UI::THUMBNAIL_SIZE_NORMAL);

	// Should save selection and restore at end
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
		
		std::string pdffile = (*docit).getFileName();
		//std::cerr << "pdf file " << pdffile << std::endl;
		
		time_t mtime;
		Glib::RefPtr<Gnome::Vfs::Uri> uri = Gnome::Vfs::Uri::create (pdffile);
		if (!uri->uri_exists()) {
			std::cerr << "File " << pdffile <<
				" does not exist, can't thumbnail" << std::endl;
			continue;
		}
		Glib::RefPtr<Gnome::Vfs::FileInfo> fileinfo = uri->get_file_info ();
		mtime = fileinfo->get_modification_time ();
		//std::cerr << "mtime " << mtime << std::endl;
		
		std::string thumbfile;
		thumbfile = thumbfac->lookup (pdffile, mtime);
		//std::cerr << "thumbnail file " << thumbfile << std::endl;
		
		Glib::RefPtr<Gdk::Pixbuf> thumbnail;
		if (thumbfile.empty()) {
			std::cerr << "Couldn't get thumbnail for " << pdffile << "\n";
		} else {
			thumbnail = Gdk::Pixbuf::create_from_file(thumbfile);
		}
		
		std::auto_ptr<Glib::Error> error;
		(*item)[docthumbnailcol_] = thumbnail;
	}
	
	// Should restore initial selection here
	
}


void TagWindow::populateTagList ()
{
	tagstore_->clear();

	// Should save selection and restore at end

	Gtk::TreeModel::iterator all = tagstore_->append();
	(*all)[taguidcol_] = ALL_TAGS_UID;
	(*all)[tagnamecol_] = "<b>All</b>";

	// Populate from taglist_
	std::vector<Tag> tagvec = taglist_->getTags();
	std::vector<Tag>::iterator it = tagvec.begin();
	std::vector<Tag>::iterator const end = tagvec.end();
	for (; it != end; ++it) {
		Gtk::TreeModel::iterator item = tagstore_->append();
		/*std::ostringstream uidstr;
		uidstr << (*it).uid_;*/
		(*item)[taguidcol_] = (*it).uid_;//uidstr.str();
		(*item)[tagnamecol_] = (*it).name_;
	}
	
	// Should restore original selection
	// instead just select first row
	tagselection_->select (tagstore_->children().begin());
}


void TagWindow::run ()
{
	Gnome::Main::run (*window_);
}


void TagWindow::constructUI ()
{
	window_ = new Gtk::Window(Gtk::WINDOW_TOPLEVEL);
	window_->set_default_size (500, 500);
	/*window_->signal_delete_event().connect (
		sigc::mem_fun (*this, &TagWindow::onQuit));*/

	constructMenu ();

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);
	window_->add (*vbox);
	
	vbox->pack_start (*uimanager_->get_widget("/MenuBar"), false, false, 0);
	Gtk::HPaned *hpaned = Gtk::manage(new Gtk::HPaned());
	vbox->pack_start (*hpaned, true, true, 6);
	
	vbox = Gtk::manage(new Gtk::VBox);
	hpaned->pack1(*vbox, Gtk::FILL);

	// Create the store for the tag list
	Gtk::TreeModel::ColumnRecord tagcols;
	tagcols.add(taguidcol_);
	tagcols.add(tagnamecol_);
	tagstore_ = Gtk::ListStore::create(tagcols);


	// Create the treeview for the tag list
	Gtk::TreeView *tags = new Gtk::TreeView(tagstore_);
	//tags->append_column("UID", taguidcol_);
	Gtk::CellRendererText *render = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeView::Column *namecol = Gtk::manage(
		new Gtk::TreeView::Column ("Tags", *render));
	namecol->add_attribute (render->property_markup (), tagnamecol_);
	tags->append_column (*namecol);


	tagselection_ = tags->get_selection();
	tagselection_->signal_changed().connect_notify (
		sigc::mem_fun (*this, &TagWindow::tagSelectionChanged));
	tagselection_->set_mode (Gtk::SELECTION_MULTIPLE);
	
	// Frame it and put it in the HPaned
	Gtk::Frame *tagsframe = new Gtk::Frame ();
	tagsframe->add(*tags);
	vbox->pack_start(*tagsframe, true, true, 0);
	
	Gtk::Toolbar& tagbar = (Gtk::Toolbar&) *uimanager_->get_widget("/TagBar");
	vbox->pack_start (tagbar, false, false, 0);
	tagbar.set_toolbar_style (Gtk::TOOLBAR_ICONS);
	

	// Create the store for the document icons
	Gtk::TreeModel::ColumnRecord iconcols;
	iconcols.add(docpointercol_);
	iconcols.add(docnamecol_);
	iconcols.add(docthumbnailcol_);
	iconstore_ = Gtk::ListStore::create(iconcols);

	// Create the IconView for the document icons
	Gtk::IconView *icons = new Gtk::IconView(iconstore_);
	icons->set_text_column(docnamecol_);
	icons->set_pixbuf_column(docthumbnailcol_);
	icons->signal_item_activated().connect (
		sigc::mem_fun (*this, &TagWindow::docActivated));
		
	icons->signal_button_press_event().connect(
		sigc::mem_fun (*this, &TagWindow::docClicked));
	
	icons->set_selection_mode (Gtk::SELECTION_MULTIPLE);
	
	docsview_ = icons;
	
	// Frame it and put it in the HPaned
	Gtk::Frame *iconsframe = new Gtk::Frame ();
	iconsframe->add(*icons);
	hpaned->pack2(*iconsframe, Gtk::EXPAND);
}


void TagWindow::constructMenu ()
{
	actiongroup_ = Gtk::ActionGroup::create();

	actiongroup_->add ( Gtk::Action::create("LibraryMenu", "_Library") );
	actiongroup_->add( Gtk::Action::create("ExportBibtex",
		Gtk::Stock::CONVERT, "E_xport to BibTeX"),
  	sigc::mem_fun(*this, &TagWindow::onExportBibtex));
	actiongroup_->add( Gtk::Action::create("Quit", Gtk::Stock::QUIT),
  	sigc::mem_fun(*this, &TagWindow::onQuit));

	actiongroup_->add ( Gtk::Action::create("TagMenu", "_Tags") );
	actiongroup_->add( Gtk::Action::create(
		"CreateTag", Gtk::Stock::NEW, "_Create Tag"),
  	sigc::mem_fun(*this, &TagWindow::onCreateTag));
	actiongroup_->add( Gtk::Action::create(
		"DeleteTag", Gtk::Stock::DELETE, "_Delete Tag"),
  	sigc::mem_fun(*this, &TagWindow::onDeleteTag));

	uimanager_ = Gtk::UIManager::create ();
	uimanager_->insert_action_group (actiongroup_);
	Glib::ustring ui = 
		"<ui>"
		"  <menubar name='MenuBar'>"
		"    <menu action='LibraryMenu'>"
		"      <menuitem action='ExportBibtex'/>"
		"      <menuitem action='Quit'/>"
		"    </menu>"
		"    <menu action='TagMenu'>"
		"      <menuitem action='CreateTag'/>"
		"      <menuitem action='DeleteTag'/>"
		"    </menu>"
		"  </menubar>"
		"  <toolbar name='TagBar'>"
		"    <toolitem action='CreateTag'/>"
		"    <toolitem action='DeleteTag'/>"
		"  </toolbar>"
		"</ui>";
	
	uimanager_->add_ui_from_string (ui);
}


TagWindow::TagWindow ()
{
	taglist_ = new TagList();
	int griduid = taglist_->newTag("Grid", Tag::ATTACH);
	int ompuid = taglist_->newTag("OpenMP", Tag::ATTACH);
	
	Document *mydoc;
	
	doclist_ = new DocumentList();
	doclist_->newDoc("file:///home/jcspray/Desktop/sc06/pap104.pdf");
	mydoc = doclist_->newDoc("file:///home/jcspray/Desktop/sc06/pap107.pdf");
	mydoc->setTag(griduid);
	doclist_->newDoc("file:///home/jcspray/Desktop/sc06/pap108.pdf");
	mydoc = doclist_->newDoc("file:///home/jcspray/Desktop/sc06/pap119.pdf");
	mydoc->setTag(ompuid);
	mydoc = doclist_->newDoc("file:///home/jcspray/Desktop/sc06/pap122.pdf");
	mydoc->setTag(griduid);
	mydoc->setTag(ompuid);

	readXML (writeXML ());

	constructUI ();
	populateDocIcons ();
	populateTagList ();
	window_->show_all ();
}


void TagWindow::docActivated (const Gtk::TreeModel::Path& path)
{
	Gtk::ListStore::iterator it = iconstore_->get_iter (path);
	Document *doc = (*it)[docpointercol_];
	std::cerr <<  doc << std::endl;
	std::cerr << doc->getFileName() << std::endl;
	Gnome::Vfs::url_show (doc->getFileName());
}


void TagWindow::tagSelectionChanged ()
{
	Gtk::TreeSelection::ListHandle_Path paths =
		tagselection_->get_selected_rows ();
	
	filtertags_.clear();
	
	bool allselected = false;
	bool anythingselected = false;
	
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
	
	actiongroup_->get_action("DeleteTag")->set_sensitive (
		anythingselected && !allselected);
	
	populateDocIcons ();
}


bool TagWindow::docClicked (GdkEventButton* event)
{
  if((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
    //m_Menu_Popup.popup(event->button, event->time);
		Gtk::TreeModel::Path clickedpath =
			docsview_->get_path_at_pos ((int)event->x, (int)event->y);

		if (clickedpath.empty()) {
			if (docsview_->get_selected_items().empty()) {
				std::cerr << "Nothing clicked, nothing selected\n";
				return true;
			}
		} else {
			if (!docsview_->path_is_selected (clickedpath)) {
				docsview_->unselect_all ();
				docsview_->select_path (clickedpath);
			}
		}
		// Should also check if we're currently multiple-item selected
		// in which case don't reset selection

		
		doccontextmenu_.items().clear();
		for (std::vector<Tag>::iterator tagit = taglist_->getTags().begin();
		     tagit != taglist_->getTags().end(); ++tagit) {
			Gtk::CheckMenuItem *item = Gtk::manage (new Gtk::CheckMenuItem((*tagit).name_));
			YesNoMaybe state = selectedDocsHaveTag ((*tagit).uid_);
			if (state == YES) {
				std::cerr << (*tagit).name_ << " : " << "Yes\n";
				item->set_active (true);
			} else if (state == MAYBE) {
				std::cerr << (*tagit).name_ << " : " << "Maybe\n";
				item->set_inconsistent (true);
				std::cerr << item->get_inconsistent() << std::endl;
			}
			doccontextmenu_.append(*item);
		}
		doccontextmenu_.show_all();
		doccontextmenu_.popup (event->button, event->time);
		
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


void TagWindow::onQuit (/*GdkEventAny *ev*/)
{
	Gnome::Main::quit ();
}


void TagWindow::onCreateTag  ()
{
	// For intelligent tags we'll need a dialog here.
	taglist_->newTag ("Unnamed Tag", Tag::ATTACH);
	populateTagList();
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
			"<b><big>Really delete tag?</big></b>\n\n"
			"Are you sure you want to delete \""
			+ (*iter)[tagnamecol_]
			+ "\"?\n\n"
			+ "When a tag is deleted it is also permanently removed "
			+ "from all documents it is currently associated with";
		
		Gtk::MessageDialog confirmdialog (
			message, true, Gtk::MESSAGE_QUESTION,
			Gtk::BUTTONS_NONE, true);
		
		confirmdialog.add_button (Gtk::Stock::CANCEL, 0);
		confirmdialog.add_button (Gtk::Stock::DELETE, 1);
		
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


void TagWindow::onExportBibtex ()
{
}


Glib::ustring TagWindow::writeXML ()
{
	std::ostringstream out;
	out << "<library>\n";
	taglist_->writeXML (out);
	doclist_->writeXML (out);
	out << "</library>\n";
	std::cerr << out.str();
	return out.str ();
}


void TagWindow::readXML (Glib::ustring XMLtext)
{
	taglist_->clear();
	doclist_->clear();
	LibraryParser parser (*taglist_, *doclist_);
	Glib::Markup::ParseContext context (parser);
	context.parse (XMLtext);
	context.end_parse ();
}

