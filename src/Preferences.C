
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#include <iostream>

#include "Utility.h"

#include "Preferences.h"


Preferences *_global_prefs;

#define CONF_PATH "/apps/referencer"

#define USE_PROXY_KEY   "/system/http_proxy/use_http_proxy"
#define HTTP_PROXY_HOST_KEY  "/system/http_proxy/host"
#define HTTP_PROXY_PORT_KEY  "/system/http_proxy/port"
#define HTTP_USE_AUTH_KEY    "/system/http_proxy/use_authentication"
#define HTTP_AUTH_USER_KEY   "/system/http_proxy/authentication_user"
#define HTTP_AUTH_PASSWD_KEY "/system/http_proxy/authentication_password"
#define PROXY_MODE_KEY "/system/proxy/mode"

Preferences::Preferences ()
{
	confclient_ = Gnome::Conf::Client::get_default_client ();

	libraryfilename_ = confclient_->get_entry (CONF_PATH "/libraryfilename");
	workoffline_ = confclient_->get_entry (CONF_PATH "/workoffline");
	uselistview_ = confclient_->get_entry (CONF_PATH "/uselistview");
	showtagpane_ = confclient_->get_entry (CONF_PATH "/showtagpane");
	doilaunch_ = confclient_->get_entry (CONF_PATH "/doilaunch");
	metadatalookup_ = confclient_->get_entry (CONF_PATH "/metadatalookup");
	width_ = confclient_->get_entry (CONF_PATH "/width");
	height_ = confclient_->get_entry (CONF_PATH "/height");

	doilaunchdefault_ = "http://dx.doi.org/<!DOI!>";
	metadatalookupdefault_ =
		"http://www.crossref.org/openurl/?id=doi:<!DOI!>&noredirect=true";

	proxymode_ = confclient_->get_entry (PROXY_MODE_KEY);
	proxyuseproxy_ = confclient_->get_entry (USE_PROXY_KEY);
	proxyuseauth_ = confclient_->get_entry (HTTP_USE_AUTH_KEY);
	proxyhost_ = confclient_->get_entry (HTTP_PROXY_HOST_KEY);
	proxyport_ = confclient_->get_entry (HTTP_PROXY_PORT_KEY);
	proxyusername_ = confclient_->get_entry (HTTP_AUTH_USER_KEY);
	proxypassword_ = confclient_->get_entry (HTTP_AUTH_PASSWD_KEY);

	if (!confclient_->dir_exists (CONF_PATH)) {
		std::cerr << "Preferences::Preferences: CONF_PATH "
			"doesn't exist, setting it up\n";

		setLibraryFilename ("");
		setShowTagPane (true);
		setUseListView (false);
		setWorkOffline (false);
		setDoiLaunch (doilaunchdefault_);
		setMetadataLookup (metadatalookupdefault_);
		setWindowSize (std::pair<int,int>(700,500));
		firsttime_ = true;
	} else {
		firsttime_ = false;
	}

	confclient_->add_dir (
		CONF_PATH,
		Gnome::Conf::CLIENT_PRELOAD_ONELEVEL);
	confclient_->add_dir (
		"/system/http_proxy",
		Gnome::Conf::CLIENT_PRELOAD_ONELEVEL);
	confclient_->add_dir (
		"/system/proxy",
		Gnome::Conf::CLIENT_PRELOAD_ONELEVEL);

	confclient_->notify_add (
		CONF_PATH,
		sigc::mem_fun (*this, &Preferences::onConfChange));
	confclient_->notify_add (
		"/system/http_proxy",
		sigc::mem_fun (*this, &Preferences::onConfChange));
	confclient_->notify_add (
		"/system/proxy",
		sigc::mem_fun (*this, &Preferences::onConfChange));

	xml_ = Gnome::Glade::Xml::create (
		Utility::findDataFile ("preferences.glade"));

	dialog_ = (Gtk::Dialog *) xml_->get_widget ("Preferences");

	doilaunchentry_ = (Gtk::Entry *) xml_->get_widget ("DoiLaunch");
	metadatalookupentry_ = (Gtk::Entry *) xml_->get_widget ("MetadataLookup");
	doilaunchentry_->signal_changed().connect (
		sigc::mem_fun (*this, &Preferences::onURLChanged));
	metadatalookupentry_->signal_changed().connect (
		sigc::mem_fun (*this, &Preferences::onURLChanged));

	Gtk::Button *button = (Gtk::Button *) xml_->get_widget ("ResetToDefaults");
	button->signal_clicked().connect (
		sigc::mem_fun (*this, &Preferences::onResetToDefaults));

	xml_->get_widget ("ProxyHost", proxyhostentry_);
	xml_->get_widget ("ProxyPort", proxyportspin_);
	xml_->get_widget ("ProxyUsername", proxyusernameentry_);
	xml_->get_widget ("ProxyPassword", proxypasswordentry_);
	xml_->get_widget ("UseWebProxy", useproxycheck_);
	xml_->get_widget ("UseAuthentication", useauthcheck_);

	proxyhostentry_->signal_changed().connect (
		sigc::mem_fun (*this, &Preferences::onProxyChanged));
	proxyportspin_->signal_value_changed().connect (
		sigc::mem_fun (*this, &Preferences::onProxyChanged));
	proxyusernameentry_->signal_changed().connect (
		sigc::mem_fun (*this, &Preferences::onProxyChanged));
	proxypasswordentry_->signal_changed().connect (
		sigc::mem_fun (*this, &Preferences::onProxyChanged));
	useproxycheck_->signal_toggled().connect (
		sigc::mem_fun (*this, &Preferences::onProxyChanged));
	useauthcheck_->signal_toggled().connect (
		sigc::mem_fun (*this, &Preferences::onProxyChanged));

	ignorechanges_ = false;
}


Preferences::~Preferences ()
{

}


void Preferences::onConfChange (int number, Gnome::Conf::Entry entry)
{
	//std::cerr << "onConfChange: '" << entry.get_key () << "'\n";
	ignorechanges_ = true;
	Glib::ustring key = entry.get_key ();

	// Settings not in dialog
	if (key == CONF_PATH "/workoffline") {
		workofflinesignal_.emit ();
	} else if (key == CONF_PATH "/uselistview") {
		uselistviewsignal_.emit ();
	} else if (key == CONF_PATH "/showtagpane") {
		showtagpanesignal_.emit ();

	// Web Service settings
	} else if (key == CONF_PATH "/doilaunch") {
		doilaunchentry_->set_text (
			entry.get_value ().get_string ());
	} else if (key == CONF_PATH "/metadatalookup") {
		metadatalookupentry_->set_text (
			entry.get_value ().get_string ());

	// Proxy settings
	} else if (key == HTTP_PROXY_HOST_KEY) {
		proxyhostentry_->set_text (
			entry.get_value ().get_string ());
	} else if (key == HTTP_PROXY_PORT_KEY) {
		proxyportspin_->get_adjustment ()->set_value (
			entry.get_value ().get_int ());
	} else if (key == HTTP_AUTH_USER_KEY) {
		proxyusernameentry_->set_text (
			entry.get_value ().get_string ());
	} else if (key == HTTP_AUTH_PASSWD_KEY) {
		proxypasswordentry_->set_text (
			entry.get_value ().get_string ());
	} else if (key == PROXY_MODE_KEY || key == USE_PROXY_KEY) {
		Glib::ustring const mode = confclient_->get_string (PROXY_MODE_KEY);
		useproxycheck_->set_active (
			mode != "none" && confclient_->get_bool (USE_PROXY_KEY));
		updateSensitivity ();
	} else if (key == HTTP_USE_AUTH_KEY) {
		useauthcheck_->set_active (entry.get_value ().get_bool ());
		updateSensitivity ();
	/* keys to ignore */
	} else if (
	    key == CONF_PATH "/width"
	    || key == CONF_PATH "/height") {
		;
	} else {
		std::cerr << "Warning: Preferences::onConfChange: "
			"unhandled key '" << key << "'\n";
	}
	//std::cerr << "Complete.\n";

	ignorechanges_ = false;
}


void Preferences::showDialog ()
{
	ignorechanges_ = true;

	doilaunchentry_->set_text (
		confclient_->get_string (doilaunch_.get_key()));
	metadatalookupentry_->set_text (
		confclient_->get_string (metadatalookup_.get_key()));

	proxyhostentry_->set_text (
		confclient_->get_string (HTTP_PROXY_HOST_KEY));
	proxyportspin_->get_adjustment ()->set_value (
		confclient_->get_int (HTTP_PROXY_PORT_KEY));
	proxyusernameentry_->set_text (
		confclient_->get_string (HTTP_AUTH_USER_KEY));
	proxypasswordentry_->set_text (
		confclient_->get_string (HTTP_AUTH_PASSWD_KEY));
	Glib::ustring const mode = confclient_->get_string (PROXY_MODE_KEY);
	useproxycheck_->set_active (
		mode != "none" && confclient_->get_bool (USE_PROXY_KEY));
	useauthcheck_->set_active (confclient_->get_bool (HTTP_USE_AUTH_KEY));

	ignorechanges_ = false;

	updateSensitivity ();

	dialog_->run ();
	dialog_->hide ();
}


using Utility::DOIURLValid;

void Preferences::onURLChanged ()
{
	if (ignorechanges_) return;

	// Should we be telling gconf to ignore us for a minute, since
	// these operations cause a useless callback to onConfChange?

	// Silently don't remember bad settings for now -- should prompt the
	// user when the dialog closes if they still haven't entered something valid
	if (DOIURLValid (doilaunchentry_->get_text ())) {
		confclient_->set (
			doilaunch_.get_key(), doilaunchentry_->get_text ());
	}

	if (DOIURLValid (metadatalookupentry_->get_text ())) {
		confclient_->set (
			metadatalookup_.get_key(), metadatalookupentry_->get_text ());
	}
}


void Preferences::updateSensitivity ()
{
	bool useproxy = useproxycheck_->get_active ();
	proxyhostentry_->set_sensitive (useproxy);
	proxyportspin_->set_sensitive (useproxy);
	useauthcheck_->set_sensitive (useproxy);

	bool useauth = useauthcheck_->get_active ();
	proxyusernameentry_->set_sensitive (useproxy && useauth);
	proxypasswordentry_->set_sensitive (useproxy && useauth);
}


void Preferences::onProxyChanged ()
{
	if (ignorechanges_) return;

	bool useproxy = useproxycheck_->get_active ();
	bool useauth = useauthcheck_->get_active ();

	confclient_->set (proxyuseproxy_.get_key (), useproxy);
	if (useproxy) {
		confclient_->set (proxymode_.get_key (), Glib::ustring ("manual"));
		confclient_->set (
			proxyhost_.get_key(), proxyhostentry_->get_text ());
		confclient_->set (
			proxyport_.get_key(),
			(int) proxyportspin_->get_adjustment ()->get_value ());
	} else {
		confclient_->set (proxymode_.get_key (), Glib::ustring ("none"));
	}

	confclient_->set (proxyuseauth_.get_key(), useauth);
	if (useproxy && useauth) {
		confclient_->set (
			proxyusername_.get_key(), proxyusernameentry_->get_text ());
		confclient_->set (
			proxypassword_.get_key(), proxypasswordentry_->get_text ());
	}

	updateSensitivity ();
}


// This is just for the button in the dialog!
void Preferences::onResetToDefaults ()
{
	doilaunchentry_->set_text (doilaunchdefault_);
	metadatalookupentry_->set_text (metadatalookupdefault_);
}


Glib::ustring Preferences::getLibraryFilename ()
{
	return confclient_->get_string (libraryfilename_.get_key());
}


void Preferences::setLibraryFilename (Glib::ustring const &filename)
{
	return confclient_->set (libraryfilename_.get_key(), filename);
}


bool Preferences::getWorkOffline ()
{
	return confclient_->get_bool (workoffline_.get_key());
}


void Preferences::setWorkOffline (bool const &offline)
{
	confclient_->set (workoffline_.get_key(), offline);
}


sigc::signal<void>& Preferences::getWorkOfflineSignal ()
{
	return workofflinesignal_;
}


bool Preferences::getUseListView ()
{
	return confclient_->get_bool (uselistview_.get_key());
}


void Preferences::setUseListView (bool const &uselistview)
{
	confclient_->set (uselistview_.get_key(), uselistview);
}


sigc::signal<void>& Preferences::getUseListViewSignal ()
{
	return uselistviewsignal_;
}


bool Preferences::getShowTagPane ()
{
	return confclient_->get_bool (showtagpane_.get_key());
}


void Preferences::setShowTagPane (bool const &showtagpane)
{
	confclient_->set (showtagpane_.get_key(), showtagpane);
}


sigc::signal<void>& Preferences::getShowTagPaneSignal ()
{
	return showtagpanesignal_;
}


using Utility::StringPair;
using Utility::twoWaySplit;

StringPair Preferences::getDoiLaunch ()
{
	return twoWaySplit (
		confclient_->get_string (doilaunch_.get_key()),
		"<!DOI!>");
}


void Preferences::setDoiLaunch (Glib::ustring const &doilaunch)
{
	confclient_->set (doilaunch_.get_key(), doilaunch);
}


StringPair Preferences::getMetadataLookup ()
{
	return twoWaySplit (
		confclient_->get_string (metadatalookup_.get_key()),
		"<!DOI!>");
}


void Preferences::setMetadataLookup (Glib::ustring const &metadatalookup)
{
	confclient_->set (metadatalookup_.get_key(), metadatalookup);
}


std::pair<int, int> Preferences::getWindowSize ()
{
	std::pair<int, int> size;
	size.first = confclient_->get_int (width_.get_key ());
	size.second = confclient_->get_int (height_.get_key ());
	return size;
}


void Preferences::setWindowSize (std::pair<int, int> size)
{
	confclient_->set (width_.get_key (), size.first);
	confclient_->set (height_.get_key (), size.second);
}
