

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <string>
#include <list>
#include <Python.h>

#include "Plugin.h"
#include "PythonPlugin.h"
#include "CrossRefPlugin.h"

class BibData;

class PluginManager
{
	public:
		std::list<Plugin*> getPlugins();
		std::list<Plugin*> getEnabledPlugins();
		void scan (std::string const &pluginDir);
		PluginManager ();
		~PluginManager ();
	private:
		std::list<PythonPlugin> pythonPlugins_;
		CrossRefPlugin crossref_;
};

extern PluginManager *_global_plugins;

#endif
