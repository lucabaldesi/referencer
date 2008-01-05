#!/usr/bin/env python
 
#  Add quote in lyx using lyxpipe

import os
import referencer
from referencer import _

referencer_plugin_info = []
referencer_plugin_info.append (["longname", _("Cite in LyX")])
referencer_plugin_info.append (["action", _("Cite in LyX")])
referencer_plugin_info.append (["tooltip", _("Cite the selected documents in LyX")])
referencer_plugin_capabilities = []
referencer_plugin_capabilities.append ("document_action")

# assign it a value to override autodetection
LYXPIPE = None
HOME = os.environ.get("HOME")
LYXPREFS = os.path.join(HOME, ".lyx/preferences")

print "lyx module loaded"

# read the path to the lyx pipe from the lyx configuration file
if not LYXPIPE and os.path.exists(LYXPREFS):
	p = open(LYXPREFS, "r")
	while True:
		l = p.readline()
		if not l:
			break

		print l
		if l.startswith("\\serverpipe "):
			LYXPIPE = l.split('"')[1] + ".in"
			break
	p.close()

def do_action (documents):
	empty = True
	s = ""
	for document in documents:
		if not empty:
			s += ","
		s += document.get_key()
		empty = False

	print "do_action: got key list ", s

	if not LYXPIPE:
		print "no pipe defined. Is lyx configured ?"
		return False
	if not os.path.exists(LYXPIPE):
		print "pipe does not exist: "+LYXPIPE + ". Does lyx run ?"
		return False

	if empty:
		return False

	try:
		p = open(LYXPIPE, 'a')
		p.write("LYXCMD:referencer:citation-insert:" + s + "\n")
		p.close()
	except:
		return False

