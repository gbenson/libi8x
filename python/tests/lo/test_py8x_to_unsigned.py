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
from . import common
import sys

class TestPy8xToUnsigned(common.TestCase):
    def test_positive(self):
        """Test py8x_to_unsigned with positive numbers."""
        for x in (0, 1, 23, sys.maxsize):
            self.assertEqual(py8x.to_unsigned(x), x)
        self.assertRaises(OverflowError, py8x.to_unsigned, sys.maxsize + 1)

    def test_negative(self):
        """Test py8x_to_unsigned with negative numbers."""
        wordsize = {0x7fffffff: 32, 0x7fffffffffffffff: 64}[sys.maxsize]
        for x in (-1, -23, -sys.maxsize, -sys.maxsize - 1):
            y = (1 << wordsize) + x
            self.assertNotEqual(x, y)
            self.assertGreater(y, sys.maxsize)
            self.assertEqual(py8x.to_unsigned(x), y)
        self.assertRaises(OverflowError, py8x.to_unsigned, -sys.maxsize - 2)
