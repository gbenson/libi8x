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

import _libi8x as py8x
import libi8x
import os
import unittest

class TestAPI(unittest.TestCase):
    CONSTANTS = {
        "DBG_MEM": 16,
        "LOG_TRACE": 8,
        "to_signed": py8x.to_signed,
        "to_unsigned": py8x.to_unsigned,
        }

    def test_payload(self):
        """Check we only import what we need for __all__."""
        for attr in dir(libi8x):
            self.assertTrue(
                attr in libi8x.__all__
                or attr.startswith("__")
                or attr in (
                    # from __future__ import X
                    "absolute_import",
                    "division",
                    "print_function",
                    # from X import Y, Z
                    "core"))

    def test_constants(self):
        """Check the values of constants we expose."""
        for attr, value in self.CONSTANTS.items():
            self.assertEqual(getattr(libi8x, attr), value)

    def test_classes(self):
        """Check all classes we expose have tests."""
        shortnames = {}
        for sn, cn in libi8x.Context._Context__LONG_CLASSNAMES.items():
            cls = getattr(libi8x.Context, cn + "_CLASS")
            shortnames[cls.__name__] = sn

        for attr in libi8x.__all__:
            if attr in self.CONSTANTS:
                continue
            if attr == "Object":
                continue
            if attr.endswith("Error"):
                continue

            # Check this class has a testfile.
            testfile = self.__testfile_for(attr.lower())
            if not os.path.exists(testfile):
                testfile = self.__testfile_for(shortnames[attr])
            self.assertTrue(os.path.exists(testfile),
                            testfile + ": file not found")

            # Check the testfile tests generic methods.
            with open(testfile) as fp:
                for line in fp.readlines():
                    if line.strip() == "def test_context(self):":
                        break
                else:
                    self.fail(testfile + ": missing tests")

    def __testfile_for(self, classname):
        return os.path.join(os.path.dirname(__file__),
                            "test_" + classname + ".py")
