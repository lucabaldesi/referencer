/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */

#include <iostream>
#include <cstring>

#include <glibmm/i18n.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>

#include "TagList.h"
#include "DocumentList.h"
#include "Progress.h"
#include "Utility.h"

#include "Library.h"

//
// Some private helper functions
//

using namespace Utility;

/**
 * This is a callback function used in the XML parser to fetch data from a GVFS
 * input stream.
 */
static int vfsRead(void * context, char * buffer, int len) {
    if (!context)
        return -1;
    return ((Gnome::Vfs::Handle*) context)->read(buffer, len);
}

/**
 * This is a callback function used in the XML parser to close a GVFS input
 * stream.
 */
static int vfsClose(void * context) {
    if (!context)
        return -1;
    ((Gnome::Vfs::Handle*) context)->close();
    return 0;
}

/**
 * This is a callback function used in the XML parser to write data to a GVFS
 * output stream.
 */
static int vfsWrite(void * context, const char * buffer, int len) {
    if (!context)
        return -1;
    return ((Gnome::Vfs::Handle*) context)->write(buffer, len);
}

/**
 * Compares the two strings and returns \c true if they equal.
 * @param strA The first string (note that it is of type <tt>const
 * xmlChar*</tt>).
 * @param strB The second string.
 * @return
 */
static inline bool xStrEq(cxStr strA, const char * strB) {
    return strcmp(strB, CSTR strA) == 0;
}

/**
 * <p>Checks whether the value of the attribute with the given name equals to
 * 'true'.</p>
 *
 * <p>If the attribute does not exist or if it misformed then this method will
 * return the default value (provided in the optional parameter 'def').</p>
 */
static inline bool parseBoolAttr(const xmlNodePtr node, const char* attrName,
        bool def = false) {
    xStr text = xmlGetProp(node, CXSTR attrName);
    def = (!text) ? def : (xStrEq(text, "true") && !xStrEq(text, "false"));
    xmlFree(text);
    return def;
}

/**
 * <p>Callbacks of this type are used in the function \ref
 * forEachChild(xmlNodePtr,LibraryData*) to process all child elements of a
 * node.</p>
 * <p>This function is called for each child of the parent node</p>
 *
 * @param child The current child.
 *
 * @param data The context data object to which to store parsed data.
 */
typedef void (*forEachChildCallback)(xmlNodePtr child, LibraryData* data);

/**
 * Goes through every child element of the node and calls the <c>func</c>
 * callback function for each child.
 */
static inline void forEachChild(xmlNodePtr node, LibraryData* data,
        forEachChildCallback func) {
    for (xmlNodePtr child = xmlFirstElementChild(node); child; child
            = xmlNextElementSibling(child)) {
        func(child, data);
    }
}

/**
 * Extracts the data from the 'manage target' element in the 'reflib' XML
 * file.
 */
static void parseManageTargetElement(xmlNodePtr manageTargetElement,
        LibraryData* data) {
    COPY_NODE(data->manage_target_, manageTargetElement);
    data->manage_braces_ = parseBoolAttr(manageTargetElement,
            LIB_ATTR_MANAGE_TARGET_BRACES);
    data->manage_utf8_ = parseBoolAttr(manageTargetElement,
            LIB_ATTR_MANAGE_TARGET_UTF8);
}

/**
 * Extracts the data from the 'library folder' element in the 'reflib' XML
 * file.
 */
static void parseLibraryFolderElement(xmlNodePtr libraryFolderElement,
        LibraryData* data) {
    COPY_NODE(data->library_folder_uri_, libraryFolderElement);
    data->library_folder_monitor_ = parseBoolAttr(libraryFolderElement,
            LIB_ATTR_LIBRARY_FOLDER_MONITOR);
}

/**
 * Extracts data from the 'tag' element in the 'reflib' XML
 * file.
 */
static void parseTagElement(xmlNodePtr tagElement, LibraryData* data) {
    if (nodeNameEq(tagElement, LIB_ELEMENT_TAG)) {
        char *name = NULL;
        int uid = 0;
        // The 'foundUid' flag is used to throw an error if it happens that we
        // have not found an ID for a tag.
        bool foundUid = false;
        for (xmlNodePtr child = xmlFirstElementChild(tagElement); child; child
                = xmlNextElementSibling(child)) {
            if (nodeNameEq(child, LIB_ELEMENT_TAG_UID)) {
                COPY_NODE_MAP(uid, child, atoi);
                foundUid = true;
            } else if (nodeNameEq(child, LIB_ELEMENT_TAG_NAME) && name == NULL) {
                name = STR xmlNodeGetContent(child);
            }
        }
        if (name && foundUid) {
            data->taglist_->loadTag(name, uid);
            xmlFree(name);
        } else {
            throw Glib::MarkupError(Glib::MarkupError::PARSE, _(
                    "Stumbled upon a tag without a name or an id."));

        }
    }
}

/**
 * Extracts the data from the 'doc' elements in the 'reflib' XML
 * file.
 */
static void parseDocElement(xmlNodePtr docElement, LibraryData* data) {
    if (nodeNameEq(docElement, LIB_ELEMENT_DOC))
        data->doclist_->insertDoc(Document(docElement));
}

//
// LibraryData implementation
//

LibraryData::LibraryData() {
    doclist_ = new DocumentList();
    taglist_ = new TagList();
	manage_braces_ = false;
	manage_utf8_ = false;
    library_folder_monitor_ = false;
}

LibraryData::~LibraryData() {
	delete doclist_;
	delete taglist_;
}

void LibraryData::clear() {
    taglist_->clear();
    doclist_->clear();
	manage_target_ = "";
	manage_braces_ = false;
	manage_utf8_ = false;
    library_folder_monitor_ = false;
    library_folder_uri_ = "";
}

void LibraryData::extractData(xmlDocPtr libDocument) throw (Glib::Exception) {
    this->clear();
    xmlNodePtr node = xmlDocGetRootElement(libDocument);
    if (node && node->type == XML_ELEMENT_NODE && nodeNameEq(node,
            LIB_ELEMENT_LIBRARY)) {
        for (xmlNodePtr child = xmlFirstElementChild(node); child; child
                = xmlNextElementSibling(child)) {
            if (nodeNameEq(child, LIB_ELEMENT_DOCLIST)) {
                // We have found the 'document list' element.
                forEachChild(child, this, &parseDocElement);
            } else if (nodeNameEq(child, LIB_ELEMENT_MANAGE_TARGET)) {
                // We have found the 'manage target' element.
                parseManageTargetElement(child, this);
            } else if (nodeNameEq(child, LIB_ELEMENT_LIBRARY_FOLDER)) {
                // We have found the 'library folder' element.
                parseLibraryFolderElement(child, this);
            } else if (nodeNameEq(child, LIB_ELEMENT_TAGLIST)) {
                // We have found the 'tag list' element.
                forEachChild(child, this, &parseTagElement);
            }
        }
    }
}

//
// Library implementation
//

Library::Library(RefWindow &tagwindow) :
tagwindow_(tagwindow) {
    data = new LibraryData();
}

Library::~Library() {
    delete data;
}

void Library::writeXML(xmlTextWriterPtr writer) {
    xmlTextWriterStartElement(writer, XSTR LIB_ELEMENT_LIBRARY);

    xmlTextWriterStartElement(writer, XSTR LIB_ELEMENT_MANAGE_TARGET);
    xmlTextWriterWriteAttribute(writer, XSTR LIB_ATTR_MANAGE_TARGET_BRACES, XSTR(data->manage_braces_ ? "true" : "false"));
    xmlTextWriterWriteAttribute(writer, XSTR LIB_ATTR_MANAGE_TARGET_UTF8, XSTR(data->manage_utf8_ ? "true" : "false"));
    xmlTextWriterWriteString(writer, XSTR data->manage_target_.c_str());
    xmlTextWriterEndElement(writer);

    xmlTextWriterStartElement(writer, XSTR LIB_ELEMENT_LIBRARY_FOLDER);
    xmlTextWriterWriteAttribute(writer, XSTR LIB_ATTR_LIBRARY_FOLDER_MONITOR, XSTR(data->library_folder_monitor_ ? "true" : "false"));
    xmlTextWriterWriteString(writer, XSTR data->library_folder_uri_.c_str());
    xmlTextWriterEndElement(writer);

    data->taglist_->writeXML(writer);
    data->doclist_->writeXML(writer);

    xmlTextWriterEndElement(writer);
}

bool Library::readXML(const Glib::ustring& libFileName) {
    Gnome::Vfs::Handle libfile;

    LibraryData* tmpData = NULL;
    xmlDocPtr libDoc = NULL;
    try {
        libfile.open(libFileName, Gnome::Vfs::OPEN_READ);
        // Parse the library XML file to get the DOM tree.
        xmlDocPtr libDoc =
                xmlReadIO(vfsRead, vfsClose, &libfile, NULL, NULL, 0);
        if (!libDoc) {
            throw Glib::MarkupError(Glib::MarkupError::PARSE, _(
                    "Could not parse the 'reflib' file."));
        }
        // Fetch the data and store it in a new LibraryData instance.
        tmpData = new LibraryData();
        tmpData->extractData(libDoc);
    } catch (const Glib::Exception& ex) {
        Utility::exceptionDialog(&ex, "opening library '"
                + Gnome::Vfs::Uri::format_for_display(libFileName) + "'");
        DELETE_AND_NULL(tmpData)
    }

    xmlFreeDoc(libDoc);
    if (tmpData) {
        delete this->data;
        this->data = tmpData;
    }
    return tmpData != NULL;
}

void Library::clear() {
    data->clear();
}

/**
 * Show a dialog prompting the user for a folder in which
 * to download documents and optionally to monitor for new documents
 * Returns true if there was a change.
 */
bool Library::libraryFolderDialog ()
{
	Glib::RefPtr<Gtk::Builder> xml = Gtk::Builder::create_from_file 
		(Utility::findDataFile ("libraryfolder.ui"));

	Gtk::FileChooserButton *location;
		xml->get_widget ("Location", location);

	Gtk::CheckButton *monitor;
		xml->get_widget ("AutomaticallyAddDocuments", monitor);

	Gtk::Dialog *dialog;
		xml->get_widget ("LibraryFolder", dialog);
 
    bool oldMonitorState = data->library_folder_monitor_;
    Glib::ustring const oldFolder = data->library_folder_uri_;

    monitor->set_active(data->library_folder_monitor_);
    location->select_uri(data->library_folder_uri_);

    dialog->run();

    data->library_folder_monitor_ = monitor->get_active();
	if (!location->get_uri().empty())
        data->library_folder_uri_ = location->get_uri();

    DEBUG(data->library_folder_uri_);

    dialog->hide();
    return ((oldMonitorState != data->library_folder_monitor_) || (oldFolder
            != data->library_folder_uri_));
}



// True on success
bool Library::load (Glib::ustring const &libfilename)
{
	Glib::RefPtr<Gio::File> libfile = Gio::File::create_for_uri (libfilename);
	Glib::RefPtr<Gio::FileInfo> fileinfo = libfile->query_info ();
	Glib::RefPtr<Gio::FileInputStream> libfile_is;

	Progress progress (tagwindow_);

	progress.start (String::ucompose (
		_("Opening %1"),
		fileinfo->get_display_name ()));

    // First try to parse the XML library file
    if (!readXML(libfilename)) {
        return false;
    }
    DEBUG(String::ucompose("Done, got %1 docs", data->doclist_->getDocs().size()));
    //XXX: progress calls commented out, since they flush events,
    //causing the thumbnail generator to run but with invalid filenames
    // -mchro

    //progress.update(0.2);

    int i = 0;
    DocumentList::Container &docs = data->doclist_->getDocs();
    DocumentList::Container::iterator docit = docs.begin();
    DocumentList::Container::iterator const docend = docs.end();
    for (; docit != docend; ++docit) {
		//progress.update (0.2 + ((double)(i++) / (double)docs.size ()) * 0.8);

        if (!docit->getRelFileName().empty()) {
			Glib::ustring full_filename;
			full_filename = Glib::build_filename (
				Glib::path_get_dirname (libfilename),
				docit->getRelFileName());
			docit->setFileName(full_filename);
		}
	}

	progress.finish ();

    return true;
}

// True on success

bool Library::save(Glib::ustring const &libfilename) {

    DEBUG("Saving to %1", libfilename);
    Glib::RefPtr<Gio::File> libfile = Gio::File::create_for_uri (libfilename
);

    DEBUG("Updating relative filenames...");
    DocumentList::Container &docs = data->doclist_->getDocs();
    DocumentList::Container::iterator docit = docs.begin();
    DocumentList::Container::iterator const docend = docs.end();
    for (; docit != docend; ++docit) {
        docit->updateRelFileName(libfilename);
    }
    DEBUG("Done.");

    DEBUG("Generating XML...");
	try {
        xmlBufferPtr buf = xmlBufferCreate();
        xmlOutputBufferPtr outBuf = xmlOutputBufferCreateBuffer(buf, NULL);
        xmlTextWriterPtr writer = xmlNewTextWriter(outBuf);
        if (writer) {
            xmlTextWriterSetIndent(writer, true);
            xmlTextWriterSetIndentString(writer, CXSTR"\t");
            xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL);
            writeXML(writer);
            xmlTextWriterEndDocument(writer);
            xmlTextWriterFlush(writer);
            xmlFreeTextWriter(writer);

            std::string new_etag;
            Glib::ustring rawtext = reinterpret_cast<const char*>(xmlBufferContent(buf));
            libfile->replace_contents (rawtext, "", new_etag);
        } else
            throw Glib::FileError(Glib::FileError::FAILED, _("Could not create an XML writer."));
    } catch (const Glib::Exception& ex) {
        Utility::exceptionDialog(&ex, "Generating 'reflib' XML file '" + libfilename + "'");
		return false;
	}
    DEBUG("Done.");

    DEBUG("Writing bibtex, manage_target_ = %1", data->manage_target_);
	// Having successfully saved the library, write the bibtex if needed
    if (!data->manage_target_.empty()) {
		// manage_target_ is either an absolute URI or a relative URI
		Glib::ustring bibtextarget_uri;
		if (Glib::uri_parse_scheme(data->manage_target_) != "") //absolute URI
			bibtextarget_uri = data->manage_target_;
		else
			bibtextarget_uri = libfile->get_parent()->resolve_relative_path(data->manage_target_)->get_uri();

		DEBUG ("bibtextarget_uri = %1", bibtextarget_uri);

		std::vector<Document*> docs;
        DocumentList::Container &docrefs = data->doclist_->getDocs();
		DocumentList::Container::iterator it = docrefs.begin();
		DocumentList::Container::iterator const end = docrefs.end();
		for (; it != end; it++) {
			docs.push_back(&(*it));
		}
		try {
		    writeBibtex (bibtextarget_uri, docs, data->manage_braces_, data->manage_utf8_);
		} catch (Glib::Exception const &ex) {
			Utility::exceptionDialog (&ex, "writing bibtex to " + bibtextarget_uri);
			return false;
		}
	}
	DEBUG ("Done.");

	return true;
}


void Library::writeBibtex (
	Glib::ustring const &biburi,
	std::vector<Document*> const &docs,
	bool const usebraces,
	bool const utf8)
{
	DEBUG ("Writing BibTex to %1", biburi);

	Glib::RefPtr<Gio::File> bibfile = Gio::File::create_for_uri (biburi);

	std::ostringstream bibtext;

	std::vector<Document*>::const_iterator it = docs.begin ();
	std::vector<Document*>::const_iterator const end = docs.end ();
	for (; it != end; ++it) {
		(*it)->writeBibtex (*this, bibtext, usebraces, utf8);
	}

	try {
		std::string new_etag;
		bibfile->replace_contents (bibtext.str(), "", new_etag);
	} catch (const Gio::Error ex) {
		Utility::exceptionDialog (&ex, "writing to BibTex file");
		return;
	}
}

void Library::manageBibtex(Glib::ustring const &target, bool const braces,
        bool const utf8) {
    data->manage_target_ = target;
    data->manage_braces_ = braces;
    data->manage_utf8_ = utf8;
}

