#!/usr/bin/env python
 
#  Add quote in lyx using lyxclient

import os
import referencer
from referencer import _

referencer_plugin_info = [
	["longname", _("Cite in LyX")]
]

referencer_plugin_info.append (["ui",
		"""
		<ui>
			<toolbar name='ToolBar'>
			<placeholder name='ToolBarPluginActions'>
				<toolitem action='_plugin_lyx_cite'/>
			</placeholder>
			</toolbar>
		</ui>
		"""])


referencer_plugin_actions = [
	{
	"name":"_plugin_lyx_cite",
	"label":_("Cite in LyX"),
	"tooltip":_("Cite the selected documents in LyX"),
	"icon":"lyx.png",
	"callback":"do_cite",
	"accelerator":"<control><shift>d"
	}
]

LYXPIPE = None
HOME = os.environ.get("HOME")
LYXPREFS = os.path.join(HOME, ".lyx/preferences")
DEFAULTPIPE = os.path.join(HOME, ".lyxpipe.in")

# read the path to the lyx pipe from the lyx configuration file
if os.path.exists(LYXPREFS):
	try:
		p = open(LYXPREFS, "r")
		while True:
			l = p.readline()
			if not l:
				break
			if l.startswith("\\serverpipe "):
				LYXPIPE = l.split('"')[1] + ".in"
				break
		p.close()
	except:
		# Fall through to lyxclient
		pass

# Locate lyxclient
lyxClientBinary = None
for dir in os.environ['PATH'].split (os.pathsep):
	exe = os.path.join (dir, "lyxclient")
	if (os.path.exists(exe)):
		lyxClientBinary = exe
		print "Found lyxclient at %s\n" % exe
		break


def do_cite (library, documents):
	empty = True
	keys = ""
	for document in documents:
		if not empty:
			keys += ","
		keys += document.get_key()
		empty = False

	if empty:
		raise "No keys"
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
	elif lyxClientBinary is not None:
		# Compose citation insertion command
		cmdStr = "LYXCMD:citation-insert " + keys + "\n"
		try:
			retval = os.system ("%s -c \"%s\"" % (lyxClientBinary, cmdStr))
		except:
			return False

		if retval != 0:
			raise "LyXClient failed, is LyX running?"
	else:
		raise "Couldn't find lyxclient"
		#return False

	return True

