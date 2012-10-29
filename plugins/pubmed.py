#!/usr/bin/env python
 
#   Simple script to query pubmed for a DOI
#   (c) Simon Greenhill, 2007
#   http://simon.net.nz/

#   Modified for integration with referencer by John Spray, 2007

import urllib
import referencer
from referencer import _

from xml.dom import minidom

referencer_plugin_info = {
	"longname":  _("PubMed DOI resolver"),
	"author":      "Simon Greenhill, John Spray"
	}

referencer_plugin_capabilities = ["doi", "pubmed"]




# Encoding: every PyUnicode that minidom gives us gets
# encoded as utf-8 into a PyString, this is what PyString_AsString on the C++
# side will expect
def get_field (doc, field):
	value = doc.getElementsByTagName(field)
	if len(value) == 0:
		return ""
	else:
		if (len(value[0].childNodes) == 0):
			return ""
		else:
			return value[0].childNodes[0].data.encode("utf-8")


def text_output(xml):
	"""Makes a simple text output from the XML returned from efetch"""
	 
	print "pubmed.text_output: calling parseString on ", len(xml) , " characters"
	print "pubmed.text_output: calling parseString on ", xml
	xmldoc = minidom.parseString(xml)
	print "pubmed.text_output: made it out of parseString"

	if len(xmldoc.getElementsByTagName("PubmedArticle")) == 0:
		raise "pubmed.text_output: PubmedArticle not found"

	output = []

	pmid = get_field (xmldoc, "PMID")
	output.append (["pmid", pmid])
	articleidlist = xmldoc.getElementsByTagName('ArticleIdList')
	if (len(articleidlist) > 0):
		articleids = articleidlist[0].getElementsByTagName('ArticleId')
	for articleid in articleids:
		idtype = articleid.attributes.get("IdType").value
		if "doi" == idtype and len(articleid.childNodes) != 0:
			output.append (["doi", articleid.childNodes[0].data.encode("utf-8")])
	 
	title = get_field (xmldoc, "ArticleTitle")
	output.append (["title", title])
	abstract = get_field (xmldoc, "AbstractText")

	authors = xmldoc.getElementsByTagName('AuthorList')[0]
	authors = authors.getElementsByTagName('Author')
	authorlist = []
	for author in authors:
		LastName = get_field (author, "LastName")
		ForeName = get_field (author, "ForeName")
		if ForeName == None or ForeName == "":
			print "pubmed.text_output: Fallback on initials"
			ForeName = get_field (author, "Initials")
		author = '%s, %s' % (LastName, ForeName)
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


def referencer_search_TEST (search_text):
	email='referencer@icculus.org'
   	tool='Referencer'
	database='pubmed'

	retmax = 100

	params = {
		'db':database,
		'tool':tool,
		'email':email,
		'term':search_text,
		'usehistory':'y',
		'retmax':retmax
	}

	# try to resolve the PubMed ID of the DOI
	url = 'http://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi?' + urllib.urlencode(params)
	data = referencer.download (_("Searching pubmed"), _("Searching pubmed for '%s'") % search_text , url);

	# parse XML output from PubMed...
	print data
	xmldoc = minidom.parseString(data)
	ids = xmldoc.getElementsByTagName('Id')

	# nothing found, exit
	# FIXME: not really an error
	if len(ids) == 0:
		raise "pubmed.referencer_search: no results"

	webenv = xmldoc.getElementsByTagName('WebEnv')
	if len(webenv) == 0:
		raise "pubmed.referencer_search: no webenv"
	webenv = webenv[0].childNodes[0].data

	query_key = xmldoc.getElementsByTagName('QueryKey')
	if len(query_key) == 0:
		raise "pubmed.referencer_search: no query_key"
	query_key = query_key[0].childNodes[0].data

	params = {
		'db':database,
		'tool':tool,
		'email':email,
		'webenv':webenv,
		'query_key':query_key,
		'retmax':retmax
	}
	url = 'http://eutils.ncbi.nlm.nih.gov/entrez/eutils/esummary.fcgi?' + urllib.urlencode(params)
	data = referencer.download (_("Retrieving pubmed summaries"), _("Retrieving summaries for '%s'") % search_text , url);

	xmldoc = minidom.parseString(data)

	results = []
	for docsum in xmldoc.getElementsByTagName('DocSum'):
		title = ""
		author = ""
		pmid = ""
		id = docsum.getElementsByTagName("Id")
		if len(id) !=0:
			pmid = id[0].childNodes[0].data
		else:
			raise "pubmed.referencer_search: docsum without id"

		for childnode in docsum.getElementsByTagName("Item"):
			if childnode.getAttribute("Name") == "Title":
				title = childnode.childNodes[0].data
			if childnode.getAttribute("Name") == "Author":
				author = childnode.childNodes[0].data

		results.append ({"token":pmid,"title":title,"author":author})

	print results

	return results

def referencer_search_result_TEST (token):
	data = get_citation_from_pmid(token)
	fields = text_output(data)

	print "referencer_search_result: token = ", token
	print "referencer_search_result: fields = ", fields

	dict = {}
	for field in fields:
		dict[field[0]] = field[1]

	return dict


def get_citation_from_doi(query, email='referencer@icculus.org', tool='Referencer', database='pubmed'):
	params = {
		'db':database,
		'tool':tool,
		'email':email,
		'term':query + "[doi]",
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
		raise "pubmed.get_citation_from_doi: DOI not found"

	# get ID
	id = ids[0].childNodes[0].data

	print "pubmed.get_citation_from_doi: DOI ", query, " has PubMed ID ", id

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

def can_resolve_metadata (doc):
    if doc.get_field ("pmid"):
        return 90
    elif doc.get_field ("doi"):
        return 50
    return -1

def resolve_metadata (doc, method):
	try:
		if (method == "doi"):
			xml = get_citation_from_doi (doc.get_field ("doi"))
		elif (method == "pubmed"):
			xml = get_citation_from_pmid (doc.get_field ("pmid"))
	except:
		print "pubmed.resolve_metadata: Got no metadata"
		# Couldn't get any metadata
		return False

	try:
		items = text_output (xml)
	except:
		print "pubmed.resolve_metadata: Couldn't parse metadata"
		# Couldn't parse XML
		return False

	itemCount = 0
	for item in items:
		print "pubmed.resolve_metadata: Setting %s:%s\n" % (item[0], item[1])
		if (len(item[1]) > 0):
			doc.set_field (item[0], item[1])
			itemCount = itemCount + 1
	return itemCount > 0

