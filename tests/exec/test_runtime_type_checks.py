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

from .. import *

class TestArgRuntimeTypeChecks(TestCase):
    TESTFUNC = "test::testfunc(oFi(i)pp)"
    ARG_0 = 0x12340001
    ARG_1 = "test::argfunc(i)i"
    ARG_2 = 0x12340002
    ARG_3 = 0x12340003

    def __testfunc_impl(self, xctx, inf, func, *args):
        self.assertEqual(func.signature, self.TESTFUNC) # sanity
        self.assertEqual((self.ARG_0,
                          xctx.context.get_funcref(self.ARG_1),
                          self.ARG_2,
                          self.ARG_3), args)

    def test_arg_types(self):
        """Check i8x_xctx_call validates function arguments."""
        self.ctx = self.ctx_new()
        self.inf = self.ctx.new_inferior()
        self.xctx = self.ctx.new_xctx()

        testfunc = self.ctx.import_native(self.TESTFUNC,
                                          self.__testfunc_impl)

        self.__do_test(self.ARG_1) # sanity
        for badsig in ("test::argfunc()i",
                       "test::argfunc(i)",
                       "test::argfunc(ii)i",
                       "test::argfunc(i)ii",
                       "test::argfunc(p)i",
                       "test::argfunc(i)p",
                       "test::argfunc(ip)i",
                       "test::argfunc(i)ip"):
            self.__do_test(badsig, ValueError)

    def __do_test(self, sig, exception=None):
        func = self.ctx.import_native(sig, self.do_not_call)
        for arg_1 in (sig, func.ref):
            if exception is None:
                rets = self.__do_call(arg_1)
                self.assertEqual(len(rets), 0)
            else:
                self.assertRaises(exception, self.__do_call, arg_1)

    def __do_call(self, arg_1):
        return self.xctx.call(self.TESTFUNC, self.inf,
                              self.ARG_0, arg_1, self.ARG_2, self.ARG_3)
