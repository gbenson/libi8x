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

EXTRA_DIST = proc_service.h

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I${top_srcdir}/libi8x

AM_CFLAGS = ${my_CFLAGS}

if HAVE_LIBELF
# It doesn't seem to be possible to cause automake/libtool to build
# a noinst shared library, so this is going to get installed and then
# removed.  I hate this completely.

lib_LTLIBRARIES = libi8x-tdbx.la
libi8x_tdbx_la_SOURCES = libthread_db.c
libi8x_tdbx_la_CFLAGS = $(LIBELF_CFLAGS)
libi8x_tdbx_la_LIBADD = $(top_builddir)/libi8x/libi8x.la $(LIBELF_LIBS)
libi8x_tdbx_la_LDFLAGS = -avoid-version -module -shared -export-dynamic

all-local:
	ln -sf .libs/libi8x-tdbx.so libthread_db.so.1

clean-local:
	rm -f libthread_db.so.1

install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/libi8x-tdbx.*
endif
