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

class TestPy8xReadMemFn(common.PopulatedTestCase):
    TESTNOTE = common.PopulatedTestCase.DEREF_NOTE

    def test_bad_argc(self):
        """Check py8x_read_mem_fn catches wrong numbers of arguments."""
        for readmem in (lambda: None,
                        lambda a: None,
                        lambda a, b: None,
                        lambda a, b, c, d: None):
            py8x.inf_set_read_mem_fn(self.inf, readmem)
            self.assertRaises(TypeError,
                              py8x.xctx_call,
                              self.xctx, self.funcref, self.inf, (5,))

    def test_bad_result_type(self):
        """Check py8x_read_mem_fn catches results of the wrong type."""
        for result in (None, 0, [1, 2, 3, 4], (0, 1, 2, 3), (4,), []):
            def readmem(inf, addr, len):
                return result
            py8x.inf_set_read_mem_fn(self.inf, readmem)
            self.assertRaises(TypeError,
                              py8x.xctx_call,
                              self.xctx, self.funcref, self.inf, (5,))

    def test_bad_result_length(self):
        """Check py8x_read_mem_fn catches results of the wrong length."""
        for size in range(10):
            if size != 4:
                def readmem(inf, addr, len):
                    return b"HeLlOmUmXyXyX"[:size]
                py8x.inf_set_read_mem_fn(self.inf, readmem)
                self.assertRaises(py8x.I8XError,
                                  py8x.xctx_call,
                                  self.xctx, self.funcref, self.inf, (5,))

    def test_exception(self):
        """Check py8x_read_mem_fn propagates exceptions."""
        class Error(Exception):
            pass
        def readmem(inf, addr, len):
            raise Error("boom")
        py8x.inf_set_read_mem_fn(self.inf, readmem)
        self.assertRaises(Error,
                          py8x.xctx_call,
                          self.xctx, self.funcref, self.inf, (5,))
