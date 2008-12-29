

#include <iostream>

#include <Python.h>

#include <glibmm/ustring.h>
#include <glibmm/i18n.h>
#include "ucompose.hpp"

#include "Document.h"
#include "PluginManager.h"
#include "Utility.h"
#include "PythonDocument.h"

#include "PythonPlugin.h"


PythonPlugin::PythonPlugin(PluginManager *owner)
{
	pGetFunc_ = NULL;
	pMod_ = NULL;
	owner_ = owner;
}


PythonPlugin::~PythonPlugin()
{
	if (loaded_) {
		if (pGetFunc_ != NULL)
			Py_DECREF (pGetFunc_);

		if (pPluginInfo_ != NULL)
			Py_DECREF (pPluginInfo_);

		Py_DECREF (pMod_);
	}
}

void freeString (void *data)
{
	delete (Glib::ustring*)data;
}

/* Convenience function for sometimes-present items */
Glib::ustring safePyDictGetItem (PyObject *dict, Glib::ustring const &key)
{
	PyObject *keyStr = PyString_FromString (key.c_str());
	if (PyDict_Contains (dict, keyStr)) {
		Py_DECREF (keyStr);
		return PyString_AsString (PyDict_GetItemString (dict, key.c_str()));
	} else {
		Py_DECREF (keyStr);
		return Glib::ustring ();
	}
}


void PythonPlugin::load (std::string const &moduleName)
{
	/* XXX this gets called before we're in the main event loop, so we can't use 
	 * displayException in the error cases here */
	moduleName_ = moduleName;
	PyObject *pName = PyString_FromString(moduleName.c_str());
	if (!pName) {
		printException ();
		DEBUG ("Plugin::load: Couldn't construct module name");
		return;
	}
	pMod_ = PyImport_Import(pName);
	Py_DECREF(pName);

	if (!pMod_) {
		printException ();
		DEBUG ("Plugin::load: Couldn't import module");
		return;
	}

	pPluginInfo_ = PyObject_GetAttrString (pMod_, "referencer_plugin_info");
	if (!pPluginInfo_) {
		DEBUG ("Plugin::load: Couldn't find plugin info");
		Py_DECREF (pMod_);
		return;
	}

	if (!PyDict_Check (pPluginInfo_)) {
		DEBUG (String::ucompose ("%1:Info dict isn't a dict!  Old plugin?", moduleName));

		Py_DECREF (pMod_);
		return;
	}


	/* Extract metadata capabilities */
	if (PyObject_HasAttrString (pMod_, "referencer_plugin_capabilities")) {
		PyObject *pCaps = PyObject_GetAttrString (pMod_, "referencer_plugin_capabilities");
		int const N = PyList_Size (pCaps);
		for (int i = 0; i < N; ++i) {
			PyObject *pCapabilityString = PyList_GetItem (pCaps, i);
			char *const cstr = PyString_AsString (pCapabilityString);
			Glib::ustring str;

			if (cstr)
				str = cstr;

			if (str.empty ())
				continue;

			if (str == "doi")
				cap_.add(PluginCapability::DOI);
			else if (str == "arxiv")
				cap_.add(PluginCapability::ARXIV);
			else if (str == "pubmed")
				cap_.add(PluginCapability::PUBMED);
			else if (str == "url")
				cap_.add(PluginCapability::URL);
		}
		Py_DECREF (pCaps);
	} else {
		DEBUG (String::ucompose ("no metadata capabilities in %1", moduleName_));
	}

	/* Extract metadata lookup function */
	if (cap_.hasMetadataCapability()) { 
		pGetFunc_ = PyObject_GetAttrString (pMod_, "resolve_metadata");
		if (!pGetFunc_) {
			DEBUG ("Couldn't find resolver");
			Py_DECREF (pMod_);
			return;
		} else {
			DEBUG1 ("Found resolver %1", pGetFunc_);
		}
	}

	/* Extract actions */
	if (PyObject_HasAttrString (pMod_, "referencer_plugin_actions")) {
		PyObject *pActions = PyObject_GetAttrString (pMod_, "referencer_plugin_actions");
		int const nActions = PyList_Size (pActions);

		/* Factory for creating stock icons */
		Glib::RefPtr<Gtk::IconFactory> iconFactory = Gtk::IconFactory::create();
		iconFactory->add_default ();

		for (int i = 0; i < nActions; ++i) {
			PyObject *pActionDict = PyList_GetItem (pActions, i);
			
			/* FIXME: don't require all fields */
			const Glib::ustring name        = safePyDictGetItem (pActionDict, "name");
			const Glib::ustring label       = safePyDictGetItem (pActionDict, "label");
			const Glib::ustring tooltip     = safePyDictGetItem (pActionDict, "tooltip");
			const Glib::ustring icon        = safePyDictGetItem (pActionDict, "icon");
			const Glib::ustring accelerator = safePyDictGetItem (pActionDict, "accelerator");
			const Glib::ustring callback    = safePyDictGetItem (pActionDict, "callback");
			const Glib::ustring sensitivity = safePyDictGetItem (pActionDict, "sensitivity");

			Glib::ustring stockStr = "_stock:";

			Gtk::StockID stockId;
			if (icon.substr(0, stockStr.length()) == stockStr) {
				/* Stock icon */
				stockId = Gtk::StockID (icon.substr(stockStr.length(), icon.length()));
			} else if (icon.length()) {
				/* Icon from file */
				try {
					Glib::ustring const iconFile = owner_->findDataFile (icon);
					Gtk::IconSource iconSource;
					iconSource.set_pixbuf( Gdk::Pixbuf::create_from_file(iconFile) );
					iconSource.set_size(Gtk::ICON_SIZE_SMALL_TOOLBAR);
					iconSource.set_size_wildcarded(); //Icon may be scaled.
					Gtk::IconSet iconSet;
					iconSet.add_source (iconSource);
					stockId = Gtk::StockID (Glib::ustring("referencer") + Glib::ustring (name));
					iconFactory->add (stockId, iconSet);
				} catch(const Glib::Exception& ex) {
					/* File not found, show error icon */
					DEBUG (ex.what());
					stockId = Gtk::StockID (Gtk::Stock::DIALOG_ERROR);
				}
			}

			Glib::RefPtr<Gtk::Action> action = Gtk::Action::create(
				name, stockId, label, tooltip);

			action->set_data ("accelerator",    new Glib::ustring (accelerator), freeString);
			action->set_data ("callback",       new Glib::ustring (callback),    freeString);
			action->set_data ("sensitivity",    new Glib::ustring (sensitivity), freeString);

			actions_.push_back (action);
		}
		Py_DECREF (pActions);
	} else {
		DEBUG (String::ucompose ("No actions in %1", moduleName_));
	}



	DEBUG (String::ucompose ("successfully loaded %1", moduleName));

	loaded_ = true;
}

bool PythonPlugin::resolve (Document &doc)
{
	bool success = false;

	std::vector<PluginCapability::Identifier> ids = cap_.get();
	std::vector<PluginCapability::Identifier>::iterator it = ids.begin();
	std::vector<PluginCapability::Identifier>::iterator const end = ids.end();
	for (; it != end; ++it) {
		success = resolveID (doc, *it);
		if (success)
			break;
	}
	

	if (success)
		doc.getBibData().setType("article");

	return success;
}


/**
 * Invoke a plugin action and return true if the plugin 
 * modifies a document or the library
 */
bool PythonPlugin::doAction (Glib::ustring const function, std::vector<Document*> docs)
{
	/* Check the callback exists */
	if (!PyObject_HasAttrString (pMod_, (char*)function.c_str())) {
		DEBUG1 ("function %1 not found", function);
		return false;
	}

	/* Look up the callback function */
	PyObject *pActionFunc = PyObject_GetAttrString (pMod_, (char*)function.c_str());

	/* Construct the 'documents' argument */
	PyObject *pDocList = PyList_New (docs.size());
	std::vector<Document*>::iterator it = docs.begin ();
	std::vector<Document*>::iterator const end = docs.end ();
	for (int i = 0; it != end; ++it, ++i) {
		referencer_document *pDoc =
			PyObject_New (referencer_document, &t_referencer_document);
		pDoc->doc_ = (*it);
		PyList_SetItem (pDocList, i, (PyObject*)pDoc);
	}
	
	/* Build the argument tuple: (library, documents) */
	/* Library is nil for now */
	PyObject *pArgs = Py_BuildValue ("(i,O)", 0, pDocList);

	/* Invoke the python */
	PyObject *pReturn = PyObject_CallObject(pActionFunc, pArgs);
	Py_DECREF (pArgs);

	if (pReturn == NULL) {
		DEBUG ("PythonPlugin::doAction: NULL return value");
		displayException ();
		return false;
	} else {
		return pReturn == Py_True;
	}
}


bool PythonPlugin::updateSensitivity (Glib::ustring const function, std::vector<Document*> docs)
{
	/* Check the callback exists */
	if (!PyObject_HasAttrString (pMod_, (char*)function.c_str())) {
		/* No sensitivity function: always enable */
		return true;
	}

	/* Look up the callback function */
	PyObject *pActionFunc = PyObject_GetAttrString (pMod_, (char*)function.c_str());

	/* Construct the 'documents' argument */
	PyObject *pDocList = PyList_New (docs.size());
	std::vector<Document*>::iterator it = docs.begin ();
	std::vector<Document*>::iterator const end = docs.end ();
	for (int i = 0; it != end; ++it, ++i) {
		referencer_document *pDoc =
			PyObject_New (referencer_document, &t_referencer_document);
		pDoc->doc_ = (*it);
		PyList_SetItem (pDocList, i, (PyObject*)pDoc);
	}
	
	/* Build the argument tuple: (library, documents) */
	/* Library is nil for now */
	PyObject *pArgs = Py_BuildValue ("(i,O)", 0, pDocList);

	/* Invoke the python */
	PyObject *pReturn = PyObject_CallObject(pActionFunc, pArgs);
	Py_DECREF (pArgs);

	if (pReturn == NULL) {
		DEBUG ("PythonPlugin::doAction: NULL return value");
		displayException ();
		return false;
	} else {
		return pReturn == Py_True;
	}
}


Glib::ustring PythonPlugin::formatException ()
{
	PyObject *pErr = PyErr_Occurred ();
	if (pErr) {
		PyObject *ptype;
		PyObject *pvalue;
		PyObject *ptraceback;

		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		PyObject *pStr;
		pStr = PyObject_Str (ptype);
		Glib::ustring const exType = PyString_AsString (pStr);
		pStr = PyObject_Str (pvalue);
		Glib::ustring const exValue = PyString_AsString (pStr);

		Glib::ustring message = String::ucompose (
			_("%1: %2\n\n%3: %4\n%5: %6\n"),
			_("Exception"),
			 (exType),
			_("Module"),
			 (getShortName()),
			_("Explanation"),
			 (exValue)
			);

		exceptionLog_ += message;

		return message;
	} else {
		return Glib::ustring ();
	}
}


void PythonPlugin::printException ()
{
	DEBUG(formatException ());
}

void PythonPlugin::displayException ()
{
	PyObject *pErr = PyErr_Occurred ();
	if (pErr) {
		PyObject *ptype;
		PyObject *pvalue;
		PyObject *ptraceback;

		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		PyObject *pStr;
		pStr = PyObject_Str (ptype);
		Glib::ustring const exType = PyString_AsString (pStr);
		pStr = PyObject_Str (pvalue);
		Glib::ustring const exValue = PyString_AsString (pStr);

		Glib::ustring message = String::ucompose (
			_("<big><b>%1: %2</b></big>\n\n%3: %4\n%5: %6"),
			_("Exception"),
			Glib::Markup::escape_text (exType),
			_("Module"),
			Glib::Markup::escape_text (getShortName()),
			_("Explanation"),
			Glib::Markup::escape_text (exValue)
			);

		Gtk::MessageDialog dialog (
			message, true,
			Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE, true);

		dialog.run ();
	}
}


bool PythonPlugin::resolveID (Document &doc, PluginCapability::Identifier id)
{
	bool success = false;
	referencer_document *pDoc =
		PyObject_New (referencer_document, &t_referencer_document);
	pDoc->doc_ = &doc;

	PyObject *pArgs = NULL;
	switch (id) {
		case PluginCapability::DOI:
			if (!doc.hasField ("doi"))
				return false;
			pArgs = Py_BuildValue ("(Os)", pDoc, "doi");
		break;
		case PluginCapability::ARXIV:
			if (!doc.hasField ("eprint"))
				return false;
			pArgs = Py_BuildValue ("(Os)", pDoc, "arxiv");
		break;
		case PluginCapability::PUBMED:
			if (!doc.hasField ("pmid"))
				return false;
			pArgs = Py_BuildValue ("(Os)", pDoc, "pubmed");
		break;
		case PluginCapability::URL:
			if (!doc.hasField ("url"))
				return false;
			pArgs = Py_BuildValue ("(Os)", pDoc, "url");
		break;
		default:
			DEBUG1 ("PythonPlugin::resolveID: warning, unhandled id type %1", id);
			return false;
	}


	PyObject *pReturn = PyObject_CallObject(pGetFunc_, pArgs);
	Py_DECREF(pArgs);
	Py_DECREF (pDoc);


	if (pReturn != NULL) {
		Py_DECREF(pReturn);
		success = (pReturn == Py_True);
	} else {
		DEBUG ("PythonPlugin::resolveID: NULL return from PyObject_CallObject");
		displayException ();
	}


	return success;
}


Glib::ustring const PythonPlugin::getLongName ()
{
	return getPluginInfoField ("longname");
}


Glib::ustring const PythonPlugin::getAuthor ()
{
	return getPluginInfoField ("author");
}


Glib::ustring const PythonPlugin::getVersion ()
{
	return getPluginInfoField ("version");
}


Glib::ustring const PythonPlugin::getUI ()
{
	return getPluginInfoField ("ui");
}


Glib::ustring const PythonPlugin::getPluginInfoField (Glib::ustring const &targetKey)
{
	if (!loaded_)
		return Glib::ustring ();

	PyObject *keyStr = PyString_FromString (targetKey.c_str());
	if (PyDict_Contains (pPluginInfo_, keyStr)) {
		Py_DECREF (keyStr);
		return Glib::ustring (
			PyString_AsString (
				PyDict_GetItemString(
					pPluginInfo_,
					targetKey.c_str())));
	} else {
		Py_DECREF (keyStr);
		return Glib::ustring ();
	}
}


bool PythonPlugin::canConfigure ()
{
	if (pMod_)
		return PyObject_HasAttrString (pMod_, "referencer_config");
	else
		return false;
}


void PythonPlugin::doConfigure ()
{
	PyObject *confFunc = PyObject_GetAttrString (pMod_, "referencer_config");
	if (!confFunc)
		return;

	PyObject *pArgs = Py_BuildValue ("()");
	PyObject *pReturn = PyObject_CallObject(confFunc, pArgs);
	if (pArgs)
		Py_DECREF (pArgs);
	if (confFunc)
		Py_DECREF (confFunc);
	if (pReturn)
		Py_DECREF (pReturn);
}


bool PythonPlugin::hasError ()
{
	return !exceptionLog_.empty();
}


Glib::ustring PythonPlugin::getError ()
{
	return exceptionLog_;
}


bool PythonPlugin::canSearch ()
{
	if (pMod_)
		return PyObject_HasAttrString (pMod_, "referencer_search")
		       && PyObject_HasAttrString (pMod_, "referencer_search_result");
	else
		return false;
}


/**
 * Invoke pSearchFunc_ and cast results from list of 
 * dictionaries into vector of maps
 */
Plugin::SearchResults PythonPlugin::doSearch (Glib::ustring const &searchTerms)
{
	/* Look up search function */
	PyObject *searchFunc = PyObject_GetAttrString (pMod_, "referencer_search");
	if (!searchFunc)
		return Plugin::SearchResults();

	/* Invoke search function */
	PyObject *pArgs = Py_BuildValue ("(s)", searchTerms.c_str());
	PyObject *pReturn = PyObject_CallObject(searchFunc, pArgs);
	Py_DECREF (pArgs);
	
	/* Copy Python result into C++ structures */
	Plugin::SearchResults retval;
	int itemCount = PyList_Size (pReturn);
	for (int i = 0; i < itemCount; ++i) {

		/* Borrowed reference */
		PyObject *dict = PyList_GetItem (pReturn, i);
		std::map<Glib::ustring, Glib::ustring> result;

		/* Iterate over all items */
		PyObject *key, *value;
		Py_ssize_t pos = 0;
		while (PyDict_Next(dict, &pos, &key, &value)) {
			result[PyString_AsString(key)] = PyString_AsString(value);
		}

		retval.push_back(result);
	}
	Py_DECREF (pReturn);

	return retval;
}


Document PythonPlugin::getSearchResult (Glib::ustring const &token)
{
	DEBUG1 ("token '%1'", token);
	/* Look up result lookup function */
	PyObject *searchFunc = PyObject_GetAttrString (pMod_, "referencer_search_result");
	if (!searchFunc)
		return Document();

	/* Invoke result lookup function */
	PyObject *pArgs = Py_BuildValue ("(s)", token.c_str());
	PyObject *pReturn = PyObject_CallObject(searchFunc, pArgs);
	Py_DECREF (pArgs);

	if (pReturn) {
		/* Compose Document from returned fields */
		Document doc;
		int itemCount = PyList_Size (pReturn);
		DEBUG1 ("got %1 fields", itemCount);

		PyObject *dict = pReturn;
		/* Borrowed reference */
		std::map<Glib::ustring, Glib::ustring> result;

		/* Iterate over all items */
		PyObject *key, *value;
		Py_ssize_t pos = 0;
		while (PyDict_Next(dict, &pos, &key, &value)) {
			DEBUG2 ("Setting %1 %2", PyString_AsString(key), PyString_AsString(value));
			doc.setField (PyString_AsString(key), PyString_AsString(value));
		}

		Py_DECREF (pReturn);

		return doc;
	} else {
		DEBUG ("PythonPlugin::getSearchResult: NULL return value");
		displayException();
	}
}

