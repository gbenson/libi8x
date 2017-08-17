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

class TestPy8xUnpackFuncRef(common.PopulatedTestCase):
    def __do_test_1(self, ref_or_sig):
        return py8x.xctx_call(self.xctx, ref_or_sig, self.inf, (5,))

    def __do_test(self, ref_or_sig, exception=None):
        if exception is None:
            rets = self.__do_test_1(ref_or_sig)
            self.assertEqual(rets, (120,))
        else:
            with self.assertRaises(exception) as cm:
                self.__do_test(ref_or_sig)
            if exception is TypeError:
                self.assertEqual(str(cm.exception),
                                 "an i8x_funcref or string is required")
            elif exception is py8x.UnresolvedFunctionError:
                self.assertEqual(cm.exception.args,
                                 ("Unresolved function", None, None))

    # Success tests.

    def test_funcref_success(self):
        """Test py8x_unpack_funcref succeeding with a funcref."""
        self.__do_test(self.funcref)

    def test_string_success(self):
        """Test py8x_unpack_funcref succeeding with a string."""
        self.__do_test("example::factorial(i)i")

    # Failure tests.

    def test_neither(self):
        """Test py8x_unpack_funcref with neither a funcref or a string."""
        self.__do_test(self, TypeError)

    def test_non_funcref_i8x_object(self):
        """Test py8x_unpack_funcref with a non-funcref i8x_object."""
        self.__do_test(self.func, TypeError)

    def test_string_bad_signature(self):
        """Test py8x_unpack_funcref with an invalid string."""
        self.__do_test("example:factorial(i)i", ValueError)

    def test_string_embedded_nul(self):
        """Test py8x_unpack_funcref with a NUL in the string."""
        self.__do_test("example::factorial(i)i\0p", ValueError)

    def test_string_unresolved(self):
        """Test py8x_unpack_funcref with a valid string that's unresolved."""
        self.__do_test("examp1e::factorial(i)i",
                       py8x.UnresolvedFunctionError)
