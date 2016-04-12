# Copyright (C) 2016 Red Hat, Inc.
# This file is part of the Infinity Note Execution Library.
#
# The Infinity Note Execution Library is free software; you can
# redistribute it and/or modify it under the terms of the GNU Lesser
# General Public License as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option)
# any later version.
#
# The Infinity Note Execution Library is distributed in the hope
# that it will be useful, but WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with the Infinity Note Execution Library; if not,
# see <http://www.gnu.org/licenses/>.

EXTRA_DIST =
CLEANFILES =
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-I${top_srcdir}/src

AM_CFLAGS = ${my_CFLAGS} \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections

AM_LDFLAGS = \
	-Wl,--gc-sections \
	-Wl,--as-needed

SED_PROCESS = \
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(SED) \
	-e 's,@VERSION\@,$(VERSION),g' \
	-e 's,@prefix\@,$(prefix),g' \
	-e 's,@exec_prefix\@,$(exec_prefix),g' \
	-e 's,@libdir\@,$(libdir),g' \
	-e 's,@includedir\@,$(includedir),g' \
	< $< > $@ || rm $@

%.pc: %.pc.in Makefile
	$(SED_PROCESS)

LIBI8X_CURRENT=0
LIBI8X_REVISION=0
LIBI8X_AGE=0

pkginclude_HEADERS = src/i8x/libi8x.h
lib_LTLIBRARIES = src/libi8x.la

src_libi8x_la_SOURCES =\
	src/libi8x-private.h \
	src/opcodes.h \
	src/chunk.c \
	src/code.c \
	src/context.c \
	src/function.c \
	src/funcref.c \
	src/list.c \
	src/object.c \
	src/note.c \
	src/readbuf.c \
	src/symref.c \
	src/util.c

EXTRA_DIST += src/libi8x.sym src/optable.c

src_libi8x_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBI8X_CURRENT):$(LIBI8X_REVISION):$(LIBI8X_AGE) \
	-Wl,--version-script=$(top_srcdir)/src/libi8x.sym
src_libi8x_la_DEPENDENCIES = ${top_srcdir}/src/libi8x.sym

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = src/libi8x.pc
EXTRA_DIST += src/libi8x.pc.in
CLEANFILES += src/libi8x.pc

TESTS = src/test-libi8x

check_PROGRAMS = src/test-libi8x
src_test_libi8x_SOURCES = src/test-libi8x.c
src_test_libi8x_LDADD = src/libi8x.la

if HAVE_LIBELF
tlsdump = examples/tlsdump
examples_tlsdump_SOURCES = examples/tlsdump.c
examples_tlsdump_CFLAGS = $(LIBELF_CFLAGS)
examples_tlsdump_LDADD = src/libi8x.la $(LIBELF_LIBS)
else
tlsdump =
endif

noinst_PROGRAMS = $(tlsdump)
