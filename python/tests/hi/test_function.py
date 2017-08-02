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

class FunctionTestCase(object):
    def test_context(self):
        """Test Function.context."""
        self.assertIs(self.func.context, self.ctx)

    def test_ref(self):
        """Test Function.ref."""
        ref = self.func.ref
        self.assertIsInstance(ref, libi8x.FunctionReference)

    def test_relocations(self):
        """Test Function.relocations."""
        relocs = self.func.relocations
        self.assertIsSequence(relocs)

class TestBytecodeFunction(common.TestCase, FunctionTestCase):
    def setUp(self):
        super(TestBytecodeFunction, self).setUp()
        self.ctx = self.ctx_new()
        self.func = self.ctx.import_bytecode(self.GOOD_NOTE)

class TestNativeFunction(common.TestCase, FunctionTestCase):
    def setUp(self):
        super(TestNativeFunction, self).setUp()
        self.ctx = self.ctx_new()
        self.func = self.ctx.import_native("test", "func", "", "",
                                           self.do_not_call)
