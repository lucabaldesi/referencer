
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <utility>

#include <gtkmm.h>
#include <gconfmm.h>

#include "Utility.h"
#include "PluginManager.h"

class Preferences {
private:
	Glib::RefPtr<Gtk::Builder> xml_;

	/*
	 * Plugins
	 */
	Gtk::TreeView *pluginView_;
	Gtk::Button *moveUpButton_;
	Gtk::Button *moveDownButton_;
	Gtk::Button *configureButton_;
	Gtk::Button *aboutButton_;
	Glib::RefPtr<Gtk::ListStore> pluginStore_;
	Glib::RefPtr<Gtk::TreeView::Selection> pluginSel_;

	Gtk::TreeModelColumn<unsigned int> colPriority_;
	Gtk::TreeModelColumn<Plugin*> colPlugin_;
	Gtk::TreeModelColumn<Glib::ustring> colShortName_;
	Gtk::TreeModelColumn<Glib::ustring> colLongName_;
	Gtk::TreeModelColumn<bool> colEnabled_;

	void onPluginToggled (Glib::ustring const &str);
	void onPluginSelect ();
	void onPluginAbout ();
	void onPluginConfigure ();

	Gnome::Conf::Entry disabledPlugins_;
	public:
	void disablePlugin (Plugin *plugin);

	void setPluginPref (Glib::ustring const &key, Glib::ustring const &value);
	Glib::ustring getPluginPref (Glib::ustring const &key);

	/*
	 * End of Plugins
	 */
	

	/*
	 * Conf for crossref plugin
	 */
private:
	Gnome::Conf::Entry crossRefUsername_;
	Gnome::Conf::Entry crossRefPassword_;
public:
	Glib::ustring getCrossRefUsername ();
	Glib::ustring getCrossRefPassword ();
	void setCrossRefUsername (Glib::ustring const &username);
	void setCrossRefPassword (Glib::ustring const &password);

	/*
	 * End of conf for crossref plugin
	 */

	/*
	 * List view options
	 */
private:
	Gnome::Conf::Entry listSortColumn_;
	Gnome::Conf::Entry listSortOrder_;
public:
	std::pair<Glib::ustring, int> getListSort ();
	void setListSort (Glib::ustring const &columnName, int const order);
	/*
	 * End of list view options
	 */

	/*
	 * Uncategorised
	 */
private:
	Gtk::Dialog *dialog_;

	Gtk::Entry *proxyhostentry_;
	Gtk::SpinButton *proxyportspin_;
	Gtk::Entry *proxyusernameentry_;
	Gtk::Entry *proxypasswordentry_;
	Gtk::CheckButton *useproxycheck_;
	Gtk::CheckButton *useauthcheck_;

	void onWorkOfflineToggled ();
	void onProxyChanged ();
	void updateSensitivity ();

	void onConfChange (int number, Gnome::Conf::Entry entry);

	Gnome::Conf::Entry workoffline_;
	Gnome::Conf::Entry uselistview_;
	Gnome::Conf::Entry showtagpane_;
	Gnome::Conf::Entry shownotespane_;
	Gnome::Conf::Entry libraryfilename_;
	Gnome::Conf::Entry width_;
	Gnome::Conf::Entry height_;
	Gnome::Conf::Entry notesheight_;

	Gnome::Conf::Entry proxymode_;
	Gnome::Conf::Entry proxyuseproxy_;
	Gnome::Conf::Entry proxyuseauth_;
	Gnome::Conf::Entry proxyhost_;
	Gnome::Conf::Entry proxyport_;
	Gnome::Conf::Entry proxyusername_;
	Gnome::Conf::Entry proxypassword_;

	sigc::signal<void> workofflinesignal_;
	sigc::signal<void> uselistviewsignal_;
	sigc::signal<void> showtagpanesignal_;
	sigc::signal<void> shownotespanesignal_;
	sigc::signal<void> plugindisabledsignal_;

	bool ignoreChanges_;

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

	sigc::signal<void>& getPluginDisabledSignal ();

	bool getUseListView ();
	void setUseListView (bool const &uselistview);
	sigc::signal<void>& getUseListViewSignal ();

	bool getShowTagPane ();
	void setShowTagPane (bool const &showtagpane);
	sigc::signal<void>& getShowTagPaneSignal ();
	
	bool getShowNotesPane ();
	void setShowNotesPane (bool const &shownotespane);
	sigc::signal<void>& getShowNotesPaneSignal ();

	typedef std::pair<Glib::ustring, Glib::ustring> StringPair;

	std::pair<int, int> getWindowSize ();
	void setWindowSize (std::pair<int, int> size);
	
	int getNotesPaneHeight ();
	void setNotesPaneHeight (int height);

	bool const getFirstTime () {return firsttime_;}
};

extern Preferences *_global_prefs;


#endif
