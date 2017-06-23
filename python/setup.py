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
# Python2 distutils can't cope with __future__.unicode_literals

from setuptools import setup, Extension
import glob # XXX
import subprocess # XXX
import os # XXX
import sys # XXX

topdir = os.path.dirname(os.path.realpath(sys.argv[0]))

subprocess.check_call((sys.executable,
                       os.path.join(topdir, "mkpy3capi.py"),
                       os.path.join(os.path.dirname(os.path.dirname(topdir)),
                                    "pycparser", "utils",
                                    "fake_libc_include")))

setup(
    ext_modules=[
        Extension("_libi8x",
                  include_dirs = ["../libi8x"],
                  #XXX libraries = ["libi8x"],
                  #XXX library_dirs = ["../libi8x/.libs"],
                  extra_objects = glob.glob("../libi8x/.libs/*.o"), #XXX
                  sources = ["libi8x.c"])
    ],
    tests_require=["nose"],
    test_suite="nose.collector")
