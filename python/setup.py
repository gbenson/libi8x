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
import os
import subprocess
import sys

here = os.path.realpath(os.path.dirname(__file__))

# Link libi8x statically if we're in a libi8x tree (tarball or git).
if os.path.basename(here) == "python":
    # Ensure everything is up-to-date.
    if "MAKELEVEL" not in os.environ:
        subprocess.check_call(("make", "-C", os.path.dirname(here)))
    import glob
    print("warning: building static _libi8x")
    extargs = {"include_dirs": ["../libi8x"],
               "extra_objects": glob.glob("../libi8x/.libs/*.o")}
else:
    extargs = {"libraries": ["i8x"]}

# Regenerate libi8x.c if we're running in a checked-out git tree.
hdrdir = os.path.join(here, "pycparser", "utils", "fake_libc_include")
if not os.path.exists(hdrdir):
    # Ensure we don't ever release packages with stale libi8x.c.
    for arg in sys.argv[1:]:
        for bail in ("register", "upload", "dist"):
            if arg.find(bail) >= 0:
                print("Please checkout from git for ‘%s’" % arg)
                sys.exit(1)
else:
    print("regenerating libi8x.c")
    subprocess.check_call((sys.executable, "mkpy3capi.py", hdrdir))

setup(
    name="libi8x",
    version="0.0.1",
    description="Python bindings for libi8x",
    license="LGPLv2.1+",
    author="Gary Benson",
    author_email="infinity@sourceware.org",
    url="https://infinitynotes.org/",
    packages=["libi8x"],
    ext_modules=[
        Extension("_libi8x", sources=["libi8x.c"], **extargs),
    ],
    tests_require=["nose"],
    test_suite="nose.collector")
