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
    def test_success(self):
        """Test py8x_xctx_call when i8x_xctx_call returns I8X_OK."""
        rets = py8x.xctx_call(self.xctx, self.funcref, self.inf, (5,))
        self.assertEqual(rets, (120,))

    def test_failure(self):
        """Test py8x_xctx_call when i8x_xctx_call returns an error."""
        ref = py8x.ctx_get_funcref(self.ctx, "exmapel::factorial(i)i")
        self.assertRaises(py8x.UnresolvedFunctionError,
                          py8x.xctx_call,
                          self.xctx, ref, self.inf, (5,))

    def test_wrong_argument_count(self):
        """Test py8x_xctx_call with the wrong number of arguments."""
        for args in ((), (5, 3)):
            with self.assertRaises(ValueError) as cm:
                py8x.xctx_call(self.xctx, self.funcref, self.inf, args)
            self.assertEqual(cm.exception.args[0],
                             "wrong number of arguments (expected 1, got %d)"
                             % len(args))

    def test_function_arg_funcref(self):
        """Test py8x_xctx_call with a funcref function argument."""
        self.__do_test_funcarg(self.funcref)

    def test_function_arg_good_string(self):
        """Test py8x_xctx_call with a string function argument."""
        self.__do_test_funcarg("example::factorial(i)i")

    def test_function_arg_bad_string(self):
        """Test py8x_xctx_call with an invalid string function argument."""
        with self.assertRaises(ValueError) as cm:
            self.__do_test_funcarg("example:factorial(i)i")

    def __do_test_funcarg(self, ref_or_sig):
        func = py8x.ctx_import_bytecode(self.ctx, self.FUNC_ARG_NOTE,
                                        "testnote", 0)
        self.assertEqual(py8x.xctx_call(self.xctx,
                                        py8x.func_get_funcref(func),
                                        self.inf,
                                        (ref_or_sig, 5)),
                         (39916800,))

    def test_function_arg_bad_type(self):
        """Check py8x_xctx_call validates function arguments."""
        func = py8x.ctx_import_bytecode(self.ctx, self.FUNC_ARG_NOTE,
                                        "testnote", 0)
        for arg in (None, 4, func):
            with self.assertRaises(TypeError) as cm:
                py8x.xctx_call(self.xctx,
                               py8x.func_get_funcref(func),
                               self.inf, (arg, 5))
            self.assertEqual(str(cm.exception),
                             "an i8x_funcref or string is required")

    def test_function_ret(self):
        """Test py8x_xctx_call with a function that returns a function."""
        func = py8x.ctx_import_bytecode(self.ctx, self.FUNC_RET_NOTE,
                                        "testnote", 0)
        rets = py8x.xctx_call(self.xctx, py8x.func_get_funcref(func),
                              self.inf, ())
        self.assertEqual(len(rets), 1)
        self.assertIs(rets[0], self.funcref)
