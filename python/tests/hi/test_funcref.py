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

class TestFunctionReference(common.TestCase):
    def setUp(self):
        super(TestFunctionReference, self).setUp()
        self.ctx = self.ctx_new()
        func = self.ctx.import_bytecode(self.GOOD_NOTE)
        self.funcref = func.ref

    def test_context(self):
        """Test FunctionReference.context."""
        self.assertIs(self.funcref.context, self.ctx)

    def test_resolved(self):
        """Test FunctionReference.is_resolved returning True."""
        self.assertTrue(self.funcref.is_resolved)

    def test_unresolved(self):
        """Test FunctionReference.is_resolved returning False."""
        self.ctx.import_native("example", "factorial", "i", "i", None)
        self.assertFalse(self.funcref.is_resolved)
