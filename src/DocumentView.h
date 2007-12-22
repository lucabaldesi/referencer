
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */


#include <gtkmm.h>

class Document;
class Library;
class RefWindow;

class DocumentView : public Gtk::VBox
{
	public:
	DocumentView (RefWindow &refwin, Library &lib, bool const uselistview);
	~DocumentView ();
	
	void populateDocStore ();
	
	void updateAllDocs ();
	void updateDoc (Document * const doc);
	void removeDoc (Document * const doc);
	void addDoc (Document * const doc);
	void updateVisible ();


	Document *getSelectedDoc ();
	std::vector<Document*> getSelectedDocs ();
	int getSelectedDocCount ();
	int getVisibleDocCount ();
	
	class Capabilities {
		public:
		bool weblink;
		bool open;
		bool getmetadata;
		Capabilities () {weblink = open = getmetadata = false;}
	};
	Capabilities getDocSelectionCapabilities ();
	
	typedef enum {
		NONE = 0,
		ALL,
		SOME
	} SubSet;
	SubSet selectedDocsHaveTag (int uid);
	
	void setUseListView (bool const &list);
	bool getUseListView ()
		{return uselistview_;}
	
	sigc::signal<void>& getSelectionChangedSignal ()
		{return selectionchangedsignal_;}
	
	// This is Gtk::Managed so when it gets packed that's it
	Gtk::Entry &getSearchEntry ()
		{return *searchentry_;}	



	private:
	RefWindow &win_;
	Library &lib_;
	
	/* The search box */
	Gtk::Entry *searchentry_;
	void onSearchChanged ();
	friend void end_search (GPtrArray * out_array, GError * error, gpointer user_data);
	std::list<Glib::ustring> trackerUris_;

	/* Signal that we fire whenever selection changes in one of our views */
	sigc::signal<void> selectionchangedsignal_;

	/* This is the actual store */
	Glib::RefPtr<Gtk::ListStore> docstore_;
	/* It's a ListStore-TreeModelFilter-TreeModelSort sandwich! */
	Glib::RefPtr<Gtk::TreeModelFilter> docstorefilter_;
	/* Things converting treeview paths into iters need to use this */
	Glib::RefPtr<Gtk::TreeModelSort> docstoresort_;

	/* The columns, a columnrecord is made in the constructor */
	Gtk::TreeModelColumn<Document*> docpointercol_;
	Gtk::TreeModelColumn<Glib::ustring> dockeycol_;
	Gtk::TreeModelColumn<Glib::ustring> doctitlecol_;
	Gtk::TreeModelColumn<Glib::ustring> docauthorscol_;
	Gtk::TreeModelColumn<Glib::ustring> docyearcol_;
	Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > docthumbnailcol_;
	#if GTK_VERSION_GE(2,12)
	Gtk::TreeModelColumn<Glib::ustring> doctooltipcol_;
	#endif
	Gtk::TreeModelColumn<bool> docvisiblecol_;

	/* Two oh-so-innocuous objects */
	Gtk::IconView *docsiconview_;
	Gtk::TreeView *docslistview_;
	
	/* This is only for the list view, iconview has inbuilt selection */
	Glib::RefPtr<Gtk::TreeSelection> docslistselection_;

	/* Double click on IconView */
	void docActivated (const Gtk::TreePath& path);
	/* Treeviews want a different prototype for the signal */
	void docListActivated (const Gtk::TreePath& path, Gtk::TreeViewColumn*) {
		docActivated (path);
	}
	
	/* ev-tooltip stuff from the bad old days */
	#if GTK_VERSION_LT(2,12)
	GtkWidget *doctooltip_;
	Document *hoverdoc_;
	void onDocMouseMotion (GdkEventMotion* event);
	void onDocMouseLeave (GdkEventCrossing *event);
	#endif
	Gtk::Menu doccontextmenu_;
	Gtk::ScrolledWindow *docsiconscroll_;
	Gtk::ScrolledWindow *docslistscroll_;
	void docSelectionChanged ();

	bool isVisible (Document * const doc);
	void loadRow (
		Gtk::TreeModel::iterator item,
		Document * const doc);

	bool docClicked (GdkEventButton* event);
	void onIconsDragData (
		const Glib::RefPtr <Gdk::DragContext> &context,
		int n1, int n2, const Gtk::SelectionData &sel, guint n3, guint n4);

	bool uselistview_;

	void onSortColumnChanged ();
};
