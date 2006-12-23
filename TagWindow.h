
#ifndef TAGWINDOW_H
#define TAGWINDOW_H

#include <vector>

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
		
		Gtk::TreeModelColumn<int> taguidcol_;
		Gtk::TreeModelColumn<Glib::ustring> tagnamecol_;

		Gtk::TreeModelColumn<Document*> docpointercol_;		
		Gtk::TreeModelColumn<Glib::ustring> docnamecol_;
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > docthumbnailcol_;
		
		Glib::RefPtr<Gtk::ListStore> tagstore_;
		Glib::RefPtr<Gtk::ListStore> iconstore_;
		
		Gtk::IconView* docsview_;
		
		Gtk::Window *window_;
		
		Glib::RefPtr<Gtk::TreeSelection> tagselection_;
		std::vector<int> filtertags_;
		
		void docActivated (const Gtk::TreePath& path);
		void tagSelectionChanged ();
		bool docClicked (GdkEventButton* event);
		
		Gtk::Menu doccontextmenu_;
		
		typedef enum {
			NO = 0,
			YES,
			MAYBE
		} YesNoMaybe;
		
		YesNoMaybe selectedDocsHaveTag (int uid);
		
	public:
		TagWindow ();
		void run ();
		

};

#endif