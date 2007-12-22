

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
		virtual void load (std::string const &moduleName) {};
		virtual void setEnabled (bool const enable);
		virtual bool resolve (BibData &bib) = 0;
		virtual Glib::ustring const getShortName () = 0;
		virtual Glib::ustring const getLongName () = 0;
		bool isEnabled () {return enabled_;}
		bool isLoaded () {return loaded_;}
	protected:
		bool loaded_;
		bool enabled_;
};

class CrossRefPlugin : public Plugin
{
	public:
		CrossRefPlugin () {loaded_ = true;};
		~CrossRefPlugin () {};
		virtual bool resolve (BibData &bib);
		virtual Glib::ustring const getShortName ();
		virtual Glib::ustring const getLongName ();
	private:
};

class PythonPlugin : public Plugin
{
	public:
		PythonPlugin ();
		~PythonPlugin ();
		virtual void load (std::string const &moduleName);
		virtual bool resolve (BibData &bib);
		virtual Glib::ustring const getShortName () {return moduleName_;}
		virtual Glib::ustring const getLongName ();
	private:
		Glib::ustring const getPluginInfoField (Glib::ustring const &targetKey);
		std::string moduleName_;
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
		std::list<PythonPlugin> pythonPlugins_;
		CrossRefPlugin crossref_;
};

extern PluginManager *_global_plugins;

#endif
