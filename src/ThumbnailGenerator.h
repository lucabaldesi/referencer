
#ifndef THUMBNAILGENERATOR_H
#define THUMBNAILGENERATOR_H

#include <glibmm.h>
#include <gdkmm.h>
#include <libgnomeuimm.h>
#include <map>

class Document;

class ThumbnailGenerator
{
	Glib::Mutex taskLock_;
	std::multimap<Glib::ustring, Document *> taskList_;

	static Glib::RefPtr<Gdk::Pixbuf> defaultthumb_;
	static Glib::RefPtr<Gdk::Pixbuf> thumbframe_;
	static Glib::RefPtr<Gnome::UI::ThumbnailFactory> thumbfac_;

	void mainLoop ();
	Glib::RefPtr<Gdk::Pixbuf> lookupThumb (Glib::ustring const &file);

	public:
	ThumbnailGenerator ();
	void run ();
	void registerRequest (Glib::ustring const &file, Document *doc);
	void deregisterRequest (Document *doc);
};

#endif
