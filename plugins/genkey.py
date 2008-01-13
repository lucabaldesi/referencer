#!/usr/bin/env python
 
#  Generate Bob08 Alice99 Alice99b type keys

import os
import referencer
from referencer import _

referencer_plugin_info = []
referencer_plugin_info.append (["longname", _("Generate keys from metadata")])
referencer_plugin_info.append (["action", _("Generate Key")])
referencer_plugin_info.append (["tooltip", _("Generate keys for the selected documents from their metadata")])
referencer_plugin_capabilities = []
referencer_plugin_capabilities.append ("document_action")

def do_action (documents):
	empty = True
	s = ""
	assigned_keys = {}
	for document in documents:
		author = document.get_field ("author")
		author = author.split("and")[0].split(",")[0].split(" ")[0]
		year = document.get_field ("year")
		if len(year) == 4:
			year = year[2:4]

		if len(author) > 0 and len(year) > 0:
			key = author + year
		elif len(author) > 0:
			key = author
		else:
		 	key = document.get_key ()

		# Make the key unique within this document set
		append = "b"
		if assigned_keys.has_key (key):
			key += append
		
		# Assumes <26 identical keys
		while assigned_keys.has_key (key):
			append = chr(ord(append[0]) + 1)
			key = key[0:len(key) - 1]
			key += append

		print key
		assigned_keys[key] = True
			
		document.set_key (key)
	
	return True


