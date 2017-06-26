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
from .. import common

class TestCase(common.TestCase):
    def ctx_new(self):
        """Standard way to create an i8x_ctx for tests."""
        return py8x.ctx_new(py8x.I8X_DBG_MEM, None)

class PopulatedTestCase(TestCase):
    """A testcase with a context and a loaded function."""

    TESTNOTE = TestCase.GOOD_NOTE

    def setUp(self):
        self.ctx = self.ctx_new()
        self.func = py8x.ctx_import_bytecode(self.ctx, self.TESTNOTE,
                                             "testnote", 0)
        self.xctx = py8x.xctx_new(self.ctx, 512)
        self.inf = py8x.inf_new(self.ctx)
        self.funcref = py8x.func_get_funcref(self.func)
