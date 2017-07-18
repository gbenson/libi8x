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
import weakref

class TestPy8xCtxSetLogFn(common.TestCase):
    def test_default(self):
        """Test get with no initial logger."""
        ctx = self.ctx_new()
        self.assertIsNone(py8x.ctx_get_log_fn(ctx))

    def test_set_at_init(self):
        """Test get with a logger passed to py8x_ctx_new."""
        class Logger(object):
            pass
        ctx = self.ctx_new(log_fn=Logger)
        self.assertIs(py8x.ctx_get_log_fn(ctx), Logger)

    def test_set_later(self):
        """Test setting a logger."""
        class Logger(object):
            pass
        ctx = self.ctx_new()
        py8x.ctx_set_log_fn(ctx, Logger)
        self.assertIs(py8x.ctx_get_log_fn(ctx), Logger)

    def test_overwrite(self):
        """Test replacing a logger."""
        class Logger(object):
            pass
        logger1 = Logger()
        ctx = self.ctx_new(log_fn=logger1)
        logger1 = weakref.ref(logger1)
        self.assertIsNotNone(logger1())

        logger2 = Logger()
        py8x.ctx_set_log_fn(ctx, logger2)
        logger2 = weakref.ref(logger2)
        self.assertIsNotNone(logger2())
        self.assertIsNone(logger1())

        del ctx
        self.assertIsNone(logger2())
