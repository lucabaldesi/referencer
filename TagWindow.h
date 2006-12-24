
#ifndef TAGWINDOW_H
#define TAGWINDOW_H

#include <vector>

class Document;
class DocumentList;
class TagList;
class Gtk::TreePath;
class Glib::ustring;

#define ALL_TAGS_UID -1

class TagWindow {

	private:
		int memberint;
		void populateDocIcons ();
		void populateTagList ();
		void constructUI ();
		void constructMenu ();
		

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
		
		Glib::RefPtr<Gtk::UIManager> uimanager_;
		Glib::RefPtr<Gtk::ActionGroup> actiongroup_;

		
		void docActivated (const Gtk::TreePath& path);
		void tagSelectionChanged ();
		void docSelectionChanged ();
		bool docClicked (GdkEventButton* event);
		void tagClicked (GdkEventButton* event);
		void onQuit (/*GdkEventAny *ev*/);
		void onCreateTag ();
		void onDeleteTag ();
		void onRenameTag ();
		void onAddDoc ();
		void onRemoveDoc ();
		void onExportBibtex ();
		
		Gtk::Menu doccontextmenu_;
		
		typedef enum {
			NO = 0,
			YES,
			MAYBE
		} YesNoMaybe;
		
		YesNoMaybe selectedDocsHaveTag (int uid);
		Glib::ustring writeXML ();
		void readXML (Glib::ustring XML);
		void loadLibrary ();
		void saveLibrary ();
		
	public:
		TagWindow ();
		~TagWindow ();
		void run ();
};

#endif
