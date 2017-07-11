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

class TestPy8xNatfuncImpl(common.PopulatedTestCase):
    def test_exec(self):
        """Test calling a native function."""
        func = py8x.ctx_import_native(self.ctx,
                                      "test", "func", "ipo", "poi",
                                      lambda x, i, f, a, b, c: (a + b - c,
                                                                b + c - a,
                                                                c + a - b))
        ref = py8x.func_get_funcref(func)
        rets = py8x.xctx_call(self.xctx, ref, self.inf, (1, 2, 3))
        self.assertEqual(rets, (0, 4, 2))

    def test_bad_argc(self):
        """Check py8x_natfunc_impl catches wrong numbers of arguments."""
        def impl(xctx, inf, func, arg):
            self.fail()
        for i in (0, 2):
            func = py8x.ctx_import_native(self.ctx,
                                          "test", "func", "i" * i, "", impl)
            ref = py8x.func_get_funcref(func)
            self.assertRaises(TypeError,
                              py8x.xctx_call,
                              self.xctx, ref, self.inf, (1,) * i)

    def test_bad_retc(self):
        """Check py8x_natfunc_impl catches wrong numbers of returns."""
        def impl(xctx, inf, func):
            return (4,)
        for i in (0, 2):
            func = py8x.ctx_import_native(self.ctx,
                                          "test", "func", "", "i" * i, impl)
            ref = py8x.func_get_funcref(func)
            self.assertRaises(py8x.I8XError,
                              py8x.xctx_call,
                              self.xctx, ref, self.inf, ())

    def test_exception(self):
        """Check py8x_natfunc_impl propagates exceptions."""
        class Error(Exception):
            pass
        def impl(xctx, inf, func):
            raise Error("boom")
        func = py8x.ctx_import_native(self.ctx, "test", "func", "", "", impl)
        ref = py8x.func_get_funcref(func)
        self.assertRaises(Error,
                          py8x.xctx_call,
                          self.xctx, ref, self.inf, ())

    def test_func_arg(self):
        """Test calling a native function with a function argument."""
        def impl(xctx, inf, this_func, func_arg, int_arg):
            return py8x.xctx_call(xctx, func_arg, self.inf, (int_arg - 3,))
        func = py8x.ctx_import_native(self.ctx,
                                      "test", "func", "Fi(i)i", "i", impl)
        ref = py8x.func_get_funcref(func)
        rets = py8x.xctx_call(self.xctx, ref, self.inf, (self.funcref, 10))
        self.assertEqual(rets, (5040,))

    def test_func_ret(self):
        """Test calling a native function with a function return."""
        def impl(xctx, inf, func, arg1, arg2):
            return (arg1 + arg2, self.funcref, arg1 - arg2)
        func = py8x.ctx_import_native(self.ctx,
                                      "test", "func", "ii", "pFi(i)p", impl)
        ref = py8x.func_get_funcref(func)
        rets = py8x.xctx_call(self.xctx, ref, self.inf, (3, 4))
        self.assertEqual(rets, (7, self.funcref,
                                py8x.to_unsigned(-1)))
