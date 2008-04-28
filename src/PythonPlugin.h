

#ifndef PYTHONPLUGIN_H
#define PYTHONPLUGIN_H

#include "Plugin.h"

class PluginManager;

class PythonPlugin : public Plugin
{
	public:
		PythonPlugin (PluginManager *owner);
		~PythonPlugin ();
		virtual void load (std::string const &moduleName);

		/* General properties */
		virtual Glib::ustring const getShortName () {return moduleName_;}
		virtual Glib::ustring const getLongName ();
		virtual Glib::ustring const getAuthor ();
		virtual Glib::ustring const getVersion ();

		/* Actions */
		virtual Glib::ustring const getUI ();
		virtual ActionList getActions () {return actions_;};
		virtual bool doAction (Glib::ustring const action, std::vector<Document*>);
		virtual bool updateSensitivity (Glib::ustring const action, std::vector<Document*>);
		
		/* Metadata lookup */
		virtual bool resolve (Document &doc);

		/* Config hook */
		virtual bool canConfigure ();
		virtual void doConfigure ();

		/* Report exceptions */
		virtual bool hasError ();
		virtual Glib::ustring getError ();
	private:
		bool resolveID (Document &doc, PluginCapability::Identifier id);

		void displayException ();
		void printException ();
		Glib::ustring formatException ();
		Glib::ustring exceptionLog_;


		Glib::ustring const getPluginInfoField (Glib::ustring const &targetKey);
		std::string moduleName_;
		PyObject *pGetFunc_;
		PyObject *pActionFunc_;
		PyObject *pPluginInfo_;
		PyObject *pMod_;

		ActionList actions_;

		PluginManager *owner_;

};

#endif

