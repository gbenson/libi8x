# Copyright (C) 2017 Red Hat, Inc.
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
	README.statictest \
	libi8x.c.in \
	libi8x.c \
	tests/README \
	$(shell find . -name '*.py')

mostlyclean-local:
	rm -rf .eggs build *.egg* *.so
	find . -name '*.pyc' | xargs rm -f

libi8x.c: mkpy3capi.py libi8x.c.in
	$(PYTHON) mkpy3capi.py pycparser/utils/fake_libc_include
