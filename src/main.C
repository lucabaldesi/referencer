
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */


#include <memory>
#include <iostream>
#include <gtkmm.h>
#include <glibmm/i18n.h>

#include <Python.h>
#include <PluginManager.h>

#include "config.h"

#include "Preferences.h"
#include "RefWindow.h"
#include "Utility.h"


int main (int argc, char **argv)
{
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	if (!Glib::thread_supported())
		Glib::thread_init(0); //So we can use GMutex.
	Gnome::Conf::init();

	std::auto_ptr<Gtk::Main> mainInstance;
	try
	{
		mainInstance = std::auto_ptr<Gtk::Main>( new Gtk::Main(argc, argv) );
	}
	catch(const Glib::Error& ex)
	{
		std::cerr << "Glom: Error while initializing gtkmm: " << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	Gio::init ();

	Glib::ustring pythonPath = "";
	/* Pick up existing python path */
	if (getenv("PYTHONPATH")) {
		pythonPath += ":";
		pythonPath += getenv("PYTHONPATH");
	}

	/* Locate user plugins */
	Glib::ustring homePlugins;
	if (getenv("HOME"))
		homePlugins = Glib::ustring(getenv("HOME")) + Glib::ustring("/.referencer/plugins");

	/* Development directory */
	Glib::ustring localPlugins = "./plugins";
	/* Systemwide */
	Glib::ustring installedPlugins = PLUGINDIR;

	/* Order is important, defines precedence */
	pythonPath += ":";
	pythonPath += localPlugins;
	pythonPath += ":";
	pythonPath += homePlugins;
	pythonPath += ":";
	pythonPath += installedPlugins;
	pythonPath += ":";
	/* Export the path */
	DEBUG (String::ucompose ("setting pythonPath to %1", pythonPath));
	setenv ("PYTHONPATH", pythonPath.c_str(), 1);
	Py_Initialize ();

	_global_plugins = new PluginManager ();
	_global_plugins->scan("./plugins");
	_global_plugins->scan(homePlugins);
	_global_plugins->scan(PLUGINDIR);

	_global_prefs = new Preferences();

	if (argc > 1 && Glib::ustring(argv[1]).substr(0,1) != "-") {
		Glib::RefPtr<Gio::File> libfile = Gio::File::create_for_commandline_arg(argv[1]);

		_global_prefs->setLibraryFilename (libfile->get_uri());
	}

	try {
		RefWindow window;
		window.run();
	} catch (Glib::Error ex) {
		Utility::exceptionDialog (&ex, _("Terminating due to unhandled exception"));
	}

	delete _global_prefs;
	delete _global_plugins;
	Py_Finalize ();

	return 0;
}
