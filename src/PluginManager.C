

#include <iostream>
#include <sys/types.h>
#include <dirent.h>

// FIXME just using this for the exception class
#include <Transfer.h>
// For exception dialog
#include <Utility.h>
#include <BibData.h>

#include <PluginManager.h>

PluginManager *_global_plugins;

PluginManager::PluginManager ()
{
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
		std::string const name = ent->d_name;
		/* Scripts are at least x.py long */
		if (name.length() < 4)
			continue;

		if (name.substr(name.size() - 3, name.size() - 1) == ".py") {
			std::string const moduleName = name.substr (0, name.size() - 3);
			std::cerr << "PluginManager::scan: Found module '" << moduleName << "'\n";


			// Check we haven't already loaded this module
			bool dupe = false;
			std::vector<Plugin>::iterator it = plugins_.begin();
			std::vector<Plugin>::iterator end = plugins_.end();
			for (; it != end; ++it) {
				if (it->shortName() == moduleName) {
					dupe = true;
				}
			}
			if (dupe)
				continue;


			Plugin newPlugin;
			plugins_.push_back (newPlugin);

			std::vector<Plugin>::iterator newbie = plugins_.end() - 1;
			(*newbie).load(moduleName);
		}
	}
	closedir (dir);
}


std::vector<Plugin*> PluginManager::getPlugins ()
{
	std::vector<Plugin*> retval;
	std::vector<Plugin>::iterator it = plugins_.begin();
	std::vector<Plugin>::iterator end = plugins_.end();

	for (; it != end; ++it) {
		retval.push_back (&(*it));
	}

	return retval;
}


std::vector<Plugin*> PluginManager::getEnabledPlugins ()
{
	std::vector<Plugin*> retval;
	std::vector<Plugin>::iterator it = plugins_.begin();
	std::vector<Plugin>::iterator end = plugins_.end();

	for (; it != end; ++it) {
		if (it->isEnabled())
			retval.push_back (&(*it));
	}

	return retval;
}


Plugin::Plugin()
{
	pGetFunc_ = NULL;
	pMod_ = NULL;
	// XXX until we have UI just enable everything
	enabled_ = true;
	loaded_ = false;
}

Plugin::~Plugin()
{
	if (loaded_) {
		Py_DECREF (pGetFunc_);
		Py_DECREF (pMod_);
	}
}

void Plugin::load (std::string const &moduleName)
{
	PyObject *pName = PyString_FromString(moduleName.c_str());
	if (!pName) {
		std::cerr << "Plugin::load: Couldn't construct module name\n";
		return;
	}
	pMod_ = PyImport_Import(pName);
	Py_DECREF(pName);

	if (!pMod_) {
		std::cerr << "Plugin::load: Couldn't import module\n";
		return;
	}

	pGetFunc_ = PyObject_GetAttrString (pMod_, "metadata_from_doi");
	if (!pGetFunc_) {
		std::cerr << "Plugin::load: Couldn't find function\n";
		Py_DECREF (pMod_);
		return;
	}

	std::cerr << "Plugin::load: successfully loaded '" << moduleName << "'\n";

	loaded_ = true;
	moduleName_ = moduleName;
}

bool Plugin::resolveDoi (BibData &bib)
{
	bool success = false;

	PyObject *pDoi = PyString_FromString (bib.getDoi().c_str());

	PyObject *pArgs = PyTuple_New(1);
	PyTuple_SetItem (pArgs, 0, pDoi);

	PyObject *pMetaData = PyObject_CallObject(pGetFunc_, pArgs);
	Py_DECREF(pArgs);
	if (pMetaData != NULL) {
		int const N = PyList_Size (pMetaData);

		if (N > 0)
			success = true;

		for (int i = 0; i < N; ++i) {
			PyObject *pItem = PyList_GetItem (pMetaData, i);
			std::string const key = PyString_AsString(PyList_GetItem (pItem, 0));
			std::string const value = PyString_AsString(PyList_GetItem (pItem, 1));

			if (key == "title") {
				bib.setTitle (value);
			} else if (key == "authors") {
				bib.setAuthors (value);
			} else if (key == "journal") {
				bib.setJournal (value);
			} else if (key == "volume") {
				bib.setVolume (value);
			} else if (key == "issue") {
				bib.setIssue (value);
			} else if (key == "year") {
				bib.setYear (value);
			} else if (key == "pages") {
				bib.setPages (value);
			} else {
				bib.addExtra (key, value);
			}
		}
		Py_DECREF(pMetaData);
	} else {
		PyObject *pErr = PyErr_Occurred ();
		if (pErr) {
			PyObject *ptype;
			PyObject *pvalue;
			PyObject *ptraceback;

			PyErr_Fetch(&ptype, &pvalue, &ptraceback);

			std::string exceptionText;
			PyObject *pStr;
			pStr = PyObject_Str (ptype);
			exceptionText += PyString_AsString (pStr);
			exceptionText += ", ";
			pStr = PyObject_Str (pvalue);
			exceptionText += PyString_AsString (pStr);

			/*
			Transfer::Exception ex(exceptionText);
			Utility::exceptionDialog (&ex, _("Downloading metadata"));*/
		}
	}

	return success;
}
