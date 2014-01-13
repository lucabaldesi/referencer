
#ifndef THUMBNAILGENERATOR_H
#define THUMBNAILGENERATOR_H

#include <glibmm.h>
#include <gdkmm.h>
#include <map>


class Document;

class ThumbnailGenerator
{
	std::multimap<Glib::ustring, Document *> taskList_;
	Glib::ustring currentFile_;
	Glib::RefPtr<Gio::File> currentUri_;
	Glib::RefPtr<Gio::File> thumbnail_file_;

	static Glib::RefPtr<Gdk::Pixbuf> defaultthumb_;
	static Glib::RefPtr<Gdk::Pixbuf> thumbframe_;

	Glib::RefPtr<Gdk::Pixbuf> lookupThumb (Glib::ustring const &file);
	void lookupThumb_async (Glib::ustring const &file);
	void _onQueryInfoAsyncReady(Glib::RefPtr<Gio::AsyncResult>& result);
	void _setDocumentThumbnail(Glib::RefPtr<Gdk::Pixbuf>& result);
	void _taskDone();
	void _taskAbort();

	ThumbnailGenerator ();
	bool run ();

	public:
	void registerRequest (Glib::ustring const &file, Document *doc);
	void deregisterRequest (Document *doc);
	Glib::RefPtr<Gdk::Pixbuf> getThumbnailSynchronous (Glib::ustring const &file);

	static ThumbnailGenerator &instance ();
};

#endif
