
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef TAGWINDOW_H
#define TAGWINDOW_H

#include <vector>
#include <map>

#include <gtkmm.h>

class Document;
class DocumentList;
class DocumentProperties;
class TagList;
class Gtk::TreePath;
class Glib::ustring;
class Library;

#define ALL_TAGS_UID -1
#define NO_TAGS_UID -2

#define PROGRAM_NAME "Referencer"
#define VERSION "1.0.4"

class TagWindow {


	private:
		Gtk::Statusbar *statusbar_;
		Gtk::ProgressBar *progressbar_;
	public:
		//Gtk::Window * getWindow () {return window_;}
		void setStatusText (Glib::ustring const &text) {statusbar_->push (text, 0);};
		Gtk::ProgressBar *getProgressBar () {return progressbar_;}
		void setSensitive (bool const sensitive);

	private:
		int memberint;
		void populateDocStore ();
		void populateTagList ();
		void constructUI ();
		void constructMenu ();

		Library *library_;

		Glib::RefPtr<Gtk::ListStore> tagstore_;
		Gtk::TreeModelColumn<int> taguidcol_;
		Gtk::TreeModelColumn<Glib::ustring> tagnamecol_;
		Gtk::TreeModelColumn<Pango::FontDescription> tagfontcol_;
		int sortTags (
			const Gtk::TreeModel::iterator& a,
			const Gtk::TreeModel::iterator& b);

		Glib::RefPtr<Gtk::ListStore> docstore_;
		Gtk::TreeModelColumn<Document*> docpointercol_;
		Gtk::TreeModelColumn<Glib::ustring> dockeycol_;
		Gtk::TreeModelColumn<Glib::ustring> doctitlecol_;
		Gtk::TreeModelColumn<Glib::ustring> docauthorscol_;
		Gtk::TreeModelColumn<Glib::ustring> docyearcol_;
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > docthumbnailcol_;
		Glib::RefPtr<Gtk::TreeSelection> docslistselection_;

		// Keep an up-to-date local copy of this, since it's
		// used so often
		bool usinglistview_;


		/* The Document View */
		Gtk::IconView *docsiconview_;
		void docActivated (const Gtk::TreePath& path);
		Gtk::TreeView *docslistview_;
		// treeviews want a different prototype for the signal
		void docListActivated (const Gtk::TreePath& path, Gtk::TreeViewColumn*) {
			docActivated (path);
		}
		GtkWidget *doctooltip_;
		Document *hoverdoc_;
		void onDocMouseMotion (GdkEventMotion* event);
		void onDocMouseLeave (GdkEventCrossing *event);
		Gtk::Menu doccontextmenu_;
		Gtk::ScrolledWindow *docsiconscroll_;
		Gtk::ScrolledWindow *docslistscroll_;
		void docSelectionChanged ();
		Document *getSelectedDoc ();
		std::vector<Document*> getSelectedDocs ();
		int getSelectedDocCount ();
		typedef enum {
			NONE = 0,
			ALL,
			SOME
		} SubSet;
		SubSet selectedDocsHaveTag (int uid);
		bool docClicked (GdkEventButton* event);
		void onIconsDragData (
			const Glib::RefPtr <Gdk::DragContext> &context,
			int n1, int n2, const Gtk::SelectionData &sel, guint n3, guint n4);
		class Capabilities {
			public:
			bool weblink;
			bool open;
			bool getmetadata;
			Capabilities () {weblink = open = getmetadata = false;}
		};
		Capabilities getDocSelectionCapabilities ();

		/* The Tags View */
		Gtk::Widget *tagpane_;
		Gtk::VBox *taggerbox_;
		Glib::RefPtr<Gtk::TreeSelection> tagselection_;
		Gtk::TreeView *tagview_;
		bool tagselectionignore_;
		bool docselectionignore_;
		std::vector<int> filtertags_;
		std::map<int, Gtk::ToggleButton*> taggerchecks_;
		bool ignoretaggerchecktoggled_;
		void taggerCheckToggled (Gtk::ToggleButton *check, int taguid);
		void tagSelectionChanged ();
		void tagClicked (GdkEventButton* event);
		void tagNameEdited (Glib::ustring const &text1, Glib::ustring const &text2);

		/* The search box */
		Gtk::Entry *searchentry_;
		void onSearchChanged ();

		/* The Document Properties dialog */
		DocumentProperties *docpropertiesdialog_;

		/* Other main window UI */
		Gtk::Window *window_;
		Glib::RefPtr<Gtk::UIManager> uimanager_;
		Glib::RefPtr<Gtk::ActionGroup> actiongroup_;


		void onWorkOfflineToggled ();
		void onQuit ();
		void onUseListViewToggled ();
		void onShowTagPaneToggled ();
		void onCreateTag ();
		void onDeleteTag ();
		void onRenameTag ();
		void onAddDocUnnamed ();
		void onAddDocByDoi ();
		void onAddDocFile ();
		void onAddDocFolder ();
		void addDocFiles (std::vector<Glib::ustring> const &filenames);
		void onPasteBibtex (GdkAtom selection);
		void onCopyCite ();
		void onRemoveDoc ();
		void onWebLinkDoc ();
		void onGetMetadataDoc ();
		void onDeleteDoc ();
		void onRenameDoc ();
		void onOpenDoc ();
		void onDocProperties ();
		void onIntroduction ();
		void onAbout ();
		void onNewLibrary ();
		void onSaveLibrary ();
		void onSaveAsLibrary ();
		void onOpenLibrary ();
		void onExportBibtex ();
		void onManageBibtex ();
		void manageBrowseDialog (Gtk::Entry *entry);
		void onImport ();
		void onPreferences ();

		/* WM events */
		bool onDelete (GdkEventAny *ev);
		void onResize (GdkEventConfigure *event);


		/* Pick up preference changes */
		void onShowTagPanePrefChanged ();
		void onUseListViewPrefChanged ();
		void onWorkOfflinePrefChanged ();

		/* Remember which folders the user browsed last */
		Glib::ustring addfolder_;
		Glib::ustring exportfolder_;
		Glib::ustring libraryfolder_;

		/* Update UI text */
		void updateTitle ();
		void updateStatusBar ();

		/* Handle dirtyness */
		bool ensureSaved ();
		void setDirty (bool const &dirty);
		bool getDirty () {return dirty_;}
		bool dirty_;

		/* Remember which file is open */
		Glib::ustring openedlib_;
		void setOpenedLib (Glib::ustring const &openedlib);


	public:
		TagWindow ();
		~TagWindow ();
		void run ();
};

#endif
