
#ifndef PYTHONDOCUMENT_H
#define PYTHONDOCUMENT_H

#include <Python.h>

class Document;

/* Does this need to be in the header? */
typedef struct {
	PyObject_HEAD
	Document *doc_;
} referencer_document;


extern PyTypeObject t_referencer_document;

#endif
