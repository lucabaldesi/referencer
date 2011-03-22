
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#ifndef LIBRARY_H
#define LIBRARY_H

#include <glibmm/ustring.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>

class Document;
class DocumentList;
class TagList;
class RefWindow;

//
// BEGIN: The names of all the elements in the 'reflib' library XML file.
//

/**
 * The name of the root element of the 'referencer library' XML file.
 */
#define LIB_ELEMENT_LIBRARY "library"
/**
 * The name of the 'library folder' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_LIBRARY_FOLDER "library_folder"
/**
 * The name of the 'monitor' attribute in the 'library folder' element.
 */
#define LIB_ATTR_LIBRARY_FOLDER_MONITOR "monitor"
/**
 * The name of the 'manage target' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_MANAGE_TARGET "manage_target"
/**
 * The name of the 'braces' attribute in the 'manage target' element.
 */
#define LIB_ATTR_MANAGE_TARGET_BRACES "braces"
/**
 * The name of the 'utf8' attribute in the 'manage target' element.
 */
#define LIB_ATTR_MANAGE_TARGET_UTF8 "utf8"
/**
 * The name of the 'tag list' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_TAGLIST "taglist"
/**
 * The name of the 'tag' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_TAG "tag"
/**
 * The name of the tag's 'uid' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_TAG_UID "uid"
/**
 * The name of the tag's 'name' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_TAG_NAME "name"
/**
 * The name of the 'doc list' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOCLIST "doclist"
/**
 * The name of the 'doc' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC "doc"
/**
 * The name of the document's 'filename' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_FILENAME "filename"
/**
 * The name of the document's 'relative filename' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_REL_FILENAME "relative_filename"
/**
 * The name of the document's 'notes' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_NOTES "notes"
/**
 * The name of the document's 'key' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_KEY "key"
/**
 * The name of the document's 'tagged' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_TAG "tagged"
/**
 * The name of the document's 'bib type' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_BIB_TYPE "bib_type"
/**
 * The name of the document's 'bib doi' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_BIB_DOI "bib_doi"
/**
 * The name of the document's 'bib title' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_BIB_TITLE "bib_title"
/**
 * The name of the document's 'bib authors' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_BIB_AUTHORS "bib_authors"
/**
 * The name of the document's 'bib journal' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_BIB_JOURNAL "bib_journal"
/**
 * The name of the document's 'bib volume' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_BIB_VOLUME "bib_volume"
/**
 * The name of the document's 'bib number' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_BIB_NUMBER "bib_number"
/**
 * The name of the document's 'bib pages' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_BIB_PAGES "bib_pages"
/**
 * The name of the document's 'bib year' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_BIB_YEAR "bib_year"
/**
 * The name of the document's 'bib extra' element in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_BIB_EXTRA "bib_extra"
/**
 * The name of the document's 'bib extra' key attribute in the 'referencer library' XML file.
 */
#define LIB_ELEMENT_DOC_BIB_EXTRA_KEY "key"

/**
 * Compares the name of the node with the given string and returns 'true' iff
 * the name of the node equals to it.
 */
inline bool nodeNameEq(const xmlNodePtr node, const char* name) {
    return strcmp(name, (const char*) node->name) == 0;
}

//
// END: The names of all the elements in the 'reflib' library XML file.
//

/**
 * Contains all library data (as stored in the 'reflib' XML file, e.g.: the
 * document and tag lists, and other configuration values).
 */
struct LibraryData {
    /**
     * Contains the list of all \ref Document "documents" in this library.
     */
    DocumentList *doclist_;
    /**
     * Contains the list of all \ref Tag "tags" in this library.
     * <p>These tags can be assigned to documents in \ref
     * LibraryData::doclist_.</p>
     */
    TagList *taglist_;
    Glib::ustring manage_target_;
    bool manage_braces_;
    bool manage_utf8_;

    Glib::ustring library_folder_uri_;
    bool library_folder_monitor_;

    /**
     * Creates an instance of the \ref LibraryData structure with default
     * configuration values and empty tag and document lists.
     */
    LibraryData();
    /**
     * Deallocates all resources (deletes the tag and document lists).
     */
    virtual ~LibraryData();
    /**
     * Clears the document list, the tag list, and resets the configuration to
     * default.
     */
    void clear();
    /**
     * <p>Extracts information about a referencer library from a parsed XML
     * document tree. Upon success, this method will set the fields of this
     * instance accordingly.</p>
     * <p>Upon failure, however, this method will throw an exception and the
     * state of this instance will be in an undefined state (partially read
     * data).</p>
     *
     * @param libDocument the XML DOM tree extracted from the referencer's
     * library XML file (the 'reflib' file).
     *
     * @exception Glib::Exception this exception is thrown if the parsing fails
     * for any reason.
     */
    void extractData(xmlDocPtr libDocument) throw (Glib::Exception);
};

class Library {
	public:
	Library (RefWindow &tagwindow);
	~Library ();

	void clear ();
	bool load (Glib::ustring const &libfilename);
	bool save (Glib::ustring const &libfilename);

	void writeXML(xmlTextWriterPtr writer);
	bool readXML(const Glib::ustring& libFileName);

	void writeBibtex (
		Glib::ustring const &bibfilename,
		std::vector<Document*> const &docs,
		bool const usebraces,
		bool const utf8);

	// The naming is BibtexFoo everywhere else, but in Library
	// we use the manage_ prefix to be consistent with the file format
	// which uses manage_ to be future-proof with more general manage
	// functionality
	void manageBibtex (
		Glib::ustring const &target,
		bool const brackets,
		bool const utf8);

    Glib::ustring getBibtexTarget() const {
        return data->manage_target_;
	}

    bool getBibtexBraces() const {
        return data->manage_braces_;
    }

    bool getBibtexUTF8() const {
        return data->manage_utf8_;
    }

    Glib::ustring getLibraryFolder() const {
        return data->library_folder_uri_;
    }

    void setLibraryFolder(Glib::ustring LibraryFolderUri) {
        data->library_folder_uri_ = LibraryFolderUri;
    }

    DocumentList* getDocList() const {
        return data->doclist_;
    }

    TagList* getTagList() const {
        return data->taglist_;
    }

    bool libraryFolderDialog();

	private:
    /**
     * Extracts information about a library from the given DOM tree.
     *
     * @param libDocument the XML DOM tree extracted from the referencer's
     * library XML file. This method will extract all
     *
     * @returns 'true' if the library data has been read successfully,
     * otherwise it returns 'false'.
     */
    bool readLibrary(xmlDocPtr libDocument) throw (Glib::Exception);

private:
    /**
     * Contains all data about this library (tags, documents and other
     * configuration).
     */
    struct LibraryData *data;

	RefWindow &tagwindow_;
};

#endif
