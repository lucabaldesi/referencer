
ACLOCAL_AMFLAGS = -I m4

## Process this file with automake to produce Makefile.in
SUBDIRS = po data libbibutils src plugins help devhelp

EXTRA_DIST =	\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	referencer.spec.in	\
	referencer.spec \
	gnome-doc-utils.make \
	m4 \
	libsexymm

DISTCLEANFILES =	\
	intltool-extract	\
	intltool-merge		\
	gnome-doc-utils.make \
	intltool-update

## Use this so distcheck doesn't cry about the mime cache
DISTCHECK_CONFIGURE_FLAGS = --disable-update-mime-database --disable-scrollkeeper

