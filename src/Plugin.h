
#ifndef PLUGIN_H
#define PLUGIN_H


class PluginCapability
{
	public:
		typedef enum {
			NONE = 0,
			DOI = 1 << 0,
			ARXIV = 1 << 1,
			MEDLINE = 1 << 2
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

		uint64_t mask_;
};

class BibData;

/*
 * Base class for metadata fetching plugins
 */
class Plugin
{
	public:
		Plugin () {enabled_ = false; loaded_ = false;};
		~Plugin () {};

		virtual void load (std::string const &moduleName) {};
		virtual bool resolve (BibData &bib) = 0;

		virtual Glib::ustring const getShortName () = 0;
		virtual Glib::ustring const getLongName () = 0;

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
