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

class TestPy8xCtxUnregisterFunc(common.PopulatedTestCase):
    def test_success(self):
        """Test py8x_ctx_unregister_func succeeding"""
        self.assertTrue(py8x.funcref_is_resolved(self.funcref))
        self.assertIsNone(py8x.ctx_unregister_func(self.ctx, self.func))
        self.assertFalse(py8x.funcref_is_resolved(self.funcref))

    def test_failure(self):
        """Test py8x_ctx_unregister_func failing"""
        py8x.ctx_unregister_func(self.ctx, self.func)
        self.assertRaises(ValueError,
                          py8x.ctx_unregister_func,
                          self.ctx, self.func)

    def test_deduplicate(self):
        """Test py8x_ctx_unregister_func on an ambigious function"""
        self.assertTrue(py8x.funcref_is_resolved(self.funcref))
        func2 = py8x.ctx_import_native(self.ctx, "example", "factorial",
                                       "i", "i", self.do_not_call)
        self.assertFalse(py8x.funcref_is_resolved(self.funcref))
        py8x.ctx_unregister_func(self.ctx, func2)
        self.assertTrue(py8x.funcref_is_resolved(self.funcref))
        self.assertEqual(py8x.xctx_call(self.xctx, self.funcref,
                                        self.inf, (8,)), (40320,))

    def test_replace(self):
        """Test py8x_ctx_unregister_func replacing a function"""
        self.assertTrue(py8x.funcref_is_resolved(self.funcref))
        func2 = py8x.ctx_import_native(self.ctx, "example", "factorial",
                                       "i", "i", lambda x, i, f, a: (-a,))
        self.assertFalse(py8x.funcref_is_resolved(self.funcref))
        py8x.ctx_unregister_func(self.ctx, self.func)
        self.assertTrue(py8x.funcref_is_resolved(self.funcref))
        self.assertEqual(py8x.xctx_call(self.xctx, self.funcref,
                                        self.inf, (8,)),
                         (py8x.to_unsigned(-8),))
