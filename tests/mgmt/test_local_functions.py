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

class TestLocalFunctions(TestCase):
    SIGNATURE = "::(i)p"

    def setUp(self):
        self.ctx = self.ctx_new()
        self.func1 = self.ctx.import_native(self.SIGNATURE, self.__func1)
        self.func2 = self.ctx.import_native(self.SIGNATURE, self.__func2)
        self.ref1 = self.func1.ref
        self.ref2 = self.func2.ref

        self.assertIsNot(self.ref1, self.ref2)
        self.assertEqual(self.ref1.signature, self.SIGNATURE)
        self.assertEqual(self.ref2.signature, self.SIGNATURE)
        self.assertFalse(self.ref1.is_global)
        self.assertFalse(self.ref2.is_global)

        self.xctx = self.ctx.new_xctx()
        self.inf = self.ctx.new_inferior()

    def __func1(self, xctx, inf, func, arg):
        self.assertIs(func, self.func1)
        return arg * arg

    def __func2(self, xctx, inf, func, arg):
        self.assertIs(func, self.func2)
        return arg + 4

    def test_separation(self):
        """Test local functions don't mask one another"""
        self.assertEqual(self.xctx.call(self.ref1, self.inf, 8), (64,))
        self.assertEqual(self.xctx.call(self.ref2, self.inf, 8), (12,))

    def test_unregister_1(self):
        """Test unregistering local functions (1)"""
        self.ctx.unregister(self.func1)
        self.assertFalse(self.ref1.is_resolved)
        self.assertTrue(self.ref2.is_resolved)
        self.assertEqual(self.xctx.call(self.ref2, self.inf, 17), (21,))
        with self.assertRaises(libi8x.UnresolvedFunctionError):
            self.xctx.call(self.ref1, self.inf, 8)
        self.ctx.unregister(self.func2)
        self.assertFalse(self.ref1.is_resolved)
        self.assertFalse(self.ref2.is_resolved)

    def test_unregister_2(self):
        """Test unregistering local functions (2)"""
        self.ctx.unregister(self.func2)
        self.assertFalse(self.ref2.is_resolved)
        self.assertTrue(self.ref1.is_resolved)
        self.assertEqual(self.xctx.call(self.ref1, self.inf, 23), (529,))
        with self.assertRaises(libi8x.UnresolvedFunctionError):
            self.xctx.call(self.ref2, self.inf, 8)
        self.ctx.unregister(self.func1)
        self.assertFalse(self.ref1.is_resolved)
        self.assertFalse(self.ref2.is_resolved)

    def test_no_call_by_sig(self):
        """Ensure local functions can't be called by signature"""
        with self.assertRaises(libi8x.UnresolvedFunctionError):
            self.xctx.call(self.SIGNATURE, self.inf, 8)
        self.ctx.unregister(self.func1)
        with self.assertRaises(libi8x.UnresolvedFunctionError):
            self.xctx.call(self.SIGNATURE, self.inf, 8)
        self.ctx.unregister(self.func2)
        with self.assertRaises(libi8x.UnresolvedFunctionError):
            self.xctx.call(self.SIGNATURE, self.inf, 8)
