
#ifndef THUMBNAILGENERATOR_H
#define THUMBNAILGENERATOR_H

#include <glibmm.h>
#include <gdkmm.h>
#include <map>


class Document;

class ThumbnailGenerator
{
	Glib::Mutex taskLock_;
	std::multimap<Glib::ustring, Document *> taskList_;

	static Glib::RefPtr<Gdk::Pixbuf> defaultthumb_;
	static Glib::RefPtr<Gdk::Pixbuf> thumbframe_;

	void mainLoop ();
	Glib::RefPtr<Gdk::Pixbuf> lookupThumb (Glib::ustring const &file);

	ThumbnailGenerator ();
	void run ();

	public:
	void registerRequest (Glib::ustring const &file, Document *doc);
	void deregisterRequest (Document *doc);
	Glib::RefPtr<Gdk::Pixbuf> getThumbnailSynchronous (Glib::ustring const &file);

	static ThumbnailGenerator &instance ();
};

#endif
