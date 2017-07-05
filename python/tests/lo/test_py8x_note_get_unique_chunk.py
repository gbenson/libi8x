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

class TestPy8xFuncGetUniqueChunk(common.PopulatedTestCase):
    PRESENT = py8x.I8_CHUNK_CODEINFO
    ABSENT = 64

    def test_basic(self):
        """Test py8x_func_get_unique_chunk on a chunk that exists."""
        note = py8x.func_get_note (self.func)
        chunk = py8x.note_get_unique_chunk(note, self.PRESENT, True)
        self.assertIsNotNone(chunk)

    def test_absent_ok(self):
        """Test py8x_func_get_unique_chunk on an optional chunk."""
        note = py8x.func_get_note (self.func)
        chunk = py8x.note_get_unique_chunk(note, self.ABSENT, False)
        self.assertIsNone(chunk)

    def test_absent_fail(self):
        """Test py8x_func_get_unique_chunk on a required absent chunk."""
        note = py8x.func_get_note (self.func)
        self.assertRaises(py8x.I8XError,
                          py8x.note_get_unique_chunk,
                          note, self.ABSENT, True)
