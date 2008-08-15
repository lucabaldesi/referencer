
#include <iostream>

#include <Python.h>
#include <structmember.h>

#include "Document.h"

#include "PythonDocument.h"

static PyObject *referencer_document_get_key (PyObject *self, PyObject *args)
{
	Glib::ustring value = ((referencer_document*)self)->doc_->getKey ();
	return PyString_FromString(value.c_str());
}


static PyObject *referencer_document_set_key (PyObject *self, PyObject *args)
{
	PyObject *value = PyTuple_GetItem (args, 0);
	((referencer_document*)self)->doc_->setKey (PyString_AsString(value));
	return Py_BuildValue ("i", 0);
}


static PyObject *referencer_document_get_filename (PyObject *self, PyObject *args)
{
	Glib::ustring value = ((referencer_document*)self)->doc_->getFileName ();
	return PyString_FromString(value.c_str());
}


static PyObject *referencer_document_set_filename (PyObject *self, PyObject *args)
{
	PyObject *value = PyTuple_GetItem (args, 0);
	((referencer_document*)self)->doc_->setFileName (PyString_AsString(value));
	return Py_BuildValue ("i", 0);
}


static PyObject *referencer_document_get_field (PyObject *self, PyObject *args)
{
	PyObject *fieldName = PyTuple_GetItem (args, 0);
	Glib::ustring value = ((referencer_document*)self)->doc_->getField (PyString_AsString(fieldName));
	return PyString_FromString(value.c_str());
}


static PyObject *referencer_document_set_field (PyObject *self, PyObject *args)
{
	PyObject *fieldName = PyTuple_GetItem (args, 0);
	PyObject *value = PyTuple_GetItem (args, 1);
	((referencer_document*)self)->doc_->setField (PyString_AsString(fieldName), PyString_AsString(value));
	return Py_BuildValue ("i", 0);
}


static PyObject *referencer_document_parse_bibtex (PyObject *self, PyObject *args)
{
	PyObject *bibtex = PyTuple_GetItem (args, 0);
	((referencer_document*)self)->doc_->parseBibtex (PyString_AsString(bibtex));
	return Py_BuildValue ("i", 0);
}


static void referencer_document_dealloc (PyObject *self)
{
	std::cerr << ">> referencer_document_dealloc\n";
}

static PyObject *referencer_document_string (PyObject *self)
{
	return PyString_FromString ("Referencer object representing a single document");
}

static int referencer_document_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	std::cerr << ">> referencer_document_init\n";

	return 0;
}


static PyMemberDef referencer_document_members[] = {
	{"ptr", T_INT, offsetof(referencer_document, doc_), 0, "a pointer"},
	{NULL}
};

static PyMethodDef referencer_document_methods[] = {
	{"get_field", referencer_document_get_field, METH_VARARGS, "Get a field"},
	{"set_field", referencer_document_set_field, METH_VARARGS, "Set a field"},
	{"get_key", referencer_document_get_key, METH_VARARGS, "Get the key"},
	{"set_key", referencer_document_set_key, METH_VARARGS, "Set the key"},
	{"get_filename", referencer_document_get_filename, METH_VARARGS, "Get the filename"},
	{"set_filename", referencer_document_set_filename, METH_VARARGS, "Set the filename"},
	{"parse_bibtex", referencer_document_parse_bibtex, METH_VARARGS, "Set fields from bibtex string"},
	{NULL, NULL, 0, NULL}
};

PyTypeObject t_referencer_document = {
	PyObject_HEAD_INIT(NULL)
	0,
	"referencer.document",
	sizeof (referencer_document),
	0,
	referencer_document_dealloc,
	0,
	0,
	0,
	0,
	referencer_document_string,
	0,
	0,
	0,
	0,
	0,
	0,
	PyObject_GenericGetAttr,
	PyObject_GenericSetAttr,
	0,
	Py_TPFLAGS_DEFAULT,
	"Referencer Document",
	0,
	0,
	0,
	0,
	0,
	0,
	referencer_document_methods,
	referencer_document_members,
	0,
	0,
	0,
	0,
	0,
	0,
	referencer_document_init,
	PyType_GenericAlloc,
	PyType_GenericNew,
	_PyObject_Del
};
