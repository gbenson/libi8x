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
import syslog

class TestPy8xCtxNew(common.TestCase):
    def test_basic(self):
        """Test py8x_ctx_new with no flags or log function."""
        ctx = py8x.ctx_new(0, None)
        self.assertIsNot(ctx, None)

    def test_logging(self):
        """Test py8x_ctx_new with logging and a log function."""
        messages = []
        def log_func(*args):
            messages.append(args)
        py8x.ctx_new(syslog.LOG_DEBUG, log_func)
        self.assertGreater(len(messages), 0)
        self.assertEqual(len(messages[0]), 5)
        priority, filename, line, function, message = messages[0]
        self.assertEqual(priority, syslog.LOG_DEBUG)
        assert filename.endswith("context.c")
        self.assertEqual(function, "i8x_ctx_new")
        assert message.startswith("ctx 0x")
        assert message.endswith(" created\n")
