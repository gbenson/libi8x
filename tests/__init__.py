# -*- coding: utf-8 -*-
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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import os
import subprocess
import sys

# Directories.
TESTDIR = os.path.dirname(__file__)     # libi8x/tests
TOPDIR = os.path.dirname(TESTDIR)       # libi8x
PYDIR = os.path.join(TOPDIR, "python")  # libi8x/python

# Build _libi8x.so if we aren't running under python/setup.py.
# This allows us to e.g. run individual tests using nosetests.
if sys.argv[0] != "setup.py":
    subprocess.check_call(
        (sys.executable, "setup.py", "build_ext", "--inplace"),
        cwd=PYDIR)
    if PYDIR not in sys.path:
        sys.path.insert(0, PYDIR)

from libi8xtest import *
import libi8x

__all__ = compat_all(
    "libi8x",
    "TestCase",
)

class TestCase(Libi8xTestCase):
    pass
