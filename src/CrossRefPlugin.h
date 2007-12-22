

#ifndef CROSSREFPLUGIN_H
#define CROSSREFPLUGIN_H

#include "Plugin.h"

class BibData;

class CrossRefPlugin : public Plugin
{
	public:
		CrossRefPlugin () {loaded_ = true;};
		~CrossRefPlugin () {};
		virtual bool resolve (BibData &bib);
		virtual Glib::ustring const getShortName ();
		virtual Glib::ustring const getLongName ();
};

#endif
