
#include <gtkmm.h>
#include <libgnomeuimm.h>
#include <libgnomevfsmm.h>

// for ostringstream
#include <sstream>

#include "TagList.h"
#include "DocumentList.h"

int main (int argc, char **argv)
{

	TagList *taglist = new TagList();
	taglist->newTag("foo", Tag::ATTACH);
	taglist->newTag("bar", Tag::ATTACH);
	
	DocumentList *doclist = new DocumentList();
	doclist->newDoc("file:///home/jcspray/Desktop/sc06/pap104.pdf");
	doclist->newDoc("file:///home/jcspray/Desktop/sc06/pap107.pdf");

	//Gnome::Main gui (argc, argv);
	Gnome::Main gui ("TagWindow", "0.0.0", Gnome::UI::module_info_get(), argc, argv);
	//Glib::thread_init();
	//Gnome::UI::module_info_get();

	Gtk::Window *window = new Gtk::Window(Gtk::WINDOW_TOPLEVEL);
	window->set_default_size (500, 500);

	Gtk::HPaned hpaned;
	window->add (hpaned);

	// Create the store for the tag list
	Gtk::TreeModel::ColumnRecord tagcols;
	Gtk::TreeModelColumn<Glib::ustring> uidcol;
	Gtk::TreeModelColumn<Glib::ustring> tagnamecol;
	tagcols.add(uidcol);
	tagcols.add(tagnamecol);
	
	Glib::RefPtr<Gtk::ListStore> tagstore = Gtk::ListStore::create(tagcols);

	Gtk::TreeModel::iterator all = tagstore->append();
	(*all)[uidcol] = "-1";
	(*all)[tagnamecol] = "All";
	
	// Populate from taglist
	std::vector<Tag> tagvec = taglist->getTags();
	std::vector<Tag>::iterator it = tagvec.begin();
	std::vector<Tag>::iterator const end = tagvec.end();
	for (; it != end; ++it) {
		Gtk::TreeModel::iterator item = tagstore->append();
		std::ostringstream uidstr;
		uidstr << (*it).uid_;
		(*item)[uidcol] = uidstr.str();
		(*item)[tagnamecol] = (*it).name_;
	}
	
	// Create the treeview for the tag list
	Gtk::TreeView tags(tagstore);
	tags.append_column("UID", uidcol);
	tags.append_column("Tag name", tagnamecol);
	hpaned.pack1(tags, true, false);

	// Create the store for the document icons
	Gtk::TreeModel::ColumnRecord iconcols;
	Gtk::TreeModelColumn<Glib::ustring> filenamecol;
	Gtk::TreeModelColumn<Glib::ustring> displaynamecol;
	Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > thumbnailcol;
	iconcols.add(displaynamecol);
	iconcols.add(thumbnailcol);
	iconcols.add(filenamecol);

	Glib::RefPtr<Gtk::ListStore> iconstore = Gtk::ListStore::create(iconcols);

	Glib::RefPtr<Gnome::UI::ThumbnailFactory> thumbfac = 
		Gnome::UI::ThumbnailFactory::create (Gnome::UI::THUMBNAIL_SIZE_NORMAL);

	// Populate from doclist
	std::vector<Document>& docvec = doclist->getDocs();
	std::vector<Document>::iterator docit = docvec.begin();
	std::vector<Document>::iterator const docend = docvec.end();
	for (; docit != docend; ++docit) {
		Gtk::TreeModel::iterator item = iconstore->append();
		(*item)[displaynamecol] = (*docit).getDisplayName();
		
		std::string pdffile = (*docit).getFileName();
		std::cerr << "pdf file " << pdffile << std::endl;
		
		time_t mtime;
		Glib::RefPtr<Gnome::Vfs::Uri> uri = Gnome::Vfs::Uri::create (pdffile);
		Glib::RefPtr<Gnome::Vfs::FileInfo> fileinfo = uri->get_file_info ();
		mtime = fileinfo->get_modification_time ();
		std::cerr << "mtime " << mtime << std::endl;
		
		
		std::string thumbfile;
		thumbfile = thumbfac->lookup (pdffile, mtime);
		std::cerr << "thumbnail file " << thumbfile << std::endl;
		
		Glib::RefPtr<Gdk::Pixbuf> thumbnail;
		if (thumbfile.empty()) {
			std::cerr << "Couldn't get thumbnail for " << pdffile << "\n";
		} else {
			thumbnail = Gdk::Pixbuf::create_from_file(thumbfile);
		}
		
		std::auto_ptr<Glib::Error> error;
		(*item)[thumbnailcol] = thumbnail;
	}

	// Create the IconView for the document icons
	Gtk::IconView icons(iconstore);
	icons.set_text_column(displaynamecol);
	icons.set_pixbuf_column(thumbnailcol);
	hpaned.pack2(icons, true, false);



	window->show_all();

	gui.run (*window);
}

