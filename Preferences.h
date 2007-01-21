
#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <libglademm.h>
#include <gtkmm.h>
#include <gconfmm.h>

#include "Utility.h"

class Preferences {
private:
	Glib::RefPtr<Gnome::Glade::Xml> xml_;
	Gtk::Dialog *dialog_;
	Gtk::CheckButton *workofflinecheck_;
	Gtk::Entry *doilaunchentry_;
	Gtk::Entry *metadatalookupentry_;
	void onWorkOfflineToggled ();
	void onURLChanged ();
	// This is just for the button in the dialog!
	void onResetToDefaults ();

	void onConfChange (int number, Gnome::Conf::Entry entry);

	Gnome::Conf::Entry workoffline_;
	Gnome::Conf::Entry doilaunch_;
	Gnome::Conf::Entry metadatalookup_;
	Gnome::Conf::Entry uselistview_;
	Gnome::Conf::Entry showtagpane_;
	Gnome::Conf::Entry libraryfilename_;

	sigc::signal<void> workofflinesignal_;
	sigc::signal<void> uselistviewsignal_;
	sigc::signal<void> showtagpanesignal_;

	Glib::ustring doilaunchdefault_;
	Glib::ustring metadatalookupdefault_;

	bool ignorechanges_;

	Glib::RefPtr<Gnome::Conf::Client> confclient_;
	
	// Set when our gconf directory didn't exist, the first time the 
	// program is run
	bool firsttime_;

public:
	Preferences ();
	~Preferences ();
	void showDialog ();

	// Nothing uses this, hopefully nothing will
	//Glib::RefPtr<Gnome::Conf::Client> getConfClient () {return confclient_;}

	Glib::ustring getLibraryFilename ();
	void setLibraryFilename (Glib::ustring const &filename);

	bool getWorkOffline ();
	void setWorkOffline (bool const &offline);
	sigc::signal<void>& getWorkOfflineSignal ();

	bool getUseListView ();
	void setUseListView (bool const &uselistview);
	sigc::signal<void>& getUseListViewSignal ();
	
	bool getShowTagPane ();
	void setShowTagPane (bool const &showtagpane);
	sigc::signal<void>& getShowTagPaneSignal ();

	typedef std::pair<Glib::ustring, Glib::ustring> StringPair;

	Utility::StringPair getDoiLaunch ();
	void setDoiLaunch (Glib::ustring const &doilaunch);

	Utility::StringPair getMetadataLookup ();
	void setMetadataLookup (Glib::ustring const &metadatalookup);
	
	bool const getFirstTime () {return firsttime_;}
};

extern Preferences *_global_prefs;


#endif
