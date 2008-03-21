resourcesdir   = $(pkgdatadir)
resources_DATA = documentproperties.glade preferences.glade introduction.html main.css thumbnail_frame.png referencer.svg unknown-document.png

desktopdir       = $(datadir)/applications
desktop_in_files = referencer.desktop.in
desktop_DATA     = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

mimedir   = $(datadir)/mime/packages
mime_DATA = referencer.xml

mimeicondir   = $(datadir)/icons/hicolor/48x48/mimetypes
mimeicon_DATA = gnome-mime-application-x-referencer.png

icondir   = $(datadir)/pixmaps
icon_DATA = referencer.svg

if UPDATE_MIME_DATABASE
install-data-hook:
	-update-mime-database "$(DESTDIR)${datadir}/mime"

uninstall-hook:
	-update-mime-database "$(DESTDIR)${datadir}/mime"
else
install-data-hook:
	true
uninstall-hook:
	true
endif

EXTRA_DIST =	\
	$(resources_DATA)	\
	$(desktop_in_files)	\
	$(mime_DATA)		\
	$(mimeicon_DATA)	\
	gnome-mime-application-x-referencer.svg \
	$(icon_DATA)

CLEANFILES =	\
	$(desktop_DATA)
