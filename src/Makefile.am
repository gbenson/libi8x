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

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I${top_srcdir}/src

AM_CFLAGS = ${my_CFLAGS} \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections \
	-fno-gcse

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

pkginclude_HEADERS = i8x/libi8x.h
lib_LTLIBRARIES = libi8x.la

libi8x_la_SOURCES = \
	libi8x-private.h \
	interp-private.h \
	funcref-private.h \
	symref-private.h \
	xctx-private.h \
	opcodes.h \
	chunk.c \
	code.c \
	context.c \
	dbg-interp.c \
	function.c \
	funcref.c \
	interp.c \
	list.c \
	object.c \
	note.c \
	readbuf.c \
	symref.c \
	type.c \
	util.c \
	validator.c \
	xctx.c

EXTRA_DIST += libi8x.sym optable.c

libi8x_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBI8X_CURRENT):$(LIBI8X_REVISION):$(LIBI8X_AGE) \
	-Wl,--version-script=$(top_srcdir)/src/libi8x.sym
libi8x_la_DEPENDENCIES = ${top_srcdir}/src/libi8x.sym

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libi8x.pc
EXTRA_DIST += libi8x.pc.in
CLEANFILES += libi8x.pc
