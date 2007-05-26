bin_PROGRAMS = referencer

INCLUDES = -DDATADIR=\""$(pkgdatadir)"\" \
	 -DGNOMELOCALEDIR=\"$(datadir)/locale\"

referencer_SOURCES =	\
	sexy-icon-entry.c \
	sexy-icon-entry.h \
	icon-entry.cc \
	icon-entry.h \
	ev-tooltip.c \
	ev-tooltip.h \
	TagWindow.C	\
	TagWindow.h	\
	Library.C \
	Library.h \
	TagList.C	\
	TagList.h	\
	DocumentList.C	\
	DocumentList.h	\
	Document.C	\
	Document.h	\
	BibData.C	\
	BibData.h	\
	DocumentProperties.C	\
	DocumentProperties.h	\
	Preferences.C	\
	Preferences.h	\
	Progress.C	\
	Progress.h	\
	Utility.C	\
	Utility.h	\
	BibUtils.C	\
	BibUtils.h	\
	Transfer.C	\
	Transfer.h	\
	CrossRefParser.h	\
	LibraryParser.h \
	ucompose.hpp

AM_CXXFLAGS = @CXXFLAGS@ $(DEPS_CFLAGS) -I$(top_srcdir)
AM_CFLAGS = @CXXFLAGS@ $(DEPS_CFLAGS) -I$(top_srcdir)
LDADD = \
	$(DEPS_LIBS) \
	$(top_builddir)/libbibutils/libbibutils.a
