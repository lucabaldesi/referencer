#!/usr/bin/env python

# Show a selected paper bibtex entries

import os
import referencer
from referencer import _

import gobject
import gtk

referencer_plugin_info = {
	"author":    "John Spray",
	"version":   "0.0",
	"longname":  "Export bibtex entries",
	"ui":
		"""
		<ui>
			<popup name='DocPopup'>
				<placeholder name='PluginDocPopupActions'>
					<menuitem action='_plugin_export-bibtex-entries'/>
				</placeholder>
			</popup>
		</ui>
		"""
	}

referencer_plugin_actions = []

action = {
	"name":"_plugin_export-bibtex-entries",
	"label":"Show Bibtex",
	"tooltip":"Show Bibtex",
	"icon":"_stock:gtk-print",
	"callback":"do_print_bibtex",
	"sensitivity":"sensitivity_print_bibtex"
}
referencer_plugin_actions.append (action)

def sensitivity_print_bibtex (library, documents):
	if (len(documents) > 0):
		return True
	else:
	 	return False

def do_print_bibtex (library, documents):
	text = ""
	for doc in documents:
		text += doc.print_bibtex(False, False)
		print text

	window = gtk.Dialog (buttons=(gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
	window.set_default_size(800,400)

	box = window.vbox
	box.show()

	sw = gtk.ScrolledWindow()
	sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
	textview = gtk.TextView()
	textbuff = textview.get_buffer()
	textview.set_editable(False)
	textview.set_wrap_mode(gtk.WRAP_WORD)
	sw.add(textview)
	sw.show()
	textview.show()
	box.pack_start(sw)
	textbuff.set_text(text)

	window.run()
	window.hide()

	return False
