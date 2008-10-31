#!/usr/bin/env python
 
#  Generate Bob08 Alice99 Alice99b type keys

import os
import referencer
from referencer import _

import gobject
import gtk

referencer_plugin_info = {
	"author":    "John Spray",  
	"version":   "0.0",
	"longname":  "Search test",
}


def referencer_search (search_text):
	return [{"title":"The Bible", "author":"God"}]

def referencer_search_result (token):
	return None
