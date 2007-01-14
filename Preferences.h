
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
	void onResetToDefaults ();

	void onConfChange (int number, Gnome::Conf::Entry entry);

	Gnome::Conf::Entry workoffline_;
	Gnome::Conf::Entry doilaunch_;
	Gnome::Conf::Entry metadatalookup_;

	sigc::signal<void> workofflinesignal_;

	Glib::ustring doilaunchdefault_;
	Glib::ustring metadatalookupdefault_;

	bool ignorechanges_;

	Glib::RefPtr<Gnome::Conf::Client> confclient_;

public:
	Preferences ();
	~Preferences ();
	void showDialog ();

	//Glib::RefPtr<Gnome::Conf::Client> getConfClient () {return confclient_;}

	bool getWorkOffline ();
	void setWorkOffline (bool const &offline);
	sigc::signal<void> getWorkOfflineSignal ();

	typedef std::pair<Glib::ustring, Glib::ustring> StringPair;
	
	Utility::StringPair getDoiLaunch ();
	void setDoiLaunch (Glib::ustring const &doilaunch);

	Utility::StringPair getMetadataLookup ();
	void setMetadataLookup (Glib::ustring const &metadatalookup);
};

extern Preferences *_global_prefs;


#endif
