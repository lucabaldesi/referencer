

#include <iostream>
#include <sys/types.h>
#include <dirent.h>

#include <glibmm/i18n.h>

#include "Python.h"

#include "BibData.h"
#include "Document.h"
#include "Preferences.h"
#include "PythonDocument.h"
#include "Transfer.h"
#include "Utility.h"

#include "ucompose.hpp"

#include <PluginManager.h>

PluginManager *_global_plugins;



static int numargs=0;

/* Return the number of arguments of the application command line */
static PyObject*
referencer_download(PyObject *self, PyObject *args)
{
	PyObject *url = PyTuple_GetItem (args, 2);
	PyObject *title = PyTuple_GetItem (args, 0);
	PyObject *message = PyTuple_GetItem (args, 1);

	PyObject *ret = NULL;
	try {
		Glib::ustring &xml = Transfer::readRemoteFile (
			PyString_AsString(title),
			PyString_AsString(message),
			PyString_AsString(url));
		ret = PyString_FromString (xml.c_str());
		DEBUG (String::ucompose ("got %1 characters", xml.length()));
	} catch (Transfer::Exception ex) {
		Utility::exceptionDialog (&ex, _("Downloading metadata"));
		Glib::ustring blank;
		ret = PyString_FromString (blank.c_str());
		
	}

	return ret;
}

/**
 * Convert a bibtex snippet into a dictionary of key/value 
 * pairs compatible with set_field in PythonDocument
 */
static PyObject *
referencer_bibtex_to_fields (PyObject *self, PyObject *args)
{
	/* Get bibtex string from argument tuple */
	PyObject *bibtex_py_str = PyTuple_GetItem (args, 0);
	char *bibtex_str = PyString_AsString(bibtex_py_str);

	/* Parse bibtex into a Document */
	Document doc;
	doc.parseBibtex (bibtex_str);

	/* Retrieve STL map of fields */
	Document::FieldMap fields = doc.getFields ();

	/* Convert STL map to python dict */
	PyObject *dict = PyDict_New();

	Document::FieldMap::iterator fieldIter = fields.begin ();
	Document::FieldMap::iterator const fieldEnd = fields.end ();
	for (; fieldIter != fieldEnd; ++fieldIter) {
		PyDict_SetItem (
			dict,
			PyString_FromString (fieldIter->first.c_str()),
			PyString_FromString (fieldIter->second.c_str()));
	}

	return dict;
}

/* Call gettext */
static PyObject*
referencer_gettext(PyObject *self, PyObject *args)
{
	PyObject *str = PyTuple_GetItem (args, 0);
	return PyString_FromString (_(PyString_AsString(str)));
}


static PyObject*
referencer_pref_set (PyObject *self, PyObject *args)
{
	PyObject *key = PyTuple_GetItem (args, 0);
	PyObject *value = PyTuple_GetItem (args, 1);

	_global_prefs->setPluginPref (PyString_AsString(key), PyString_AsString(value));

	return Py_True;
}


static PyObject*
referencer_pref_get (PyObject *self, PyObject *args)
{
	PyObject *key = PyTuple_GetItem (args, 0);
	Glib::ustring value = _global_prefs->getPluginPref (PyString_AsString(key));
	return PyString_FromString (value.c_str());
}


/**
 * Update UI, and teturn true if the user has requested cancellation
 */
static PyObject *
referencer_poll_cancellation (PyObject *self, PyObject *args)
{
	/* Advance GTK+ */
	while (Gtk::Main::events_pending())
		Gtk::Main::iteration (); 

	/* Invoke any registered callback */
	bool cancelled = false;
	if (_global_plugins->progressCallback_) {
		cancelled = (*(_global_plugins->progressCallback_)) (_global_plugins->progressObject_);
	}

	/* Advance GTK+ */
	while (Gtk::Main::events_pending())
		Gtk::Main::iteration (); 

	return cancelled ? Py_True : Py_False;
}


static PyMethodDef ReferencerMethods[] = {
	{"download", referencer_download, METH_VARARGS,
		"Retrieve a remote file"},
	{"bibtex_to_fields", referencer_bibtex_to_fields, METH_VARARGS,
		"Convert bibtex to referencer document fields"},
	{"pref_get", referencer_pref_get, METH_VARARGS,
		 "Get configuration item"},
	{"pref_set", referencer_pref_set, METH_VARARGS,
		 "Set configuration item"},
	{"poll_cancellation", referencer_poll_cancellation, METH_VARARGS,
		 "Update UI and poll for cancellation"},
	{"_", referencer_gettext, METH_VARARGS,
		"Translate a string"},
	{NULL, NULL, 0, NULL}
};


PluginManager::PluginManager ()
{
	PyObject *module = Py_InitModule ("referencer", ReferencerMethods);

	PyType_Ready (&t_referencer_document);
	PyObject_SetAttrString (module, "document", (PyObject*)&t_referencer_document);

	progressCallback_ = NULL;
}

PluginManager::~PluginManager ()
{
}

void PluginManager::scan (std::string const &pluginDir)
{
	DIR *dir = opendir (pluginDir.c_str());
	if (!dir) {
		// Fail silently, allow the caller to call this
		// spuriously on directories that only might exist
		return;
	}

	pythonPaths_.push_back (pluginDir);

	struct dirent *ent;
	while ((ent = readdir(dir))) {
		/* People with non-ascii filesystem are pretty screwed, eh? */
		std::string const name = ent->d_name;
		/* Scripts are at least x.py long */
		if (name.length() < 4)
			continue;

		if (name.substr(name.size() - 3, name.size() - 1) == ".py") {
			std::string const moduleName = name.substr (0, name.size() - 3);
			DEBUG (String::ucompose("found module %1", moduleName));


			// Check we haven't already loaded this module
			bool dupe = false;
			std::list<PythonPlugin>::iterator it = pythonPlugins_.begin();
			std::list<PythonPlugin>::iterator end = pythonPlugins_.end();
			for (; it != end; ++it) {
				if (it->getShortName() == moduleName) {
					dupe = true;
				}
			}
			if (dupe)
				continue;


			PythonPlugin newPlugin(this);
			pythonPlugins_.push_front (newPlugin);

			std::list<PythonPlugin>::iterator newbie = pythonPlugins_.begin();
			(*newbie).load(moduleName);
		}
	}
	closedir (dir);
}


Glib::ustring PluginManager::findDataFile (Glib::ustring const file)
{
	std::vector<Glib::ustring>::iterator it = pythonPaths_.begin ();
	std::vector<Glib::ustring>::iterator const end = pythonPaths_.end ();
	for (; it != end; ++it) {
		Glib::ustring filename = Glib::build_filename (*it, file);

		if (filename.substr(0,2) == Glib::ustring ("./")) {
			filename = Glib::get_current_dir () + filename.substr (1, filename.length());
		}

		Glib::RefPtr<Gio::File> uri = Gio::File::create_for_path (filename);

		DEBUG ("Trying %1", filename);

		if (uri->query_exists ())
			return filename;
	}

	return Glib::ustring ();
}

PluginManager::PluginList PluginManager::getPlugins ()
{
	PluginList retval;

	std::list<PythonPlugin>::iterator it = pythonPlugins_.begin();
	std::list<PythonPlugin>::iterator end = pythonPlugins_.end();
	for (; it != end; ++it) {
		retval.push_back (&(*it));
	}

	retval.push_back (&crossref_);
	retval.push_back (&arxiv_);

	return retval;
}


PluginManager::PluginList PluginManager::getEnabledPlugins ()
{
	std::list<Plugin*> retval;
	std::list<PythonPlugin>::iterator it = pythonPlugins_.begin();
	std::list<PythonPlugin>::iterator end = pythonPlugins_.end();

	for (; it != end; ++it) {
		if (it->isEnabled())
			retval.push_back (&(*it));
	}
	
	if (arxiv_.isEnabled ())
		retval.push_back (&arxiv_);


	// Try to keep crossref as a last resort due
	// to the "last-name only issue"
	if (crossref_.isEnabled ())
		retval.push_back (&crossref_);

	return retval;
}

