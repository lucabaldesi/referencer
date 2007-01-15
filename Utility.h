

#ifndef UTILITY_H
#define UTILITY_H

#include <utility>
#include <vector>

#include <gtkmm.h>
#include <libglademm.h>
#include <libgnomevfsmm.h>

namespace Utility {
	typedef std::pair <Glib::ustring, Glib::ustring> StringPair;

	bool DOIURLValid (
		Glib::ustring const &url);

	StringPair twoWaySplit (
		Glib::ustring const &str,
		Glib::ustring const &divider);

	Glib::ustring strip (
		Glib::ustring const &victim,
		Glib::ustring const &unwanted);

	Glib::ustring ensureExtension (
		Glib::ustring const &filename,
		Glib::ustring const &extension);

	Glib::RefPtr<Gnome::Glade::Xml> openGlade (
		Glib::ustring const &filename);

	void exceptionDialog (
		Glib::Exception const *ex, Glib::ustring const &context);

	std::vector<Glib::ustring> recurseFolder (
		Glib::ustring const &rootfoldername);

	std::vector<Glib::RefPtr<Gnome::Vfs::Uri> > parseUriList (
		char *list);
		
	Glib::ustring escapeBibtexAccents (
		Glib::ustring target);
		
	int wvConvertUnicodeToLaTeX(
		gunichar char16,
		Glib::ustring &out);
}

#endif
