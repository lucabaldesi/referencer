
#include <gtkmm.h>
#include <libgnomeuimm.h>
#include <libgnomevfsmm.h>

// for ostringstream
#include <sstream>

#include "TagWindow.h"
#include "TagList.h"
#include "DocumentList.h"

int main (int argc, char **argv)
{
	Gnome::Main gui ("TagWindow", "0.0.0", Gnome::UI::module_info_get(), argc, argv);
	Gnome::Vfs::init ();

	TagWindow window;
	
	window.run();
	
	return 0;
}


void TagWindow::populateDocIcons ()
{
	Glib::RefPtr<Gnome::UI::ThumbnailFactory> thumbfac = 
		Gnome::UI::ThumbnailFactory::create (Gnome::UI::THUMBNAIL_SIZE_NORMAL);

	// Populate from doclist_
	std::vector<Document>& docvec = doclist_->getDocs();
	std::vector<Document>::iterator docit = docvec.begin();
	std::vector<Document>::iterator const docend = docvec.end();
	for (; docit != docend; ++docit) {
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
}


void TagWindow::populateTagList ()
{
	tagstore_->clear();

	Gtk::TreeModel::iterator all = tagstore_->append();
	(*all)[taguidcol_] = "-1";
	(*all)[tagnamecol_] = "<b>All</b>";

	// Populate from taglist_
	std::vector<Tag> tagvec = taglist_->getTags();
	std::vector<Tag>::iterator it = tagvec.begin();
	std::vector<Tag>::iterator const end = tagvec.end();
	for (; it != end; ++it) {
		Gtk::TreeModel::iterator item = tagstore_->append();
		std::ostringstream uidstr;
		uidstr << (*it).uid_;
		(*item)[taguidcol_] = uidstr.str();
		(*item)[tagnamecol_] = (*it).name_;
	}
}


void TagWindow::run ()
{
	Gnome::Main::run (*window_);
}


void TagWindow::constructUI ()
{
window_ = new Gtk::Window(Gtk::WINDOW_TOPLEVEL);
	window_->set_default_size (500, 500);

	Gtk::HPaned *hpaned = new Gtk::HPaned();
	window_->add (*hpaned);

	// Create the store for the tag list
	Gtk::TreeModel::ColumnRecord tagcols;
	tagcols.add(taguidcol_);
	tagcols.add(tagnamecol_);
	tagstore_ = Gtk::ListStore::create(tagcols);

	// Create the treeview for the tag list
	Gtk::TreeView *tags = new Gtk::TreeView(tagstore_);
	tags->append_column("UID", taguidcol_);
	Gtk::CellRendererText *render = Gtk::manage(new Gtk::CellRendererText());
	Gtk::TreeView::Column *namecol = Gtk::manage(
		new Gtk::TreeView::Column ("Tag name", *render));
	namecol->add_attribute (render->property_markup (), tagnamecol_);
	tags->append_column (*namecol);

	
	// Frame it and put it in the HPaned
	Gtk::Frame *tagsframe = new Gtk::Frame ();
	tagsframe->add(*tags);
	hpaned->pack1(*tagsframe, Gtk::FILL);
	

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
	icons->signal_item_activated().connect (sigc::mem_fun (*this, &TagWindow::docActivated));
	
	// Frame it and put it in the HPaned
	Gtk::Frame *iconsframe = new Gtk::Frame ();
	iconsframe->add(*icons);
	hpaned->pack2(*iconsframe, Gtk::EXPAND);
}


TagWindow::TagWindow ()
{
	taglist_ = new TagList();
	taglist_->newTag("Grid", Tag::ATTACH);
	taglist_->newTag("OpenMP", Tag::ATTACH);
	
	doclist_ = new DocumentList();
	doclist_->newDoc("file:///home/jcspray/Desktop/sc06/pap104.pdf");
	doclist_->newDoc("file:///home/jcspray/Desktop/sc06/pap107.pdf");
	doclist_->newDoc("file:///home/jcspray/Desktop/sc06/pap108.pdf");
	doclist_->newDoc("file:///home/jcspray/Desktop/sc06/pap119.pdf");
	doclist_->newDoc("file:///home/jcspray/Desktop/sc06/pap122.pdf");

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

// When we add a tag we need to escape it to avoid markup

