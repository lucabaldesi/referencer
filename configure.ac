dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.13)

dnl ============================================================================
dnl Package name and version number (user defined)
dnl ============================================================================
AC_INIT([referencer], [1.1.6], [referencer@icculus.org])

AC_CONFIG_SRCDIR([src/RefWindow.C])

AM_CONFIG_HEADER([config.h])

dnl ============================================================================
dnl -Wno-portablity: We do not want 'automake' to print out a lot of portability
dnl warning messages (since 'gnome-doc-utils.make' uses GNU extensions a lot).
dnl ============================================================================
AM_INIT_AUTOMAKE([-Wno-portability])

AC_CONFIG_MACRO_DIR([m4])

GNOME_COMMON_INIT
GNOME_DOC_INIT

AC_LANG_CPLUSPLUS

dnl ============================================================================
dnl Checks for programs.
dnl ============================================================================
AC_PROG_CXX
AC_PROG_RANLIB

dnl ============================================================================
dnl Checks for libraries.
dnl ============================================================================
PKG_CHECK_MODULES(DEPS, [
			libgnomeuimm-2.6 >= 2.12.0
			gtkmm-2.4 >= 2.12.0
dnl TODO: make dbus-glib and tracker optional
dnl			dbus-glib-1 >= 0.74
dnl			tracker >= 0.6.3
			gconfmm-2.6 >= 2.12.0
			gnome-vfsmm-2.6 >= 2.12.0
			poppler-glib >= 0.5.0
			libxml-2.0 >= 2.7.6
			])
AC_CHECK_LIB(boost_regex, [main], , [
		AC_CHECK_LIB(boost_regex-mt, [main], ,
		AC_MSG_ERROR([boost_regex not found. Stopped]))
])

AC_CHECK_HEADERS(boost/regex.hpp)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)
LIBS="$LIBS $DEPS_LIBS"

dnl ============================================================================
dnl Special case for poppler-glib < 0.6
dnl ============================================================================
case $($PKG_CONFIG --modversion poppler-glib) in
	0.5.*)
		AC_DEFINE(OLD_POPPLER,"1","Using poppler < 0.6");;
esac
AC_CHECK_FUNCS([poppler_page_get_selected_text])

dnl ============================================================================
dnl I18n
dnl ============================================================================
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",GETTEXT_PACKAGE)
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL

dnl ============================================================================
dnl Add flag to disable updating of mime database
dnl ============================================================================
AC_ARG_ENABLE(update-mime-database,
        AC_HELP_STRING([--disable-update-mime-database],
        [do not run the update-mime-database utility (mostly useful for package maintainers) ]))
if test "$enable_update_mime_database" == "no"; then
    AC_MSG_NOTICE([Disabling update mime database.])
fi
AM_CONDITIONAL(UPDATE_MIME_DATABASE, test "$enable_update_mime_database" != "no")

dnl ============================================================================
dnl Add flag telling us whether we are in the 'maintainer' mode.
dnl ============================================================================
AC_ARG_ENABLE(maintainer-mode,
        AC_HELP_STRING([--enable-maintainer-mode],
        [indicates whether the build system should be set up for maintainer mode]))
if test "$enable_maintainer_mode" == "yes"; then
    AC_MSG_NOTICE([Setting up the build system for maintainer mode.])
fi
AM_CONDITIONAL(MAINTAINER_MODE, test "$enable_maintainer_mode" == "yes")

dnl ================================================================
dnl Python
dnl ================================================================
AC_MSG_CHECKING([whether Python support is requested])
AC_ARG_ENABLE([python],
	AS_HELP_STRING([--enable-python],[Enable python support]),
	[enable_python=$enableval have_python=$enableval],
	[enable_python=autodetect have_python=yes])
AC_MSG_RESULT([$enable_python])

if test "x$have_python" != "xno"; then
	AM_PATH_PYTHON([2.3],[],[no])
	if test "x$PYTHON" = "x:"; then
		have_python=no
	fi
fi

if test "x$have_python" != "xno"; then
	AM_CHECK_PYTHON_HEADERS([],[have_python=no])
fi

if test "x$have_python" != "xno"; then
	PY_PREFIX=`$PYTHON -c 'import sys ; print sys.prefix'`
	PY_EXEC_PREFIX=`$PYTHON -c 'import sys ; print sys.exec_prefix'`
	PYTHON_LIBS="-lpython$PYTHON_VERSION"
	PYTHON_LIB_LOC="-L$PY_EXEC_PREFIX/lib/python$PYTHON_VERSION/config"
	PYTHON_CFLAGS="-I$PY_PREFIX/include/python$PYTHON_VERSION"
	PYTHON_MAKEFILE="$PY_EXEC_PREFIX/lib/python$PYTHON_VERSION/config/Makefile"
    if test -f "$PYTHON_MAKEFILE"; then
	PYTHON_LOCALMODLIBS=`sed -n -e 's/^LOCALMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_BASEMODLIBS=`sed -n -e 's/^BASEMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_OTHER_LIBS=`sed -n -e 's/^LIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_EXTRA_LIBS="$PYTHON_LOCALMODLIBS $PYTHON_BASEMODLIBS $PYTHON_OTHER_LIBS"
    else
	    PYTHON_EXTRA_LIBS=""
    fi
	AC_SUBST([PYTHON_LIBS])
	AC_SUBST([PYTHON_LIB_LOC])
	AC_SUBST([PYTHON_CFLAGS])
	AC_SUBST([PYTHON_EXTRA_LIBS])
fi

if test "x$have_python" != "xyes"; then
	if test "x$enable_python" = "xyes"; then
		AC_MSG_ERROR([Python not found])
	elif test "x$enable_python" = "xautodetect"; then
		enable_python=no
		AC_MSG_WARN([Python not found, disabling python support])
	fi
fi

if test "x$have_python" != "xno" -a "x$enable_python" != "xno"; then
	enable_python=yes
	AC_DEFINE([ENABLE_PYTHON],[1],[Define to compile with python support])
fi

AM_CONDITIONAL([ENABLE_PYTHON],[test "x$enable_python" = "xyes"])

dnl ============================================================================
dnl This allows the bug-report script to know whether python has been enabled
dnl ============================================================================
AC_SUBST(enable_python)

dnl ============================================================================
dnl Create makefiles
dnl ============================================================================
AC_CONFIG_FILES(
	Makefile
	libbibutils/Makefile
	src/Makefile
	data/Makefile
	data/referencer.desktop.in
	plugins/Makefile
	help/Makefile
	devhelp/Makefile
	po/Makefile.in
	referencer.spec
)

AC_OUTPUT
