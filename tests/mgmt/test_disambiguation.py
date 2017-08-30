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

class TestDisambiguation(TestCase):
    def setUp(self):
        self.ctx = self.ctx_new()
        self.func1 = self.ctx.import_bytecode(self.GOOD_NOTE)
        self.func2 = self.ctx.import_native(self.func1.signature,
                                            lambda x, i, f, a: (-a,))
        self.ref = self.func1.ref
        self.assertIs(self.ref, self.func2.ref)
        self.assertTrue(self.ref.is_global)

        self.xctx = self.ctx.new_xctx()
        self.inf = self.ctx.new_inferior()

    def test_unregister_1(self):
        """Test unregistering global functions (1)"""
        self.assertFalse(self.ref.is_resolved)
        with self.assertRaises(libi8x.UnresolvedFunctionError):
            self.xctx.call(self.ref, self.inf, 7)
        self.ctx.unregister(self.func2)
        self.assertTrue(self.ref.is_resolved)
        self.assertEqual(self.xctx.call(self.ref, self.inf, 7), (5040,))

    def test_unregister_2(self):
        """Test unregistering global functions (2)"""
        self.assertFalse(self.ref.is_resolved)
        with self.assertRaises(libi8x.UnresolvedFunctionError):
            self.xctx.call(self.ref, self.inf, 7)
        self.ctx.unregister(self.func1)
        self.assertTrue(self.ref.is_resolved)
        self.assertEqual(self.xctx.call(self.ref, self.inf, 7),
                         (libi8x.to_unsigned(-7),))
