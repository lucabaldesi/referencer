
#include <gtkmm.h>

#include "TagList.h"

int main (int argc, char **argv)
{

	TagList taglist;
	taglist.newTag("Mongo", Tag::ATTACH);
	taglist.newTag("Bongo", Tag::ATTACH);
	taglist.print ();
	
	return 0;

	Gtk::Main gui (argc, argv);

	Gtk::Window *window = new Gtk::Window(Gtk::WINDOW_TOPLEVEL);

	Gtk::HBox box;

	window->add (box);

	Gtk::TreeModel::ColumnRecord cols;
	Gtk::TreeModelColumn<Glib::ustring> filenamecol;
	Gtk::TreeModelColumn<Glib::ustring> displaynamecol;
	//Gtk::TreeModelColumn<Gdk::Pixbuf> thumbnailcol;
	cols.add(displaynamecol);
	//cols.add(thumbnailcol);
	cols.add(filenamecol);

	Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(cols);

	(*store->append())[displaynamecol] = "Display Name";

	Gtk::IconView icons(store);
	icons.set_text_column(displaynamecol);
	//icons.set_pixbuf_column(thumbnailcol);

	box.pack_start(icons);

	window->show_all();

	gui.run (*window);
}

