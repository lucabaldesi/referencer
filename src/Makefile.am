ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = referencer

INCLUDES = -DDATADIR=\""$(pkgdatadir)"\" \
	 -DGNOMELOCALEDIR=\"$(datadir)/locale\" \
	 -DPLUGINDIR=\""$(pkglibdir)"\"

LDADD = \
	$(DEPS_LIBS) \
	$(top_builddir)/libbibutils/libbibutils.a

if ENABLE_PYTHON
INCLUDES += \
        $(NO_STRICT_ALIASING_CFLAGS)    \
        $(PYGTK_CFLAGS)                 \
        $(PYTHON_CFLAGS)                \
        $(AM_CFLAGS)

LDADD += $(PYTHON_LIBS)
endif



referencer_SOURCES =	\
	ArxivPlugin.C   \
	ArxivPlugin.h   \
	BibData.C	\
	BibData.h	\
	BibUtils.C	\
	BibUtils.h	\
	CrossRefPlugin.C	\
	CrossRefPlugin.h	\
	Document.C	\
	Document.h	\
	DocumentList.C	\
	DocumentList.h	\
	DocumentProperties.C	\
	DocumentProperties.h	\
	DocumentTypes.C \
	DocumentTypes.h \
	DocumentView.C \
	DocumentView.h \
	ev-tooltip.c \
	ev-tooltip.h \
	icon-entry.cc \
	icon-entry.h \
	Library.C \
	Library.h \
	LibraryParser.h \
	Linker.C \
	Linker.h \
	main.C \
	Plugin.h \
	PluginManager.C \
	PluginManager.h \
	Preferences.C	\
	Preferences.h	\
	Progress.C	\
	Progress.h	\
	PythonDocument.C \
	PythonDocument.h \
	PythonPlugin.C \
	PythonPlugin.h \
	sexy-enum-types.h \
	sexy-icon-entry.c \
	sexy-icon-entry.h \
	TagList.C	\
	TagList.h	\
	ThumbnailGenerator.C \
	ThumbnailGenerator.h \
	RefWindow.C	\
	RefWindow.h	\
	Transfer.C	\
	Transfer.h	\
	ucompose.hpp \
	Utility.C	\
	Utility.h

AM_CXXFLAGS = @CXXFLAGS@ $(DEPS_CFLAGS) -I$(top_srcdir)
AM_CFLAGS = @CXXFLAGS@ $(DEPS_CFLAGS) -I$(top_srcdir)
