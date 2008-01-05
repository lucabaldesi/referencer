
#include <Python.h>
#include <structmember.h>

#include "Document.h"

#include "PythonDocument.h"

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
