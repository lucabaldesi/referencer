bin_PROGRAMS = referencer

INCLUDES = -DDATADIR=\""$(pkgdatadir)"\"

referencer_SOURCES =	\
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
	ProgressDialog.C	\
	ProgressDialog.h	\
	Utility.C	\
	Utility.h	\
	BibUtils.C	\
	BibUtils.h	\
	Transfer.C	\
	Transfer.h	\
	CrossRefParser.h	\
	LibraryParser.h

AM_CXXFLAGS = @CXXFLAGS@ $(DEPS_CFLAGS) -I$(top_srcdir)
LDADD = \
	$(DEPS_LIBS) \
	$(top_builddir)/libbibutils/libbibutils.a
