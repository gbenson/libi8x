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
                                      "test::func(ipo)poi",
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
                                          "test::func(%s)" % ("i" * i),
                                          impl)
            ref = py8x.func_get_funcref(func)
            self.assertRaises(TypeError,
                              py8x.xctx_call,
                              self.xctx, ref, self.inf, (1,) * i)

    def test_bad_retc(self):
        """Check py8x_natfunc_impl catches wrong numbers of returns."""
        def testfunc(*args):
            return rets

        for expect_retc in range(5):
            func = py8x.ctx_import_native(self.ctx,
                                          "test::func()%s"
                                          % ("i" * expect_retc),
                                          testfunc)
            ref = py8x.func_get_funcref(func)

            for rets in (None, (), [], 1, (4,), [4], (2, 3), [2, 3]):
                print("%s: %s" % (expect_retc, rets))
                if rets is None:
                    actual_retc = 0
                else:
                    try:
                        actual_retc = len(rets)
                    except TypeError:
                        actual_retc = 1

                if actual_retc == expect_retc:
                    py8x.xctx_call(self.xctx, ref, self.inf, ())
                    continue

                with self.assertRaises(ValueError) as cm:
                    py8x.xctx_call(self.xctx, ref, self.inf, ())
                self.assertEqual(cm.exception.args[0],
                                 "wrong number of returns "
                                 + "(expected %d, got %d)"
                                 % (expect_retc, actual_retc))

    def test_exception(self):
        """Check py8x_natfunc_impl propagates exceptions."""
        class Error(Exception):
            pass
        def impl(xctx, inf, func):
            raise Error("boom")
        func = py8x.ctx_import_native(self.ctx, "test::func()", impl)
        ref = py8x.func_get_funcref(func)
        self.assertRaises(Error,
                          py8x.xctx_call,
                          self.xctx, ref, self.inf, ())

    def test_func_arg(self):
        """Test calling a native function with a function argument."""
        def impl(xctx, inf, this_func, func_arg, int_arg):
            return py8x.xctx_call(xctx, func_arg, self.inf, (int_arg - 3,))
        func = py8x.ctx_import_native(self.ctx,
                                      "test::func(Fi(i)i)i", impl)
        ref = py8x.func_get_funcref(func)
        rets = py8x.xctx_call(self.xctx, ref, self.inf, (self.funcref, 10))
        self.assertEqual(rets, (5040,))

    def test_func_ret_funcref(self):
        """Test calling a native function returning a funcref."""
        self.__do_test_funcret(self.funcref)

    def test_func_ret_good_string(self):
        """Test calling a native function returning a function signature."""
        self.__do_test_funcret("example::factorial(i)i")

    def test_func_ret_bad_string(self):
        """Test calling a native function returning a bad signature."""
        with self.assertRaises(ValueError):
            self.__do_test_funcret("example:factorial(i)i")

    def test_func_ret_bad_type(self):
        """Test calling a native function returning a non-function object."""
        for ret in (None, 4, self.func):
            with self.assertRaises(TypeError) as cm:
                self.__do_test_funcret(ret)
            self.assertEqual(str(cm.exception),
                             "an i8x_funcref or string is required")

    def __do_test_funcret(self, ret):
        def impl(xctx, inf, func, arg1, arg2):
            return (arg1 + arg2, ret, arg1 - arg2)
        func = py8x.ctx_import_native(self.ctx,
                                      "test::func(ii)pFi(i)p", impl)
        try:
            ref = py8x.func_get_funcref(func)
            rets = py8x.xctx_call(self.xctx, ref, self.inf, (3, 4))
            self.assertEqual(rets, (7, self.funcref, py8x.to_unsigned(-1)))
        finally:
            py8x.ctx_unregister_func(self.ctx, func)

    def __do_integer_return_test(self, value_in, value_out):
        def impl(xctx, inf, func):
            return value_in
        func = py8x.ctx_import_native(self.ctx, "test::func()i", impl)
        ref = py8x.func_get_funcref(func)
        rets = py8x.xctx_call(self.xctx, ref, self.inf, ())
        self.assertEqual(rets, (value_out,))

    def test_return_negative(self):
        """Test a native function returning a negative value."""
        self.__do_integer_return_test(-17, py8x.to_unsigned(-17))

    def test_return_large_positive(self):
        """Test a native function returning a large positive value."""
        value = py8x.to_unsigned(-23)
        self.assertGreater(value, 0)
        self.__do_integer_return_test(value, value)

    def test_return_convertable(self):
        """Test a native function returning a convertable object."""
        class Value(object):
            def __int__(self):
                return 14
        value = Value()
        self.__do_integer_return_test(value, int(value))
