
#ifndef DOCUMENTTYPES_H
#define DOCUMENTTYPES_H

#include <map>
#include <vector>

#include <glibmm/ustring.h>

#include "CaseFoldCompare.h"

class DocumentField {
	// for the moment internalName is bibtexName and vice versa
	public:
	Glib::ustring internalName_;
	Glib::ustring displayName_;
	bool shortField_;

	DocumentField (
		Glib::ustring internalName,
		Glib::ustring displayName,
		bool shortField)
	{
		internalName_ = internalName;
		displayName_ = displayName;
		shortField_ = shortField;
	}

	DocumentField () {}
};


class DocumentType {
	public:
	std::vector<DocumentField> requiredFields_;
	std::vector<DocumentField> optionalFields_;
	Glib::ustring bibtexName_;
	Glib::ustring displayName_;
};


class DocumentTypeManager {
	public:
	typedef std::map<Glib::ustring, DocumentField, casefoldCompare> FieldsMap;
	typedef std::map<Glib::ustring, DocumentType, casefoldCompare> TypesMap;
	private:

	FieldsMap documentFields_;
	TypesMap documentTypes_;

	void registerField (
		Glib::ustring internalName,
		Glib::ustring displayName,
		bool shortField);

	void addField (
		DocumentType &type,
		Glib::ustring internalName,
		bool required);

	public:
	DocumentTypeManager ();
	DocumentType getType (Glib::ustring bibtexName);
	// Not const so that one can [] on it
	TypesMap &getTypes () {return documentTypes_;}
};

#endif
