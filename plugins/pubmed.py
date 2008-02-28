#!/usr/bin/env python
 
#   Simple script to query pubmed for a DOI
#   (c) Simon Greenhill, 2007
#   http://simon.net.nz/

#   Modified for integration with referencer by John Spray, 2007

import urllib
import referencer
from referencer import _

from xml.dom import minidom

referencer_plugin_info = []
referencer_plugin_info.append (["longname", _("PubMed DOI resolver")])
referencer_plugin_capabilities = []
referencer_plugin_capabilities.append ("doi")
referencer_plugin_capabilities.append ("pubmed")


def get_citation_from_doi(query, email='referencer@icculus.org', tool='Referencer', database='pubmed'):
	params = {
		'db':database,
		'tool':tool,
		'email':email,
		'term':query,
		'usehistory':'y',
		'retmax':1
	}

	# try to resolve the PubMed ID of the DOI
	url = 'http://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi?' + urllib.urlencode(params)
	data = referencer.download (_("Resolving DOI"), _("Finding PubMed ID from DOI %s") % query , url);

	# parse XML output from PubMed...
	xmldoc = minidom.parseString(data)
	ids = xmldoc.getElementsByTagName('Id')

	# nothing found, exit
	if len(ids) == 0:
		raise "DOI not found"

	# get ID
	id = ids[0].childNodes[0].data

	print "DOI ", query, " has PubMed ID ", id

	return get_citation_from_pmid (id)
 
def get_citation_from_pmid (pmid, email='referencer@icculus.org', tool='Referencer', database='pubmed'):
	params = {
		'db':database,
		'tool':tool,
		'email':email,
		'id':pmid,
		'retmode':'xml'
	}

	# get citation info:
	url = 'http://eutils.ncbi.nlm.nih.gov/entrez/eutils/efetch.fcgi?' + urllib.urlencode(params)
	data = referencer.download (_("Resolving PubMed ID"), _("Fetching metadata from NCBI for PubMed ID %s") % pmid, url);

	return data


# Encoding: every PyUnicode that minidom gives us gets
# encoded as utf-8 into a PyString, this is what PyString_AsString on the C++
# side will expect
def get_field (doc, field):
	value = doc.getElementsByTagName(field)
	print "get_field: value = ", value
	if len(value) == 0:
		return ""
	else:
		if (len(value[0].childNodes) == 0):
			return ""
		else:
			return value[0].childNodes[0].data.encode("utf-8")


def text_output(xml):
	"""Makes a simple text output from the XML returned from efetch"""
	 
	print "calling parseString on ", len(xml) , " characters"
	print "calling parseString on ", xml
	xmldoc = minidom.parseString(xml)
	print "made it out of parseString"

	if len(xmldoc.getElementsByTagName("PubmedArticle")) == 0:
		raise "PubmedArticle not found"

	output = []

	pmid = get_field (xmldoc, "PMID")
	output.append (["pmid", pmid])
	title = get_field (xmldoc, "ArticleTitle")
	output.append (["title", title])
	abstract = get_field (xmldoc, "AbstractText")

	authors = xmldoc.getElementsByTagName('AuthorList')[0]
	authors = authors.getElementsByTagName('Author')
	authorlist = []
	for author in authors:
		LastName = get_field (author, "LastName")
		Initials = get_field (author, "Initials")
		author = '%s, %s' % (LastName, Initials)
		authorlist.append(author)
	 
	authorstring = ""
	for author in authorlist:
		if (len(authorstring) > 0):
			authorstring += " and "
		authorstring += author
	if (len(authorstring) > 0):
		output.append (["author", authorstring])

	journalinfo = xmldoc.getElementsByTagName('Journal')
	if len(journalinfo) > 0:
		journal = get_field (journalinfo[0], "ISOAbbreviation")
		if len(journal) == 0:
			journal = get_field (journalinfo[0], "Title")
		output.append (["journal", journal])

		journalinfo = journalinfo[0].getElementsByTagName('JournalIssue')
		if len(journalinfo) > 0:
			volume = get_field (journalinfo[0], "Volume")
			issue = get_field (journalinfo[0], "Issue")
			year = get_field (journalinfo[0], "Year")
			output.append (["volume", volume])
			output.append (["number", issue])
			output.append (["year", year])
	
	pages = get_field (xmldoc, "MedlinePgn")
	output.append (["pages", pages])

	output2 = []
	for pair in output:
		if len(pair[1]) > 0:
			output2.append(pair)

	return output2

def resolve_metadata (doc, method):
	try:
		if (method == "doi"):
			xml = get_citation_from_doi (doc.get_field ("doi"))
		elif (method == "pubmed"):
			xml = get_citation_from_pmid (doc.get_field ("pmid"))
	except:
		print "pubmed.py:resolve_metadata: Got no metadata"
		# Couldn't get any metadata
		return False

	try:
		items = text_output (xml)
	except:
		print "pubmed.py:resolve_metadata: Couldn't parse metadata"
		# Couldn't parse XML
		return False

	itemCount = 0
	for item in items:
		print "pubmed: Setting %s:%s\n" % (item[0], item[1])
		if (len(item[1]) > 0):
			doc.set_field (item[0], item[1])
			itemCount = itemCount + 1
	return itemCount > 0

