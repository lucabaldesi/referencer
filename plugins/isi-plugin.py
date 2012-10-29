#!/usr/bin/env python
 
# A referencer plugin to get document info from ISI Web of Science using 
# the Title/Author/Year fields of the document (any or all of them)
#
# Copyright 2008 Mario Castro, Yoav Avitzour
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


import os
import referencer
from referencer import _
import sys, urllib2, urllib
import gobject
import gtk

from xml.dom import minidom

try:
    from titlecase import titlecase
    UseTitlecase = True
except ImportError:
    UseTitlecase = False

DEFAULTMAXRECORDS = 10

referencer_plugin_info = {
    "author": "Mario Castro, Yoav Avitzour",
    "version": "0.1",
    "ui":
        """
        <ui>
            <menubar name='MenuBar'>
                <menu action='DocMenu'>
                    <placeholder name='PluginDocMenuActions'>
                        <menuitem action='_plugin_isi'/>
                    </placeholder>
                </menu>
            </menubar>
            <toolbar name='ToolBar'>
                <placeholder name='PluginToolBarActions'>
                    <toolitem action='_plugin_isi'/>
                </placeholder>
            </toolbar>
            <popup name='DocPopup'>
                <placeholder name='PluginDocPopupActions'>
                    <menuitem action='_plugin_isi'/>
                </placeholder>
            </popup>
        </ui>
        
        """,
    "longname": _("ISI Web of Science resolver (requires subscription)"),
    "action": _("Get metadata from ISI Web of Science"),
    "tooltip": _("ISI Web of Science resolver (requires subscription)")}

referencer_plugin_actions = [{
        "name":"_plugin_isi",
        "label":_("ISI Info"),
        "tooltip":_("Retrieve metadata for the selected documents from ISI Web of Science"),
        "icon":"_stock:gtk-edit",
        "callback":"do_action",
        "sensitivity":"sensitivity_genkey",
        "accelerator":"<control>i"
        }]

class isiRec:
    def __init__(self, document = None, firstrec = None):
        self.authors=''
        self.abstract=''
        self.keywords=''
        self.journal=''
        self.doi=''
        self.pages=''
        self.title=''
        self.year=''
        self.volume=''
        if document is not None:
            self.get_record_from_document(document,firstrec)

    def set_fields_from_data(self,data):
        xmldoc = minidom.parseString(data)
        self.authors=get_field(xmldoc,"primaryauthor")
        more_authors=get_fields(xmldoc,"author",' and ')
        if(len(more_authors)>0):
            self.authors+=' and '+more_authors
        self.authors = capitalize_authors(self.authors)
        self.abstract=get_field(xmldoc,"p")
        self.keywords=get_fields(xmldoc,"keyword",', ')
        self.journal=get_field(xmldoc,"source_title")
        if self.journal.isupper():
            if UseTitlecase:
                self.journal = titlecase(self.journal.lower())
            else:
                self.journal = self.journal.title()
        doi=get_last_field(xmldoc,"article_no")
        if len(doi) > 0:
            self.doi = doi[4:]
        else:
            self.doi = doi
        self.pages=get_field(xmldoc,"bib_pages")
        if self.pages == '-':
            artn = get_field(xmldoc,"article_no")
            self.pages = artn[4:]
        self.title=get_field(xmldoc,"item_title")
        if self.title.isupper():
            if UseTitlecase:
                self.title = titlecase(self.title.lower())
            else:
                self.title = self.title.title()
        self.year=get_attribute_from_field(xmldoc,"bib_issue","year")
        self.volume=get_attribute_from_field(xmldoc,"bib_issue","vol")

    def get_data(self,document, firstrec=None, numrecs=None):
        title = document.get_field("title")
        year = document.get_field ("year")
        author= document.get_field ("author")
        if firstrec is None:
            firstrec = 1
        if numrecs is None:
            numrecs = 1

        url='http://estipub.isiknowledge.com/esti/cgi?databaseID=WOS&SID=Q1mNFhCECOk6c8aELLh&rspType=xml&method=searchRetrieve'
        url += \
            '&firstRec=' + str(firstrec) + \
            '&numRecs=' + str(numrecs) + \
            '&query=' + get_query(document)
        data = referencer.download(_("Obtaining data from ISI-WebOfScience"), 
                                   _("Fetching data for %s/%s/%s") 
                                   % (author,title,year), url);
        return data

    def get_record_from_document(self,document,firstrec=None, numrecs=None):
        print 'firstrec = ', firstrec
        data = self.get_data(document,firstrec,numrecs)
        self.set_fields_from_data(data)

    def set_document_from_record(self,document):
        if (len(self.year)>0):
            document.set_field("year",self.year)
        if (len(self.volume)>0):
            document.set_field("volume",self.volume)
        if (len(self.title)>0):
            document.set_field("title",self.title)
        if (len(self.authors)>0):
            document.set_field("author",self.authors)
        if (len(self.doi)>0):
            document.set_field("doi",self.doi)
        if (len(self.journal)>0):
            document.set_field("journal",self.journal)
        if (len(self.pages)>0):
            document.set_field("pages",self.pages)
        if (len(self.abstract)>0):
            document.set_field("abstract",self.abstract)
        if (len(self.keywords)>0):
            document.set_field("keywords",self.keywords)
        return document

class preferencesDialog(gtk.Dialog):
    def __init__(self, parent = None):
        gtk.Dialog.__init__(self,"ISI plugin preferences",
                            parent,
                            gtk.DIALOG_MODAL | 
                            gtk.DIALOG_DESTROY_WITH_PARENT,
                            (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                             gtk.STOCK_OK, gtk.RESPONSE_OK))
        hbox = gtk.HBox()
        label = gtk.Label("Maximum no. of records to retreive from ISI")
        adjustment = gtk.Adjustment(value=get_MAXRECORDS(), lower=0, upper=100, 
                                    step_incr=1, page_incr=1)
        self.MaxRecords = gtk.SpinButton(adjustment,1)
        hbox.pack_start(label)
        hbox.pack_start(self.MaxRecords)
        self.vbox.pack_start(hbox,padding=3)
        hbox = gtk.HBox()
        text = """The ISI plugin uses the Title, Author and Year to find a matching record in the ISI database. If there is more than one match, a Record Chooser dialog opens. Set here the maximum number of records to retrieve for such cases."""
        label = gtk.Label(text)
        label.set_line_wrap(True)
        image = gtk.Image()
        image.set_from_stock(gtk.STOCK_DIALOG_INFO,gtk.ICON_SIZE_DIALOG)
        hbox.pack_start(image)
        hbox.pack_start(label)
        self.vbox.pack_start(hbox,padding=3)
        self.vbox.show_all()

class documentDisplay(gtk.Window):
    def __init__(self,document = None, parent = None):
        gtk.Window.__init__(self)
        self.set_title("Current document properties")
        self.myparent = parent
        self.set_transient_for(parent)
        self.set_destroy_with_parent(True)
        self.vbox = gtk.VBox()
        self.add(self.vbox)
        if document is not None:
            fields = ['title','author','journal','year','pages']
            table = gtk.Table(len(fields),2)
            row = 0
            for field in fields:
                label = gtk.Label()
                label.set_markup("<b>"+field.title()+": "+"</b>")
                label.set_alignment(0.0, 0.5)
                value = gtk.Label(document.get_field(field))
                value.set_line_wrap(True)
                value.set_alignment(0.0, 0.5) 
                table.attach(label,0,1,row,row+1,gtk.FILL)
                table.attach(value,1,2,row,row+1,gtk.EXPAND|gtk.FILL)
                row += 1
            self.vbox.pack_start(table)
        self.vbox.show_all()

    def set_position(self):
        (xp,yp) = self.myparent.get_position()
        (wp,hp) = self.get_size()
        x = xp
        y = yp - hp - 30
        self.move(x,y)

class noRecordFound(gtk.Dialog):
    def __init__(self, document = None, parent = None):
        gtk.Dialog.__init__(self,"ISI plugin",
                            parent,
                            gtk.DIALOG_MODAL | 
                            gtk.DIALOG_DESTROY_WITH_PARENT,
                            (gtk.STOCK_OK, gtk.RESPONSE_OK))
        text = """
<b>   ISI query did not find any matching records</b>
        
 You may try one of the following:

 * Remove words representing symbols from the title
   (such as alpha, \\alpha, etc.)
 * Remove hyphened words from the title
 * Remember that the ISI database does not contain 
   records earlier than 1975
"""
        label = gtk.Label()
        label.set_markup(text)
        self.vbox.pack_start(label)
        self.vbox.show_all()
        self.showdoc = gtk.Button("Show document")
        self.showdoc.connect("clicked",self.show_document_details)
        self.showdoc.show()
        label = gtk.Label()
        label.show()
        self.action_area.pack_start(self.showdoc)
        self.action_area.pack_start(label,True,True)
        self.action_area.reorder_child(self.showdoc,0)
        self.action_area.reorder_child(label,1)
        self.docDisp = documentDisplay(document,self)

    def show_document_details(self, *args):
        if self.docDisp.get_property("visible") == True:
            self.docDisp.hide()
            self.showdoc.set_label("Show document")
        else:
            self.docDisp.set_position()
            self.docDisp.show()
            self.showdoc.set_label("Hide document")

class recordChooser(gtk.Dialog):
    def __init__(self,document = None, records = None, parent = None):
        gtk.Dialog.__init__(self,"ISI record chooser dialog",
                            parent,
                            gtk.DIALOG_DESTROY_WITH_PARENT,
                            (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                             gtk.STOCK_OK, gtk.RESPONSE_OK))
        self.document = document
        self.records = records
        self.recTree = gtk.TreeStore(bool,str,str,bool)
        self.treeview = gtk.TreeView(self.recTree)
        self.tvcolumn0 = gtk.TreeViewColumn()  
        self.tvcolumn1 = gtk.TreeViewColumn()  
        self.tvcolumn2 = gtk.TreeViewColumn()  
        scwindow = gtk.ScrolledWindow()
        scwindow.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_AUTOMATIC)
        scwindow.add_with_viewport(self.treeview)
        self.vbox.pack_start(scwindow,True,True,0)
        self.treeview.append_column(self.tvcolumn0)
        self.treeview.append_column(self.tvcolumn1)
        self.treeview.append_column(self.tvcolumn2)
        self.tcell = gtk.CellRendererText()
        self.bcell = gtk.CellRendererToggle()
        self.bcell.set_radio(True)
        self.bcell.connect("toggled", self.toggled_cb, (self.recTree, 3)) 
        self.bcell.set_property('activatable',True)
        self.tvcolumn0.pack_start(self.bcell, True)
        self.tvcolumn0.add_attribute(self.bcell, 'visible', 0)
        self.tvcolumn0.add_attribute(self.bcell, 'active', 3)
        self.tvcolumn1.pack_start(self.tcell, True)
        self.tvcolumn1.add_attribute(self.tcell, 'text', 1)
        self.tvcolumn2.pack_start(self.tcell, True)
        self.tvcolumn2.add_attribute(self.tcell, 'text', 2)
        self.fields = ['authors','title','journal','volume','pages','year']
        if records is not None:
            self.add_records()
        self.current_record = 0
        self.treeview.expand_all()
        self.resize(400,300)
        self.vbox.show_all()
        self.showdoc = gtk.Button("Show document")
        self.showdoc.connect("clicked",self.show_document_details)
        self.showdoc.show()
        label = gtk.Label()
        label.show()
        self.action_area.pack_start(self.showdoc)
        self.action_area.pack_start(label,True,True)
        self.action_area.reorder_child(self.showdoc,0)
        self.action_area.reorder_child(label,1)
        self.docDisp = documentDisplay(self.document,self)

    def add_records(self):
        outerrow = 0
        for rec in self.records:
            outeriter = self.recTree.insert(None,outerrow)
            if outerrow == 0:
                isactive = True
            else:
                isactive = False
            self.recTree.set(outeriter,0,True,
                             1,'Record '+str(outerrow+1),3,isactive)
            for innerrow in range(len(self.fields)):
                self.recTree.set(self.recTree.insert(outeriter,innerrow),
                                 0,False,
                                 1,self.fields[innerrow],
                                 2,eval('rec.'+self.fields[innerrow]),
                                 3,False)
            outerrow += 1
            
    def toggled_cb(self, cell, path, user_data):
        model, column = user_data
        for row in model:
            row[column] = False
        model[path][column] = True
        self.current_record = path
        return

    def show_document_details(self, *args):
        if self.docDisp.get_property("visible") == True:
            self.docDisp.hide()
            self.showdoc.set_label("Show document")
        else:
            self.docDisp.set_position()
            self.docDisp.show()
            self.showdoc.set_label("Hide document")

def capitalize_authors(authors):
    spltau = authors.split()
    nau = (len(spltau)+1)/3
    for i in range(nau):
        spltau[3*i] = spltau[3*i].capitalize()
        spltau[3*i+1] = spltau[3*i+1].upper()
    return ' '.join(spltau)

def get_fields (doc, field, separator):
    value = doc.getElementsByTagName(field)
    output=''
    if len(value) == 0:
        return ""
    else:
        length=len(value)
        if (len(value[0].childNodes) == 0):
            return ""
        else:
            #for items in value:
            for index in range(length-1):
                output+=value[index].childNodes[0].data.encode("utf-8")+separator
        return output+value[length-1].childNodes[0].data.encode("utf-8")

def get_last_field (doc, field):
    value = doc.getElementsByTagName(field)
    if len(value) == 0:
        return ""
    else:
        if (len(value[0].childNodes) == 0):
            return ""
        else:
            for items in value:
                last=items.childNodes[0].data.encode("utf-8")
            return last

def get_field (doc, field):
    value = doc.getElementsByTagName(field)
    if len(value) == 0:
        return ""
    else:
        if (len(value[0].childNodes) == 0):
            return ""
        else:
            return value[0].childNodes[0].data.encode("utf-8")


def get_attribute_from_field (doc, field, attr):
    value = doc.getElementsByTagName(field)
    return value[0].getAttribute(attr)

def get_number_of_records (document):

    title = document.get_field("title")
    year = document.get_field ("year")
    author= document.get_field ("author")

    url0='http://estipub.isiknowledge.com/esti/cgi?databaseID=WOS&rspType=xml&method=search&firstRec=1&numRecs=1'
    url0+= '&query='+get_query(document) 
    data0 = referencer.download(
        _("Obtaining data from ISI-WebOfScience"), 
        _("Fetching number of ocurrences for %s/%s/%s") % (author,title,year), 
        url0)
    print data0
    xmldoc0 = minidom.parseString(data0)
    recordsFound=get_field(xmldoc0,"recordsFound")
    return int(recordsFound)

def get_query(document):

    query = ''

    title = document.get_field("title")
    title = remove_non_ascii_chars(title)
    if len(title) > 0:
        ti=urllib.urlencode([('','('+title+')')])
        query = 'TI'+ti+'&'
    year = document.get_field ("year")
    if len(year)>0:
        ye=urllib.urlencode([('','('+year+')')])
        query+= 'PY'+ye+'&'
    author= document.get_field ("author")
    author = remove_non_ascii_chars(author)
    if len(author)>0:
        au=urllib.urlencode([('','('+author+')')])
        query+= 'AU'+au+'&'

    return query

def remove_non_ascii_chars(si):
    for s in si:
        if ord(s)>126 or ord(s) < 32:
            si = si.replace(s,'',1)
    return si

#>-- Start the record chooser dialog in case more than one matching
#>-- record was found
def choose_record(document,nrecs):
#    CurrentRecord = isiRec()
    records = []
    for rec in range(nrecs):
        irec = isiRec(document,rec+1)
        records.append(irec)
    recChoose = recordChooser(document,records)
    response = recChoose.run()
    if response == int(gtk.RESPONSE_OK):
        currentrec = recChoose.current_record
        currentrec = records[int(currentrec)]
    else:
        currentrec = False
    recChoose.destroy()
    return currentrec

def get_MAXRECORDS():
    maxrecords = referencer.pref_get ("isi_maxrecords")
    if (len(maxrecords)==0):
        maxrecords = DEFAULTMAXRECORDS
    return int(maxrecords)

def set_MAXRECORDS(maxrecords):
    referencer.pref_set ("isi_maxrecords", str(maxrecords))

#>-- Main referencer function
def do_action(library,documents):
    empty = True
    s = ""
    assigned_keys = {}
    for document in documents:
        nrecs=get_number_of_records(document)
        if nrecs>1:
            MAXRECORDS = get_MAXRECORDS()
            rec = choose_record(document,min(nrecs,MAXRECORDS))
            if rec is not False:
                rec.set_document_from_record(document)
        elif nrecs == 1:
            rec = isiRec(document)
            rec.set_document_from_record(document)
        elif nrecs == 0:
            noRec = noRecordFound(document)
            response = noRec.run()
            noRec.destroy()
    return True

#>-- Main referencer preferences function
def referencer_config():
    prefs = preferencesDialog()
    response = prefs.run()
    if response == int(gtk.RESPONSE_OK):
        set_MAXRECORDS(prefs.MaxRecords.get_value_as_int())
    prefs.destroy()


