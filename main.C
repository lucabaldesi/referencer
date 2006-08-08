
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

void *textfunc (void *stream, char *text, int len)
{
	GooString *str = (GooString *)stream;
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

	g_message ("Reading metadata...");

	GooString *meta = doc->readMetadata ();
	if (meta) {
		g_message ("Got metadata string '%s'\n", meta->getCString());
	} else {
		g_message ("No metadata");
	}

	GooString *outputfile = new GooString("output.txt");
	GBool physLayout = gFalse;
	GBool rawOrder = gFalse;
	
	int firstpage = 1;
	int lastpage = doc->getNumPages();

	GooString *textdump = new GooString("");

	TextOutputDev *output = new TextOutputDev(
		(TextOutputFunc) textfunc,
		textdump,
		physLayout,
		rawOrder);

	if (output->isOk()) {
		g_message ("Extracting text...");
		doc->displayPages(output, firstpage, lastpage, 72, 72, 0,
			gTrue, gFalse, gFalse);
		delete output;	
	} else {
		delete output;
		g_error ("Could not create text output device\n");
	}

	printf ("%s", textdump->getCString());

	delete doc;
	
	return 0;
}

void guess_journal (char *raw, BibData *bib)
{
	unsigned int const n_titles = 1;
	char search_titles[n_titles][]= {
		"PHYSICAL REVIEW B"
	};
	char bib_titles[n_titles][] = {
		"Phys. Rev. B"
	};

	for (unsigned int i = 0; i < n_titles; ++i) {
		if (strstr (raw, search_titles[i])) {
			bib->setJournal (bib_titles[i]);
			break;
		}
	}
}
