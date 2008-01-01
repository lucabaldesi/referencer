#!/usr/bin/env python
 
#   Simple script to query pubmed for a DOI
#   (c) Simon Greenhill, 2007
#   http://simon.net.nz/

#   Modified for integration with referencer by John Spray, 2007

import urllib
import urllib2
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
		raise "DoiNotFound"

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


def text_output(xml):
	"""Makes a simple text output from the XML returned from efetch"""
	 
	print "calling parseString on ", len(xml) , " characters"
	xmldoc = minidom.parseString(xml)
	print "made it out of parseString"

	# Encoding: every PyUnicode that minidom gives us gets
	# cast into a PyString, this is what PyString_AsString on the C++
	# side will expect

	pmid = xmldoc.getElementsByTagName('PMID')[0].childNodes[0].data.encode("utf-8")
	 
	title = xmldoc.getElementsByTagName('ArticleTitle')[0]
	title = title.childNodes[0].data.encode("utf-8")
	 
	abstract = xmldoc.getElementsByTagName('AbstractText')[0]
	abstract = abstract.childNodes[0].data.encode("utf-8")
	 
	authors = xmldoc.getElementsByTagName('AuthorList')[0]
	authors = authors.getElementsByTagName('Author')
	authorlist = []
	for author in authors:
		LastName = author.getElementsByTagName('LastName')[0].childNodes[0].data.encode("utf-8")
		Initials = author.getElementsByTagName('Initials')[0].childNodes[0].data.encode("utf-8")
		author = '%s, %s' % (LastName, Initials)
		authorlist.append(author)
	 
	journalinfo = xmldoc.getElementsByTagName('Journal')[0]
	journal = journalinfo.getElementsByTagName('Title')[0].childNodes[0].data.encode("utf-8")
	journalinfo = journalinfo.getElementsByTagName('JournalIssue')[0]
	volume = journalinfo.getElementsByTagName('Volume')[0].childNodes[0].data.encode("utf-8")
	issue = journalinfo.getElementsByTagName('Issue')[0].childNodes[0].data.encode("utf-8")
	year = journalinfo.getElementsByTagName('Year')[0].childNodes[0].data.encode("utf-8")
	 
	# this is a bit odd?
	pages = xmldoc.getElementsByTagName('MedlinePgn')[0].childNodes[0].data.encode("utf-8")
	 
	output = []
	if (len(pmid) > 0):
		output.append (["pmid", pmid])
	output.append (["title", title])
	output.append (["journal", journal])
	output.append (["volume", volume])
	output.append (["number", issue])
	output.append (["year", year])
	output.append (["pages", pages])
	authorstring = ""
	for author in authorlist:
		if (len(authorstring) > 0):
			authorstring += " and "
		authorstring += author
	if (len(authorstring) > 0):
		output.append (["author", authorstring])
	return output

def resolve_metadata (code, type):
	if (type == "doi"):
		print "pubmed.py: resolving doi ", code
		xml = get_citation_from_doi (code)
		return text_output(xml)
	if (type == "pubmed"):
		print "pubmed.py: resolving pmid ", code
		xml = get_citation_from_pmid (code)
		return text_output(xml)
	else:
		return []
