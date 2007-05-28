## Process this file with automake to produce Makefile.in
SUBDIRS = po data libbibutils src

EXTRA_DIST =	\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	referencer.spec.in	\
	referencer.spec \
	libsexymm

DISTCLEANFILES =	\
	intltool-extract	\
	intltool-merge		\
	intltool-update

## Use this so distcheck doesn't cry about the mime cache
DISTCHECK_CONFIGURE_FLAGS = --disable-update-mime-database

