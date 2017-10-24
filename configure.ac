dnl Copyright (C) 2016-17 Red Hat, Inc.
dnl This file is part of the Infinity Note Execution Library.
dnl
dnl The Infinity Note Execution Library is free software; you can
dnl redistribute it and/or modify it under the terms of the GNU Lesser
dnl General Public License as published by the Free Software
dnl Foundation; either version 2.1 of the License, or (at your option)
dnl any later version.
dnl
dnl The Infinity Note Execution Library is distributed in the hope
dnl that it will be useful, but WITHOUT ANY WARRANTY; without even the
dnl implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
dnl PURPOSE.  See the GNU Lesser General Public License for more
dnl details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with the Infinity Note Execution Library; if not,
dnl see <http://www.gnu.org/licenses/>.

AC_PREREQ(2.60)
AC_INIT([libi8x],
        [0.0.5],
        [infinity@sourceware.org],
        [libi8x],
        [https://sourceware.org/gdb/wiki/Infinity])
AC_CONFIG_SRCDIR([libi8x/libi8x.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([
	check-news
	foreign
	1.11
	-Wall
	-Wno-portability
	silent-rules
	tar-pax
	no-dist-gzip
	dist-xz
	subdir-objects
])
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_CONFIG_MACRO_DIR([m4])
AM_SILENT_RULES([yes])
LT_INIT
AC_PREFIX_DEFAULT([/usr])

AC_PROG_SED
AC_PROG_MKDIR_P

AC_CHECK_FUNCS([ \
	__secure_getenv \
	secure_getenv\
])

AC_CHECK_PROGS(PYTHON, [python3 python python2], "")

dnl Check for libelf, lifted and extended from glib.  Note that only
dnl the tlsdump example requires this, the main library does not use
dnl libelf and will build just fine without.
AC_ARG_ENABLE(libelf,
              AS_HELP_STRING([--disable-libelf],
	                     [build without libelf support]))
AS_IF([ test "x$enable_libelf" != "xno"],[
PKG_CHECK_MODULES([LIBELF], [libelf >= 0.8.12],
                  [have_libelf=yes], [have_libelf=maybe])
AS_IF([ test $have_libelf = maybe ], [
  AC_CHECK_LIB([elf], [elf_begin], [:], [have_libelf=no])
  AC_CHECK_LIB([elf], [elf_kind], [:], [have_libelf=no])
  AC_CHECK_LIB([elf], [elf_nextscn], [:], [have_libelf=no])
  AC_CHECK_LIB([elf], [gelf_getshdr], [:], [have_libelf=no])
  AC_CHECK_LIB([elf], [elf_getdata], [:], [have_libelf=no])
  AC_CHECK_LIB([elf], [gelf_getnote], [:], [have_libelf=no])
  AC_CHECK_LIB([elf], [elf_end], [:], [have_libelf=no])

  AC_CHECK_HEADER([libelf.h], [:], [have_libelf=no])
  AC_CHECK_HEADER([gelf.h], [:], [have_libelf=no])

  if test $have_libelf != no; then
    LIBELF_LIBS=-lelf
    have_libelf=yes
  fi
])
])
AM_CONDITIONAL([HAVE_LIBELF], [test x$have_libelf = xyes])

my_CFLAGS="\
-Werror \
-Wall \
-Wchar-subscripts \
-Wformat-security \
-Wmissing-declarations \
-Wmissing-prototypes \
-Wnested-externs \
-Wpointer-arith \
-Wshadow \
-Wsign-compare \
-Wstrict-prototypes \
-Wtype-limits \
"
AC_SUBST([my_CFLAGS])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	Makefile
	contrib/Makefile
	examples/Makefile
	libi8x/Makefile
	libi8x-test/Makefile
	python/Makefile
	tests/Makefile
	libi8x.spec])
AC_CONFIG_FILES([tests/python-tests], [chmod +x tests/python-tests])

AC_OUTPUT
