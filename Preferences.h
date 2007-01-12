
#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <libglademm.h>
#include <gtkmm.h>

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


	bool workoffline_;
	Glib::ustring doilaunch_;
	Glib::ustring metadatalookup_;

	Glib::ustring doilaunchdefault_;
	Glib::ustring metadatalookupdefault_;

public:
	Preferences ();
	void showDialog ();

	bool getWorkOffline () {return workoffline_;}
	void setWorkOffline (bool const &offline) {workoffline_ = offline;}
	
	Glib::ustring getDoiLaunch ();
	void setDoiLaunch (Glib::ustring const &doilaunch)
		{doilaunch_ = doilaunch;};

	Glib::ustring getMetadataLookup ();
	void setMetadataLookup (Glib::ustring const &metadatalookup)
		{metadatalookup_ = metadatalookup;};	
};

#endif
