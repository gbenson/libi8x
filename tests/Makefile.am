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

EXTRA_DIST = \
	corpus \
	$(shell find . -name '*.py')

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I${top_srcdir}/libi8x \
	-I${top_srcdir}/libi8x-test \
	-DLIBI8X_TEST_SRCDIR='"${srcdir}"'

AM_CFLAGS = ${my_CFLAGS}

AM_LDFLAGS = \
	$(top_builddir)/libi8x-test/libi8x-test.la

check_PROGRAMS = \
	bugs/test-bug-0001 \
	bugs/test-bug-0002 \
	bugs/test-bug-0003 \
	exec/ops/test-deref \
	exec/test-factorial \
	exec/test-smoke \
	valid/test-corpus

check_SCRIPTS = \
	python-tests

TESTS = $(check_PROGRAMS) $(check_SCRIPTS)
