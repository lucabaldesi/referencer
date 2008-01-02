

#include <iostream>

#include <Python.h>

#include <glibmm/ustring.h>

#include "Document.h"

#include "PythonPlugin.h"




PythonPlugin::PythonPlugin()
{
	pGetFunc_ = NULL;
	pMod_ = NULL;
}

PythonPlugin::~PythonPlugin()
{
	if (loaded_) {
		Py_DECREF (pGetFunc_);
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

	pGetFunc_ = PyObject_GetAttrString (pMod_, "resolve_metadata");
	if (!pGetFunc_) {
		std::cerr << "Plugin::load: Couldn't find resolver\n";
		Py_DECREF (pMod_);
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
	}
	Py_DECREF (pCaps);

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

bool PythonPlugin::resolveID (Document &doc, PluginCapability::Identifier id)
{
	bool success = false;

	PyObject *pArgs = NULL;
	switch (id) {
		case PluginCapability::DOI:
			if (!doc.hasField ("doi"))
				return false;
			pArgs = Py_BuildValue ("(ss)", doc.getField("doi").c_str(), "doi");
		break;
		case PluginCapability::ARXIV:
			if (!doc.hasField ("eprint"))
				return false;
			pArgs = Py_BuildValue ("(ss)", doc.getField("eprint").c_str(), "arxiv");
		break;
		case PluginCapability::PUBMED:
			if (!doc.hasField ("pmid"))
				return false;
			pArgs = Py_BuildValue ("(ss)", doc.getField("pmid").c_str(), "pubmed");
		break;
		default:
			std::cerr << "PythonPlugin::resolveID: warning, unhandled id type "
				<< id << "\n";
			return false;
	}


	PyObject *pMetaData = PyObject_CallObject(pGetFunc_, pArgs);
	Py_DECREF(pArgs);

	if (pMetaData != NULL) {
		int const N = PyList_Size (pMetaData);

		if (N > 0)
			success = true;

		for (int i = 0; i < N; ++i) {
			PyObject *pItem = PyList_GetItem (pMetaData, i);
			const char *cKey = PyString_AsString(PyList_GetItem (pItem, 0));
			const char *cValue = PyString_AsString(PyList_GetItem (pItem, 1));

			Glib::ustring key;
			Glib::ustring value;
			if (cKey)
				key = Glib::ustring (cKey);
			if (cValue)
				value = Glib::ustring (cValue);

			doc.setField (key, value);
		}
		Py_DECREF(pMetaData);
	} else {
		std::cerr << "NULL return from PyObject_CallObject\n";
		PyObject *pErr = PyErr_Occurred ();
		if (pErr) {
			PyObject *ptype;
			PyObject *pvalue;
			PyObject *ptraceback;

			PyErr_Fetch(&ptype, &pvalue, &ptraceback);

			std::string exceptionText;
			PyObject *pStr;
			pStr = PyObject_Str (ptype);
			exceptionText += PyString_AsString (pStr);
			exceptionText += ", ";
			pStr = PyObject_Str (pvalue);
			exceptionText += PyString_AsString (pStr);

			/*
			Transfer::Exception ex(exceptionText);
			Utility::exceptionDialog (&ex, _("Downloading metadata"));*/
		}
	}

	return success;
}


Glib::ustring const PythonPlugin::getLongName ()
{
	return getPluginInfoField ("longname");
}


Glib::ustring const PythonPlugin::getPluginInfoField (Glib::ustring const &targetKey)
{
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


