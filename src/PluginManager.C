

#include <iostream>
#include <sys/types.h>
#include <dirent.h>

#include <glibmm/i18n.h>

#include "Preferences.h"
// FIXME just using this for the exception class
#include <Transfer.h>
// For exception dialog
#include <Utility.h>
#include <BibData.h>

#include "ucompose.hpp"

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
	
	if (crossref_.isEnabled ())
		retval.push_back (&crossref_);

	return retval;
}

