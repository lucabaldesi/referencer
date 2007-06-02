
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

	sigc::signal<void> selectionchangedsignal_;

	Glib::RefPtr<Gtk::ListStore> docstore_;
	Gtk::TreeModelColumn<Document*> docpointercol_;
	Gtk::TreeModelColumn<Glib::ustring> dockeycol_;
	Gtk::TreeModelColumn<Glib::ustring> doctitlecol_;
	Gtk::TreeModelColumn<Glib::ustring> docauthorscol_;
	Gtk::TreeModelColumn<Glib::ustring> docyearcol_;
	Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > docthumbnailcol_;
	Glib::RefPtr<Gtk::TreeSelection> docslistselection_;

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

	bool docClicked (GdkEventButton* event);
	void onIconsDragData (
		const Glib::RefPtr <Gdk::DragContext> &context,
		int n1, int n2, const Gtk::SelectionData &sel, guint n3, guint n4);

	bool uselistview_;

};
