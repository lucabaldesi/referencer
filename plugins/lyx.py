#!/usr/bin/env python
 
#  Add quote in lyx using lyxclient

import os
import referencer
from referencer import _

referencer_plugin_info = []
referencer_plugin_info.append (["longname", _("Cite in LyX")])
referencer_plugin_info.append (["action", _("Cite in LyX")])
referencer_plugin_info.append (["tooltip", _("Cite the selected documents in LyX")])
referencer_plugin_info.append (["icon", "lyx.png"])
referencer_plugin_capabilities = []
referencer_plugin_capabilities.append ("document_action")

print "lyx module loaded"

LYXPIPE = None
HOME = os.environ.get("HOME")
LYXPREFS = os.path.join(HOME, ".lyx/preferences")
DEFAULTPIPE = os.path.join(HOME, ".lyxpipe.in")

# read the path to the lyx pipe from the lyx configuration file
if os.path.exists(LYXPREFS):
	p = open(LYXPREFS, "r")
	while True:
		l = p.readline()
		if not l:
			break
		if l.startswith("\\serverpipe "):
			LYXPIPE = l.split('"')[1] + ".in"
			break
p.close()

# Locate lyxclient
lyxClientBinary = None
for dir in os.environ['PATH'].split (os.pathsep):
	exe = os.path.join (dir, "lyxclient")
	if (os.path.exists(exe)):
		lyxClientBinary = exe
		print "Found lyxclient at %s\n" % exe
		break


def do_action (documents):
	empty = True
	keys = ""
	for document in documents:
		if not empty:
			keys += ","
		keys += document.get_key()
		empty = False

	if empty:
		return False

	existing_lyxpipe = None
	if LYXPIPE and os.path.exists(LYXPIPE):
		existing_lyxpipe = LYXPIPE
	elif os.path.exists(DEFAULTPIPE):
		existing_lyxpipe = DEFAULTPIPE

	if existing_lyxpipe is not None:	
		try:
			print "using lyx pipe: "+existing_lyxpipe
			p = open(existing_lyxpipe, 'a')
			p.write("LYXCMD:referencer:citation-insert:" + keys + "\n")
			p.close()
			return True
		except:
			print "Error using the lyx pipe"

	if lyxClientBinary is None:
		print "Couldn't find lyxclient"
		return False

	# Compose citation insertion command
	cmdStr = "LYXCMD:citation-insert " + keys + "\n"
	try:
		os.system ("%s -c \"%s\"" % (lyxClientBinary, cmdStr))
	except:
		return False

	return True

