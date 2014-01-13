
#include "Document.h"
#include "Utility.h"

#include "ThumbnailGenerator.h"


Glib::RefPtr<Gdk::Pixbuf> ThumbnailGenerator::defaultthumb_;
Glib::RefPtr<Gdk::Pixbuf> ThumbnailGenerator::thumbframe_;

ThumbnailGenerator &ThumbnailGenerator::instance ()
{
	static ThumbnailGenerator inst;

	return inst;
}

ThumbnailGenerator::ThumbnailGenerator ()
{
	Glib::RefPtr<Gdk::Pixbuf> thumbnail = Gdk::Pixbuf::create_from_file
		(Utility::findDataFile ("unknown-document.png"));
	float const desiredwidth = 64.0 + 9;
	int oldwidth = thumbnail->get_width ();
	int oldheight = thumbnail->get_height ();
	int newwidth = (int)desiredwidth;
	int newheight = (int)((float)oldheight * (desiredwidth / (float)oldwidth));
	thumbnail = thumbnail->scale_simple (
		newwidth, newheight, Gdk::INTERP_BILINEAR);
	defaultthumb_ = thumbnail;

	thumbframe_ = Gdk::Pixbuf::create_from_file (
		Utility::findDataFile ("thumbnail_frame.png"));

	//We run when idle
	Glib::signal_idle().connect(
		sigc::mem_fun (this, &ThumbnailGenerator::run));
}


bool ThumbnailGenerator::run ()
{
	//Stuff to do?
	Glib::ustring file;

	bool gotJob = false;
	if (taskList_.size () > 0) {
		std::multimap<Glib::ustring, Document *>::iterator it = taskList_.begin ();
		std::pair<Glib::ustring, Document *> task = *it;

		file = task.first;
		gotJob = true;
		DEBUG("gotJob: '%1'", file);
		lookupThumb_async(file);
	}

	return false;
}

Glib::RefPtr<Gdk::Pixbuf> ThumbnailGenerator::getThumbnailSynchronous (Glib::ustring const &file)
{
	return lookupThumb (file);
}

/* Async lookup thumbnail routines {{{ */
void ThumbnailGenerator::lookupThumb_async (Glib::ustring const &file)
{
	currentFile_ = file;
	if (!file.empty ()) {
		currentUri_ = Gio::File::create_for_uri (currentFile_);

		currentUri_->query_info_async(sigc::mem_fun(this, &ThumbnailGenerator::_onQueryInfoAsyncReady), "thumbnail::path");
	}
	else {
		_taskAbort();
	}
}

void ThumbnailGenerator::_onQueryInfoAsyncReady(Glib::RefPtr<Gio::AsyncResult>& result)
{
	try {
		Glib::RefPtr<Gio::FileInfo> fileinfo = currentUri_->query_info_finish(result);
		Glib::ustring thumbnail_path = fileinfo->get_attribute_as_string("thumbnail::path");
		thumbnail_file_ = Gio::File::create_for_path(thumbnail_path);

		if (thumbnail_path != "") {
			int const desiredwidth = 64 + 9;
			Glib::RefPtr<Gdk::Pixbuf> result = 
				Gdk::Pixbuf::create_from_stream_at_scale(
					thumbnail_file_->read(),
					desiredwidth,
					-1,
					true);
			_setDocumentThumbnail (result);
		}
		else {
			_taskAbort();
		}

	}
	catch(const Glib::Exception& ex)
	{
		DEBUG("Exception caught: %1", ex.what());
		_taskAbort();
	}
}


void ThumbnailGenerator::_setDocumentThumbnail(Glib::RefPtr<Gdk::Pixbuf>& result)
{
	int const left_offset = 3;
	int const top_offset = 3;
	int const right_offset = 6;
	int const bottom_offset = 6;
	Glib::RefPtr<Gdk::Pixbuf> thumbnail = Utility::eelEmbedImageInFrame (
		result, thumbframe_,
		left_offset, top_offset, right_offset, bottom_offset);

	typedef std::multimap<Glib::ustring, Document*>::iterator Iterator;
	const std::pair<Iterator,Iterator> docs = taskList_.equal_range(currentFile_);
	for (Iterator i = docs.first; i!= docs.second; ++i) {
		Document *doc = i->second;
		doc->setThumbnail (thumbnail);
	}

	_taskDone();
}



void ThumbnailGenerator::_taskDone ()
{
	taskList_.erase (currentFile_);
	currentFile_ = "";
	this->run();
}

void ThumbnailGenerator::_taskAbort ()
{
	Glib::RefPtr<Gdk::Pixbuf> thumbnail;
	thumbnail = defaultthumb_;

	typedef std::multimap<Glib::ustring, Document*>::iterator Iterator;
	const std::pair<Iterator,Iterator> docs = taskList_.equal_range(currentFile_);
	for (Iterator i = docs.first; i!= docs.second; ++i) {
		Document *doc = i->second;
		doc->setThumbnail (thumbnail);
	}
	_taskDone();
}
/* }}} */


// XXX old code: drop at some point?
Glib::RefPtr<Gdk::Pixbuf> ThumbnailGenerator::lookupThumb (Glib::ustring const &file)
{
	Glib::RefPtr<Gdk::Pixbuf> thumbnail;
	GdkPixbuf* thumbnail_c;

	Glib::RefPtr<Gio::File> uri = Gio::File::create_for_uri (file);


	if (!file.empty () && Utility::uriIsFast (uri) && uri->query_exists()) {
		Glib::RefPtr<Gio::FileInfo> fileinfo = uri->query_info ();

		Glib::ustring thumbnail_path = fileinfo->get_attribute_as_string("thumbnail::path");
		//DEBUG("gio thumbnail path: '%1'", thumbnail_path);
		if (thumbnail_path != "") {
			thumbnail = Gdk::Pixbuf::create_from_file(thumbnail_path);
		}
	}

	// Disallow crazy sized thumbs
	int const minimumDimension = 8;

	if (!thumbnail || thumbnail->get_width() < minimumDimension || thumbnail->get_height() < minimumDimension) {
		if (defaultthumb_) {
			thumbnail = defaultthumb_;
		} else {
			thumbnail = Gdk::Pixbuf::create_from_file
				(Utility::findDataFile ("unknown-document.png"));
			float const desiredwidth = 64.0 + 9;
			int oldwidth = thumbnail->get_width ();
			int oldheight = thumbnail->get_height ();
			int newwidth = (int)desiredwidth;
			int newheight = (int)((float)oldheight * (desiredwidth / (float)oldwidth));
			thumbnail = thumbnail->scale_simple (
				newwidth, newheight, Gdk::INTERP_BILINEAR);
			defaultthumb_ = thumbnail;
		}
	} else {
		float const desiredwidth = 64.0;
		int oldwidth = thumbnail->get_width ();
		int oldheight = thumbnail->get_height ();
		int newwidth = (int)desiredwidth;
		int newheight = (int)((float)oldheight * (desiredwidth / (float)oldwidth));
		thumbnail = thumbnail->scale_simple (
			newwidth, newheight, Gdk::INTERP_BILINEAR);

		if (!thumbframe_) {
			thumbframe_ = Gdk::Pixbuf::create_from_file (
				Utility::findDataFile ("thumbnail_frame.png"));
		}

		int const left_offset = 3;
		int const top_offset = 3;
		int const right_offset = 6;
		int const bottom_offset = 6;
		thumbnail = Utility::eelEmbedImageInFrame (
			thumbnail, thumbframe_,
			left_offset, top_offset, right_offset, bottom_offset);
	}

	return thumbnail;
}

void ThumbnailGenerator::registerRequest (Glib::ustring const &file, Document *doc)
{
	taskList_.insert (std::pair<Glib::ustring, Document*>(file,doc));
}

void ThumbnailGenerator::deregisterRequest (Document *doc)
{
	/* Erase exactly one entry which has its second field equal to doc */
	typedef std::multimap<Glib::ustring, Document*>::iterator Iterator;
	for (Iterator i = taskList_.begin(); i!= taskList_.end(); ++i) {
		if (i->second == doc) {
			taskList_.erase (i);
			return;
		}
	}
}


