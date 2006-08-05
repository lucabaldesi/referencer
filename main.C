
#include <stdlib.h>

#include <PDFDoc.h>
#include <goo/gtypes.h>
#include <goo/GooString.h>
#include <GlobalParams.h>
#include <TextOutputDev.h>
// ***
#include <PSOutputDev.h>

#include <gtk/gtk.h>

void *textfunc (void *stream, char *text, int len)
{
	printf ("%d\n", len);
}

int main (int argc, char **argv)
{
	if (argc < 2) {
		g_error ("No filename given!");
	}

	gtk_set_locale ();
	gtk_init (&argc, &argv);
	
	GlobalParams *params = new GlobalParams (NULL);
	
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

	GooString *outputfile = new GooString("output.text");
	GBool physLayout = gFalse;
	GBool rawOrder = gFalse;
	GBool append = gFalse;
	
	int firstpage = 1;
	int lastpage = doc->getNumPages();

	TextOutputDev *output = new TextOutputDev(
		outputfile->getCString(),
		physLayout,
		rawOrder,
		append);

	/*TextOutputDev *output = new TextOutputDev(
		(TextOutputFunc) textfunc,
		NULL,
		physLayout,
		rawOrder);*/

	/*PSOutputDev *output = new PSOutputDev(
		"output.ps", doc->getXRef(), doc->getCatalog(),
		firstpage, lastpage, psModePS);*/

	if (output->isOk()) {
		g_message ("Extracting text...");
		doc->displayPages(output, firstpage, lastpage, 72, 72, 0,
			gTrue, gFalse, gFalse);
		delete output;	
	} else {
		delete output;
		g_error ("Could not create text output device\n");
	}

	delete doc;
	
	return 0;
}
