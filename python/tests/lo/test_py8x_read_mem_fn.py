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
import sys

class TestPy8xReadMemFn(common.PopulatedTestCase):
    TESTNOTE = common.PopulatedTestCase.DEREF_NOTE

    @staticmethod
    def __readmem(inf, addr, len):
        """A correct read_memory function."""
        return b"HeLlOmUmXoXoX"[addr:addr + len]

    EXPECT_RESULT = struct.unpack({"little": b"<",
                                   "big": b">"}[sys.byteorder] + b"I",
                                  b"lOmU")

    def __do_test(self):
        result = py8x.xctx_call(self.xctx, self.funcref, self.inf, (3,))
        self.assertEqual(result, self.EXPECT_RESULT)

    def test_no_readmem_func(self):
        """Test py8x_read_mem_fn with no memory reader function."""
        self.assertRaises(py8x.I8XError, self.__do_test)

    def test_read_memory(self):
        """Test reading memory."""
        py8x.inf_set_read_mem_fn(self.inf, self.__readmem)
        self.__do_test()

    def test_bad_argc(self):
        """Check py8x_read_mem_fn catches wrong numbers of arguments."""
        for readmem in (lambda: None,
                        lambda a: None,
                        lambda a, b: None,
                        lambda a, b, c, d: None):
            py8x.inf_set_read_mem_fn(self.inf, readmem)
            self.assertRaises(TypeError, self.__do_test)

    def test_bad_result_type(self):
        """Check py8x_read_mem_fn catches results of the wrong type."""
        for result in (None, 0, [1, 2, 3, 4], (0, 1, 2, 3), (4,), []):
            def readmem(inf, addr, len):
                return result
            py8x.inf_set_read_mem_fn(self.inf, readmem)
            self.assertRaises(TypeError, self.__do_test)

    def test_bad_result_length(self):
        """Check py8x_read_mem_fn catches results of the wrong length."""
        for size in range(10):
            if size != 4:
                def readmem(inf, addr, len):
                    return b"HeLlOmUmXyXyX"[:size]
                py8x.inf_set_read_mem_fn(self.inf, readmem)
                self.assertRaises(py8x.I8XError, self.__do_test)

    def test_exception(self):
        """Check py8x_read_mem_fn propagates exceptions."""
        class Error(Exception):
            pass
        def readmem(inf, addr, len):
            raise Error("boom")
        py8x.inf_set_read_mem_fn(self.inf, readmem)
        self.assertRaises(Error, self.__do_test)
