
#ifndef PLUGIN_H
#define PLUGIN_H

#include "Document.h"

#include <gtkmm.h>
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
			DOCUMENT_ACTION = 1 << 3,
			URL = 1 << 4,
			SEARCH = 1 << 5
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

		static Glib::ustring getFriendlyName (Identifier const id) {
			switch (id) {
				case DOCUMENT_ACTION:
					return "Document action";
				case DOI:
					return "DOI";
				case ARXIV:
					return "arXiv e-print";
				case PUBMED:
					return "PubMed ID";
				case URL:
					return "Web URL";
				case SEARCH:
					return "Search";
				case NONE:
				default:
					return "Invalid PluginCapability for display";

			}
		}	

		static Glib::ustring getFieldName (Identifier const id) {
			switch (id) {
				case DOI:
					return "doi";
				case ARXIV:
					return "eprint";
				case PUBMED:
					return "pmid";
				case URL:
					return "url";

				case NONE:
				case SEARCH:
				case DOCUMENT_ACTION:
				default:
					return "";

			}
		}	

		/**
		 * Returns a list of capability IDs suitable for display in 
		 * a list of document ID types
		 */
		static std::vector<Identifier> getMetadataCapabilities () {
			std::vector<Identifier> retval;
			retval.push_back (DOI);
			retval.push_back (PUBMED);
			retval.push_back (ARXIV);
			retval.push_back (URL);
			return retval;
		}

		bool hasMetadataCapability () {
			return has (DOI) || has (ARXIV) || has (PUBMED) || has (URL);
		}
};



/*
 * Base class for metadata fetching plugins
 */
class Plugin
{
	public:
		typedef std::map <Glib::ustring, Glib::ustring> SearchResult;
		typedef std::vector< SearchResult > SearchResults;

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

		/* Error reporting */
		virtual bool hasError () {return false;}
		virtual Glib::ustring getError () {return "";}

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

		
		/* Searching */
		virtual bool canSearch () {return false;}
		virtual SearchResults doSearch (Glib::ustring const &searchTerms) {return SearchResults();}
		virtual Document getSearchResult (Glib::ustring const &token) {throw std::logic_error("Unimplemented");};

	protected:
		bool loaded_;
		bool enabled_;
};
#endif
