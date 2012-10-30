#!/usr/bin/env python

# A referencer plugin to get bibtex info for a document from DBLP
# http://dblp.org - The DBLP Computer Science Bibliography
#
# Copyright 2012 Mads Chr. Olesen <mads@mchro.dk>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import urllib
try:
    import referencer
    from referencer import _
except:
    #for doctesting
    referencer = None
    _ = lambda x: x

from xml.dom import minidom

referencer_plugin_info = {
	"longname":  _("DBLP resolver"),
	"author":      "Mads Chr. Olesen <mads@mchro.dk>"
	}

referencer_plugin_capabilities = ["resolve_metadata"]

def can_resolve_metadata (doc):
    author = doc.get_field ("author")
    #let's pretend this is an easter egg ;-)
    if author and "Mads" in author and "Olesen" in author:
        return 80
    elif "DBLP" in str(doc.get_field("bibsource")):
        return 80
    elif doc.get_field ("title"):
        return 20
    elif author and doc.get_field("year"):
        #this can be searched for, but will result in too many hits for now
        #could be re-enabled when some GUI comes in
        return -1
    return -1

def get_first_author (authors):
    if not authors:
        return ""
    if authors.find(" and") > 0:
        return authors[:authors.find(" and")]
    else:
        return authors

def parse_hits_get_urls(data):
    """
>>> data = '<?xml version="1.0" encoding="UTF-8"?>\\r\\n<result>\\r\\n<query id="74501">Chip and PIN is Broken 2010</query>\\r\\n<status code="12">OK</status>\\r\\n<time unit="msecs">52.60</time>\\r\\n<completions total="1" computed="1" sent="0">\\r\\n</completions>\\r\\n<suggestions computed="0" sent="0">\\r\\n</suggestions>\\r\\n<hits total="1" computed="1" sent="1" first="0">\\r\\n<hit score="14" id="1597245">\\r\\n<title><![CDATA[<dblp:authors><dblp:author>Steven J. Murdoch</dblp:author><dblp:author>Saar Drimer</dblp:author><dblp:author>Ross J. Anderson</dblp:author><dblp:author>Mike Bond</dblp:author></dblp:authors><dblp:title ee="http://doi.ieeecomputersociety.org/10.1109/SP.2010.33">Chip and PIN is Broken. </dblp:title><dblp:venue url="db/conf/sp/sp2010.html#MurdochDAB10" conference="IEEE Symposium on Security and Privacy" year="2010" pages="433-446">IEEE Symposium on Security and Privacy 2010:433-446</dblp:venue><dblp:year>2010</dblp:year><dblp:type>inproceedings</dblp:type>]]></title>\\r\\n<url>http://www.dblp.org/rec/bibtex/conf/sp/MurdochDAB10</url>\\r\\n</hit>\\r\\n</hits>\\r\\n</result>\\r\\n'
>>> parse_hits_get_urls(data)
[u'http://www.dblp.org/rec/bibtex/conf/sp/MurdochDAB10']
"""
    urls = []
    xmldoc = minidom.parseString(data)
    xmlhits = xmldoc.getElementsByTagName("hit")

    for xmlhit in xmlhits:
        xmlurls = xmlhit.getElementsByTagName("url")
        if len(xmlurls) == 1:
            urls += [xmlurls[0].childNodes[0].nodeValue]
    return urls

def get_bibtex_xml_from_url(url):
    url = url + ".xml"
    data = referencer.download (_("Searching DBLP"), _("Fetching metadata from DBLP for url '%s'") % url, url);
    return data

def bibtex_xml_to_bibtex(data):
    """
>>> data = '<?xml version="1.0"?>\\n<dblp>\\n<inproceedings key="conf/sp/MurdochDAB10" mdate="2010-07-13">\\n<author>Steven J. Murdoch</author>\\n<author>Saar Drimer</author>\\n<author>Ross J. Anderson</author>\\n<author>Mike Bond</author>\\n<title>Chip and PIN is Broken.</title>\\n<pages>433-446</pages>\\n<year>2010</year>\\n<booktitle>IEEE Symposium on Security and Privacy</booktitle>\\n<ee>http://doi.ieeecomputersociety.org/10.1109/SP.2010.33</ee>\\n<crossref>conf/sp/2010</crossref>\\n<url>db/conf/sp/sp2010.html#MurdochDAB10</url>\\n</inproceedings>\\n</dblp>\\n'
>>> bibtex_xml_to_bibtex(data)
u'@inproceedings{conf/sp/MurdochDAB10,\\nauthor = {Steven J. Murdoch and Saar Drimer and Ross J. Anderson and Mike Bond},\\ntitle = {Chip and PIN is Broken.},\\nbooktitle = {IEEE Symposium on Security and Privacy},\\nyear = {2010},\\npages = {433-446},\\nee = {http://doi.ieeecomputersociety.org/10.1109/SP.2010.33},\\nbibsource = {DBLP, http://dblp.uni-trier.de},\\nurl = {http://dblp.org/db/conf/sp/sp2010.html#MurdochDAB10},\\n}'
"""
    keys = ["author", "title", "booktitle", "year", "pages", "ee", "bibsource", "url"]
    entry = {}
    xmldoc = minidom.parseString(data)

    for k in keys:
        try:
            entry[k] = xmldoc.getElementsByTagName(k)[0].childNodes[0].nodeValue
        except:
            entry[k] = ""

    authors = []
    for a in xmldoc.getElementsByTagName("author"):
        authors += [a.childNodes[0].nodeValue]
    entry["author"] = " and ".join(authors)

    entry["bibsource"] = "DBLP, http://dblp.uni-trier.de"
    entry["url"] = "http://dblp.org/" + entry["url"]
    if entry["title"][-1] == ".":
        entry["title"] = entry["title"][:-1]

    xmlentry = [x for x in xmldoc.getElementsByTagName("dblp")[0].childNodes if x.nodeType == x.ELEMENT_NODE][0]
    doctype = xmlentry.tagName
    key = xmlentry.getAttribute("key")

    toret = ["@%s{%s," % (doctype, key)]
    for k in keys:
        toret += ["%s = {%s}," % (k, entry[k])]
    toret += ["}"]
    return "\n".join(toret)

def resolve_metadata (doc, method=None):
    # try with title, otherwise try with author + year
    title = doc.get_field("title")
    if title:
        searchTerms = [title]
    else:
        searchTerms = [get_first_author(doc.get_field("author"))]
        searchTerms += [doc.get_field("year")]

    searchTerm = " ".join(searchTerms)
    for c in "(),.{}!\"':=#%$/&[]+":
        searchTerm = searchTerm.replace(c, "")
    while searchTerm.find("  ") > 0: #remove double spaces
        searchTerm = searchTerm.replace("  ", " ")
    
    #print "DBLP:searchTerm:", repr(searchTerm)

    url = "http://www.dblp.org/search/api/?%s&h=1000&c=0&f=0&format=xml" % (urllib.urlencode({'q': searchTerm}))
    print "DBLP:url:", repr(url)
    data = referencer.download (_("Searching DBLP"), _("Fetching metadata from DBLP for search query '%s'") % searchTerm, url);

    hits = parse_hits_get_urls(data)

    if len(hits) != 1:
        #XXX, display UI?
        print "DBLP: Not exactly one hit, giving up"
        return False

    print "DBLP:hits:", hits

    bibtex_xml = get_bibtex_xml_from_url(hits[0])
    #print bibtex_xml
    bibtex = bibtex_xml_to_bibtex(bibtex_xml)
    #print bibtex
    doc.parse_bibtex(bibtex)
    return True
