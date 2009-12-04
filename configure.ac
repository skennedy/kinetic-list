AC_PREREQ(2.61)

# package version number (not shared library version)
# odd micro numbers indicate in-progress development
# even micro numbers indicate released versions
m4_define(kinetic_list_version_major, 0)
m4_define(kinetic_list_version_minor, 1)
m4_define(kinetic_list_version_micro, 0)

m4_define([kinetic_list_version],
          [kinetic_list_version_major.kinetic_list_version_minor.kinetic_list_version_micro])
m4_define([kinetic_list_api_version],
          [kinetic_list_version_major.kinetic_list_version_minor])

AC_INIT(kinetic_list, kinetic_list_version)
AM_INIT_AUTOMAKE([0.1])
AC_CONFIG_HEADERS(config.h)

# Checks for programs.
AC_PROG_CC
#AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC

# enable pkg-config
PKG_PROG_PKG_CONFIG

# Checks for library functions.
#AC_CHECK_FUNCS([memset])

PKG_CHECK_MODULES(CLUTTER, clutter-1.0)
AC_SUBST(CLUTTER_CFLAGS)
AC_SUBST(CLUTTER_LIBS)

PKG_CHECK_MODULES(MX, mx-1.0)
AC_SUBST(MX_CFLAGS)
AC_SUBST(MX_LIBS)

AC_OUTPUT([
Makefile
src/Makefile
])

echo ""
echo "   ${PACKAGE_NAME} $VERSION"
echo "   ====================="
echo ""
echo "   To build the project, run \"make\""
echo ""


