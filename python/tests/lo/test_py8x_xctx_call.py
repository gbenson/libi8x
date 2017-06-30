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

class TestPy8xXctxCall(common.PopulatedTestCase):
    def test_correct(self):
        """Test a correct py8x_xctx_call."""
        rets = py8x.xctx_call(self.xctx, self.funcref, self.inf, (5,))
        self.assertEqual(rets, (120,))

    def test_unresolved(self):
        """Test py8x_xctx_call of an unresolved function."""
        ref = py8x.ctx_get_funcref (self.ctx, "exmapel", "factorial", "i", "i")
        self.assertRaises(py8x.I8XError,
                          py8x.xctx_call,
                          self.xctx, ref, self.inf, (5,))

    def test_wrong_argument_count(self):
        """Test py8x_xctx_call with the wrong number of arguments."""
        for args in ((), (5, 3)):
            self.assertRaises(py8x.I8XError,
                              py8x.xctx_call,
                              self.xctx, self.funcref, self.inf, args)

    def test_function_arg(self):
        """Test py8x_xctx_call with a function argument."""
        func = py8x.ctx_import_bytecode(self.ctx, self.FUNC_ARG_NOTE,
                                        "testnote", 0)
        self.assertEqual(py8x.xctx_call(self.xctx,
                                        py8x.func_get_funcref(func),
                                        self.inf,
                                        (self.funcref, 5)),
                         (39916800,))

    def test_function_arg_bad_type(self):
        """Check py8x_xctx_call validates function arguments."""
        func = py8x.ctx_import_bytecode(self.ctx, self.FUNC_ARG_NOTE,
                                        "testnote", 0)
        self.assertRaises(AttributeError,
                          py8x.xctx_call,
                          self.xctx, py8x.func_get_funcref(func),
                          self.inf, (4, 5))

    def test_function_ret(self):
        """Test py8x_xctx_call with a function that returns a function."""
        func = py8x.ctx_import_bytecode(self.ctx, self.FUNC_RET_NOTE,
                                        "testnote", 0)
        rets = py8x.xctx_call(self.xctx, py8x.func_get_funcref(func),
                              self.inf, ())
        self.assertEqual(len(rets), 1)
        self.assertIs(rets[0], self.funcref)
