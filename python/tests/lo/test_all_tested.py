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
import os
import unittest

class TestAllTested(unittest.TestCase):
    def test_all_tested(self):
        """Check we test everything we export."""
        format = os.path.join(os.path.dirname(__file__), "test_py8x_%s.py")
        unchecked = []
        for attr in dir(py8x):
            if attr.startswith("__"):
                continue
            if attr.startswith("I8"):
                continue
            if attr.endswith("Error"):
                continue
            if os.path.exists(format % attr):
                continue

            # Allow get and set tests to be combined
            tmp = None
            if attr.find("_get_") >= 0:
                tmp = attr.replace("_get_", "_get_set_")
            elif attr.find("_set_") >= 0:
                tmp = attr.replace("_set_", "_get_set_")
            if tmp is not None and os.path.exists(format % tmp):
                continue

            unchecked.append(attr)
        if unchecked:
            self.fail("unchecked: " + " ".join(unchecked))
