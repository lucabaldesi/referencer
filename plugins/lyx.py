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

	# Locate lyxclient
	lyxClientBinary = ""
	for dir in os.environ['PATH'].split (os.pathsep):
		exe = os.path.join (dir, "lyxclient")
		if (os.path.exists(exe)):
			lyxClientBinary = exe
			print "Found lyxclient at %s\n" % exe
			break

	if (len(lyxClientBinary) == 0):
		print "Couldn't find lyxclient"
		return False

	# Compose citation insertion command
	cmdStr = "LYXCMD:citation-insert " + keys + "\n"
	try:
		os.system ("%s -c \"%s\"" % (lyxClientBinary, cmdStr))
	except:
		return False

	return True

