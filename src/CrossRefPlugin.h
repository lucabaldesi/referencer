

#ifndef CROSSREFPLUGIN_H
#define CROSSREFPLUGIN_H



#include "Plugin.h"

class Document;

class CrossRefPlugin : public Plugin
{
	public:
		CrossRefPlugin ();
		~CrossRefPlugin () {};
		virtual int canResolve (Document &doc);
		virtual bool resolve (Document &doc);
		virtual Glib::ustring const getShortName ();
		virtual Glib::ustring const getLongName ();
		virtual Glib::ustring const getAuthor ();
		virtual Glib::ustring const getVersion ();
		virtual bool canConfigure () {return true;};
		virtual void doConfigure ();

	private:
		bool ignoreChanges_;
		void onPrefsChanged ();
		Glib::RefPtr<Gtk::Builder> xml_;
		Gtk::Dialog *dialog_;
		Gtk::Entry  *usernameEntry_;
		Gtk::Entry  *passwordEntry_;
};

#endif
