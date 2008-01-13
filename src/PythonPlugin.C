

#include <iostream>

#include <Python.h>

#include <glibmm/ustring.h>
#include <glibmm/i18n.h>
#include "ucompose.hpp"

#include "Document.h"

#include "PythonDocument.h"
#include "PythonPlugin.h"




PythonPlugin::PythonPlugin()
{
	pGetFunc_ = NULL;
	pActionFunc_ = NULL;
	pMod_ = NULL;
}

PythonPlugin::~PythonPlugin()
{
	if (loaded_) {
		if (pGetFunc_ != NULL)
			Py_DECREF (pGetFunc_);
		if (pActionFunc_ != NULL)
			Py_DECREF (pActionFunc_);
		if (pPluginInfo_ != NULL)
			Py_DECREF (pPluginInfo_);
		Py_DECREF (pMod_);
	}
}



void PythonPlugin::load (std::string const &moduleName)
{
	moduleName_ = moduleName;
	PyObject *pName = PyString_FromString(moduleName.c_str());
	if (!pName) {
		std::cerr << "Plugin::load: Couldn't construct module name\n";
		return;
	}
	pMod_ = PyImport_Import(pName);
	Py_DECREF(pName);

	if (!pMod_) {
		std::cerr << "Plugin::load: Couldn't import module\n";
		return;
	}

	pPluginInfo_ = PyObject_GetAttrString (pMod_, "referencer_plugin_info");
	if (!pPluginInfo_) {
		std::cerr << "Plugin::load: Couldn't find plugin info\n";
		Py_DECREF (pMod_);
		return;
	}


	PyObject *pCaps = PyObject_GetAttrString (pMod_, "referencer_plugin_capabilities");
	if (!pCaps) {
		std::cerr << "Plugin::load: Couldn't find plugin capabilities\n";
		Py_DECREF (pMod_);
		return;
	}

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
		else if (str == "document_action")
			cap_.add(PluginCapability::DOCUMENT_ACTION);
	}
	Py_DECREF (pCaps);

	if (cap_.has(PluginCapability::DOCUMENT_ACTION)) { 
		pActionFunc_ = PyObject_GetAttrString (pMod_, "do_action");
		if (!pActionFunc_) {
			std::cerr << "Plugin::load: Couldn't find action callback\n";
			Py_DECREF (pMod_);
			return;
		}
	}

	if (cap_.has(PluginCapability::DOI) || cap_.has(PluginCapability::ARXIV) || 
		cap_.has(PluginCapability::PUBMED)) { 
		pGetFunc_ = PyObject_GetAttrString (pMod_, "resolve_metadata");
		if (!pGetFunc_) {
			std::cerr << "Plugin::load: Couldn't find resolver\n";
			Py_DECREF (pMod_);
			return;
		}
	}

	std::cerr << "Plugin::load: successfully loaded '" << moduleName << "'\n";

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

bool PythonPlugin::doAction (std::vector<Document*> docs)
{
	PyObject *pDocList = PyList_New (docs.size());
	std::vector<Document*>::iterator it = docs.begin ();
	std::vector<Document*>::iterator const end = docs.end ();
	for (int i = 0; it != end; ++it, ++i) {
		referencer_document *pDoc =
			PyObject_New (referencer_document, &t_referencer_document);
		pDoc->doc_ = (*it);
		PyList_SetItem (pDocList, i, (PyObject*)pDoc);
	}
	
	PyObject *pArgs = Py_BuildValue ("(O)", pDocList);
	PyObject *pReturn = PyObject_CallObject(pActionFunc_, pArgs);
	Py_DECREF (pArgs);

	if (pReturn == NULL) {
		std::cerr << "PythonPlugin::doAction: NULL return value\n";
		displayException ();
		return false;
	} else {
		return pReturn == Py_True;
	}
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
		default:
			std::cerr << "PythonPlugin::resolveID: warning, unhandled id type "
				<< id << "\n";
			return false;
	}


	PyObject *pReturn = PyObject_CallObject(pGetFunc_, pArgs);
	Py_DECREF(pArgs);
	Py_DECREF (pDoc);


	if (pReturn != NULL) {
		Py_DECREF(pReturn);
		success = (pReturn == Py_True);
	} else {
		std::cerr << "PythonPlugin::resolveID: NULL return from PyObject_CallObject\n";
		displayException ();
	}


	return success;
}


Glib::ustring const PythonPlugin::getLongName ()
{
	return getPluginInfoField ("longname");
}


Glib::ustring const PythonPlugin::getActionTooltip ()
{
	return getPluginInfoField ("tooltip");
}


Glib::ustring const PythonPlugin::getActionText ()
{
	return getPluginInfoField ("action");
}

Glib::ustring const PythonPlugin::getActionIcon ()
{
	return getPluginInfoField ("icon");
}


Glib::ustring const PythonPlugin::getPluginInfoField (Glib::ustring const &targetKey)
{
	if (!loaded_)
		return Glib::ustring ();

	int const N = PyList_Size (pPluginInfo_);

	for (int i = 0; i < N; ++i) {
		PyObject *pItem = PyList_GetItem (pPluginInfo_, i);
		// Need decrefs on these getitems?
		const char *cKey = PyString_AsString(PyList_GetItem (pItem, 0));
		const char *cValue = PyString_AsString(PyList_GetItem (pItem, 1));

		Glib::ustring key;
		Glib::ustring value;
		if (cKey)
			key = Glib::ustring (cKey);
		if (cValue)
			value = Glib::ustring (cValue);

		if (key == targetKey)
			return value;
	}

	return Glib::ustring ();
}


