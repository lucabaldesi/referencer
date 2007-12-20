

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <string>
#include <vector>
#include <Python.h>

class BibData;

class Plugin
{
	public:
		Plugin ();
		~Plugin ();
		void load (std::string const &moduleName);
		void enabled ();
		void disable ();
		bool resolveDoi (BibData &bib);
		bool isEnabled () {return enabled_;}
		bool isLoaded () {return loaded_;}
		std::string shortName () {return moduleName_;}
	private:
		std::string moduleName_;
		bool loaded_;
		bool enabled_;
		PyObject *pGetFunc_;
		PyObject *pMod_;
};


class PluginManager
{
	public:
		std::vector<Plugin*> getPlugins();
		std::vector<Plugin*> getEnabledPlugins();
		void scan (std::string const &pluginDir);
		PluginManager ();
		~PluginManager ();
	private:
		std::vector<Plugin> plugins_;
};

extern PluginManager *_global_plugins;

#endif
