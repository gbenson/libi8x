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
import os
import syslog
import weakref

class TestContext(common.TestCase):
    def test_context(self):
        """Check there is no Context.context method."""
        ctx = self.ctx_new()
        self.assertFalse(hasattr(ctx, "context"))

    def test_libi8x_defaults(self):
        """Test Context.__init__ with no flags or logger."""
        ctx = self.ctx_new(flags=0)
        self.assertIsInstance(ctx, libi8x.Context)
        self.assertEqual(self.ctx_new_flags, 0)
        self.assertIs(self.ctx_new_logger, None)
        self.assertTrue(ctx.log_priority == 0
                        or "I8X_LOG" in os.environ)
        self.assertIs(ctx.logger, None)
        del ctx
        self.assertEqual(self._i8xlog, [])

    def test_testcase_defaults(self):
        """Test Context.__init__ with testcase flags and logger."""
        # This test looks like it does nothing, but it'll fail in
        # tearDown if Context.__init__ creates circular references.
        ctx = self.ctx_new()
        self.assertIsInstance(ctx, libi8x.Context)
        self.assertNotEqual(self._i8xlog, [])

    def test_log_priority(self):
        """Test Context.log_priority."""
        ctx = self.ctx_new()

        # First test the default, which in the testsuite is
        # logging debug messages to self._i8xlog.
        self.assertEqual(ctx.log_priority, syslog.LOG_DEBUG)
        startlen = len(self._i8xlog)
        ctx.new_inferior() # emit some messages
        self.assertGreater(len(self._i8xlog), startlen)

        # Now test changing the priority.
        ctx.log_priority = syslog.LOG_WARNING
        try:
            # Check the value changed.
            self.assertEqual(ctx.log_priority, syslog.LOG_WARNING)
            # Check changing the value changed the behaviour.
            startlen = len(self._i8xlog)
            ctx.new_inferior() # shouldn't log anything now
            self.assertEqual(len(self._i8xlog), startlen)
        finally:
            # Restore the value for the tearDown checks.
            ctx.log_priority = syslog.LOG_DEBUG

    def test_logger(self):
        """Test Context.logger."""
        ctx = self.ctx_new()

        # The testsuite default stores messages in self._i8xlog.
        self.assertEqual(ctx.logger, self._logger)
        startlen = len(self._i8xlog)

        # Install a new logger.
        testlog = []
        def testlogger(*args):
            testlog.append(args)
        tlwr = weakref.ref(testlogger)

        ctx.logger = testlogger
        try:
            self.assertIs(ctx.logger, testlogger)

            self.assertIsNotNone(tlwr())
            del testlogger
            self.assertIsNotNone(tlwr())

            # Check messages go to our log.
            ctx.new_inferior()
            self.assertEqual(len(self._i8xlog), startlen)
            self.assertGreater(len(testlog), 0)
        finally:
            # Put the old logger back.
            ctx.logger = self._logger
            self.assertEqual(ctx.logger, self._logger)

            # Check we didn't leak references.
            self.assertIsNone(tlwr())

            # Check messages go to the original log.
            testlen = len(testlog)
            ctx.new_inferior()
            self.assertEqual(len(testlog), testlen)
            self.assertGreater(len(self._i8xlog), startlen)

    def test_new_inferior(self):
        """Test Context.new_inferior."""
        ctx = self.ctx_new()
        inf = ctx.new_inferior()
        self.assertIsInstance(inf, libi8x.Inferior)

    def test_new_xctx(self):
        """Test Context.new_xctx."""
        ctx = self.ctx_new()
        xctx = ctx.new_xctx()
        self.assertIsInstance(xctx, libi8x.ExecutionContext)

    def test_functions(self):
        """Test Context.functions."""
        ctx = self.ctx_new()
        self.assertIsSequence(ctx.functions)

    def test_unregister(self):
        """Test Context.unregister."""
        ctx = self.ctx_new()
        func = ctx.import_bytecode(self.GOOD_NOTE)
        self.assertEqual(len(ctx.functions), 1)
        ctx.unregister(func)
        self.assertEqual(len(ctx.functions), 0)

    def test_import_bytecode(self):
        """Test Context.import_bytecode."""
        ctx = self.ctx_new()
        func = ctx.import_bytecode(self.GOOD_NOTE)
        self.assertIsInstance(func, libi8x.Function)

    def test_import_native(self):
        """Test Context.import_native."""
        ctx = self.ctx_new()
        func = ctx.import_native("test", "func", "", "", self.do_not_call)
        self.assertIsInstance(func, libi8x.Function)
