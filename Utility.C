
#include <iostream>

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
  GList *list = gnome_vfs_uri_list_parse (listtxt);
	std::vector<Glib::RefPtr<Gnome::Vfs::Uri> > uris;

  for (GList *p = list; p != NULL; p = g_list_next (p)) {
		Glib::RefPtr<Gnome::Vfs::Uri> uri =
			Glib::wrap ((GnomeVFSURI*)(p->data), true);
		uris.push_back (uri);

    p = g_list_next (p);
  }

  gnome_vfs_uri_list_free (list);

	return uris;
}


Glib::ustring escapeBibtexAccents (
	Glib::ustring target)
{
	/*static Glib::ustring replaceables;
	if (replaceables.length() == 0) {
		replaceables = "äöüÄÖÜß";
	}

	static std::vector<Glib::ustring> replacements;
	if (replacements.size() == 0) {
		replacements.push_back ("\"a");
		replacements.push_back ("\"o");
		replacements.push_back ("\"u");
		replacements.push_back ("\"A");
		replacements.push_back ("\"O");
		replacements.push_back ("\"U");
		replacements.push_back ("\"s");
	}*/

	for (int i = 0; i < target.length(); ++i) {
		gunichar letter = target[i];
		if (letter < 128) {
			continue;
		} else {
			/*int pos = replaceables.find (letter);
			if (pos != Glib::ustring::npos) {
				target.erase (i, 1);
				target.insert (i, replacements[pos]);
			}*/
			Glib::ustring replacement;
			int gotone = wvConvertUnicodeToLaTeX (letter, replacement);
			if (gotone) {
				target.erase (i, 1);
				target.insert (i, replacement);
			}
		}
		
		std::cerr << letter << "\n";
	}

	std::cerr << "Final target = " << target << "\n";

	return target;
}


/* w00t, copied and pasted from AbiWord */

#undef printf
#define printf(x) out = (x); 

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
		/* Fix up these as math characters: */
		case 0xb1:
			printf("$\\pm$");
			return(1);
		case 0xb2:
			printf("$\\mathtwosuperior$");
			return(1);
		case 0xb3:
			printf("$\\maththreesuperior$");
			return(1);
		case 0xb5:
			printf("$\\mu$");
			return(1);
		case 0xb9:
			printf("$\\mathonesuperior$");
			return(1);
		case 0xdc:
			printf("\\ldots{}");
			return(1);
		}
		return(0);
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


}
