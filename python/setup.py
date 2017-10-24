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
from codecs import open
import os
import subprocess
import sys

VERSION = "0.0.1"

here = os.path.realpath(os.path.dirname(__file__))

# Link libi8x statically if we're in a libi8x tree (tarball or git),
# and run C libi8x's Python tests too as well as our own.
py8x_version = VERSION
statictest_readme = os.path.join(here, "README.statictest")
if os.path.exists(statictest_readme):
    # Don't install static bindings system-wide.
    for index, arg in enumerate(sys.argv):
        if index > 0 and arg.startswith("install"):
            if "--user" not in sys.argv[index + 1:]:
                with open(statictest_readme, encoding="utf-8") as fp:
                    sys.stderr.write(fp.read())
                sys.exit(1)

    # Ensure everything is up-to-date.
    if "MAKELEVEL" not in os.environ:
        subprocess.check_call(("make", "-C", os.path.dirname(here)))

    # Set up what to build.
    import glob
    print("warning: building static _libi8x")
    py8x_version += "-static"
    srcdir = os.environ.get("LIBI8X_TEST_SRCDIR", "..")
    extargs = {"include_dirs": [os.path.join(srcdir, "libi8x")],
               "extra_objects": glob.glob("../libi8x/.libs/*.o")}

    # Collect C libi8x tests as well as our own.
    testsuite="statictest.collector"
else:
    extargs = {"libraries": ["i8x"]}
    testsuite="nose.collector"

if not "define_macros" in extargs:
    extargs["define_macros"] = []
extargs["define_macros"].append(("PY8X_VERSION", '"%s"' % py8x_version))

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
    version=VERSION,
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
    test_suite=testsuite)
