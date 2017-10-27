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

from .. import *

import glob
import os
import re
import subprocess

TOP_SRCDIR = os.environ.get("LIBI8X_TEST_SRCDIR", TOP_BUILDDIR)

class Symbol(object):
    PUBLIC_PREFIX = "i8x_"
    PRIVATE_PREFIX = "__i8xtest_"

    def __init__(self, env, version, name):
        env.assertTrue(version.startswith("LIBI8X_"))
        self.is_public = name.startswith(self.PUBLIC_PREFIX)
        env.assertEqual(self.is_public, version != "LIBI8X_PRIVATE")
        if not self.is_public:
            env.assertTrue(name.startswith(self.PRIVATE_PREFIX))
            name = self.PUBLIC_PREFIX + name[len(self.PRIVATE_PREFIX):]
        self.name = name

class TestAllTested(APITestCase):
    _i8x_testfile_fmt = os.path.join(TOP_SRCDIR, "tests", "api", "test-%s.c")

    # XXX All functions used by py8x are considered API-tested FOR NOW.
    with open(os.path.join(TOP_SRCDIR, "python", "libi8x.c")) as fp:
        __LIBI8X_C = fp.read()
        del fp

    def _has_testcase(self, function):
        """Return True if the specified function has an API testcase."""
        self.assertTrue(function.startswith("i8x_"))
        if super(TestAllTested, self)._has_testcase(function):
            return True
        if super(TestAllTested, self)._has_testcase("py" + function[1:]):
            return True  # Exposed and tested via py8x
        return self.__LIBI8X_C.find(function + " (") != -1

    LIBI8X_SO = os.path.join(TOP_BUILDDIR, "libi8x", ".libs", "libi8x.so")

    def objdump(self, *args):
        cmd = ("objdump",) + args
        try:
            return subprocess.check_output(cmd).decode("utf-8").split("\n")
        except:
            self.skipTest("objdump failed")

    def soname(self, filename):
        for line in self.objdump("-p", filename):
            line = line.strip().split(None, 1)
            if line and line[0] == "SONAME":
                return line[1]
        self.fail("No SONAME")

    @property
    def libi8x_so_symbols(self):
        self.assertTrue(os.path.exists(self.LIBI8X_SO))
        for line in self.objdump("-T", self.LIBI8X_SO):
            line = line.strip().split()
            print(line)
            for sect in (".text", ".opd"):
                if sect in line and line[-1] != sect:
                    break
            else:
                continue
            if line[-2].startswith("0x"):
                line.pop(-2)
            yield Symbol(self, *line[-2:])

    def test_all_tested(self):
        """Check we test everything we publicly export."""
        self.assertAllTested(symbol.name
                             for symbol in self.libi8x_so_symbols
                             if symbol.is_public)

    @property
    def libi8x_c_exports(self):
        for filename in glob.glob(os.path.join(TOP_SRCDIR, "libi8x", "*.c")):
            with open(filename) as fp:
                src = fp.read()
            for chunk in src.split("I8X_EXPORT")[1:]:
                name = chunk.split("\n", 1)[1].split(" (", 1)[0]
                if name != "INTERPRETER":
                    yield name

    def test_defined_visibility(self):
        """Check every function's visibility is correct."""
        exports = list(sorted(self.libi8x_c_exports))

        # Sanity check.
        symbols = dict((symbol.name, symbol)
                       for symbol in self.libi8x_so_symbols)
        for symbol in symbols.values():
            self.assertIn(symbol.name, exports)

        # The test itself.
        self.assertAllTested(name
                             for name in exports
                             if symbols[name].is_public)

    @property
    def libi8x_h_exports(self):
        with open(os.path.join(TOP_SRCDIR, "libi8x", "libi8x.h")) as fp:
            for line in fp.readlines():
                if line.startswith("#"):
                    continue
                match = re.search(r"(i8x_\w+) \(", line)
                if match is None:
                    continue
                name = match.group(1)
                if not name.endswith("_fn_t"):
                    yield name

    def test_libi8x_h_inclusion(self):
        """Check every prototype in libi8x.h is exported."""
        exports = list(sorted(self.libi8x_h_exports))

        # Sanity check.
        symbols = dict((symbol.name, symbol)
                       for symbol in self.libi8x_so_symbols
                       if symbol.is_public)
        for symbol in symbols.values():
            self.assertIn(symbol.name, exports)

        # The test itself.
        self.assertAllTested(exports)

    def test_libtool_output(self):
        """Check the filesystem libtool created."""

        unversioned_link = self.LIBI8X_SO
        print("unversioned_link =", unversioned_link)
        self.assertTrue(os.path.exists(unversioned_link))
        libdir, check = os.path.split(unversioned_link)
        self.assertEqual(check, "libi8x.so")

        filename = os.path.realpath(unversioned_link)
        print("filename         =", filename)
        self.assertNotEqual(filename, unversioned_link)
        self.assertTrue(os.path.exists(filename))
        self.assertEqual(os.path.dirname(filename), libdir)

        versioned_link = os.path.join(libdir, self.soname(filename))
        print("versioned_link   =", versioned_link)
        self.assertNotEqual(versioned_link, unversioned_link)
        self.assertNotEqual(versioned_link, filename)
        self.assertTrue(os.path.exists(versioned_link))
        self.assertEqual(os.path.realpath(versioned_link), filename)
