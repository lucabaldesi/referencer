#!/usr/bin/env python
 
#  Generate Bob08 Alice99 Alice99b type keys

import os
import referencer
from referencer import _

import gobject
import gtk

referencer_plugin_info = []
referencer_plugin_info.append (["author", "John Spray"])
referencer_plugin_info.append (["version", "1.1.2"])
referencer_plugin_info.append (["configure", "configure"])
referencer_plugin_info.append (["longname", _("Generate keys from metadata")])
referencer_plugin_info.append (["ui",
		"""
		<ui>
			<menubar name='MenuBar'>
				<menu action='ToolsMenu'>
					<placeholder name ='PluginToolsActions'>
						<menuitem action='_plugin_genkey_genkey'/>
					</placeholder>
				</menu>
			</menubar>

			<popup name='DocPopup'>
				<menuitem action='_plugin_genkey_genkey'/>
			</popup>
		</ui>
		"""])

referencer_plugin_actions = []

action = {
	"name":"_plugin_genkey_genkey",
	"label":_("Generate Key"),
	"tooltip":_("Generate keys for the selected documents from their metadata"),
	"icon":"lyx.png",
	"callback":"do_genkey",
	"sensitivity":"sensitivity_genkey",
	"accelerator":"<control>g"
}
referencer_plugin_actions.append (action)

def sensitivity_genkey (library, documents):
	if (len(documents) > 0):
		return True
	else:
	 	return False

# library is always Nil, it's only there for future proofing
# documents is a list of referencer.document
def do_genkey (library, documents):
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

def configure():
	pass
