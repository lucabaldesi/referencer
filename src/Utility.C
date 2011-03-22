
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */




/*
 * The wvConvertUnicodeToLaTeX function is based on the function of the same
 * name in the wv library, Copyright (C) Caolan McNamara, Dom Lachowicz, and others
 */

/*
 * The eel_* functions are copied from libeel, Copyright © 2000 Eazel, Inc.
 * The art_rgb_run_alpha function in Document.C is copied from libart, Copyright © Raph Levien 1998
*/

/* 
 * mozUrlSelectionToUTF8 is derived from gnome-terminal
 * Copyright © Havoc Pennington 2001
*/

#include <gtkmm.h>
// The gtkmm in Ubuntu 6.04 doesn't seem to get this in <gtkmm.h>
#include <gtkmm/icontheme.h>
#include <glibmm/i18n.h>
#include <giomm/file.h>

#include "ucompose.hpp"

#include <iostream>

#include "Utility.h"

namespace Utility {

Glib::ustring firstAuthor (
		Glib::ustring const &authors)
{
	Glib::ustring::size_type n = authors.find (" and");
	if (n != Glib::ustring::npos) {
		return authors.substr(0, n);
	} else {
		return authors;
	}
}

Glib::ustring wrap (
	Glib::ustring const &str,
	Glib::ustring::size_type width,
	int lines,
	bool const pad)
{
	/*
	 * Note on wrapping
	 *
	 * This is used for padding out entries in the iconview
	 * What we do is pad to 85% of the remaining chars needed
	 * to reach width, with em-spaces.  It's a hack.  It just
	 * about works most of the time.
	 */
	Glib::ustring::size_type n = 0;
	int line = 0;
	if (lines == -1)
		lines = 99999;

	unsigned int remainder = str.size();

	Glib::ustring wrapped;

	gunichar wideSpace = 0x2002;
	Glib::ustring padElement = Glib::ustring (1, wideSpace);

	while (line < lines) {
		if (line > 0)
			wrapped += Glib::ustring("\n");
		if (remainder > width) {
			Glib::ustring snaffle = str.substr (n, width);
			if (snaffle.substr(0,1) == " ")
				snaffle = snaffle.substr(1, snaffle.size() - 1);

			Glib::ustring::size_type bite = snaffle.rfind (" ");
			if (bite == Glib::ustring::npos) {
				bite = snaffle.size();
			} else {
				snaffle = snaffle.substr(0, bite);
			}

			if (line == lines - 1) {
				if (snaffle.size() + 3 < width) {
					snaffle += Glib::ustring ("...");
				} else {
					snaffle = snaffle.substr(0, snaffle.size() - 3);
					snaffle += Glib::ustring ("...");
				}
			}
			
			wrapped += snaffle;
			n += snaffle.size();
			remainder -= snaffle.size();

			if (pad) {
				int mspaces = (int)((float)(width - snaffle.size()) * 0.85);
				for (int i = 0; i < mspaces; ++i)
					wrapped += padElement;
			}

			line++;
		} else {
			Glib::ustring snaffle = str.substr (n, remainder);
			if (snaffle.substr(0,1) == " ")
				snaffle = snaffle.substr(1, snaffle.size() - 1);

			wrapped += snaffle;
			remainder -= snaffle.size();

			if (pad) {
				int mspaces = (int)((float)(width - snaffle.size()) * 0.85);
				for (int i = 0; i < mspaces; ++i)
					wrapped += padElement;
			}

			break;
		}
	}

	return wrapped;
}

bool hasExtension (
	Glib::ustring const &filename,
	Glib::ustring const &ex)
{
	Glib::ustring::size_type pos = filename.find ("." + ex);
	if (pos != Glib::ustring::npos) {
		if (pos + ex.size () + 1 == filename.size())  {
			return true;
		}
	}

	return false;
}


bool uriIsFast (
	Glib::RefPtr<Gio::File> uri)
{
  Glib::ustring const scheme = uri->get_uri_scheme ();
	if (scheme == "file" || scheme == "smb") {
		return true;
	} else {
		return false;
	}
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


Glib::ustring findDataFile (
	Glib::ustring const &filename)
{
	Glib::ustring localfile;
	if (Glib::path_is_absolute (filename)) {
		localfile = filename;
	} else {
		localfile = Glib::build_filename (
			Glib::get_current_dir (), "data");
		localfile = Glib::build_filename (
			localfile, filename);
	}

	Glib::RefPtr<Gnome::Vfs::Uri> uri =
		Gnome::Vfs::Uri::create (localfile);

	if (uri->uri_exists ()) {
		return localfile;
	} else {
		Glib::ustring const installedfile =
			Glib::build_filename (DATADIR, filename);
		uri = Gnome::Vfs::Uri::create (installedfile);
		if (uri->uri_exists ()) {
			return installedfile;
		}
	}

	// Fall through
	DEBUG ("Utility::findDataFile: couldn't "
		"find file '%1'", filename);
	return Glib::ustring();
}


bool fileExists (
	Glib::ustring const &filename)
{
	if (filename.empty ())
		return false;

	Glib::RefPtr<Gnome::Vfs::Uri> uri =
		Gnome::Vfs::Uri::create (filename);

	return uri->uri_exists ();
}


void exceptionDialog (
	Glib::Exception const *ex, Glib::ustring const &context)
{
	//gdk_threads_enter ();
	Glib::ustring message = String::ucompose (
		_("<big><b>%1: %2</b></big>\n\n%3"),
		_("Exception"),
		Glib::Markup::escape_text (ex->what ()),
		String::ucompose (
			_("The operation underway was: \"%1\""),
			context)
		);

	Gtk::MessageDialog dialog (
		message, true,
		Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE, true);
	//gdk_threads_leave ();

	/*
	 * run() contains this:

	  GDK_THREADS_LEAVE ();
  g_main_loop_run (ri.loop);
  GDK_THREADS_ENTER ();
  */

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

	Glib::ustring basename = Glib::filename_display_basename (info->get_name ());

	bool is_reflib = Utility::hasExtension (basename, "reflib");
	bool is_bib = Utility::hasExtension (basename, "bib");
	bool is_dotfile = basename[0] == '.';

	if (basename == ".svn" || basename == "CVS" || is_reflib || is_bib || is_dotfile) {
		return true;
	} else if (info->get_type () == Gnome::Vfs::FILE_TYPE_DIRECTORY) {
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


void writeBibKey (
	std::ostringstream &out,
	Glib::ustring key,
	Glib::ustring const & value,
	bool const usebraces,
	bool const utf8)
{
	if (!key.validate()) {
		DEBUG ("Bad unicode");
		return;
	}

	if (!value.validate ()) {
		DEBUG (String::ucompose ("Bad unicode for key %1", key));
		return;
	}

	if (!value.empty ()) {
		// Okay to always append comma, since bibtex doesn't mind the trailing one
		if (utf8) {
			/* Exception to rule for braces for pages, since 
			 * {{100--200}} causes problems for some reason */
			if (usebraces && key.lowercase() != "pages")
				out << "\t" << key << " = {{" << value << "}},\n";
			else
				out << "\t" << key << " = {" << value << "},\n";
		} else {
			if (usebraces && key.lowercase() != "pages")
				out << "\t" << key << " = {{" << escapeBibtexAccents (value) << "}},\n";
			else
				out << "\t" << key << " = {" << escapeBibtexAccents (value) << "},\n";
		}
	}
}


std::string escapeBibtexAccents (
	Glib::ustring target)
{
	for (Glib::ustring::size_type i = 0; i < target.length(); ++i) {
		gunichar letter = target[i];
		if (letter < 128) {
			// Rationale: although in general we pass through {,},\ etc to allow
			// the user to use his own latex-isms, the ampersand has no legitimate
			// purpose in a bibtex string and is quite common in titles etc.
			if (letter == '&' && i > 0 && target[i - 1] != '\\') {
				target.erase (i, 1);
				target.insert (i, "\\&");
				i += 1;
			}
			continue;
		} else {
			Glib::ustring replacement;

			int gotone = wvConvertUnicodeToLaTeX (letter, replacement);
			if (!gotone) {
				DEBUG ("escapeBibtexAccents: no replacement found for '%1'", letter);
				replacement = "*";
			}

			target.erase (i, 1);
			target.insert (i, replacement);
			i += replacement.length () - 1;
		}
	}

	return target;
}


Glib::ustring firstCap (
	Glib::ustring original)
{
	original = original.lowercase ();
	original = Glib::Unicode::toupper (
		original[0]) + original.substr (1, original.length());
	return original;
}


Glib::ustring relPath (
	Glib::ustring parent,
	Glib::ustring child)
{
	if (parent.empty () || child.empty ()) {
		return "";
	}

	Glib::ustring separator = Glib::build_filename ("-", "-");
	separator = separator.substr (1, separator.length() - 2);

	std::vector<Glib::ustring> libparts;

	Glib::ustring::size_type next;
	while ((next = parent.find (separator)) != Glib::ustring::npos) {
		Glib::ustring chunk = parent.substr (0, next);
		libparts.push_back (chunk);
		parent = parent.substr (next + 1, parent.length() - 1);
	}

	std::vector<Glib::ustring> docparts;

	while ((next = child.find (separator)) != Glib::ustring::npos) {
		Glib::ustring chunk = child.substr (0, next);
		docparts.push_back (chunk);
		child = child.substr (next + 1, child.length() - 1);
	}

	// Should use Gnome::Vfs::Uri::is_parent?
	// and Gnome::Vfs::Uri::resolve_relative?

	bool ischild = true;

	for (Glib::ustring::size_type i = 0; i < libparts.size() - 1; ++i) {
		if (docparts.size() < i + 1 || libparts[i] != docparts[i]) {
			ischild = false;
			break;
		}
	}

	Glib::ustring relfilename = "";
	if (ischild) {
		for (Glib::ustring::size_type i = libparts.size(); i < docparts.size(); ++i) {
			relfilename += docparts[i];
			relfilename += separator;
		}
		relfilename += child;
	}

	return relfilename;
}


Glib::RefPtr<Gdk::Pixbuf> getThemeIcon(Glib::ustring const &iconname)
{
	Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
	if (!theme) {
		DEBUG ("Utility::getThemeIcon: failed to load default theme");
		return Glib::RefPtr<Gdk::Pixbuf> (NULL);
	}

	if (!iconname.empty()) {
		if (theme->has_icon(iconname)) {
			Glib::RefPtr<Gdk::Pixbuf> pixbuf =
				theme->load_icon(iconname, 96, Gtk::ICON_LOOKUP_FORCE_SVG);
			
			if (!pixbuf)
				DEBUG ("Utility::getThemeIcon: icon '%1' failed to load", iconname);
				return pixbuf;
		} else {
			DEBUG ("Utility::getThemeIcon: icon '%1' no found", iconname);
		}
	}

	// Fall through on failure
	return Glib::RefPtr<Gdk::Pixbuf> (NULL);
}


Glib::RefPtr<Gdk::Pixbuf> getThemeMenuIcon(Glib::ustring const &iconname) {
    Glib::RefPtr<Gdk::Pixbuf> icon = getThemeIcon (iconname);
    if (icon)
	return icon->scale_simple (16, 16, Gdk::INTERP_BILINEAR);
    else
	return icon;
}


void deleteFile (
	Glib::ustring const &target_uri_str)
{
	// Caller is responsible for catching Gnome::Vfs exceptions
	Glib::RefPtr<Gio::File> file = Gio::File::create_for_uri(target_uri_str);
    file->remove();
}


#if 0
// This DOES NOT WORK.  Transfer::transfer always throws an exception
void moveToTrash (
	Glib::ustring const &target_uri_str)
{
	// This and following Gnome::Vfs funcs may throw exceptions,
	// but it's our caller's responsibility to catch them.
	Glib::RefPtr<Gnome::Vfs::Uri> target =
		Gnome::Vfs::Uri::create (target_uri_str);

	GnomeVFSURI *newuri = NULL;

	GnomeVFSResult res = gnome_vfs_find_directory (
		target->gobj (),
		GNOME_VFS_DIRECTORY_KIND_TRASH,
		&newuri,
		TRUE,
		TRUE,
		0);

	if (res != GNOME_VFS_OK || newuri == NULL) {
		throw (new Glib::FileError (Glib::FileError::NO_SUCH_ENTITY,
			"Cannot find trash"));
	}


	Glib::RefPtr<Gnome::Vfs::Uri> trash = Glib::wrap (newuri);

	if (trash->is_parent (target, true)) {
		throw (new Glib::FileError (Glib::FileError::EXISTS,
			"File is already in trash"));
	}

	Glib::ustring shortname = target->extract_short_name ();

	Glib::RefPtr<Gnome::Vfs::Uri> dest = trash->append_path (shortname);
	try{
	std::cerr << target->to_string () << "\n";
	std::cerr << dest->to_string () << "\n";
	Gnome::Vfs::Transfer::transfer (
		target,
		dest,
		Gnome::Vfs::XFER_REMOVESOURCE,
		Gnome::Vfs::XFER_ERROR_MODE_ABORT,
		Gnome::Vfs::XFER_OVERWRITE_MODE_SKIP);
	} catch (Gnome::Vfs::exception ex) {
		std::cerr << "2: " << ex.what () << "\n";
	}
}
#endif


/* w00t, copied and pasted from AbiWord
	and then iso8859-1 stuff added */

#undef printf
#define printf(x) out = ("{" x "}");

int wvConvertUnicodeToLaTeX(gunichar char16, Glib::ustring &out)
	{
	//DEBUG: printf("%d,%c\n",char16,char16);

	/*
	german and scandinavian characters, MV 1.7.2000
	See man iso_8859_1

 	This requires the inputencoding latin1 package,
 	see latin1.def. Chars in range 160...255 are just
	put through as these are legal iso-8859-1 symbols.
	(see above)

	Best way to do it until LaTeX is Unicode enabled
	(Omega project).
	-- MV 4.7.2000

	We use a separate if-statement here ... the 'case range'
	construct is gcc specific :-(  -- MV 13/07/2000
	*/

	if ( (char16 >= 0xa0) && (char16 <= 0xff) )
	{
	switch(char16)
		{
		// I don't see how 0xdc ("U) should be ldots...
		/*case 0xdc:
			printf("\\ldots{}");
			return(1);
		}*/
		case 0xA0:
			printf("~"); /* NO-BREAK (SPACE ) */
			return(1);
		case 0xA1:
			printf("!`"); /* ¡ (INVERTED EXCLAMATION MARK ) */
			return(1);
#if 0
		case 0xA2:
			printf(""); /* ¢ (CENT SIGN ) */
			return(1);
#endif
		case 0xA3:
			printf("\\pounds"); /* £ (POUND SIGN ) */
			return(1);
#if 0
		case 0xA4:
			printf(""); /* ¤ (CURRENCY SIGN ) */
			return(1);
		case 0xA5:
			printf(""); /* ¥ (YEN SIGN ) */
			return(1);
		case 0xA6:
			printf(""); /* ¦ (BROKEN BAR ) */
			return(1);
#endif
		case 0xA7:
			printf("\\S"); /* § (SECTION SIGN ) */
			return(1);
		case 0xA8:
			printf("\\\"{}"); /* ¨ (DIAERESIS ) */
			return(1);
		case 0xA9:
			printf("\\copyright"); /* © (COPYRIGHT SIGN ) */
			return(1);
#if 0
		case 0xAA:
			printf(""); /* ª (FEMININE ORDINAL INDICATOR ) */
			return(1);
#endif
		case 0xAB:
			printf("$\\ll$"); /* « (LEFT-POINTING DOUBLE ANGLE QUOTATION MARK ) */
			return(1);
		case 0xAC:
			printf("$\\neg$"); /* ¬ (NOT SIGN ) */
			return(1);
		case 0xAD:
			printf("-"); /* ­ (SOFT HYPHEN ) */
			return(1);
		case 0xAE:
			printf("\\textregistered"); /* ® (REGISTERED SIGN ) */
			return(1);
		case 0xAF:
			printf("$^-$"); /* ¯ (MACRON ) */
			return(1);
		case 0xB0:
			printf("$^\\circ$"); /* ° (DEGREE SIGN ) */
			return(1);
		case 0xB1:
			printf("$\\pm$"); /* ± (PLUS-MINUS SIGN ) */
			return(1);
		case 0xB2:
			printf("$\\mathtwosuperior$"); /* ² (SUPERSCRIPT TWO ) */
			return(1);
		case 0xB3:
			printf("$\\maththreesuperior$"); /* ³ (SUPERSCRIPT THREE ) */
			return(1);
		case 0xB4:
			printf("\\'{}"); /* ´ (ACUTE ACCENT ) */
			return(1);
		case 0xB5:
			printf("$\\mu$"); /* µ (MICRO SIGN ) */
			return(1);
		case 0xB6:
			printf("\\P"); /* ¶ (PILCROW SIGN ) */
			return(1);
		case 0xB7:
			printf("$\\cdot$"); /* · (MIDDLE DOT ) */
			return(1);
		case 0xB8:
			printf("\\c{}"); /* ¸ (CEDILLA ) */
			return(1);
		case 0xB9:
			printf("$\\mathonesuperior$"); /* ¹ (SUPERSCRIPT ONE ) */
			return(1);
#if 0
		case 0xBA:
			printf(""); /* º (MASCULINE ORDINAL INDICATOR ) */
			return(1);
#endif
		case 0xBB:
			printf("$\\gg$"); /* » (RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK ) */
			return(1);
		case 0xBC:
			printf("$\\frac{1}{4}$"); /* ¼ (VULGAR FRACTION ONE QUARTER) */
			return(1);
		case 0xBD:
			printf("$\\frac{1}{2}$"); /* ½ (VULGAR FRACTION ONE HALF) */
			return(1);
		case 0xBE:
			printf("$\\frac{3}{4}$"); /* ¾ (VULGAR FRACTION THREE QUARTERS) */
			return(1);
		case 0xBF:
			printf("?'"); /* ¿ (INVERTED QUESTION MARK ) */
			return(1);
		case 0xC0:
			printf("\\`{A}"); /* À (LATIN CAPITAL LETTER A WITH GRAVE ) */
			return(1);
		case 0xC1:
			printf("\\'{A}"); /* Á (LATIN CAPITAL LETTER A WITH ACUTE ) */
			return(1);
		case 0xC2:
			printf("\\^{A}"); /* Â (LATIN CAPITAL LETTER A WITH CIRCUMFLEX ) */
			return(1);
		case 0xC3:
			printf("\\~{A}"); /* Ã (LATIN CAPITAL LETTER A WITH TILDE ) */
			return(1);
		case 0xC4:
			printf("\\\"{A}"); /* Ä (LATIN CAPITAL LETTER A WITH DIAERESIS ) */
			return(1);
		case 0xC5:
			printf("\\u{A}"); /* Å (LATIN CAPITAL LETTER A WITH RING ABOVE ) */
			return(1);
		case 0xC6:
			printf("\\AE"); /* Æ (LATIN CAPITAL LETTER AE) */
			return(1);
		case 0xC7:
			printf("\\c{C}"); /* Ç (LATIN CAPITAL LETTER C WITH CEDILLA ) */
			return(1);
		case 0xC8:
			printf("\\`{E}"); /* È (LATIN CAPITAL LETTER E WITH GRAVE ) */
			return(1);
		case 0xC9:
			printf("\\'{E}"); /* É (LATIN CAPITAL LETTER E WITH ACUTE ) */
			return(1);
		case 0xCA:
			printf("\\^{E}"); /* Ê (LATIN CAPITAL LETTER E WITH CIRCUMFLEX ) */
			return(1);
		case 0xCB:
			printf("\\\"{E}"); /* Ë (LATIN CAPITAL LETTER E WITH DIAERESIS ) */
			return(1);
		case 0xCC:
			printf("\\`{I}"); /* Ì (LATIN CAPITAL LETTER I WITH GRAVE ) */
			return(1);
		case 0xCD:
			printf("\\'{I}"); /* Í (LATIN CAPITAL LETTER I WITH ACUTE ) */
			return(1);
		case 0xCE:
			printf("\\^{I}"); /* Î (LATIN CAPITAL LETTER I WITH CIRCUMFLEX ) */
			return(1);
		case 0xCF:
			printf("\\\"{I}"); /* Ï (LATIN CAPITAL LETTER I WITH DIAERESIS ) */
			return(1);
		case 0xD0:
			printf("\\OE"); /* Ð (LATIN CAPITAL LETTER ETH) */
			return(1);
		case 0xD1:
			printf("\\~{N}"); /* Ñ (LATIN CAPITAL LETTER N WITH TILDE ) */
			return(1);
		case 0xD2:
			printf("\\`{O}"); /* Ò (LATIN CAPITAL LETTER O WITH GRAVE ) */
			return(1);
		case 0xD3:
			printf("\\'{O}"); /* Ó (LATIN CAPITAL LETTER O WITH ACUTE ) */
			return(1);
		case 0xD4:
			printf("\\^{O}"); /* Ô (LATIN CAPITAL LETTER O WITH CIRCUMFLEX ) */
			return(1);
		case 0xD5:
			printf("\\~{O}"); /* Õ (LATIN CAPITAL LETTER O WITH TILDE ) */
			return(1);
		case 0xD6:
			printf("\\\"{O}"); /* Ö (LATIN CAPITAL LETTER O WITH DIAERESIS ) */
			return(1);
		case 0xD7:
			printf("$\\times$"); /* × (MULTIPLICATION SIGN ) */
			return(1);
		case 0xD8:
			printf("\\O"); /* Ø (LATIN CAPITAL LETTER O WITH STROKE ) */
			return(1);
		case 0xD9:
			printf("\\`{U}"); /* Ù (LATIN CAPITAL LETTER U WITH GRAVE ) */
			return(1);
		case 0xDA:
			printf("\\'{U}"); /* Ú (LATIN CAPITAL LETTER U WITH ACUTE ) */
			return(1);
		case 0xDB:
			printf("\\^{U}"); /* Û (LATIN CAPITAL LETTER U WITH CIRCUMFLEX ) */
			return(1);
		case 0xDC:
			printf("\\\"{U}"); /* Ü (LATIN CAPITAL LETTER U WITH DIAERESIS ) */
			return(1);
		case 0xDD:
			printf("\\'{U}"); /* Ý (LATIN CAPITAL LETTER Y WITH ACUTE ) */
			return(1);
		case 0xDE:
			printf("\\L"); /* Þ (LATIN CAPITAL LETTER THORN) */
			return(1);
		case 0xDF:
			printf("\\ss{}"); /* ß (LATIN SMALL LETTER SHARP S ) */
			return(1);
		case 0xE0:
			printf("\\`{a}"); /* à (LATIN SMALL LETTER A WITH GRAVE ) */
			return(1);
		case 0xE1:
			printf("\\'{a}"); /* á (LATIN SMALL LETTER A WITH ACUTE ) */
			return(1);
		case 0xE2:
			printf("\\^{a}"); /* â (LATIN SMALL LETTER A WITH CIRCUMFLEX ) */
			return(1);
		case 0xE3:
			printf("\\~{a}"); /* ã (LATIN SMALL LETTER A WITH TILDE ) */
			return(1);
		case 0xE4:
			printf("\\\"{a}"); /* ä (LATIN SMALL LETTER A WITH DIAERESIS ) */
			return(1);
		case 0xE5:
			printf("\\aa"); /* å (LATIN SMALL LETTER A WITH RING ABOVE ) */
			return(1);
		case 0xE6:
			printf("\\ae"); /* æ (LATIN SMALL LETTER AE) */
			return(1);
		case 0xE7:
			printf("\\c{c}"); /* ç (LATIN SMALL LETTER C WITH CEDILLA ) */
			return(1);
		case 0xE8:
			printf("\\`{e}"); /* è (LATIN SMALL LETTER E WITH GRAVE ) */
			return(1);
		case 0xE9:
			printf("\\'{e}"); /* é (LATIN SMALL LETTER E WITH ACUTE ) */
			return(1);
		case 0xEA:
			printf("\\^{e}"); /* ê (LATIN SMALL LETTER E WITH CIRCUMFLEX ) */
			return(1);
		case 0xEB:
			printf("\\\"{e}"); /* ë (LATIN SMALL LETTER E WITH DIAERESIS ) */
			return(1);
		case 0xEC:
			printf("\\`{i}"); /* ì (LATIN SMALL LETTER I WITH GRAVE ) */
			return(1);
		case 0xED:
			printf("\\'{i}"); /* í (LATIN SMALL LETTER I WITH ACUTE ) */
			return(1);
		case 0xEE:
			printf("\\^{i}"); /* î (LATIN SMALL LETTER I WITH CIRCUMFLEX ) */
			return(1);
		case 0xEF:
			printf("\\\"{i}"); /* ï (LATIN SMALL LETTER I WITH DIAERESIS ) */
			return(1);
		case 0xF0:
			printf("\\oe"); /* ð (LATIN SMALL LETTER ETH) */
			return(1);
		case 0xF1:
			printf("\\~{n}"); /* ñ (LATIN SMALL LETTER N WITH TILDE ) */
			return(1);
		case 0xF2:
			printf("\\`{o}"); /* ò (LATIN SMALL LETTER O WITH GRAVE ) */
			return(1);
		case 0xF3:
			printf("\\'{o}"); /* ó (LATIN SMALL LETTER O WITH ACUTE ) */
			return(1);
		case 0xF4:
			printf("\\^{o}"); /* ô (LATIN SMALL LETTER O WITH CIRCUMFLEX ) */
			return(1);
		case 0xF5:
			printf("\\~{o}"); /* õ (LATIN SMALL LETTER O WITH TILDE ) */
			return(1);
		case 0xF6:
			printf("\\\"{o}"); /* ö (LATIN SMALL LETTER O WITH DIAERESIS ) */
			return(1);
		case 0xF7:
			printf("$\\div$"); /* ÷ (DIVISION SIGN ) */
			return(1);
		case 0xF8:
			printf("\\o"); /* ø (LATIN SMALL LETTER O WITH STROKE ) */
			return(1);
		case 0xF9:
			printf("\\`{u}"); /* ù (LATIN SMALL LETTER U WITH GRAVE ) */
			return(1);
		case 0xFA:
			printf("\\'{u}"); /* ú (LATIN SMALL LETTER U WITH ACUTE ) */
			return(1);
		case 0xFB:
			printf("\\^{u}"); /* û (LATIN SMALL LETTER U WITH CIRCUMFLEX ) */
			return(1);
		case 0xFC:
			printf("\\\"{u}"); /* ü (LATIN SMALL LETTER U WITH DIAERESIS ) */
			return(1);
		case 0xFD:
			printf("\\'{y}"); /* ý (LATIN SMALL LETTER Y WITH ACUTE ) */
			return(1);
		case 0xFE:
			printf("\\l"); /* þ (LATIN SMALL LETTER THORN) */
			return(1);
		case 0xFF:
			printf("\\\"{y}"); /* ÿ (LATIN SMALL LETTER Y WITH DIAERESIS ) */
			return(1);
		return(0);
	}
	}
	switch(char16)
		{
		case 37:
			printf("\\%%");
			return(1);
		case 11:
			printf("\\\\\n");
			return(1);
		case 30:
		case 31:

		case 12:
		case 13:
		case 14:
		case 7:
			return(1);
		case 45:
			printf("-");
			return(1);
		case 34:
			printf("\"");
			return(1);
		case 35:
			printf("\\#"); /* MV 14.8.2000 */
			return(1);
		case 36:
			printf("\\$"); /* MV 14.8.2000 */
			return(1);
		case 38:
			printf("\\&"); /* MV 1.7.2000 */
			return(1);
		case 60:
			printf("$<$");
			return(1);
		case 62:
			printf("$>$");
			return(1);

		case 0xF8E7:
		/* without this, things should work in theory, but not for me */
			printf("_");
			return(1);

	/* Added some new Unicode characters. It's probably difficult
           to write these characters in AbiWord, though ... :(
           -- 2000-08-11 huftis@bigfoot.com */

		case 0x0100:
			printf("\\=A"); /* A with macron */
			return(1);
		case 0x0101:
			printf("\\=a");  /* a with macron */
			return(1);
		case 0x0102:
			printf("\\u{A}");  /* A with breve */
			return(1);
		case 0x0103:
			printf("\\u{a}");  /* a with breve */
			return(1);

		case 0x0106:
			printf("\\'C");  /* C with acute */
			return(1);
		case 0x0107:
			printf("\\'c");  /* c with acute */
			return(1);
		case 0x0108:
			printf("\\^C");  /* C with circumflex */
			return(1);
		case 0x0109:
			printf("\\^c");  /* c with circumflex */
			return(1);
		case 0x010A:
			printf("\\.C");  /* C with dot above */
			return(1);
		case 0x010B:
			printf("\\.c");  /* c with dot above */
			return(1);
		case 0x010C:
			printf("\\v{C}");  /* C with caron */
			return(1);
		case 0x010D:
			printf("\\v{c}");  /* c with caron */
			return(1);
		case 0x010E:
			printf("\\v{D}");  /* D with caron */
			return(1);
		case 0x010F:
			printf("\\v{d}");  /* d with caron */
			return(1);
		case 0x0110:
			printf("\\DJ{}");  /* D with stroke */
			return(1);
		case 0x0111:
			printf("\\dj{}");  /* d with stroke */
			return(1);
		case 0x0112:
			printf("\\=E");  /* E with macron */
			return(1);
		case 0x0113:
			printf("\\=e");  /* e with macron */
			return(1);
		case 0x0114:
			printf("\\u{E}");  /* E with breve */
			return(1);
		case 0x0115:
			printf("\\u{e}");  /* e with breve */
			return(1);
		case 0x0116:
			printf("\\.E");  /* E with dot above */
			return(1);
		case 0x0117:
			printf("\\.e");  /* e with dot above */
			return(1);

		case 0x011A:
			printf("\\v{E}");  /* E with caron */
			return(1);
		case 0x011B:
			printf("\\v{e}");  /* e with caron */
			return(1);
		case 0x011C:
			printf("\\^G");  /* G with circumflex */
			return(1);
		case 0x011D:
			printf("\\^g");  /* g with circumflex */
			return(1);
		case 0x011E:
			printf("\\u{G}");  /* G with breve */
			return(1);
		case 0x011F:
			printf("\\u{g}");  /* g with breve */
			return(1);
		case 0x0120:
			printf("\\.G");  /* G with dot above */
			return(1);
		case 0x0121:
			printf("\\u{g}");  /* g with dot above */
			return(1);
		case 0x0122:
			printf("^H");  /* H with circumflex */
			return(1);
		case 0x0123:
			printf("^h");  /* h with circumflex */
			return(1);

		case 0x0128:
			printf("\\~I");  /* I with tilde */
			return(1);
		case 0x0129:
			printf("\\~{\\i}");  /* i with tilde (dotless) */
			return(1);
		case 0x012A:
			printf("\\=I");  /* I with macron */
			return(1);
		case 0x012B:
			printf("\\={\\i}");  /* i with macron (dotless) */
			return(1);
		case 0x012C:
			printf("\\u{I}");  /* I with breve */
			return(1);
		case 0x012D:
			printf("\\u{\\i}");  /* i with breve */
			return(1);

		case 0x0130:
			printf("\\.I");  /* I with dot above */
			return(1);
		case 0x0131:
			printf("\\i{}");  /* dotless i */
			return(1);
		case 0x0132:
			printf("IJ");  /* IJ ligature */
			return(1);
		case 0x0133:
			printf("ij");  /* ij ligature  */
			return(1);
		case 0x0134:
			printf("\\^J");  /* J with circumflex (dotless) */
			return(1);
		case 0x0135:
			printf("\\^{\\j}");  /* j with circumflex (dotless) */
			return(1);
		case 0x0136:
			printf("\\c{K}");  /* K with cedilla */
			return(1);
		case 0x0137:
			printf("\\c{k}");  /* k with cedilla */
			return(1);

		case 0x0138:
			printf("k");  /* NOTE: Not the correct character (kra), but similar */
			return(1);

		case 0x0139:
			printf("\\'L");  /* L with acute */
			return(1);
		case 0x013A:
			printf("\\'l");  /* l with acute  */
			return(1);
		case 0x013B:
			printf("\\c{L}");  /* L with cedilla */
			return(1);
		case 0x013C:
			printf("\\c{l}");  /* l with cedilla */
			return(1);
		case 0x013D:
			printf("\\v{L}");  /* L with caron */
			return(1);
		case 0x013E:
			printf("\\v{l}");  /* l with caron */
			return(1);
		case 0x0141:
			printf("\\L{}");  /* L with stroke */
			return(1);
		case 0x0142:
			printf("\\l{}");  /* l with stroke  */
			return(1);
		case 0x0143:
			printf("\\'N");  /* N with acute */
			return(1);
		case 0x0144:
			printf("\\'n");  /* n with acute */
			return(1);
		case 0x0145:
			printf("\\c{N}");  /* N with cedilla */
			return(1);
		case 0x0146:
			printf("\\c{n}");  /* n with cedilla */
			return(1);
		case 0x0147:
			printf("\\v{N}");  /* N with caron */
			return(1);
		case 0x0148:
			printf("\\v{n}");  /* n with caron */
			return(1);
		case 0x0149:
			printf("'n");  /* n preceed with apostroph  */
			return(1);
		case 0x014A:
			printf("\\NG{}");  /* ENG character */
			return(1);
		case 0x014B:
			printf("\\ng{}");  /* eng character */
			return(1);
		case 0x014C:
			printf("\\=O");  /* O with macron */
			return(1);
		case 0x014D:
			printf("\\=o");  /* o with macron */
			return(1);
		case 0x014E:
			printf("\\u{O}");  /* O with breve */
			return(1);
		case 0x014F:
			printf("\\u{o}");  /* o with breve */
			return(1);
		case 0x0150:
			printf("\\H{O}");  /* O with double acute */
			return(1);
		case 0x0151:
			printf("\\H{o}");  /* o with double acute */
			return(1);
		case 0x0152:
			printf("\\OE{}");  /* OE ligature */
			return(1);
		case 0x0153:
			printf("\\oe{}");  /* oe ligature */
			return(1);
		case 0x0154:
			printf("\\'R");  /* R with acute */
			return(1);
		case 0x0155:
			printf("\\'r");  /* r with acute */
			return(1);
		case 0x0156:
			printf("\\c{R}");  /* R with cedilla */
			return(1);
		case 0x0157:
			printf("\\c{r}");  /* r with cedilla */
			return(1);
		case 0x0158:
			printf("\\v{R}");  /* R with caron */
			return(1);
		case 0x0159:
			printf("\\v{r}");  /* r with caron */
			return(1);
		case 0x015A:
			printf("\\'S");  /* S with acute */
			return(1);
		case 0x015B:
			printf("\\'s");  /* s with acute */
			return(1);
		case 0x015C:
			printf("\\^S");  /* S with circumflex */
			return(1);
		case 0x015D:
			printf("\\^s");  /* c with circumflex */
			return(1);
		case 0x015E:
			printf("\\c{S}");  /* S with cedilla */
			return(1);
		case 0x015F:
			printf("\\c{s}");  /* s with cedilla */
			return(1);
		case 0x0160:
			printf("\\v{S}");  /* S with caron */
			return(1);
		case 0x0161:
			printf("\\v{s}");  /* s with caron */
			return(1);
		case 0x0162:
			printf("\\c{T}");  /* T with cedilla */
			return(1);
		case 0x0163:
			printf("\\c{t}");  /* t with cedilla */
			return(1);
		case 0x0164:
			printf("\\v{T}");  /* T with caron */
			return(1);
		case 0x0165:
			printf("\\v{t}");  /* t with caron */
			return(1);

		case 0x0168:
			printf("\\~U");  /* U with tilde */
			return(1);
		case 0x0169:
			printf("\\~u");  /* u with tilde */
			return(1);
		case 0x016A:
			printf("\\=U");  /* U with macron */
			return(1);

		/* Greek (thanks Petr Vanicek!): */
		case 0x0391:
			printf("$\\Alpha$");
			return(1);
		case 0x0392:
			printf("$\\Beta$");
			return(1);
		case 0x0393:
			printf("$\\Gamma$");
			return(1);
		case 0x0394:
			printf("$\\Delta$");
			return(1);
		case 0x0395:
			printf("$\\Epsilon$");
			return(1);
		case 0x0396:
			printf("$\\Zeta$");
			return(1);
		case 0x0397:
			printf("$\\Eta$");
			return(1);
		case 0x0398:
			printf("$\\Theta$");
			return(1);
		case 0x0399:
			printf("$\\Iota$");
			return(1);
		case 0x039a:
			printf("$\\Kappa$");
			return(1);
		case 0x039b:
			printf("$\\Lambda$");
			return(1);
		case 0x039c:
			printf("$\\Mu$");
			return(1);
		case 0x039d:
			printf("$\\Nu$");
			return(1);
		case 0x039e:
			printf("$\\Xi$");
			return(1);
		case 0x039f:
			printf("$\\Omicron$");
			return(1);
		case 0x03a0:
			printf("$\\Pi$");
			return(1);
		case 0x03a1:
			printf("$\\Rho$");
			return(1);

		case 0x03a3:
			printf("$\\Sigma$");
			return(1);
		case 0x03a4:
			printf("$\\Tau$");
			return(1);
		case 0x03a5:
			printf("$\\Upsilon$");
			return(1);
		case 0x03a6:
			printf("$\\Phi$");
			return(1);
		case 0x03a7:
			printf("$\\Chi$");
			return(1);
		case 0x03a8:
			printf("$\\Psi$");
			return(1);
		case 0x03a9:
			printf("$\\Omega$");
			return(1);

		/* ...and lower case: */

		case 0x03b1:
			printf("$\\alpha$");
			return(1);
		case 0x03b2:
			printf("$\\beta$");
			return(1);
		case 0x03b3:
			printf("$\\gamma$");
			return(1);
		case 0x03b4:
			printf("$\\delta$");
			return(1);
		case 0x03b5:
			printf("$\\epsilon$");
			return(1);
		case 0x03b6:
			printf("$\\zeta$");
			return(1);
		case 0x03b7:
			printf("$\\eta$");
			return(1);
		case 0x03b8:
			printf("$\\theta$");
			return(1);
		case 0x03b9:
			printf("$\\iota$");
			return(1);
		case 0x03ba:
			printf("$\\kappa$");
			return(1);
		case 0x03bb:
			printf("$\\lambda$");
			return(1);
		case 0x03bc:
			printf("$\\mu$");
			return(1);
		case 0x03bd:
			printf("$\\nu$");
			return(1);
		case 0x03be:
			printf("$\\xi$");
			return(1);
		case 0x03bf:
			printf("$\\omicron$");
			return(1);
		case 0x03c0:
			printf("$\\pi$");
			return(1);
		case 0x03c1:
			printf("$\\rho$");
			return(1);

		case 0x03c3:
			printf("$\\sigma$");
			return(1);
		case 0x03c4:
			printf("$\\tau$");
			return(1);
		case 0x03c5:
			printf("$\\upsilon$");
			return(1);
		case 0x03c6:
			printf("$\\phi$");
			return(1);
		case 0x03c7:
			printf("$\\chi$");
			return(1);
		case 0x03c8:
			printf("$\\psi$");
			return(1);
		case 0x03c9:
			printf("$\\omega$");
			return(1);

	/* More math, typical inline: */
		case 0x2111:
			printf("$\\Im$");
			return(1);
		case 0x2118:
			printf("$\\wp$");   /* Weierstrass p */
			return(1);
		case 0x211c:
			printf("$\\Re$");
			return(1);
		case 0x2122:
			printf("\\texttrademark"); /* ™ (TRADEMARK SIGN ) */
			return (1);
		case 0x2135:
			printf("$\\aleph$");
			return(1);

		case 0x2190:
			printf("$\\leftarrow$");
			return(1);
		case 0x2191:
			printf("$\\uparrow$");
			return(1);
		case 0x2192:
			printf("$\\rightarrow$");
			return(1);
		case 0x2193:
			printf("$\\downarrow$");
			return(1);
		case 0x21d0:
			printf("$\\Leftarrow$");
			return(1);
		case 0x21d1:
			printf("$\\Uparrow$");
			return(1);
		case 0x21d2:
			printf("$\\Rightarrow$");
			return(1);
		case 0x21d3:
			printf("$\\Downarrow$");
			return(1);
		case 0x21d4:
			printf("$\\Leftrightarrow$");
			return(1);

		case 0x2200:
			printf("$\\forall$");
			return(1);
		case 0x2202:
			printf("$\\partial$");
			return(1);
		case 0x2203:
			printf("$\\exists$");
			return(1);
		case 0x2205:
			printf("$\\emptyset$");
			return(1);
		case 0x2207:
			printf("$\\nabla$");
			return(1);
		case 0x2208:
			printf("$\\in$");   /* element of */
			return(1);
		case 0x2209:
			printf("$\\notin$");   /* not an element of */
			return(1);
		case 0x220b:
			printf("$\\ni$");   /* contains as member */
			return(1);
		case 0x221a:
			printf("$\\surd$"); 	/* sq root */
			return(1);
		case 0x2212:
			printf("$-$");		/* minus */
			return(1);
		case 0x221d:
			printf("$\\propto$");
			return(1);
		case 0x221e:
			printf("$\\infty$");
			return(1);
		case 0x2220:
			printf("$\\angle$");
			return(1);
		case 0x2227:
			printf("$\\land$"); /* logical and */
			return(1);
		case 0x2228:
			printf("$\\lor$");   /* logical or */
			return(1);
		case 0x2229:
			printf("$\\cap$"); /* intersection */
			return(1);
		case 0x222a:
			printf("$\\cup$"); /* union */
			return(1);
		case 0x223c:
			printf("$\\sim$"); /* similar to  */
			return(1);
		case 0x2248:
			printf("$\\approx$");
			return(1);
		case 0x2261:
			printf("$\\equiv$");
			return(1);
		case 0x2260:
			printf("$\\neq$");
			return(1);
		case 0x2264:
			printf("$\\leq$");
			return(1);
		case 0x2265:
			printf("$\\geq$");
			return(1);
		case 0x2282:
			printf("$\\subset$");
			return(1);
		case 0x2283:
			printf("$\\supset$");
			return(1);
		case 0x2284:
			printf("$\\notsubset$");
			return(1);
		case 0x2286:
			printf("$\\subseteq$");
			return(1);
		case 0x2287:
			printf("$\\supseteq$");
			return(1);
		case 0x2295:
			printf("$\\oplus$");   /* circled plus */
			return(1);
		case 0x2297:
			printf("$\\otimes$");
			return(1);
		case 0x22a5:
			printf("$\\perp$");	/* perpendicular */
			return(1);




		case 0x2660:
			printf("$\\spadesuit$");
			return(1);
		case 0x2663:
			printf("$\\clubsuit$");
			return(1);
		case 0x2665:
			printf("$\\heartsuit$");
			return(1);
		case 0x2666:
			printf("$\\diamondsuit$");
			return(1);


		case 0x01C7:
			printf("LJ");  /* the LJ letter */
			return(1);
		case 0x01C8:
			printf("Lj");  /* the Lj letter */
			return(1);
		case 0x01C9:
			printf("lj");  /* the lj letter */
			return(1);
		case 0x01CA:
			printf("NJ");  /* the NJ letter */
			return(1);
		case 0x01CB:
			printf("Nj");  /* the Nj letter */
			return(1);
		case 0x01CC:
			printf("nj");  /* the nj letter */
			return(1);
		case 0x01CD:
			printf("\\v{A}");  /* A with caron */
			return(1);
		case 0x01CE:
			printf("\\v{a}");  /* a with caron */
			return(1);
		case 0x01CF:
			printf("\\v{I}");  /* I with caron */
			return(1);
		case 0x01D0:
			printf("\\v{\\i}");  /* i with caron (dotless) */
			return(1);
		case 0x01D1:
			printf("\\v{O}");  /* O with caron */
			return(1);
		case 0x01D2:
			printf("\\v{o}");  /* o with caron */
			return(1);
		case 0x01D3:
			printf("\\v{U}");  /* U with caron */
			return(1);
		case 0x01D4:
			printf("\\v{u}");  /* u with caron */
			return(1);

		case 0x01E6:
			printf("\\v{G}");  /* G with caron */
			return(1);
		case 0x01E7:
			printf("\\v{g}");  /* g with caron */
			return(1);
		case 0x01E8:
			printf("\\v{K}");  /* K with caron */
			return(1);
		case 0x01E9:
			printf("\\v{k}");  /* k with caron */
			return(1);


		case 0x01F0:
			printf("\\v{\\j}");  /* j with caron (dotless) */
			return(1);
		case 0x01F1:
			printf("DZ");  /* the DZ letter */
			return(1);
		case 0x01F2:
			printf("Dz");  /* the Dz letter */
			return(1);
		case 0x01F3:
			printf("dz");  /* the dz letter */
			return(1);
		case 0x01F4:
			printf("\\'G");  /* G with acute */
			return(1);
		case 0x01F5:
			printf("\\'g");  /* g with acute */
			return(1);

		case 0x01FA:
			printf("\\'{\\AA}");  /* Å with acute */
			return(1);
		case 0x01FB:
			printf("\\'{\\aa}");  /* å with acute */
			return(1);
		case 0x01FC:
			printf("\\'{\\AE}");  /* Æ with acute */
			return(1);
		case 0x01FD:
			printf("\\'{\\ae}");  /* æ with acute */
			return(1);
		case 0x01FE:
			printf("\\'{\\O}");  /* Ø with acute */
			return(1);
		case 0x01FF:
			printf("\\'{\\o}");  /* ø with acute */
			return(1);

		case 0x2010:
			printf("-"); /* hyphen */
			return(1);
		case 0x2011:
			printf("-"); /* non-breaking hyphen (is there a way to get this in LaTeX?) */
			return(1);
		case 0x2012:
			printf("--"); /* figure dash (similar to en-dash) */
			return(1);
		case 0x2013:
			/*
			soft-hyphen? Or en-dash? I find that making
			this a soft-hyphen works very well, but makes
			the occasional "hard" word-connection hyphen
			(like the "-" in roller-coaster) disappear.
			(Are these actually en-dashes? Dunno.)
			How does MS Word distinguish between the 0x2013's
			that signify soft hyphens and those that signify
			word-connection hyphens? wvware should be able
			to as well. -- MV 8.7.2000

			U+2013 is the en-dash character and not a soft
			hyphen. Soft hyphen is U+00AD. Changing to
			"--". -- 2000-08-11 huftis@bigfoot.com
			*/
			printf("--");
			return(1);

		case 0x016B:
			printf("\\=u");  /* u with macron */
			return(1);
		case 0x016C:
			printf("\\u{U}");  /* U with breve */
			return(1);
		case 0x016D:
			printf("\\u{u}");  /* u with breve */
			return(1);
		case 0x016E:
			printf("\\r{U}");  /* U with ring above */
			return(1);
		case 0x016F:
			printf("\\r{u}");  /* u with ring above */
			return(1);
		case 0x0170:
			printf("\\H{U}");  /* U with double acute */
			return(1);
		case 0x0171:
			printf("\\H{u}");  /* u with double acute */
			return(1);

		case 0x0174:
			printf("\\^W");  /* W with circumflex */
			return(1);
		case 0x0175:
			printf("\\^w");  /* w with circumflex */
			return(1);
		case 0x0176:
			printf("\\^Y");  /* Y with circumflex */
			return(1);
		case 0x0177:
			printf("\\^y");  /* y with circumflex */
			return(1);
		case 0x0178:
			printf("\\\"Y");  /* Y with diaeresis */
			return(1);
		case 0x0179:
			printf("\\'Z");  /* Z with acute */
			return(1);
		case 0x017A:
			printf("\\'z");  /* z with acute */
			return(1);
		case 0x017B:
			printf("\\.Z");  /* Z with dot above */
			return(1);
		case 0x017C:
			printf("\\.z");  /* z with dot above */
			return(1);
		case 0x017D:
			printf("\\v{Z}");  /* Z with caron */
			return(1);
		case 0x017E:
			printf("\\v{z}");  /* z with caron */
			return(1);

	/* Windows specials (MV 4.7.2000). More could be added.
	See http://www.hut.fi/u/jkorpela/www/windows-chars.html
	*/

		case 0x2000:
			printf ("\\enspace"); /* en space */
			return (1);
		case 0x2001:
			printf ("\\emspace"); /* en space */
			return (1);
		case 0x2002:
			printf ("\\enspace"); /* en space */
			return (1);
		case 0x2003:
			printf ("\\emspace"); /* en space */
			return (1);

		case 0x2009:
			printf ("\\thinspace"); /* thin space */
			return (1);

		case 0x2014:
			printf("---"); /* em-dash */
			return(1);
		case 0x2018:
			printf("`");  /* left single quote, Win */
			return(1);
		case 0x2019:
			printf("'");  /* Right single quote, Win */
			return(1);
		case 0x201A:
			printf("\\quotesinglbase{}");  /* single low 99 quotation mark */
			return(1);
		case 0x201C:
			printf("``");  /* inverted double quotation mark */
			return(1);
		case 0x201D:
			printf("''");  /* double q.m. */
			return(1);
		case 0x201E:
			printf("\\quotedblbase{}");  /* double low 99 quotation mark */
			return(1);
		case 0x2020:
			printf("\\dag{}");  /* dagger */
			return(1);
		case 0x2021:
			printf("\\ddag{}");  /* double dagger */
			return(1);
		case 0x2022:
			printf("$\\bullet$");  /* bullet */
			return(1);
		case 0x2023:
			printf("$\\bullet$");  /* NOTE: Not a real triangular bullet */
			return(1);

		case 0x2024:
			printf(".");  /* One dot leader (for use in TOCs) */
			return(1);
		case 0x2025:
			printf("..");  /* Two dot leader (for use in TOCs) */
			return(1);
		case 0x2026:
			printf("\\ldots{}"); /* ellipsis */
			return(1);

		case 0x2039:
			printf("\\guilsinglleft{}");  /* single left angle quotation mark */
			return(1);
		case 0x203A:
			printf("\\guilsinglright{}"); /* single right angle quotation mark */
			return(1);

		case 0x203C:
			printf("!!"); /* double exclamation mark */
			return(1);

		case 0x2215:
			printf("$/$");  /* Division slash */
			return(1);

		case 0x2030:
			printf("o/oo");
			return(1);

		case 0x20ac:
			printf("\\euro");
                        /* No known implementation ;-)

			TODO
                        Shouldn't we use the package 'eurofont'?
                        -- 2000-08-15 huftis@bigfoot.com
                        */
			return(1);

		case 0x2160:
			printf("I"); /* Roman numeral I */
			return(1);
		case 0x2161:
			printf("II"); /* Roman numeral II */
			return(1);
		case 0x2162:
			printf("III"); /* Roman numeral III */
			return(1);
		case 0x2163:
			printf("IV"); /* Roman numeral IV */
			return(1);
		case 0x2164:
			printf("V"); /* Roman numeral V */
			return(1);
		case 0x2165:
			printf("VI"); /* Roman numeral VI */
			return(1);
		case 0x2166:
			printf("VII"); /* Roman numeral VII */
			return(1);
		case 0x2167:
			printf("VIII"); /* Roman numeral VIII */
			return(1);
		case 0x2168:
			printf("IX"); /* Roman numeral IX */
			return(1);
		case 0x2169:
			printf("X"); /* Roman numeral X */
			return(1);
		case 0x216A:
			printf("XI"); /* Roman numeral XI */
			return(1);
		case 0x216B:
			printf("XII"); /* Roman numeral XII */
			return(1);
		case 0x216C:
			printf("L"); /* Roman numeral L */
			return(1);
		case 0x216D:
			printf("C"); /* Roman numeral C */
			return(1);
		case 0x216E:
			printf("D"); /* Roman numeral D */
			return(1);
		case 0x216F:
			printf("M"); /* Roman numeral M */
			return(1);
		case 0x2170:
			printf("i"); /* Roman numeral i */
			return(1);
		case 0x2171:
			printf("ii"); /* Roman numeral ii */
			return(1);
		case 0x2172:
			printf("iii"); /* Roman numeral iii */
			return(1);
		case 0x2173:
			printf("iv"); /* Roman numeral iv */
			return(1);
		case 0x2174:
			printf("v"); /* Roman numeral v */
			return(1);
		case 0x2175:
			printf("vi"); /* Roman numeral vi */
			return(1);
		case 0x2176:
			printf("vii"); /* Roman numeral vii */
			return(1);
		case 0x2177:
			printf("viii"); /* Roman numeral viii */
			return(1);
		case 0x2178:
			printf("ix"); /* Roman numeral ix */
			return(1);
		case 0x2179:
			printf("x"); /* Roman numeral x */
			return(1);
		case 0x217A:
			printf("xi"); /* Roman numeral xi */
			return(1);
		case 0x217B:
			printf("xiii"); /* Roman numeral xii */
			return(1);
		case 0x217C:
			printf("l"); /* Roman numeral l */
			return(1);
		case 0x217D:
			printf("c"); /* Roman numeral c */
			return(1);
		case 0x217E:
			printf("d"); /* Roman numeral d */
			return(1);
		case 0x217F:
			printf("m"); /* Roman numeral m */
			return(1);

		}
	/* Debugging aid: */
	return(0);
	}
#undef printf


typedef unsigned char art_u8;
/* Render a semitransparent run of solid color over an existing RGB
   buffer. */
static void
art_rgb_run_alpha (art_u8 *buf, art_u8 r, art_u8 g, art_u8 b, int alpha, int n)
{
  int i;
  int v;

  for (i = 0; i < n; i++)
    {
      v = *buf;
      *buf++ = v + (((r - v) * alpha + 0x80) >> 8);
      v = *buf;
      *buf++ = v + (((g - v) * alpha + 0x80) >> 8);
      v = *buf;
      *buf++ = v + (((b - v) * alpha + 0x80) >> 8);
    }
}

/* utility to stretch a frame to the desired size */

static void
draw_frame_row (GdkPixbuf *frame_image, int target_width, int source_width, int source_v_position, int dest_v_position, GdkPixbuf *result_pixbuf, int left_offset, int height)
{
	int remaining_width, h_offset, slab_width;

	remaining_width = target_width;
	h_offset = 0;
	while (remaining_width > 0) {
		slab_width = remaining_width > source_width ? source_width : remaining_width;
		gdk_pixbuf_copy_area (frame_image, left_offset, source_v_position, slab_width, height, result_pixbuf, left_offset + h_offset, dest_v_position);
		remaining_width -= slab_width;
		h_offset += slab_width;
	}
}

/* utility to draw the middle section of the frame in a loop */
static void
draw_frame_column (GdkPixbuf *frame_image, int target_height, int source_height, int source_h_position, int dest_h_position, GdkPixbuf *result_pixbuf, int top_offset, int width)
{
	int remaining_height, v_offset, slab_height;

	remaining_height = target_height;
	v_offset = 0;
	while (remaining_height > 0) {
		slab_height = remaining_height > source_height ? source_height : remaining_height;
		gdk_pixbuf_copy_area (frame_image, source_h_position, top_offset, width, slab_height, result_pixbuf, dest_h_position, top_offset + v_offset);
		remaining_height -= slab_height;
		v_offset += slab_height;
	}
}

static GdkPixbuf *
eel_stretch_frame_image (GdkPixbuf *frame_image, int left_offset, int top_offset, int right_offset, int bottom_offset,
				int dest_width, int dest_height, gboolean fill_flag)
{
	GdkPixbuf *result_pixbuf;
	guchar *pixels_ptr;
	int frame_width, frame_height;
	int y, row_stride;
	int target_width, target_frame_width;
	int target_height, target_frame_height;

	frame_width  = gdk_pixbuf_get_width  (frame_image);
	frame_height = gdk_pixbuf_get_height (frame_image );

	if (fill_flag) {
		result_pixbuf = gdk_pixbuf_scale_simple (frame_image, dest_width, dest_height, GDK_INTERP_NEAREST);
	} else {
		result_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, dest_width, dest_height);
	}
	row_stride = gdk_pixbuf_get_rowstride (result_pixbuf);
	pixels_ptr = gdk_pixbuf_get_pixels (result_pixbuf);

	/* clear the new pixbuf */
	if (!fill_flag) {
		for (y = 0; y < dest_height; y++) {
			art_rgb_run_alpha (pixels_ptr, 255, 255, 255, 255, dest_width);
			pixels_ptr += row_stride;
		}
	}

	target_width  = dest_width - left_offset - right_offset;
	target_frame_width = frame_width - left_offset - right_offset;

	target_height  = dest_height - top_offset - bottom_offset;
	target_frame_height = frame_height - top_offset - bottom_offset;

	/* draw the left top corner  and top row */
	gdk_pixbuf_copy_area (frame_image, 0, 0, left_offset, top_offset, result_pixbuf, 0,  0);
	draw_frame_row (frame_image, target_width, target_frame_width, 0, 0, result_pixbuf, left_offset, top_offset);

	/* draw the right top corner and left column */
	gdk_pixbuf_copy_area (frame_image, frame_width - right_offset, 0, right_offset, top_offset, result_pixbuf, dest_width - right_offset,  0);
	draw_frame_column (frame_image, target_height, target_frame_height, 0, 0, result_pixbuf, top_offset, left_offset);

	/* draw the bottom right corner and bottom row */
	gdk_pixbuf_copy_area (frame_image, frame_width - right_offset, frame_height - bottom_offset, right_offset, bottom_offset, result_pixbuf, dest_width - right_offset,  dest_height - bottom_offset);
	draw_frame_row (frame_image, target_width, target_frame_width, frame_height - bottom_offset, dest_height - bottom_offset, result_pixbuf, left_offset, bottom_offset);

	/* draw the bottom left corner and the right column */
	gdk_pixbuf_copy_area (frame_image, 0, frame_height - bottom_offset, left_offset, bottom_offset, result_pixbuf, 0,  dest_height - bottom_offset);
	draw_frame_column (frame_image, target_height, target_frame_height, frame_width - right_offset, dest_width - right_offset, result_pixbuf, top_offset, right_offset);

	return result_pixbuf;
}


/* draw an arbitrary frame around an image, with the result passed back in a newly allocated pixbuf */
static GdkPixbuf *
eel_embed_image_in_frame (GdkPixbuf *source_image, GdkPixbuf *frame_image, int left_offset, int top_offset, int right_offset, int bottom_offset)
{
	GdkPixbuf *result_pixbuf;
	int source_width, source_height;
	int dest_width, dest_height;

	source_width  = gdk_pixbuf_get_width  (source_image);
	source_height = gdk_pixbuf_get_height (source_image);

	dest_width  = source_width  + left_offset + right_offset;
	dest_height = source_height + top_offset  + bottom_offset;

	result_pixbuf = eel_stretch_frame_image (frame_image, left_offset, top_offset, right_offset, bottom_offset,
						      dest_width, dest_height, FALSE);

	/* Finally, copy the source image into the framed area */
	gdk_pixbuf_copy_area (source_image, 0, 0, source_width, source_height, result_pixbuf, left_offset,  top_offset);

	return result_pixbuf;
}


Glib::RefPtr<Gdk::Pixbuf> eelEmbedImageInFrame (
	Glib::RefPtr<Gdk::Pixbuf> source,
	Glib::RefPtr<Gdk::Pixbuf> frame,
	int left, int top, int right, int bottom)
{
		return Glib::wrap (eel_embed_image_in_frame (
			source->gobj(), frame->gobj(),
			left, top, right, bottom), false);
}


/**
 * The body of this function comes from termina-screen.c in
 * gnome-terminal-2.18.0, Copyright 2001 Havoc Pennington
 */
Glib::ustring mozUrlSelectionToUTF8 (
	Gtk::SelectionData const &sel)
{
	GString *str;
	int i;
	const guint16 *char_data;
	int char_len;

	/* MOZ_URL is in UCS-2 but in format 8. BROKEN!
	 *
	 * The data contains the URL, a \n, then the
	 * title of the web page.
	 */
	if (sel.get_format() != 8 ||
			sel.get_length() == 0 ||
			(sel.get_length() % 2) != 0)
	{
		g_printerr ("Mozilla url dropped on terminal had wrong format (%d) or length (%d)\n",
				sel.get_format(),
				sel.get_length());
		return Glib::ustring();
	}

	str = g_string_new (NULL);

	char_len = sel.get_length() / 2;
	char_data = (const guint16*) sel.get_data();
	i = 0;
	while (i < char_len)
	{
		if (char_data[i] == '\n')
			break;

		g_string_append_unichar (str, (gunichar) char_data[i]);

		++i;
	}

	Glib::ustring cppString = Glib::ustring(str->str, str->len);

	g_string_free (str, TRUE);

	return cppString;
}


Glib::ustring trimWhiteSpace (Glib::ustring const &str)
{
	Glib::ustring::size_type pos1 = str.find_first_not_of(' ');
	Glib::ustring::size_type pos2 = str.find_last_not_of(' ');
	return str.substr(
			pos1 == Glib::ustring::npos ? 0 : pos1, 
			pos2 == Glib::ustring::npos ? str.length() - 1 : pos2 - pos1 + 1);
}


Glib::ustring trimLeadingString (Glib::ustring const &str, Glib::ustring const &leader)
{
	if (str.find(leader) == 0) {
		return str.substr(leader.length(), str.length() - leader.length());
	} else {
		return str;
	}
}


/*
 * Print a debug message, handling unprintable characters gracefully
 */
void debug (Glib::ustring tag, Glib::ustring msg)
{
	static Glib::ustring lastTag;

	if (tag != lastTag) {
		std::cerr << tag << ":\n";
		lastTag = tag;
	}

	if (!msg.validate()) {
		std::cerr << __FUNCTION__ << ": Invalid UTF-8  in msg\n";
		return;
	}

	std::string localeCharset;
	Glib::get_charset (localeCharset);

	std::string localised =
		Glib::convert_with_fallback (
			msg,
			localeCharset,
			"UTF-8");

	std::cerr << "\t" << localised << "\n";
}

/* [bert] Added this function to remove leading "a", "an" or "the"
 * from an English string, for comparison purposes.
 */
Glib::ustring removeLeadingArticle(Glib::ustring const &str)
{
  Glib::ustring::size_type p;

  /* See if the string starts with "A" "An", or "The". If so, remove
   * this text from the string.
   */
  p = str.find(' ');
  if (p != Glib::ustring::npos) {
    Glib::ustring firstword = str.substr(0, p);
    if (firstword.compare(_("A")) == 0 ||
	firstword.compare(_("An")) == 0 ||
	firstword.compare(_("The")) == 0) {
      return str.substr(p); // Return string with leading article removed.
    }
  }
  return str;			// Return the string unmodified by default.
}

/* 
 * Converts a Glib::TimeVal date to POSIX time
 */
time_t timeValToPosix(Glib::TimeVal time_val)
{
  Glib::ustring time_str = time_val.as_iso8601 ();
    const char* time_cstr = time_str.c_str ();

    /* We need to create a tm strcut from a iso8601 C string and create a 
   	 * timestamp from it
     */
    struct tm ctime;
    strptime(time_cstr, "%FT%T%z", &ctime);

    return mktime(&ctime);
}

}

TextDialog::TextDialog (
		Glib::ustring const &title,
		Glib::ustring const &text)
{
	Gtk::Dialog (title, true, false);

	Glib::RefPtr<Gtk::TextBuffer> buffer = Gtk::TextBuffer::create ();
	buffer->set_text (text);

	Gtk::TextView *view = Gtk::manage (new Gtk::TextView (buffer));
	view->set_wrap_mode (Gtk::WRAP_WORD_CHAR);

	Gtk::ScrolledWindow *scroll = Gtk::manage (new Gtk::ScrolledWindow ());
	scroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	scroll->add (*view);

	get_vbox()->pack_start (*scroll);
	add_button (Gtk::Stock::CLOSE, 0);

	set_size_request (400, 300);

	show_all ();
}

