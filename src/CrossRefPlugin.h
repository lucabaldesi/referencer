

#ifndef CROSSREFPLUGIN_H
#define CROSSREFPLUGIN_H

#include "Plugin.h"

class Document;

class CrossRefPlugin : public Plugin
{
	public:
		CrossRefPlugin () {
			loaded_ = true;
			cap_.add(PluginCapability::DOI);
		}
		~CrossRefPlugin () {};
		virtual bool resolve (Document &doc);
		virtual Glib::ustring const getShortName ();
		virtual Glib::ustring const getLongName ();
};

#endif
