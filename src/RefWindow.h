
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
#include <libglademm.h>

#include "Plugin.h"

class Gtk::TreePath;
class Glib::ustring;

class Document;
class DocumentList;
class DocumentProperties;
class DocumentView;
class Library;
class Progress;
class TagList;

#define ALL_TAGS_UID -1
#define NO_TAGS_UID -2
#define SEPARATOR_UID -3

#define DISPLAY_PROGRAM "Referencer"

/**
 * This is a bit of a mess in terms of what's public.
 * The public members are generally used by things like DocumentView
 */

class RefWindow {

	public:
		RefWindow ();
		~RefWindow ();
		void run ();
	
		void setStatusText (Glib::ustring const &text) {statusbar_->push (text, 0);};
		Gtk::ProgressBar *getProgressBar ()
			{return progressbar_;}
		Gtk::Statusbar *getStatusBar ()
			{return statusbar_;}
		void setSensitive (bool const sensitive);

		void addDocFiles (std::vector<Glib::ustring> const &filenames);

		Progress *getProgress ()
			{return progress_;}

		/* Other main window UI */
		// DocumentView needs this for its tooltip
		Gtk::Window *window_;
		// DocumentView needs this for popup menu
		Glib::RefPtr<Gtk::UIManager> uimanager_;
		// DocumentView needs this for sensitivity
		Glib::RefPtr<Gtk::ActionGroup> actiongroup_;
		// DocumentView needs this for populateDocStore
		std::vector<int> filtertags_;
		
		/* DocumentView needs this for inline edits */
		void setDirty (bool const &dirty);

		void openProperties (Document *doc);
	private:
		bool ignoreDocSelectionChanged_;
		void clearTagList ();
		void populateTagList ();
		void updateTagSizes ();
		/* Construct main window */
		void constructUI ();
		/* Construct uimanager stuff */
		void constructMenu ();
		/* Debugging: print UI tree and action list */
		void printUI ();

		Library *library_;

		Progress *progress_;

		/* The status bar */
		Gtk::Statusbar *statusbar_;
		Gtk::ProgressBar *progressbar_;
		Gtk::Image *offlineicon_;

		/* The Documents View */
		DocumentView *docview_;

		/* Which plugin have added UI elements */
		std::map<Glib::ustring, Gtk::UIManager::ui_merge_id> pluginUI_;

		/* The Tags View */
		Glib::ustring tagoldname_;
		Glib::RefPtr<Gtk::ListStore> tagstore_;
		Gtk::TreeModelColumn<int> taguidcol_;
		Gtk::TreeModelColumn<Glib::ustring> tagnamecol_;
		Gtk::TreeModelColumn<Pango::FontDescription> tagfontcol_;
		int sortTags (
			const Gtk::TreeModel::iterator& a,
			const Gtk::TreeModel::iterator& b);
		Gtk::Widget *tagpane_;
		Glib::RefPtr<Gtk::TreeSelection> tagselection_;
		Gtk::TreeView *tagview_;
		bool ignoreTagSelectionChanged_;
		
		/* Notes Stuff */
		Gtk::VBox *notespane_;
		Gtk::Label *noteslabel_;
		Gtk::TextView *notesview_;
		bool notesbufferignore_;
		Glib::RefPtr<Gtk::TextBuffer> notesbuffer_;
		void updateNotesPane ();
		void onNotesExport ();
		void onNotesChanged ();
		void onNotesClose ();
		void onNotesPaneResize (Gdk::Rectangle &allocation);

		class TagUI {
			public:
			Glib::RefPtr<Gtk::ToggleAction> action;
			Gtk::UIManager::ui_merge_id merge;
		};
		typedef std::map<int, TagUI> TaggerUIMap;
		TaggerUIMap taggerUI_;

		bool ignoreTaggerActionToggled_;
		void taggerActionToggled (Glib::RefPtr<Gtk::ToggleAction> action, int taguid);
		void tagSelectionChanged ();

		void tagClicked (GdkEventButton* event);
		void tagNameEdited (Glib::ustring const &text1, Glib::ustring const &text2);
		void tagNameEditingStarted (Gtk::CellEditable *, Glib::ustring const &path);
		bool tagSeparator (const Glib::RefPtr<Gtk::TreeModel> &model, const Gtk::TreeModel::iterator &iter);
		void tagCellRenderer (Gtk::CellRenderer *cell, const Gtk::TreeModel::iterator &iter) const;

		/* The Document Properties dialog */
		DocumentProperties *docpropertiesdialog_;

		void docSelectionChanged ();

		void onWorkOfflineToggled ();
		void updateOfflineIcon ();
		void onQuit ();
		void onUseListViewToggled ();
		void onShowTagPaneToggled ();
		void onShowNotesPaneToggled ();
		int  createTag ();
		void onCreateTag ();
		void onCreateAndAttachTag ();
		void onDeleteTag ();
		void onRenameTag ();
		void onAddDocUnnamed ();
		void onAddDocById ();
		void onAddDocFile ();
		void onAddDocFolder ();
		void onSearch ();

		/* Helpers for addDocFiles */
		void onAddDocFilesCancel       (Gtk::Button *button, Gtk::ProgressBar *progress);
		bool cancelAddDocFiles_;
		void onAddDocFilesTag          (std::vector<Document*> &docs);

		class TaggerDialog : public Gtk::Dialog {
			Gtk::TreeModelColumn<Glib::ustring> nameColumn_;
			Gtk::TreeModelColumn<int>           uidColumn_;
			Gtk::TreeModelColumn<bool>          selectedColumn_;
			Glib::RefPtr<Gtk::ListStore>        model_;

			Gtk::Dialog *dialog_;
			Gtk::TreeView *view_;
			Gtk::ScrolledWindow *scroll_;
			Gtk::CellRendererToggle *toggle_;

			std::map<int, bool> selections_;

			void toggled (Glib::ustring const &path);
			void populate ();
			void onCreateTag ();

			RefWindow *parent_;
			TagList *taglist_;
			public:
			TaggerDialog (RefWindow *window, TagList *taglist);
			std::vector<int> tagPrompt ();
		};
		class SearchDialog {
			public:
			SearchDialog (Library &library, DocumentView &view);
			void run (); 

			static bool progressCallback (void *ptr);
			bool progress ();

			static bool pluginsExist ();

			private:
			Library &library_;
			DocumentView &documentView_;

			void search ();
			void addSelected ();
			void updateSensitivity ();
			void resultActivated (const Gtk::TreePath &path, Gtk::TreeViewColumn * col);

			Glib::RefPtr<Gnome::Glade::Xml> xml_;

			Gtk::Dialog *dialog_;
			Gtk::Button *searchButton_;
			Gtk::Button *addButton_;
			Gtk::Entry *searchEntry_;
			Gtk::ComboBox *pluginCombo_;
			Gtk::TreeView *resultView_;
			Gtk::Button *cancelButton_;
			Gtk::ProgressBar *progressbar_;

			Glib::RefPtr<Gtk::ListStore>        pluginModel_;
			Gtk::TreeModelColumn<Plugin*>       pluginPtrColumn_;
			Gtk::TreeModelColumn<Glib::ustring> pluginNameColumn_;

			Glib::RefPtr<Gtk::ListStore>        resultModel_;
			Gtk::TreeModelColumn<Glib::ustring> resultTokenColumn_;
			Gtk::TreeModelColumn<Glib::ustring> resultTitleColumn_;
			Gtk::TreeModelColumn<Glib::ustring> resultAuthorColumn_;
		};

		public:
                void signalException ();
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
		void onLibraryFolder ();
		void manageBrowseDialog (Gtk::Entry *entry);
		void onImport ();
		void onPreferences ();
		void onFind ();
		void onPluginRun (Glib::ustring const action, Plugin* plugin);

		/* WM events */
		bool onDelete (GdkEventAny *ev);
		void onResize (GdkEventConfigure *event);

		/* Pick up preference changes */
		void onShowTagPanePrefChanged ();
		void onShowNotesPanePrefChanged ();
		void onUseListViewPrefChanged ();
		void onWorkOfflinePrefChanged ();
		void onEnabledPluginsPrefChanged ();

		/* Remember which folders the user browsed last */
		Glib::ustring addfolder_;
		Glib::ustring exportfolder_;
		Glib::ustring libraryfolder_;

		/* Update UI text */
		void updateTitle ();
		void updateStatusBar ();

		/* Handle dirtyness */
		bool ensureSaved ();
		bool getDirty () {return dirty_;}
		bool dirty_;

		/* Remember which file is open */
		Glib::ustring openedlib_;
		void setOpenedLib (Glib::ustring const &openedlib);
		public:
		Glib::ustring const &getOpenedLib ()
			{return openedlib_;}
		private:
};

#endif
