AC_INIT(cryptominisat,4.0.0)

AC_CANONICAL_HOST

AC_CONFIG_SRCDIR(cryptominisat4/cryptominisat.cpp)

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE

AC_PROG_LIBTOOL

AC_PROG_INSTALL

AC_PROG_CXX

AX_CXX_COMPILE_STDCXX_11

# Debugging support
AC_ARG_ENABLE(debug, [  --enable-debug          Enable assert() statements for debugging.])

if test "x$enable_debug" = x"yes"; then
   DEBUG_FLAGS="-g"
   AC_SUBST(DEBUG_FLAGS)
else
   AC_DEFINE(NDEBUG,1,[Define whether debugging is enabled])
fi

# M4RI support

AC_ARG_WITH(m4ri,
        AS_HELP_STRING([--with-m4ri@<:@=VALUE@:>@], [Optional prefix for M4RI library]),[cryptominisat4_config_m4riprefix=$withval])

have_m4ri=false        
if test -n "$cryptominisat4_config_m4riprefix"; then
   CRYPTOMINISAT4_M4RI_PREFIX=${cryptominisat4_config_m4riprefix}
   AC_SUBST(CRYPTOMINISAT4_M4RI_PREFIX)
   CRYPTOMINISAT4_M4RI_LDFLAGS="-L${cryptominisat4_config_m4riprefix}/lib"
   AC_SUBST(CRYPTOMINISAT4_M4RI_LDFLAGS)
   CRYPTOMINISAT4_M4RI_CFLAGS="-I${cryptominisat4_config_m4riprefix}/include"
   AC_SUBST(CRYPTOMINISAT4_M4RI_CFLAGS)

   AX_M4RI_CFLAGS()
   M4RI_CFLAGS=$ax_cv_m4ri_cflags
   AC_SUBST(M4RI_CFLAGS)

   have_m4ri=true
fi

AM_CONDITIONAL([HAVE_M4RI], []test x$have_m4ri = xtrue)

AC_DEFINE(STATS_NEEDED,1,[something]) # do we need stats? Yes we do!


RELEASE="AC_PACKAGE_VERSION"
AC_SUBST(RELEASE)

AC_PROG_MAKE_SET

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

