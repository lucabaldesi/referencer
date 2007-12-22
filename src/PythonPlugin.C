

#include <iostream>

#include <Python.h>

#include <glibmm/ustring.h>

#include "BibData.h"

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

	pGetFunc_ = PyObject_GetAttrString (pMod_, "metadata_from_doi");
	if (!pGetFunc_) {
		std::cerr << "Plugin::load: Couldn't find doi resolver\n";
		Py_DECREF (pMod_);
		return;
	}

	pPluginInfo_ = PyObject_GetAttrString (pMod_, "referencer_plugin_info");
	if (!pPluginInfo_) {
		std::cerr << "Plugin::load: Couldn't find plugin info\n";
		Py_DECREF (pMod_);
		return;
	}


	/* FIXME assume all python plugins are DOI only */
	cap_.add(PluginCapability::DOI);

	std::cerr << "Plugin::load: successfully loaded '" << moduleName << "'\n";

	loaded_ = true;
	moduleName_ = moduleName;
}

bool PythonPlugin::resolve (BibData &bib)
{
	bool success = false;

	PyObject *pDoi = PyString_FromString (bib.getDoi().c_str());

	PyObject *pArgs = PyTuple_New(1);
	PyTuple_SetItem (pArgs, 0, pDoi);

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

			if (key == "title") {
				bib.setTitle (value);
			} else if (key == "authors") {
				bib.setAuthors (value);
			} else if (key == "journal") {
				bib.setJournal (value);
			} else if (key == "volume") {
				bib.setVolume (value);
			} else if (key == "issue") {
				bib.setIssue (value);
			} else if (key == "year") {
				bib.setYear (value);
			} else if (key == "pages") {
				bib.setPages (value);
			} else {
				bib.addExtra (key, value);
			}
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


