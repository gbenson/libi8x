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
import struct

class TestPy8xInfSetReadMemFn(common.PopulatedTestCase):
    TESTNOTE = common.PopulatedTestCase.DEREF_NOTE

    def test_unset(self):
        """Test deref when no memory reader function is set."""
        self.assertRaises(py8x.Error,
                          py8x.xctx_call,
                          self.xctx, self.funcref, self.inf, (5,))

    def test_set_to_None(self):
        """Test deref when memory reader function is set to None."""
        py8x.inf_set_read_mem_fn(self.inf, None)
        self.test_unset()

    def test_set(self):
        """Test deref when a user memory reader function is set."""
        STORAGE = b"HeLlOmUm"
        def readmem(inf, addr, len):
            return STORAGE[addr:addr + len]
        py8x.inf_set_read_mem_fn(self.inf, readmem)
        self.assertIn(py8x.xctx_call(self.xctx, self.funcref, self.inf, (3,)),
                      [struct.unpack(fmt, b"lOmU") for fmt in (b">I", b"<I")])

    def test_exception(self):
        """Check py8x_read_mem_fn propagates exceptions."""
        class Error(Exception):
            pass
        def readmem(inf, addr, len):
            raise Error("boom")
        py8x.inf_set_read_mem_fn(self.inf, readmem)
        self.assertRaises(Error,
                          py8x.xctx_call,
                          self.xctx, self.funcref, self.inf, (2,))
