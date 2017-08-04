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
import unittest

class TestConstants(unittest.TestCase):
    def test_i8x_ctx_new_flags(self):
        """Test i8x_ctx_new flags."""
        self.assertEqual(py8x.LOG_TRACE, 0x08)
        self.assertEqual(py8x.DBG_MEM, 0x10)

    def test_chunk_types(self):
        """Test chunk types constants."""
        self.assertEqual(py8x.I8_CHUNK_SIGNATURE, 1)
        self.assertEqual(py8x.I8_CHUNK_BYTECODE, 2)
        self.assertEqual(py8x.I8_CHUNK_EXTERNALS, 3)
        self.assertEqual(py8x.I8_CHUNK_STRINGS, 4)
        self.assertEqual(py8x.I8_CHUNK_CODEINFO, 5)

    def test_byte_orderings(self):
        """Test byte order constants."""
        self.assertEqual(py8x.BYTE_ORDER_UNKNOWN, 0)
        self.assertEqual(py8x.BYTE_ORDER_NATIVE, 1)
        self.assertEqual(py8x.BYTE_ORDER_REVERSED, 2)

    def test_error_codes(self):
        """Test error codes."""
        self.assertEqual(py8x.EINVAL, -98)

        self.assertEqual(py8x.NOTE_CORRUPT, -199)
        self.assertEqual(py8x.NOTE_UNHANDLED, -198)
        self.assertEqual(py8x.NOTE_INVALID, -197)

        self.assertEqual(py8x.UNRESOLVED_FUNCTION, -299)
        self.assertEqual(py8x.STACK_OVERFLOW, -298)
        self.assertEqual(py8x.RELOC_FAILED, -297)
        self.assertEqual(py8x.READ_MEM_FAILED, -296)
        self.assertEqual(py8x.DIVIDE_BY_ZERO, -295)
        self.assertEqual(py8x.NATCALL_FAILED, -294)
