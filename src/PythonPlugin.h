

#ifndef PYTHONPLUGIN_H
#define PYTHONPLUGIN_H

#include "Plugin.h"

class PythonPlugin : public Plugin
{
	public:
		PythonPlugin ();
		~PythonPlugin ();
		virtual void load (std::string const &moduleName);
		virtual bool resolve (Document &doc);
		virtual bool doAction (std::vector<Document*>);
		virtual Glib::ustring const getShortName () {return moduleName_;}
		virtual Glib::ustring const getLongName ();
		virtual Glib::ustring const getActionText ();
		virtual Glib::ustring const getActionTooltip ();
		virtual Glib::ustring const getActionIcon ();

	private:
		bool resolveID (Document &doc, PluginCapability::Identifier id);
		Glib::ustring const getPluginInfoField (Glib::ustring const &targetKey);
		std::string moduleName_;
		PyObject *pGetFunc_;
		PyObject *pActionFunc_;
		PyObject *pPluginInfo_;
		PyObject *pMod_;
};

#endif

