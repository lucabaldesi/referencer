
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

class Gtk::TreePath;
class Glib::ustring;

class Document;
class DocumentList;
class DocumentProperties;
class DocumentView;
class Library;
class TagList;

#define ALL_TAGS_UID -1
#define NO_TAGS_UID -2

#define DISPLAY_PROGRAM "Referencer"

class RefWindow {
	private:
		Gtk::Statusbar *statusbar_;
		Gtk::ProgressBar *progressbar_;
	public:
		//Gtk::Window * getWindow () {return window_;}
		void setStatusText (Glib::ustring const &text) {statusbar_->push (text, 0);};
		Gtk::ProgressBar *getProgressBar ()
			{return progressbar_;}
		void setSensitive (bool const sensitive);
		
		
		void addDocFiles (std::vector<Glib::ustring> const &filenames);


		/* Other main window UI */
		// DocumentView needs this for its tooltip
		Gtk::Window *window_;
		// DocumentView needs this for popup menu
		Glib::RefPtr<Gtk::UIManager> uimanager_;
		// DocumentView needs this for sensitibity
		Glib::RefPtr<Gtk::ActionGroup> actiongroup_;
		// DocumentView needs this for populateDocStore
		std::vector<int> filtertags_;
		
	private:
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

		/* The Documents View */
		DocumentView *docview_;

		/* The Tags View */
		Gtk::Widget *tagpane_;
		Gtk::VBox *taggerbox_;
		Glib::RefPtr<Gtk::TreeSelection> tagselection_;
		Gtk::TreeView *tagview_;
		bool tagselectionignore_;
		bool docselectionignore_;
		std::map<int, Gtk::ToggleButton*> taggerchecks_;
		bool ignoretaggerchecktoggled_;
		void taggerCheckToggled (Gtk::ToggleButton *check, int taguid);
		void tagSelectionChanged ();
		void tagClicked (GdkEventButton* event);
		void tagNameEdited (Glib::ustring const &text1, Glib::ustring const &text2);

		/* The Document Properties dialog */
		DocumentProperties *docpropertiesdialog_;
		void docSelectionChanged ();

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
		public:
		void onPasteBibtex (GdkAtom selection);
		private:
		void onCopyCite ();
		void onRemoveDoc ();
		void onGetMetadataDoc ();
		void onDeleteDoc ();
		void onRenameDoc ();
		public:
		void onOpenDoc ();
		void onDocProperties ();
		void onWebLinkDoc ();
		private:
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
		void onFind ();

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
		public:
		void updateStatusBar ();
		private:

		/* Handle dirtyness */
		bool ensureSaved ();
		void setDirty (bool const &dirty);
		bool getDirty () {return dirty_;}
		bool dirty_;

		/* Remember which file is open */
		Glib::ustring openedlib_;
		void setOpenedLib (Glib::ustring const &openedlib);


	public:
		RefWindow ();
		~RefWindow ();
		void run ();
};

#endif
