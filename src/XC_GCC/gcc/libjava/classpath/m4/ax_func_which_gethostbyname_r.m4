# m4/ax_func_which_gethostbyname_r.m4

# Copyright ? 2005 Caolan McNamara <caolan@skynet.ie>
# Copyright ? 2005 Daniel Richard G. <skunk@iskunk.org>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# As a special exception, the respective Autoconf Macro's copyright
# owner gives unlimited permission to copy, distribute and modify the
# configure scripts that are the output of Autoconf when processing
# the Macro. You need not follow the terms of the GNU General Public
# License when using or distributing such scripts, even though
# portions of the text of the Macro appear in them. The GNU General
# Public License (GPL) does govern all other use of the material that
# constitutes the Autoconf Macro.

# This special exception to the GPL applies to versions of the
# Autoconf Macro released by the Autoconf Macro Archive. When you make
# and distribute a modified version of the Autoconf Macro, you may
# extend this special exception to the GPL to apply to your modified
# version as well.


AC_DEFUN([AX_FUNC_WHICH_GETHOSTBYNAME_R], [

    AC_LANG_PUSH(C)
    AC_MSG_CHECKING([how many arguments gethostbyname_r() takes])

    AC_CACHE_VAL(ac_cv_func_which_gethostbyname_r, [

################################################################

ac_cv_func_which_gethostbyname_r=unknown

#
# ONE ARGUMENT (sanity check)
#

# This should fail, as there is no variant of gethostbyname_r() that takes
# a single argument. If it actually compiles, then we can assume that
# netdb.h is not declaring the function, and the compiler is thereby
# assuming an implicit prototype. In which case, we're out of luck.
#
AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM(
        [[#include <netdb.h>]],
        [[
            char *name = "www.gnu.org";
            (void)gethostbyname_r(name) /* ; */
        ]]),
    ac_cv_func_which_gethostbyname_r=no)

#
# SIX ARGUMENTS
# (e.g. Linux)
#

if test "$ac_cv_func_which_gethostbyname_r" = "unknown"; then

AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM(
        [[#include <netdb.h>]],
        [[
            char *name = "www.gnu.org";
            struct hostent ret, *retp;
            char buf@<:@1024@:>@;
            int buflen = 1024;
            int my_h_errno;
            (void)gethostbyname_r(name, &ret, buf, buflen, &retp, &my_h_errno) /* ; */
        ]]),
    ac_cv_func_which_gethostbyname_r=six)

fi

#
# FIVE ARGUMENTS
# (e.g. Solaris)
#

if test "$ac_cv_func_which_gethostbyname_r" = "unknown"; then

AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM(
        [[#include <netdb.h>]],
        [[
            char *name = "www.gnu.org";
            struct hostent ret;
            char buf@<:@1024@:>@;
            int buflen = 1024;
            int my_h_errno;
            (void)gethostbyname_r(name, &ret, buf, buflen, &my_h_errno) /* ; */
        ]]),
    ac_cv_func_which_gethostbyname_r=five)

fi

#
# THREE ARGUMENTS
# (e.g. AIX, HP-UX, Tru64)
#

if test "$ac_cv_func_which_gethostbyname_r" = "unknown"; then

AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM(
        [[#include <netdb.h>]],
        [[
            char *name = "www.gnu.org";
            struct hostent ret;
            struct hostent_data data;
            (void)gethostbyname_r(name, &ret, &data) /* ; */
        ]]),
    ac_cv_func_which_gethostbyname_r=three)

fi

################################################################

]) dnl end AC_CACHE_VAL

case "$ac_cv_func_which_gethostbyname_r" in
    three)
    AC_MSG_RESULT([three])
    AC_DEFINE([HAVE_FUNC_GETHOSTBYNAME_R_3], 1, [three-argument gethostbyname_r])
    ;;

    five)
    AC_MSG_RESULT([five])
    AC_DEFINE([HAVE_FUNC_GETHOSTBYNAME_R_5], 1, [five-argument gethostbyname_r])
    ;;

    six)
    AC_MSG_RESULT([six])
    AC_DEFINE([HAVE_FUNC_GETHOSTBYNAME_R_6], 1, [six-argument gethostbyname_r])
    ;;

    no)
    AC_MSG_RESULT([cannot find function declaration in netdb.h])
    ;;

    unknown)
    AC_MSG_RESULT([can't tell])
    ;;

    *)
    AC_MSG_ERROR([internal error])
    ;;
esac

AC_LANG_POP(C)

]) dnl end AC_DEFUN
