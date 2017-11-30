# Copyright (C) 2016-17 Red Hat, Inc.
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
	-include $(top_builddir)/config.h

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

# ABI version of libi8x.so, for the SONAME.  Ideally we'll resolve
# all potential incompatibilities using versioned symbols and this
# will never be bumped.
LIBI8X_SONAME_VERSION = 1

include_HEADERS = libi8x.h
lib_LTLIBRARIES = libi8x.la

libi8x_la_SOURCES = \
	libi8x-private.h \
	libi8x-test-private.h \
	interp-private.h \
	funcref-private.h \
	inferior-private.h \
	reloc-private.h \
	xctx-private.h \
	archspec.h \
	opcodes.h \
	chunk.c \
	code.c \
	context.c \
	dbg-interp.c \
	errors.c \
	function.c \
	funcref.c \
	inferior.c \
	interp.c \
	list.c \
	object.c \
	note.c \
	readbuf.c \
	reloc.c \
	type.c \
	validator.c \
	xctx.c

EXTRA_DIST += libi8x.sym optable.c

libi8x_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBI8X_SONAME_VERSION):0:0 \
	-Wl,--version-script=$(top_srcdir)/libi8x/libi8x.sym
libi8x_la_DEPENDENCIES = ${top_srcdir}/libi8x/libi8x.sym

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libi8x.pc
EXTRA_DIST += libi8x.pc.in
CLEANFILES += libi8x.pc

# From ldconfig.c:
#
#   If the path the link points to isn't its soname or it is not
#   the .so symlink for ld(1), we treat it as a normal file.
#
#   You should always do this:
#
#      libfoo.so -> SONAME -> Arbitrary package-chosen name.
#
#   e.g. libfoo.so -> libfoo.so.1 -> libfooimp.so.9.99.
#   Given a SONAME of libfoo.so.1.
#
#   You should *never* do this:
#
#      libfoo.so -> libfooimp.so.9.99
#
#   If you do, and your SONAME is libfoo.so.1, then libfoo.so fails
#   to point at the SONAME.  In that case ldconfig may consider
#   libfoo.so as another implementation of SONAME and will create
#   symlinks against it causing problems when you try to upgrade or
#   downgrade.  The problems will arise because ldconfig will,
#   depending on directory ordering, creat symlinks against libfoo.so
#   e.g. libfoo.so.1.2 -> libfoo.so, but when libfoo.so is removed
#   (typically by the removal of a development pacakge not required
#   for the runtime) it will break the libfoo.so.1.2 symlink and the
#   application will fail to start.
install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/libi8x.so
	rm -f $(DESTDIR)$(libdir)/libi8x.so.$(LIBI8X_SONAME_VERSION)
	mv $(DESTDIR)$(libdir)/libi8x.so.$(LIBI8X_SONAME_VERSION).0.0 \
	    $(DESTDIR)$(libdir)/libi8x-$(VERSION).so
	$(LN_S) libi8x-$(VERSION).so \
	    $(DESTDIR)$(libdir)/libi8x.so.$(LIBI8X_SONAME_VERSION)
	$(LN_S) libi8x.so.$(LIBI8X_SONAME_VERSION) \
	    $(DESTDIR)$(libdir)/libi8x.so

	mv $(DESTDIR)$(libdir)/libi8x.la $(DESTDIR)$(libdir)/libi8x.la.orig
	$(SED) \
	    -e "s,\\.so\\.$(LIBI8X_SONAME_VERSION)\\.0\\.0,-$(VERSION).so,g" \
	    < $(DESTDIR)$(libdir)/libi8x.la.orig \
	    > $(DESTDIR)$(libdir)/libi8x.la
	rm -f $(DESTDIR)$(libdir)/libi8x.la.orig
