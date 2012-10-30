import gtk
import re
import referencer
from referencer import _

# A referencer plugin to get bibtex info for a book from Google Books
#
# Copyright 2011 Phoenix87
# Copyright 2012 Mads Chr. Olesen <mads@mchro.dk>

referencer_plugin_info = {
    "author":    "Phoenix87",
    "version":    "0.2",
    "longname": _("Fetch BiBTeX metadata from Google Books")
}

referencer_plugin_capabilities = ["url"]

pattern = r'http://books[.]google[.](.+?)/books.+?id=([a-zA-Z0-9_]+)'

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
    url = r'http://books.google.%s/books?id=%s' % (res.group(1), res.group(2))
    doc.set_field("url", url)

    bibtex_url = r'http://books.google.%s/books?id=%s&output=bibtex' % (res.group(1), res.group(2))
    #print "url: ", repr(bibtex_url)
    bib = referencer.download("Fetching BiBTeX data", "Downloading BiBTeX metadata for the book...", bibtex_url)
    #print "bib:", repr(bib)
    doc.parse_bibtex(bib)

    doc.set_type("book")
    return True
