
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */




#ifndef UTILITY_H
#define UTILITY_H

#include <utility>
#include <vector>
#include <sstream>

#include <giomm/file.h>
#include <gtkmm.h>
#include <libgnomevfsmm.h>

#include "ucompose.hpp"
#define DEBUG(x,...) Utility::debug(__PRETTY_FUNCTION__, String::ucompose(x, ##__VA_ARGS__))

namespace Utility {
	typedef std::pair <Glib::ustring, Glib::ustring> StringPair;

	Glib::ustring wrap (
			Glib::ustring const &str, 
			Glib::ustring::size_type width, 
			int lines,
			bool const pad);

	Glib::ustring firstAuthor (
			Glib::ustring const &authors);

	bool hasExtension (
		Glib::ustring const &filename,
		Glib::ustring const &ex);

	bool uriIsFast (
		Glib::RefPtr<Gio::File> uri);

	StringPair twoWaySplit (
		Glib::ustring const &str,
		Glib::ustring const &divider);

	Glib::ustring strip (
		Glib::ustring const &victim,
		Glib::ustring const &unwanted);

	Glib::ustring ensureExtension (
		Glib::ustring const &filename,
		Glib::ustring const &extension);

	Glib::ustring findDataFile (
		Glib::ustring const &filename);

	bool fileExists (
		Glib::ustring const &filename);

	void exceptionDialog (
		Glib::Exception const *ex, Glib::ustring const &context);

	std::vector<Glib::ustring> recurseFolder (
		Glib::ustring const &rootfoldername);

	void moveToTrash (
		Glib::ustring const &uri);

	void deleteFile (
		Glib::ustring const &target_uri_str);

	void writeBibKey (
		std::ostringstream &out,
		Glib::ustring key,
		Glib::ustring const & value,
		bool const usebraces,
		bool const utf8);

	std::string escapeBibtexAccents (
		Glib::ustring target);

	Glib::ustring firstCap (
		Glib::ustring original);

	Glib::ustring relPath (
		Glib::ustring parent,
		Glib::ustring child);

	Glib::RefPtr<Gdk::Pixbuf> getThemeIcon(Glib::ustring const &iconname);
	Glib::RefPtr<Gdk::Pixbuf> getThemeMenuIcon(Glib::ustring const &iconname);

	int wvConvertUnicodeToLaTeX(
		gunichar char16,
		Glib::ustring &out);

	Glib::RefPtr<Gdk::Pixbuf> eelEmbedImageInFrame (
		Glib::RefPtr<Gdk::Pixbuf> source,
		Glib::RefPtr<Gdk::Pixbuf> frame,
		int left, int top, int right, int bottom);
		
	Glib::ustring mozUrlSelectionToUTF8 (
		Gtk::SelectionData const &sel);

	Glib::ustring trimWhiteSpace (Glib::ustring const &str);
	Glib::ustring trimLeadingString (Glib::ustring const &str, Glib::ustring const &leader);
	void debug (Glib::ustring tag, Glib::ustring msg);

	/* [bert] Added this function to remove leading "a", "an" or
	 * "the" from an English string, for comparison purposes.
	 */
	Glib::ustring removeLeadingArticle(Glib::ustring const &str);

  	time_t timeValToPosix(Glib::TimeVal time_val);
}


class TextDialog : public Gtk::Dialog
{
	public:
		TextDialog (
				Glib::ustring const &title,
				Glib::ustring const &text);

};

#endif
