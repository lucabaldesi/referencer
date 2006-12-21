
#include <stdlib.h>

#include <PDFDoc.h>
#include <goo/gtypes.h>
#include <goo/GooString.h>
#include <GlobalParams.h>
#include <TextOutputDev.h>

#include <gtkmm.h>

// For strstr
#include <string.h>

#include "BibData.h"

void guess_journal (Glib::ustring &raw, BibData *bib);

void *textfunc (void *stream, char *text, int len)
{
	Glib::ustring *str = (Glib::ustring *)stream;
	str->append(text);
}

int main (int argc, char **argv)
{
	if (argc < 2) {
		g_error ("No filename given!");
	}

	gtk_set_locale ();
	gtk_init (&argc, &argv);
	
	globalParams = new GlobalParams (NULL);
	
	GooString *filename = new GooString (argv[1]);
	PDFDoc *doc = new PDFDoc (filename, NULL, NULL);
	if (doc->isOk())
		g_message ("Loaded '%s' successfully, %d pages",
			filename->getCString (),
			doc->getNumPages ());
	else
		g_error ("Failed to load '%s'", filename->getCString());

	// Obsolete - we use poppler's getInfo instead
	/*GooString *metagoo = doc->readMetadata ();
	Glib::ustring meta;
	if (metagoo) {
		g_message ("Got metadata string.");
		meta = metagoo->getCString();	
		delete metagoo;
	} else {
		g_message ("No metadata found.");
	}*/

	int firstpage = 1;
	int lastpage = doc->getNumPages();
	GBool physLayout = gFalse;
	GBool rawOrder = gFalse;

	Glib::ustring textdump;
	TextOutputDev *output = new TextOutputDev(
		(TextOutputFunc) textfunc,
		&textdump,
		physLayout,
		rawOrder);

	if (output->isOk()) {
		g_message ("Extracting text...");
		doc->displayPages(output, firstpage, lastpage, 72, 72, 0,
			gTrue, gFalse, gFalse);
		delete output;	
	} else {
		delete output;
		g_error ("Could not create text output device");
	}

	if (!textdump.empty())
		g_message ("Got text.");
	else
		g_message ("No text found.");

	BibData *bib = new BibData();

	bib->guessYear (textdump);
	bib->guessAuthors (textdump);

	bib->guessJournal (textdump);
	bib->guessVolumeNumberPage (textdump);
	
	Glib::ustring cppdump = textdump;
	
	textdump = "";
	
	FILE *out = fopen("dump.txt", "w");
	fwrite (cppdump.c_str(), 1, strlen(cppdump.c_str()), out);
	fclose (out);

	bib->print();

	delete doc;
	
	return 0;
}

