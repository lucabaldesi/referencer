

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <string>
#include <list>
#include <Python.h>

class BibData;

class Plugin
{
	public:
		Plugin ();
		~Plugin ();
		void load (std::string const &moduleName);
		void setEnabled (bool const enable);
		bool resolveDoi (BibData &bib);
		bool isEnabled () {return enabled_;}
		bool isLoaded () {return loaded_;}
		Glib::ustring const getShortName () {return moduleName_;}
		Glib::ustring const getLongName ();
	private:
		Glib::ustring const getPluginInfoField (Glib::ustring const &targetKey);
		std::string moduleName_;
		bool loaded_;
		bool enabled_;
		PyObject *pGetFunc_;
		PyObject *pPluginInfo_;
		PyObject *pMod_;
};


class PluginManager
{
	public:
		std::list<Plugin*> getPlugins();
		std::list<Plugin*> getEnabledPlugins();
		void scan (std::string const &pluginDir);
		PluginManager ();
		~PluginManager ();
	private:
		std::list<Plugin> plugins_;
};

extern PluginManager *_global_plugins;

#endif
