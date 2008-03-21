

#ifndef PYTHONPLUGIN_H
#define PYTHONPLUGIN_H

#include "Plugin.h"

class PythonPlugin : public Plugin
{
	public:
		PythonPlugin ();
		~PythonPlugin ();
		virtual void load (std::string const &moduleName);

		/* General properties */
		virtual Glib::ustring const getShortName () {return moduleName_;}
		virtual Glib::ustring const getLongName ();

		/* Actions */
		virtual Glib::ustring const getUI ();
		virtual ActionList getActions () {return actions_;};
		virtual bool doAction (Glib::ustring const action, std::vector<Document*>);
		
		/* Metadata lookup */
		virtual bool resolve (Document &doc);

	private:
		bool resolveID (Document &doc, PluginCapability::Identifier id);
		void displayException ();
		Glib::ustring const getPluginInfoField (Glib::ustring const &targetKey);
		std::string moduleName_;
		PyObject *pGetFunc_;
		PyObject *pActionFunc_;
		PyObject *pPluginInfo_;
		PyObject *pMod_;

		ActionList actions_;

};

#endif

