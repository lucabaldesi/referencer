#!/usr/bin/env python
 
#   Simple script to query pubmed for a DOI
#   (c) Simon Greenhill, 2007
#   http://simon.net.nz/

import urllib
from xml.dom import minidom

def get_citation_from_doi(query, email='YOUR EMAIL GOES HERE', tool='SimonsPythonQuery', database='pubmed'):
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
	data = urllib.urlopen(url).read()

	# parse XML output from PubMed...
	xmldoc = minidom.parseString(data)
	ids = xmldoc.getElementsByTagName('Id')

	# nothing found, exit
	if len(ids) == 0:
		raise "DoiNotFound"

	# get ID
	id = ids[0].childNodes[0].data

	# remove unwanted parameters
	params.pop('term')
	params.pop('usehistory')
	params.pop('retmax')
	# and add new ones...
	params['id'] = id

	params['retmode'] = 'xml'

	# get citation info:
	url = 'http://eutils.ncbi.nlm.nih.gov/entrez/eutils/efetch.fcgi?' + urllib.urlencode(params)
	data = urllib.urlopen(url).read()

	return data
 
def text_output(xml):
	"""Makes a simple text output from the XML returned from efetch"""
	 
	xmldoc = minidom.parseString(xml)
	 
	title = xmldoc.getElementsByTagName('ArticleTitle')[0]
	title = title.childNodes[0].data
	 
	abstract = xmldoc.getElementsByTagName('AbstractText')[0]
	abstract = abstract.childNodes[0].data
	 
	authors = xmldoc.getElementsByTagName('AuthorList')[0]
	authors = authors.getElementsByTagName('Author')
	authorlist = []
	for author in authors:
		LastName = author.getElementsByTagName('LastName')[0].childNodes[0].data
		Initials = author.getElementsByTagName('Initials')[0].childNodes[0].data
		author = '%s, %s' % (LastName, Initials)
		authorlist.append(author)
	 
	journalinfo = xmldoc.getElementsByTagName('Journal')[0]
	journal = journalinfo.getElementsByTagName('Title')[0].childNodes[0].data
	journalinfo = journalinfo.getElementsByTagName('JournalIssue')[0]
	volume = journalinfo.getElementsByTagName('Volume')[0].childNodes[0].data
	issue = journalinfo.getElementsByTagName('Issue')[0].childNodes[0].data
	year = journalinfo.getElementsByTagName('Year')[0].childNodes[0].data
	 
	# this is a bit odd?
	pages = xmldoc.getElementsByTagName('MedlinePgn')[0].childNodes[0].data
	 
	output = []
	output.append (["title", title])
	output.append (["journal", journal])
	output.append (["volume", volume])
	output.append (["issue", issue])
	output.append (["year", year])
	output.append (["pages", pages])
	authorstring = ""
	for author in authorlist:
		if (len(authorstring) > 0):
			authorstring += " and "
		authorstring += author
	if (len(authorstring) > 0):
		output.append (["authors", authorstring])
	return output

def metadata_from_doi (doi):
	xml = get_citation_from_doi (doi)
	return text_output(xml)
