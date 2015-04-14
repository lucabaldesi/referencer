import gtk
import re
import referencer
from referencer import _

# A referencer plugin to get bibtex info for a book from Google Books
#
# Copyright 2011 Phoenix87
# Copyright 2012 Mads Chr. Olesen <mads@mchro.dk>
# Copyright 2015 Gabriele N. Tornetta <phoenix1987@gmail.com>

referencer_plugin_info = {
    "author":   "Gabriele N. Tornetta (a.k.a. Phoenix87)",
    "version":  "1.0",
    "longname": _("Fetch BiBTeX metadata from Google Books")
}

referencer_plugin_capabilities = ["url"]

pattern = r'https?://books[.]google[.](.+?)/books.+?id=(.+)'

def can_resolve_metadata (doc):
	url = doc.get_field("url")
	if not url:
		return -1
	res = re.match(pattern, url)
	if res:
		return 90
	return -1

def resolve_metadata(doc, method):
	url = doc.get_field("url")
	
	res = re.match(pattern, url)
	url = r'https://books.google.%s/books?id=%s' % (res.group(1), res.group(2).split("&")[0])
	doc.set_field("url", url)
	
	data = referencer.download("Reading Google Books web page", "Parsing the content of the Google Books page...", url)
	
	bib = referencer.download("Fetching BiBTeX data", "Downloading BiBTeX metadata for the book...", url + "&output=bibtex")
	doc.parse_bibtex(bib)
		
	return True
