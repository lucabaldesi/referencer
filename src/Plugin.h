
#ifndef PLUGIN_H
#define PLUGIN_H

class BibData;

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
	protected:
		bool loaded_;
		bool enabled_;
};

#endif
