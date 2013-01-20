#!/usr/bin/env python
 
# ADS metadata scraper, Copyright 2008 John Spray

import urllib
import referencer
from referencer import _

from xml.dom import minidom

referencer_plugin_info = {
	"author":   "John Spray",
	"longname": _("NASA Astrophysics Data System DOI resolver")
	}
referencer_plugin_capabilities = ["doi"]

# Encoding: every PyUnicode that minidom gives us gets
# encoded as utf-8 into a PyString, this is what PyString_AsString on the C++
# side will expect
def get_field (doc, field):
	value = doc.getElementsByTagName(field)
	print "get_field: value = ", value
	if len(value) == 0:
		return ""
	else:
		return value[0].childNodes[0].data.encode("utf-8")

def can_resolve_metadata (doc):
    if doc.get_field("doi"):
        return 50
    return -1

def resolve_metadata (doc, method):
	if method != "doi":
		return False

	doi = doc.get_field("doi")
	params = {
		'data_type':"XML",
		'doi':doi
	}

	url = "http://adsabs.harvard.edu/cgi-bin/nph-bib_query?" + urllib.urlencode (params)
	data = referencer.download (_("Resolving DOI"), _("Fetching metadata from NASA ADS for DOI %s") % doi, url);

	if data.find ("retrieved=\"1\"") == -1:
		print "Couldn't get info from ADS"
		return False

	fields = []
	try:
		xmldoc = minidom.parseString (data)
		fields.append (["journal", get_field(xmldoc, "journal")])
		fields.append (["title",   get_field(xmldoc, "title")])
		fields.append (["volume",  get_field(xmldoc, "volume")])
		fields.append (["issue",  get_field(xmldoc, "issue")])
		fields.append (["year", get_field(xmldoc, "pubdate").partition(' ')[2]])
		fields.append (["month", str.lower(get_field(xmldoc, "pubdate").partition(' ')[0])])
		fields.append (["Adsurl", xmldoc.getElementsByTagName('url')[-1].childNodes[0].data.encode("utf-8")])

		authors = xmldoc.getElementsByTagName('author')
		authorString = ""
		first = True
		for author in authors:
			name = author.childNodes[0].data.encode("utf-8")
			if (first == False):
				authorString += " and "
			print "got author", name
			authorString += name
			first = False

		fields.append (["author", authorString])

		print "appended authors"
		pages = get_field (xmldoc, "page")
		print "getting lastPage"
		lastPage = get_field (xmldoc, "lastpage")
		if (len(lastPage) > 0):
			pages += "-"
			pages += lastPage

		print "got pages " , pages
		fields.append (["pages", pages])
		print "appended pages"
	except:
		print "exception"
		return False

	for field in fields:
		if len(field[1]) > 0:
			doc.set_field(field[0], field[1]) 

	return True

