

#ifndef ARXIVPLUGIN_H
#define ARXIVPLUGIN_H

#include "Plugin.h"

class Document;

class ArxivPlugin : public Plugin
{
	public:
		ArxivPlugin () {
			loaded_ = true;
			cap_.add(PluginCapability::ARXIV);
		}
		~ArxivPlugin () {};
		virtual bool resolve (Document &doc);
		virtual Glib::ustring const getShortName ();
		virtual Glib::ustring const getLongName ();
};

#endif
