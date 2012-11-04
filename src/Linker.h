
#ifndef LINKER_H
#define LINKER_H

#include <glibmm/ustring.h>

class Document;
class RefWindow;

class Linker {
	public:
	virtual bool canLink (Document const *doc) {return false;};
	virtual void doLink (Document *doc);
	virtual Glib::ustring getURL (Document *doc);
	virtual Glib::ustring getLabel () {return Glib::ustring();};
	virtual Glib::ustring getName () = 0;

	void createUI (RefWindow *window, DocumentView *view);

	Linker() {}
	virtual ~Linker() {}
};


class DoiLinker : public Linker {
	public:
	bool canLink (Document const *doc);
	virtual Glib::ustring getURL (Document *doc);
	virtual Glib::ustring getName () {return "doi";}
	Glib::ustring getLabel ();
};

class ArxivLinker : public Linker {
	public:
	bool canLink (Document const *doc);
	virtual Glib::ustring getURL (Document *doc);
	virtual Glib::ustring getName () {return "arxiv";}
	Glib::ustring getLabel ();
};


class UrlLinker : public Linker {
	public:
	bool canLink (Document const *doc);
	virtual Glib::ustring getURL (Document *doc);
	virtual Glib::ustring getName () {return "url";}
	Glib::ustring getLabel ();
};


class PubmedLinker : public Linker {
	public:
	bool canLink (Document const *doc);
	virtual Glib::ustring getURL (Document *doc);
	virtual Glib::ustring getName () {return "pubmed";}
	Glib::ustring getLabel ();
};


class GoogleLinker : public Linker {
	public:
	bool canLink (Document const *doc);
	virtual Glib::ustring getURL (Document *doc);
	virtual Glib::ustring getName () {return "google";}
	Glib::ustring getLabel ();
};

#endif
