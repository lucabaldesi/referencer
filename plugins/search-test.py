#!/usr/bin/env python
 
#  Generate Bob08 Alice99 Alice99b type keys

import os
import referencer
from referencer import _

import gobject
import gtk
import time

referencer_plugin_info = {
	"author":    "John Spray",  
	"version":   "0.0",
	"longname":  "Search test",
}

results = [{"token":"GOD00", "title":"The Holy Bible", "author":"The lord our God"}]

def referencer_search (search_text):
	for n in range(0,28):
		#time.sleep (0.1)
		foo =  referencer.poll_cancellation()

	return results

def referencer_search_result (token):
	for result in results:
		if result["token"] == token:
			return result

	return {}
