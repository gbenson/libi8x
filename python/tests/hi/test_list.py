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

from . import common
import libi8x

class TestList(common.TestCase):

    # Functions to import notes.  We use Context.functions as
    # our test list.  These functions are called by __do_test
    # to populate it.

    def import_1(self, ctx):
        return ctx.import_bytecode(self.GOOD_NOTE)

    def import_2(self, ctx):
        return ctx.import_native("test", "func1", "", "", None)

    def import_3(self, ctx):
        return ctx.import_bytecode(self.DEREF_NOTE)

    def import_4(self, ctx):
        return ctx.import_bytecode(self.GOOD_NOTE)

    def import_5(self, ctx):
        return ctx.import_native("test", "func2", "", "", None)

    # List testing framework.

    def __do_test(self, check_list):
        loaders = sorted(attr for attr in dir(self)
                         if attr.startswith("import_"))

        ctx = self.ctx_new()
        expect = []
        actual = ctx.functions

        for index, loader in enumerate(loaders):
            check_list(expect, actual)
            func = getattr(self, loader)(ctx)
            expect.append(func)
        check_list(expect, actual)

    # Individual list tests.

    def test_length(self):
        """Test _I8XList.__len__."""
        def check(expect, actual):
            self.assertEqual(len(actual), len(expect))
        self.__do_test(check)

    def test_iteration(self):
        """Test _I8XList.__iter__."""
        def check(expect, actual):
            eiter = iter(expect)
            aiter = iter(actual)

            # Test _I8XListIterator.__iter__
            self.assertIs(iter(aiter), aiter)

            # Test _I8XListIterator.__next__
            STOP = object()
            while True:
                try:
                    eitem = next(eiter)
                except StopIteration:
                    eitem = STOP

                if eitem is STOP:
                    self.assertRaises(StopIteration, next, aiter)
                    break
                else:
                    aitem = next(aiter)
                    self.assertIs(eitem, aitem)
        self.__do_test(check)

    def test_convert_to_python_list(self):
        """Test _I8XList.__iter__ implicitly."""
        def check(expect, actual):
            self.assertEqual(expect, list(actual))
        self.__do_test(check)
