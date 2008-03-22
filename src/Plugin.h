
#ifndef PLUGIN_H
#define PLUGIN_H

#include <vector>

class PluginCapability
{
	public:
		/*
		 * There are if()elseif() blocks on these
		 * in various places, beware when adding a 
		 * new one
		 */
		typedef enum {
			NONE = 0,
			DOI = 1 << 0,
			ARXIV = 1 << 1,
			PUBMED = 1 << 2,
			DOCUMENT_ACTION = 1 << 3
		} Identifier;

		void add (Identifier const id) {
			mask_ |= id;
		}

		bool has (Identifier const id) const {
			return mask_ & id;
		}
		
		bool hasAny (PluginCapability const cap) const {
			return mask_ & cap.mask_;
		}
		
		bool empty () const {
			return mask_ == NONE;
		}

		PluginCapability () {
			mask_ = NONE;
		}

		std::vector<Identifier> get ()
		{
			std::vector<Identifier> ids;
			for (uint64_t i = 1; i <= mask_; i *= 2) {
				if (mask_ & i) {
					ids.push_back ((Identifier)i);
				}
			}

			return ids;
		}

		uint64_t mask_;
};

class Document;

/*
 * Base class for metadata fetching plugins
 */
class Plugin
{
	public:
		Plugin () {enabled_ = false; loaded_ = false;}
		virtual ~Plugin () {};

		virtual void load (std::string const &moduleName) {};
		virtual bool resolve (Document &doc) = 0;

		virtual Glib::ustring const getShortName () = 0;
		virtual Glib::ustring const getLongName () = 0;
		virtual Glib::ustring const getAuthor () = 0;
		virtual Glib::ustring const getVersion () = 0;

		virtual Glib::ustring const getUI () {return Glib::ustring();}
		typedef std::vector<Glib::RefPtr<Gtk::Action> > ActionList;
		virtual ActionList getActions ()
		{
			return ActionList ();
		};
		virtual bool doAction          (Glib::ustring const action, std::vector<Document*>) {return false;}
		virtual bool updateSensitivity (Glib::ustring const action, std::vector<Document*>) {return true;}

		/* Configuration hook */
		virtual bool canConfigure () {return false;}
		virtual void doConfigure  () {};

		bool isEnabled () {return enabled_;}
		bool isLoaded () {return loaded_;}
		virtual void setEnabled (bool const enable)
		{
			if (loaded_)
				enabled_ = enable;
			else
				enabled_ = false;
		}

		PluginCapability cap_;

	protected:
		bool loaded_;
		bool enabled_;
};
#endif
