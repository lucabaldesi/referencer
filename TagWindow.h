
#ifndef TAGWINDOW_H
#define TAGWINDOW_H

#include <vector>
#include <map>

class Document;
class DocumentList;
class TagList;
class Gtk::TreePath;
class Glib::ustring;

#define ALL_TAGS_UID -1
#define PROGRAM_NAME "TagWindow"
#define VERSION "0.0"

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
		
		Gtk::IconView *docsview_;
		Gtk::VBox *taggerbox_;
		std::map<int, Gtk::CheckButton*> taggerchecks_;
		bool ignoretaggerchecktoggled_;
		Gtk::Window *window_;
		
		Glib::RefPtr<Gtk::TreeSelection> tagselection_;
		Gtk::TreeView *tagview_;
		bool tagselectionignore_;
		std::vector<int> filtertags_;
		
		Glib::RefPtr<Gtk::UIManager> uimanager_;
		Glib::RefPtr<Gtk::ActionGroup> actiongroup_;

		void taggerCheckToggled (Gtk::CheckButton *check, int taguid);
		void docActivated (const Gtk::TreePath& path);
		void tagSelectionChanged ();
		void docSelectionChanged ();
		bool docClicked (GdkEventButton* event);
		void tagClicked (GdkEventButton* event);
		void tagNameEdited (Glib::ustring const &text1, Glib::ustring const &text2);
		void onQuit (/*GdkEventAny *ev*/);
		void onCreateTag ();
		void onDeleteTag ();
		void onRenameTag ();
		void onAddDocFile ();
		void onAddDocFolder ();
		void addDocFiles (std::vector<Glib::ustring> const &filenames);
		void onRemoveDoc ();
		void onDoiLookupDoc ();
		void onOpenDoc ();
		void onExportBibtex ();
		void onAbout ();
		
		Gtk::Menu doccontextmenu_;
		
		typedef enum {
			NO = 0,
			YES,
			MAYBE
		} YesNoMaybe;
		
		YesNoMaybe selectedDocsHaveTag (int uid);
		Document *getSelectedDoc ();
		Glib::ustring writeXML ();
		void readXML (Glib::ustring XML);
		void loadLibrary ();
		void saveLibrary ();
		
		// Memory of where the user added files from
		Glib::ustring addfolder_;
		Glib::ustring exportfolder_;
		
	public:
		TagWindow ();
		~TagWindow ();
		void run ();
};

#endif
