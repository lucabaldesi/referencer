
#include <iostream>

#include <libgnomevfsmm.h>

#include "Utility.h"

namespace Utility {

bool DOIURLValid (
	Glib::ustring const &url)
{
	return url.find ("<!DOI!>") != Glib::ustring::npos;
}


StringPair twoWaySplit (
	Glib::ustring const &str,
	Glib::ustring const &divider)
{
	StringPair retval;
	int pos = str.find (divider);
	if (pos == (int)Glib::ustring::npos) {
		retval.first = str;
	} else {
		retval.first = str.substr (0, pos);	
		retval.second = str.substr (pos + divider.length(), str.length ());
	}

	return retval;
}


Glib::ustring strip (
	Glib::ustring const &victim,
	Glib::ustring const &unwanted)
{
	Glib::ustring stripped = victim;
	while (stripped.find (unwanted) != Glib::ustring::npos) {
		int pos = stripped.find (unwanted);
		stripped = stripped.substr (0, pos)
			+ stripped.substr (pos + unwanted.length (), stripped.length ());
	}

	return stripped;
}


Glib::ustring ensureExtension (
	Glib::ustring const &filename,
	Glib::ustring const &extension)
{
	if (filename.find (".") != Glib::ustring::npos)
		return filename;
	else {
		return filename + "." + extension;
	}
}


Glib::RefPtr<Gnome::Glade::Xml> openGlade (
	Glib::ustring const &filename)
{
	Glib::RefPtr<Gnome::Glade::Xml> xml;

	// Make the path absolute only so we can use uri_exists -- glade itself
	// doesn't care if it's relative.
	Glib::ustring localfile;
	if (Glib::path_is_absolute (filename)) {
		localfile = filename;
	} else {
		localfile = Glib::build_filename (Glib::get_current_dir (), filename);
	}

	Glib::RefPtr<Gnome::Vfs::Uri> uri = Gnome::Vfs::Uri::create (localfile);

	if (uri->uri_exists ()) {
		return Gnome::Glade::Xml::create (localfile);
	} else {
		Glib::ustring const installedfile = Glib::build_filename (DATADIR, filename);
		uri = Gnome::Vfs::Uri::create (installedfile);
		if (uri->uri_exists ()) {
			return Gnome::Glade::Xml::create (installedfile);
		}
	}

	// Fall through
	std::cerr << "Utility::openGlade: couldn't "
		"find glade file '" << filename << "'\n";
	return Glib::RefPtr<Gnome::Glade::Xml> (NULL);
}


void exceptionDialog (
	Glib::Exception const *ex, Glib::ustring const &context)
{
	/*Glib::ustring message =
		"<big><b>Exception!</b></big>\n\n"
		"A problem was encountered while "
		+ context
		+ ".  The problem was '"
		+ ex->what ()
		+ "'.";*/

	Glib::ustring message =
		"<big><b>" 
		+ ex->what ()
		+ "</b></big>\n\n"
		"This problem was encountered while "
		+ context
		+ ".";

	Gtk::MessageDialog dialog (
		message, true,
		Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE, true);

	dialog.run ();
}


static Glib::ustring _basepath;
std::vector<Glib::ustring> _filestoadd;

bool onAddDocFolderRecurse (const Glib::ustring& rel_path, const Glib::RefPtr<const Gnome::Vfs::FileInfo>& info, bool recursing_will_loop, bool& recurse)
{
	// We escape to be consistent with the escaped URI that FileChooser
	// gave us for basepath
	Glib::ustring fullname =
		Glib::build_filename (
			_basepath, Gnome::Vfs::escape_string(info->get_name()));

	if (info->get_type () == Gnome::Vfs::FILE_TYPE_DIRECTORY) {
		Gnome::Vfs::DirectoryHandle dir;
		Glib::ustring tmp = _basepath;
		_basepath = fullname;
		dir.visit (
			fullname,
			Gnome::Vfs::FILE_INFO_DEFAULT,
			Gnome::Vfs::DIRECTORY_VISIT_DEFAULT,
			&onAddDocFolderRecurse);
		_basepath = tmp;
	} else {
		_filestoadd.push_back (fullname);
	}

	// What does this retval do?
	return true;
}

std::vector<Glib::ustring> recurseFolder (
	Glib::ustring const &rootfoldername)
{
	Gnome::Vfs::DirectoryHandle dir;
	_basepath = rootfoldername;
	_filestoadd.clear();
	dir.visit (
		rootfoldername,
		Gnome::Vfs::FILE_INFO_DEFAULT,
		Gnome::Vfs::DIRECTORY_VISIT_DEFAULT,
		&onAddDocFolderRecurse);

	// A hurtful copy to avoid _filestoadd sticking around afterwards.
	std::vector<Glib::ustring> filescopy;
	filescopy = _filestoadd;
	_filestoadd.clear ();

	return filescopy;
}


std::vector<Glib::RefPtr<Gnome::Vfs::Uri> > parseUriList (
	char *listtxt)
{
  GList *list = NULL;
  GList *p = NULL;

  list = gnome_vfs_uri_list_parse (listtxt);
  p = list;

	std::vector<Glib::RefPtr<Gnome::Vfs::Uri> > uris;

  while (p != NULL) {
		Glib::RefPtr<Gnome::Vfs::Uri> uri =
			Glib::wrap ((GnomeVFSURI*)(p->data), true);
		uris.push_back (uri);

    p = g_list_next (p);
  }

  gnome_vfs_uri_list_free (list);

	return uris;
}

}
