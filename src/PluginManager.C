

#include <iostream>
#include <sys/types.h>
#include <dirent.h>

#include <glibmm/i18n.h>

#include "Python.h"

#include "Preferences.h"
// FIXME just using this for the exception class
#include <Transfer.h>
// For exception dialog
#include <Utility.h>
#include <BibData.h>

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

	std::cerr << "url = " << url << "\n";
	char *urlStr = PyString_AsString (url);
	std::cerr << "urlStr = " << urlStr << "\n";
	/* XXX catch exceptions */
	PyObject *ret = NULL;
	try {
		Glib::ustring &xml = Transfer::readRemoteFile (
			PyString_AsString(title),
			PyString_AsString(message),
			PyString_AsString(url));
		ret = PyString_FromString (xml.c_str());
		std::cerr << "referencer_download: got " << xml.length() << " characters\n";
	} catch (Transfer::Exception ex) {
		Utility::exceptionDialog (&ex, _("Downloading metadata"));
		Glib::ustring blank;
		ret = PyString_FromString (blank.c_str());
		
	}

	return ret;
}

static PyMethodDef ReferencerMethods[] = {
    {"download", referencer_download, METH_VARARGS,
     "Retrieve a remote file"},
    {NULL, NULL, 0, NULL}
};




PluginManager::PluginManager ()
{
	Py_InitModule ("referencer", ReferencerMethods);
}

PluginManager::~PluginManager ()
{
}

void PluginManager::scan (std::string const &pluginDir)
{
	DIR *dir = opendir (pluginDir.c_str());
	if (!dir) {
		// Fail silently, allow the caller to call this
		// spuriously on directorys that only might exist
		return;
	}

	struct dirent *ent;
	while ((ent = readdir(dir))) {
		/* People with non-ascii filesystem are pretty screwed, eh? */
		std::string const name = ent->d_name;
		/* Scripts are at least x.py long */
		if (name.length() < 4)
			continue;

		if (name.substr(name.size() - 3, name.size() - 1) == ".py") {
			std::string const moduleName = name.substr (0, name.size() - 3);
			std::cerr << "PluginManager::scan: Found module '" << moduleName << "'\n";


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


			PythonPlugin newPlugin;
			pythonPlugins_.push_front (newPlugin);

			std::list<PythonPlugin>::iterator newbie = pythonPlugins_.begin();
			(*newbie).load(moduleName);
		}
	}
	closedir (dir);
}


std::list<Plugin*> PluginManager::getPlugins ()
{
	std::list<Plugin*> retval;
	std::list<PythonPlugin>::iterator it = pythonPlugins_.begin();
	std::list<PythonPlugin>::iterator end = pythonPlugins_.end();

	for (; it != end; ++it) {
		retval.push_back (&(*it));
	}

	retval.push_back (&crossref_);
	retval.push_back (&arxiv_);

	return retval;
}


std::list<Plugin*> PluginManager::getEnabledPlugins ()
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

