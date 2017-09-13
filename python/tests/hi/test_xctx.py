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

from . import *

class TestExecutionContext(TestCase):
    def test_context(self):
        """Test ExecutionContext.context."""
        ctx = self.ctx_new()
        xctx = ctx.new_xctx()
        self.assertIs(xctx.context, ctx)

    def test_is_persistent(self):
        """Test ExecutionContext.is_persistent."""
        self.xctx = self.ctx_new().new_xctx()
        self._test_persistence("xctx")

    def test_call(self):
        """Test ExecutionContext.call."""
        ctx = self.ctx_new()
        ref = ctx.import_bytecode(self.GOOD_NOTE).ref
        inf = ctx.new_inferior()
        xctx = ctx.new_xctx()
        rets = xctx.call(ref, inf, 5)
        self.assertEqual(rets, (120,))
