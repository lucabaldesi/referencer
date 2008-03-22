
/*
 * Referencer is released under the GNU General Public License v2
 * See the COPYING file for licensing details.
 *
 * Copyright 2007 John Spray
 * (Exceptions listed in README)
 *
 */



#include <iostream>

#include <glibmm/i18n.h>

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
	crossRefUsername_ = confclient_->get_entry (CONF_PATH "/crossrefusername");
	crossRefPassword_ = confclient_->get_entry (CONF_PATH "/crossrefpassword");
	width_ = confclient_->get_entry (CONF_PATH "/width");
	height_ = confclient_->get_entry (CONF_PATH "/height");

	proxymode_ = confclient_->get_entry (PROXY_MODE_KEY);
	proxyuseproxy_ = confclient_->get_entry (USE_PROXY_KEY);
	proxyuseauth_ = confclient_->get_entry (HTTP_USE_AUTH_KEY);
	proxyhost_ = confclient_->get_entry (HTTP_PROXY_HOST_KEY);
	proxyport_ = confclient_->get_entry (HTTP_PROXY_PORT_KEY);
	proxyusername_ = confclient_->get_entry (HTTP_AUTH_USER_KEY);
	proxypassword_ = confclient_->get_entry (HTTP_AUTH_PASSWD_KEY);


	/*
	 * List view options
	 */
	listSortColumn_ = confclient_->get_entry (CONF_PATH "/listsortcolumn");
	listSortOrder_ = confclient_->get_entry (CONF_PATH "/listsortorder");


	if (!confclient_->dir_exists (CONF_PATH)) {
		std::cerr << "Preferences::Preferences: CONF_PATH "
			"doesn't exist, setting it up\n";

		setLibraryFilename ("");
		setShowTagPane (true);
		setUseListView (false);
		setWorkOffline (false);
		setWindowSize (std::pair<int,int>(700,500));
		setListSort (-1, 0);
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

	/*
	 * Plugins
	 */
	disabledPlugins_ = confclient_->get_entry (CONF_PATH "/disabledplugins");
	Gnome::Conf::SListHandle_ValueString disable =
		confclient_->get_string_list (disabledPlugins_.get_key ());

	configureButton_ = (Gtk::Button*) xml_->get_widget ("PluginConfigure");
	configureButton_->signal_clicked().connect (
		sigc::mem_fun (*this, &Preferences::onPluginConfigure));
	aboutButton_ =     (Gtk::Button*) xml_->get_widget ("PluginAbout");
	aboutButton_->signal_clicked().connect (
		sigc::mem_fun (*this, &Preferences::onPluginAbout));

	
	// Iterate over all plugins
	std::list<Plugin*> plugins = _global_plugins->getPlugins();
	std::list<Plugin*>::iterator pit = plugins.begin();
	std::list<Plugin*>::iterator const pend = plugins.end();
	for (; pit != pend; pit++) {
		// All enabled unless disabled
		(*pit)->setEnabled(true);

		Gnome::Conf::SListHandle_ValueString::iterator dit = disable.begin();
		Gnome::Conf::SListHandle_ValueString::iterator const dend = disable.end();
		for (; dit != dend; ++dit) {
			if ((*pit)->getShortName() == (*dit)) {
				(*pit)->setEnabled(false);
				std::cerr << "disabling plugin " << (*pit)->getShortName() << "\n";
			}
		}
	}

	Gtk::TreeModel::ColumnRecord pluginCols;
	pluginCols.add (colPriority_);
	pluginCols.add (colPlugin_);
	pluginCols.add (colEnabled_);
	pluginCols.add (colShortName_);
	pluginCols.add (colLongName_);
	pluginStore_ = Gtk::ListStore::create (pluginCols);

	// Re-use plugins list from enable/disable stage above
	std::list<Plugin*>::iterator it = plugins.begin();
	std::list<Plugin*>::iterator const end = plugins.end();
	for (int count = 0; it != end; ++it, ++count) {
		Gtk::TreeModel::iterator item = pluginStore_->append();
		(*item)[colPriority_] = count;
		(*item)[colShortName_] = (*it)->getShortName ();
		(*item)[colLongName_] = (*it)->getLongName ();
		(*item)[colEnabled_] = (*it)->isEnabled ();
		(*item)[colPlugin_] = (*it);
	}

	xml_->get_widget ("Plugins", pluginView_);

	Gtk::CellRendererToggle *toggle = Gtk::manage (new Gtk::CellRendererToggle);
	toggle->property_activatable() = true;
	toggle->signal_toggled().connect (sigc::mem_fun (*this, &Preferences::onPluginToggled));
	Gtk::TreeViewColumn *enabled = Gtk::manage (new Gtk::TreeViewColumn (_("Enabled"), *toggle));
	enabled->add_attribute (toggle->property_active(), colEnabled_);
	pluginView_->append_column (*enabled);

	Gtk::TreeViewColumn *shortName = Gtk::manage (new Gtk::TreeViewColumn (_("Module"), colShortName_));
	pluginView_->append_column (*shortName);
	Gtk::TreeViewColumn *longName = Gtk::manage (new Gtk::TreeViewColumn (_("Description"), colLongName_));
	pluginView_->append_column (*longName);

	pluginView_->set_model (pluginStore_);
	pluginSel_ = pluginView_->get_selection ();
	pluginSel_->signal_changed().connect(
		sigc::mem_fun (*this, &Preferences::onPluginSelect));
	onPluginSelect ();


	/*
	 * End of Plugins
	 */
	ignoreChanges_ = false;
}



Preferences::~Preferences ()
{

}


void Preferences::onConfChange (int number, Gnome::Conf::Entry entry)
{
	//std::cerr << "onConfChange: '" << entry.get_key () << "'\n";
	ignoreChanges_ = true;
	Glib::ustring key = entry.get_key ();

	// Settings not in dialog
	if (key == CONF_PATH "/workoffline") {
		workofflinesignal_.emit ();
	} else if (key == CONF_PATH "/uselistview") {
		uselistviewsignal_.emit ();
	} else if (key == CONF_PATH "/showtagpane") {
		showtagpanesignal_.emit ();

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
	} else if (key == CONF_PATH "/disabledplugins") {
		plugindisabledsignal_.emit ();

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

	ignoreChanges_ = false;
}


void Preferences::showDialog ()
{
	ignoreChanges_ = true;

	/*
	 * Plugin stuff was populated at construction time
	 * and should remain in a consistent state thereafter
	 */

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

	ignoreChanges_ = false;

	updateSensitivity ();

	dialog_->run ();
	dialog_->hide ();
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
	if (ignoreChanges_) return;

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

sigc::signal<void>& Preferences::getPluginDisabledSignal ()
{
	return plugindisabledsignal_;
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


Glib::ustring Preferences::getCrossRefUsername ()
{
	return confclient_->get_string (crossRefUsername_.get_key());
}


Glib::ustring Preferences::getCrossRefPassword ()
{
	return confclient_->get_string (crossRefPassword_.get_key());
}


void Preferences::setCrossRefUsername (Glib::ustring const &username)
{
	confclient_->set (crossRefUsername_.get_key(), username);
}


void Preferences::setCrossRefPassword (Glib::ustring const &password)
{
	confclient_->set (crossRefPassword_.get_key(), password);
}


std::pair<int, int> Preferences::getWindowSize ()
{
	std::pair<int, int> size;
	size.first = confclient_->get_int (width_.get_key ());
	size.second = confclient_->get_int (height_.get_key ());
	// Cope with upgrading
	if (size.first == 0 || size.second == 0) {
		size.first = 700;
		size.second = 500;
	}
	return size;
}


void Preferences::setWindowSize (std::pair<int, int> size)
{
	confclient_->set (width_.get_key (), size.first);
	confclient_->set (height_.get_key (), size.second);
}


std::pair<int, int> Preferences::getListSort ()
{
	std::pair<int, int> retval;
	retval.first = confclient_->get_int (listSortColumn_.get_key());
	retval.second = confclient_->get_int (listSortOrder_.get_key());

	/*
	 * XXX bit cracky, 0 is an invalid value because it
	 * refers to the pointer col that gtk doesn't know how
	 * to sort.  -1 means "sort on no columns".  This shouldn't
	 * happen if we have a proper gconf schema
	 */
	if (retval.first == 0) {
		retval.first = -1;
	} 

	return retval;
}


void Preferences::setListSort (int const column, int const order)
{
	confclient_->set (listSortColumn_.get_key(), column);
	confclient_->set (listSortOrder_.get_key(), order);
}


void Preferences::onPluginToggled (Glib::ustring const &str)
{
	Gtk::TreePath path(str);

	Gtk::TreeModel::iterator it = pluginStore_->get_iter (path);
	bool enable = !(*it)[colEnabled_];
	Plugin *plugin = (*it)[colPlugin_];
		plugin->setEnabled (enable);
	if (enable)
	{
		std::cerr << "enabling plugin" << std::endl;
		// TODO: add UI
	} else
	{
		std::cerr << "disabling plugin" << std::endl;
		// TODO: remove UI
	}
	(*it)[colEnabled_] = plugin->isEnabled ();

	std::vector<Glib::ustring> disable =
		confclient_->get_string_list (disabledPlugins_.get_key ());
	std::vector<Glib::ustring>::iterator dit = disable.begin();
	std::vector<Glib::ustring>::iterator const dend = disable.end();
	if (plugin->isEnabled() == true) {
		// Remove from gconf list of disabled plugins
		for (; dit != dend; ++dit) {
			if (*dit == plugin->getShortName()) {
				disable.erase(dit);
				break;
			}
		}
	} else {
		// Add to gconf list of disabled plugins
		bool found = false;
		for (; dit != dend; ++dit)
			if (*dit == plugin->getShortName())
				found = true;
		if (!found)
			disable.push_back(plugin->getShortName());

	}
	confclient_->set_string_list (disabledPlugins_.get_key(), disable);
}



void Preferences::disablePlugin (Plugin *plugin)
{
	Gtk::ListStore::iterator it = pluginStore_->children().begin();
	Gtk::ListStore::iterator const end = pluginStore_->children().end();
	for (; it != end; ++it) {
		if ((*it)[colPlugin_] == plugin) {
			(*it)[colEnabled_] = false;
		}
	}

	plugin->setEnabled (false);

	std::vector<Glib::ustring> disable =
		confclient_->get_string_list (disabledPlugins_.get_key ());
	std::vector<Glib::ustring>::iterator dit = disable.begin();
	std::vector<Glib::ustring>::iterator const dend = disable.end();


	bool found = false;
	for (; dit != dend; ++dit)
		if (*dit == plugin->getShortName())
			found = true;
	if (!found)
		disable.push_back(plugin->getShortName());

	confclient_->set_string_list (disabledPlugins_.get_key(), disable);
}


/**
 * Update sensitivities on plugin selection changed
 */
void Preferences::onPluginSelect ()
{
	if (pluginSel_->count_selected_rows () == 0) {
		aboutButton_->set_sensitive(false);	
		configureButton_->set_sensitive(false);	
	} else {
		Gtk::ListStore::iterator it = pluginSel_->get_selected();
		Plugin *plugin = (*it)[colPlugin_];
		aboutButton_->set_sensitive(true);
		configureButton_->set_sensitive(plugin->canConfigure());
	}
}


/**
 * Display a Gtk::AboutDialog for the selected plugin
 */
void Preferences::onPluginAbout ()
{
	Gtk::ListStore::iterator it = pluginSel_->get_selected();
	Plugin *plugin = (*it)[colPlugin_];

	Gtk::AboutDialog dialog;
	dialog.set_name (plugin->getShortName());
	dialog.set_version (plugin->getVersion());
	dialog.set_comments (plugin->getLongName());
	if (!plugin->getAuthor().empty()) {
	    dialog.set_copyright ("Copyright " + plugin->getAuthor());
	}
//	dialog.set_website ("http://icculus.org/referencer/");
/*	dialog.set_logo (
		Gdk::Pixbuf::create_from_file (
			Utility::findDataFile ("referencer.svg"),
			128, 128));*/
	dialog.run ();
}


/**
 * Call the selected plugin's configuration hook if it has one
 */
void Preferences::onPluginConfigure ()
{
	Gtk::ListStore::iterator it = pluginSel_->get_selected();
	Plugin *plugin = (*it)[colPlugin_];
	plugin->doConfigure ();
}
