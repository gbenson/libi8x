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
import syslog

class TestContext(common.TestCase):
    def test_libi8x_defaults(self):
        """Test Context.__init__ with no flags or logger."""
        ctx = self.ctx_new(flags=0)
        self.assertIsInstance(ctx, libi8x.Context)
        self.assertEqual(self.ctx_new_flags, 0)
        self.assertIs(self.ctx_new_logger, None)
        del ctx
        self.assertEqual(self._i8xlog, [])

    def test_testcase_defaults(self):
        """Test Context.__init__ with testcase flags and logger."""
        # This test looks like it does nothing, but it'll fail in
        # tearDown if Context.__init__ creates circular references.
        ctx = self.ctx_new()
        self.assertIsInstance(ctx, libi8x.Context)
        self.assertNotEqual(self._i8xlog, [])

    def test_new_inferior(self):
        """Test Context.new_inferior."""
        ctx = self.ctx_new()
        inf = ctx.new_inferior()
        self.assertIsInstance(inf, libi8x.Inferior)
