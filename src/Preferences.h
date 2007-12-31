
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

#include <libglademm.h>
#include <gtkmm.h>
#include <gconfmm.h>

#include "Utility.h"
#include "PluginManager.h"

class Preferences {
private:
	Glib::RefPtr<Gnome::Glade::Xml> xml_;

	/*
	 * Plugins
	 */
	Gtk::TreeView *pluginView_;
	Gtk::Button *moveUpButton_;
	Gtk::Button *moveDownButton_;
	Glib::RefPtr<Gtk::ListStore> pluginStore_;

	Gtk::TreeModelColumn<unsigned int> colPriority_;
	Gtk::TreeModelColumn<Plugin*> colPlugin_;
	Gtk::TreeModelColumn<Glib::ustring> colShortName_;
	Gtk::TreeModelColumn<Glib::ustring> colLongName_;
	Gtk::TreeModelColumn<bool> colEnabled_;

	void onPluginToggled (Glib::ustring const &str);

	Gnome::Conf::Entry disabledPlugins_;
	public:
	void disablePlugin (Plugin *plugin);
	private:
	/*
	 * End of Plugins
	 */

	/*
	 * Conf for crossref plugin
	 */
private:
	Gnome::Conf::Entry crossRefUsername_;
	Gnome::Conf::Entry crossRefPassword_;
	Gtk::Entry *crossRefUsernameEntry_;
	Gtk::Entry *crossRefPasswordEntry_;
	void onCrossRefChanged ();
public:
	Glib::ustring getCrossRefUsername ();
	Glib::ustring getCrossRefPassword ();

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
	std::pair<int, int> getListSort ();
	void setListSort (int const column, int const order);
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
	Gnome::Conf::Entry libraryfilename_;
	Gnome::Conf::Entry width_;
	Gnome::Conf::Entry height_;

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

	bool getUseListView ();
	void setUseListView (bool const &uselistview);
	sigc::signal<void>& getUseListViewSignal ();

	bool getShowTagPane ();
	void setShowTagPane (bool const &showtagpane);
	sigc::signal<void>& getShowTagPaneSignal ();

	typedef std::pair<Glib::ustring, Glib::ustring> StringPair;

	std::pair<int, int> getWindowSize ();
	void setWindowSize (std::pair<int, int> size);

	bool const getFirstTime () {return firsttime_;}
};

extern Preferences *_global_prefs;


#endif
