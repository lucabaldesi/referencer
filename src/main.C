
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */


#include <gtkmm.h>
#include <libgnomeuimm.h>
#include <libgnomevfsmm.h>
#include <glibmm/i18n.h>

#include <Python.h>

#include "config.h"

#include "Preferences.h"
#include "RefWindow.h"
#include "Utility.h"


int main (int argc, char **argv)
{
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	Gnome::Main gui (PACKAGE, VERSION,
		Gnome::UI::module_info_get(), argc, argv);

	Gnome::Vfs::init ();

	std::string pythonpath = "./plugins:"PLUGINDIR;
	if (getenv("PYTHONPATH"))
		pythonpath += getenv("PYTHONPATH");
	setenv ("PYTHONPATH", pythonpath.c_str(), 1);
	Py_Initialize ();

	_global_prefs = new Preferences();

	if (argc > 1) {
		Glib::ustring libfile = argv[1];
		if (!Glib::path_is_absolute (libfile)) {
			libfile = Glib::build_filename (Glib::get_current_dir (), libfile);
		}

		libfile = Gnome::Vfs::get_uri_from_local_path (libfile);

		_global_prefs->setLibraryFilename (libfile);
	}

	try {
		RefWindow window;
		window.run();
	} catch (Glib::Error ex) {
		Utility::exceptionDialog (&ex, _("Terminating due to unhandled exception"));
	}

	delete _global_prefs;

	Py_Finalize ();

	return 0;
}
