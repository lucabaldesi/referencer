
#ifndef LINKER_H
#define LINKER_H

#include <glibmm/ustring.h>

class Document;

class Linker {
	public:
	virtual bool canLink (Document const *doc) {return false;};
	virtual void doLink (Document *doc);
	virtual Glib::ustring getLabel () {return Glib::ustring();};

	Linker() {}
	virtual ~Linker() {}
};


class DoiLinker : public Linker {
	public:
	bool canLink (Document const *doc);
	void doLink (Document *doc);
	Glib::ustring getLabel ();
};

class ArxivLinker : public Linker {
	public:
	bool canLink (Document const *doc);
	void doLink (Document *doc);
	Glib::ustring getLabel ();
};


class UrlLinker : public Linker {
	public:
	bool canLink (Document const *doc);
	void doLink (Document *doc);
	Glib::ustring getLabel ();
};


class PubmedLinker : public Linker {
	public:
	bool canLink (Document const *doc);
	void doLink (Document *doc);
	Glib::ustring getLabel ();
};


class GoogleLinker : public Linker {
	public:
	bool canLink (Document const *doc);
	void doLink (Document *doc);
	Glib::ustring getLabel ();
};

#endif
