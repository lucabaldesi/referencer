

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <string>
#include <list>
#include <Python.h>

#include "Plugin.h"
#include "PythonPlugin.h"
#include "CrossRefPlugin.h"
#include "ArxivPlugin.h"

class BibData;

class PluginManager
{
	public:
		PluginManager ();
		~PluginManager ();

		/* List of loaded plugins */
		std::list<Plugin*> getPlugins();

		/* List of enabled plugins */
		std::list<Plugin*> getEnabledPlugins();

		/* Load all plugins in the directory */
		void scan (std::string const &pluginDir);

		/* Look for data files in plugin paths */
		Glib::ustring findDataFile (Glib::ustring file);

	private:
		/* Python module search locations */
		std::vector<Glib::ustring> pythonPaths_;

		/* Loaded python modules */
		std::list<PythonPlugin> pythonPlugins_;

		/* Builtin plugins */
		CrossRefPlugin crossref_;
		ArxivPlugin arxiv_;

};

extern PluginManager *_global_plugins;

#endif
