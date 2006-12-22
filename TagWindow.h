
#ifndef TAGWINDOW_H
#define TAGWINDOW_H

class Document;
class DocumentList;
class TagList;
class Gtk::TreePath;

class TagWindow {
	private:
		void populateDocIcons ();
		void populateTagList ();
		void constructUI ();
		
		DocumentList *doclist_;
		TagList *taglist_;
		
		Gtk::TreeModelColumn<Glib::ustring> taguidcol_;
		Gtk::TreeModelColumn<Glib::ustring> tagnamecol_;

		Gtk::TreeModelColumn<Document*> docpointercol_;		
		Gtk::TreeModelColumn<Glib::ustring> docnamecol_;
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > docthumbnailcol_;
		
		Glib::RefPtr<Gtk::ListStore> tagstore_;
		Glib::RefPtr<Gtk::ListStore> iconstore_;
		
		Gtk::Window *window_;
		
	public:
		TagWindow ();
		void run ();
		
		void docActivated (const Gtk::TreePath& path);
};

#endif
