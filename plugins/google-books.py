import gtk
import re
import referencer
from referencer import _

referencer_plugin_info = {
	"author"	:	"Phoenix87",
	"version"	:	"0.1",
	"longname"	: _("Fetch BiBTeX metadata from Google Books")
}

referencer_plugin_capabilities = ["url"]

def resolve_metadata(doc, method):
	url = doc.get_field("url")
	
	pattern = r'http://books[.]google[.](.+?)/books.+?id=([a-zA-Z0-9]+)'
	res = re.match(pattern, url)
	url = r'http://books.google.%s/books?id=%s' % (res.group(1), res.group(2))
	doc.set_field("url", url)
	
	data = referencer.download("Reading Google Books web page", "Parsing the content of the Google Books page...", url)
	pattern = r'<a class="gb-button " href="(.+?)" dir=(.+?)>BiBTeX</a>'
	res = re.findall(pattern, data)
	if res:
		bib = referencer.download("Fetching BiBTeX data", "Downloading BiBTeX metadata for the book...", res[0][0])
		doc.parse_bibtex(bib)
		pattern = r'@(.+?)[{]'
		res = re.match(pattern, bib)
		#if res:
		#	doc.set_field("ngenre", res.group(1).lower())
		return True
		
	return False
