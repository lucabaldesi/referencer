
#include <libgnomevfsmm.h>

#include "Document.h"
#include "Utility.h"

#include "ThumbnailGenerator.h"


Glib::RefPtr<Gdk::Pixbuf> ThumbnailGenerator::defaultthumb_;
Glib::RefPtr<Gdk::Pixbuf> ThumbnailGenerator::thumbframe_;
Glib::RefPtr<Gnome::UI::ThumbnailFactory> ThumbnailGenerator::thumbfac_;

ThumbnailGenerator &ThumbnailGenerator::instance ()
{
	static ThumbnailGenerator inst;

	return inst;
}

ThumbnailGenerator::ThumbnailGenerator ()
{
	Glib::signal_idle().connect(
		sigc::bind_return (sigc::mem_fun (this, &ThumbnailGenerator::run), false));
}


void ThumbnailGenerator::run ()
{
	Glib::Thread::create(
		sigc::mem_fun (*this, &ThumbnailGenerator::mainLoop), false);
}

void ThumbnailGenerator::mainLoop ()
{
	for (;;) {
		Glib::ustring file;

		bool gotJob = false;
		taskLock_.lock ();
		if (taskList_.size () > 0) {
			std::multimap<Glib::ustring, Document *>::iterator it = taskList_.begin ();
			std::pair<Glib::ustring, Document *> task = *it;

			file = task.first;
			gotJob = true;
		}

		taskLock_.unlock ();

		if (!gotJob) {
			sleep (1);
			continue;
		}
		
		Glib::RefPtr<Gdk::Pixbuf> result;
		gdk_threads_enter ();
		result = lookupThumb (file);
		gdk_threads_leave ();
		
		if (result) {
			gdk_threads_enter ();
			taskLock_.lock ();

			typedef std::multimap<Glib::ustring, Document*>::iterator Iterator;
			const std::pair<Iterator,Iterator> docs = taskList_.equal_range(file);
			for (Iterator i = docs.first; i!= docs.second; ++i) {
				Document *doc = i->second;
				doc->setThumbnail (result);
			}

			taskList_.erase (file);

			taskLock_.unlock ();
			gdk_threads_leave ();
			sleep (0.1);
		}
	}
}

Glib::RefPtr<Gdk::Pixbuf> ThumbnailGenerator::getThumbnailSynchronous (Glib::ustring const &file)
{
	return lookupThumb (file);
}

Glib::RefPtr<Gdk::Pixbuf> ThumbnailGenerator::lookupThumb (Glib::ustring const &file)
{
	Glib::RefPtr<Gdk::Pixbuf> thumbnail;

	Glib::RefPtr<Gnome::Vfs::Uri> uri = Gnome::Vfs::Uri::create (file);
	if (!file.empty () && Utility::uriIsFast (uri) && uri->uri_exists()) {
		Glib::RefPtr<Gnome::Vfs::FileInfo> fileinfo = uri->get_file_info ();
		time_t mtime = fileinfo->get_modification_time ();

		if (!thumbfac_)
			thumbfac_ = Gnome::UI::ThumbnailFactory::create (Gnome::UI::THUMBNAIL_SIZE_NORMAL);

		Glib::ustring thumbfile;
		thumbfile = thumbfac_->lookup (file, mtime);

		/*
		 * Upsettingly, this leads to two full opens-read-close operations 
		 * on the thumbnail.  ThumbnailFactory does one, presumably to 
		 * read the PNG metadata, then we do it again to get the 
		 * image itself.
		 */

		// Should we be using Gnome::UI::icon_lookup_sync?
		if (thumbfile.empty()) {
			DEBUG ("Couldn't find thumbnail:'%1'", file);
			if (thumbfac_->has_valid_failed_thumbnail (file, mtime)) {
				DEBUG ("Has valid failed thumbnail: '%1'", file);
			} else {
				DEBUG ("Generate thumbnail: '%1'", file);
				Glib::ustring mimetype = 
					(uri->get_file_info (Gnome::Vfs::FILE_INFO_GET_MIME_TYPE))->get_mime_type ();

				thumbnail = thumbfac_->generate_thumbnail (file, mimetype);
				if (thumbnail) {
					thumbfac_->save_thumbnail (thumbnail, file, mtime);
				} else {
					DEBUG ("Failed to generate thumbnail: '%1'", file);
					thumbfac_->create_failed_thumbnail (file, mtime);
				}
			}

		} else {
			thumbnail = Gdk::Pixbuf::create_from_file (thumbfile);
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

/*
 * Always called holding gdk_threads lock
 */
void ThumbnailGenerator::registerRequest (Glib::ustring const &file, Document *doc)
{
	taskLock_.lock ();
	taskList_.insert (std::pair<Glib::ustring, Document*>(file,doc));
	taskLock_.unlock ();
}

/*
 * Always called holding gdk_threads lock
 */
void ThumbnailGenerator::deregisterRequest (Document *doc)
{
	taskLock_.lock ();

	/* Erase exactly one entry which has its second field equal to doc */
	typedef std::multimap<Glib::ustring, Document*>::iterator Iterator;
	for (Iterator i = taskList_.begin(); i!= taskList_.end(); ++i) {
		if (i->second == doc) {
			taskList_.erase (i);
			taskLock_.unlock ();
			return;
		}
	}

	taskLock_.unlock ();
}


