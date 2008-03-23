#!/usr/bin/env python
 
#  Generate Bob08 Alice99 Alice99b type keys

import os
import referencer
from referencer import _

import gobject
import gtk

referencer_plugin_info = {
	"author":    "John Spray",  
	"version":   "1.1.2",
	"longname":  _("Generate keys from metadata"),
	"ui":
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
		"""
	}

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

	format = referencer.pref_get ("genkey_format")
	if (len(format)==0):
		format = "%a%y"

	# Prompt the user for the key format
	dialog = gtk.Dialog (buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT, gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
	dialog.set_has_separator (False)
	dialog.vbox.set_spacing (6)
	hbox = gtk.HBox (spacing=6)
	label = gtk.Label ("Key format:")
	entry = gtk.Entry ()
	entry.set_text (format)
	dialog.vbox.pack_start (hbox)
	hbox.pack_start (label)
	hbox.pack_start (entry)
	
	label = gtk.Label ("Markers:\n\t%y = two-digit year\n\t%Y = four-digit year\n\t%a = first author's surname")
	dialog.vbox.pack_start (label)

	dialog.show_all ()
	response = dialog.run ()
	dialog.hide ()

	if (response == gtk.RESPONSE_REJECT):
		return False
	
	format = entry.get_text ()

	for document in documents:
		author = document.get_field ("author")
		author = author.split("and")[0].split(",")[0].split(" ")[0]
		year = document.get_field ("year")

		if len(year) == 4:
			shortYear = year[2:4]

		if (len(author) > 0):
			# replace %y with shortYear
			# replace %Y with year
			# replace %a with first author's last name
			key = format
			key = key.replace ("%y", shortYear)
			key = key.replace ("%Y", shortYear)
			key = key.replace ("%a", author)
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
	

	referencer.pref_set ("genkey_format", format)

	return True
