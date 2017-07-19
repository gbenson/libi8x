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
import os
import sys
import syslog
import tempfile

class StreamGrabber(object):
    def __init__(self, stream):
        self.stream = stream
        self.fileno = stream.fileno()
        self.is_active = False

    def __del__(self):
        tf = getattr(self, "tempfile", None)
        if tf is not None:
            tf.close()

    def __enter__(self):
        assert not self.is_active
        self.tempfile = tempfile.TemporaryFile()
        self.stream.flush()
        self.saved_fd = os.dup(self.fileno)
        os.dup2(self.tempfile.fileno(), self.fileno)
        self.is_active = True
        return self

    def __exit__(self, exception_type, exception, traceback):
        assert self.is_active
        self.stream.flush()
        os.dup2(self.saved_fd, self.fileno)
        os.close(self.saved_fd)
        self.is_active = False
        if exception is not None:
            raise exception

class StderrGrabber(StreamGrabber):
    def __init__(self):
        super(StderrGrabber, self).__init__(sys.stderr)
        assert self.fileno == 2

class LoggerMixin(object):
    @property
    def has_messages(self):
        return len(self.messages) > 0

    @property
    def is_empty(self):
        return not self.has_messages

    def check_line(self, index, func, *args):
        assert self.has_messages
        args += tuple(self.messages[index])
        func(*args)

class StderrLog(LoggerMixin, StderrGrabber):
    """Context manager to catch libi8x log messages to stderr."""

    def __exit__(self, *args):
        super(StderrLog, self).__exit__(*args)
        self.messages = list(self.__get_messages())

    def __get_messages(self, prefix="py8x: "):
        assert not self.is_active
        self.tempfile.seek(0)
        for line in self.tempfile.readlines():
            line = line.decode("utf-8")
            assert line.startswith(prefix)
            assert line[-1] == "\n"
            yield line[len(prefix):-1].split(": ", 1)

class UserLogger(LoggerMixin):
    """User logger to pass to py8x_ctx_{new,set_log_fn}."""

    def __init__(self):
        self.messages = []

    def __call__(self, pri, fn, ln, func, msg):
        assert msg[-1] == "\n"
        self.messages.append((func, msg[:-1]))

class TestPy8xLog(common.TestCase):

    # Helpers.

    def check_pkgver(self, func, msg):
        """Check a log entry is a name+version announcement."""
        self.assertEqual(func, "i8x_ctx_new")
        self.assertTrue(msg.startswith("libi8x "))

    def __check_action(self, cls, action, msg):
        self.assertTrue(msg.startswith(cls + " 0x"))
        self.assertTrue(msg.endswith(" " + action))

    def check_new(self, cls, func, msg):
        """Check a log entry is an object being created."""
        self.assertEqual(func, "i8x_ob_new")
        self.__check_action(cls, "created", msg)

    def check_encap(self, cls, func, msg):
        """Check a log entry is a capsule being created."""
        self.assertEqual(func, "py8x_encapsulate_2")
        self.__check_action(cls, "capsule created", msg)

    def check_decap(self, cls, func, msg):
        """Check a log entry is a capsule being released."""
        self.assertEqual(func, "py8x_ob_unref")
        self.__check_action(cls, "capsule released", msg)

    def check_unref(self, cls, func, msg):
        """Check a log entry is an object being released."""
        self.assertEqual(func, "i8x_ob_unref_1")
        self.__check_action(cls, "released", msg)

    def check_avail(self, sig, func, msg):
        """Check a log entry is a function becoming available."""
        self.assertEqual(func, "i8x_ctx_fire_availability_observer")
        self.assertEqual(msg, sig + " became available")

    # Tests for where messages flow to.

    def __check_flowtest_result(self, log1, log2=None):
        """Check log(s) for tests that check where messages appear."""
        log1.check_line(0, self.check_pkgver)
        if log2 is None:
            # Single-logger test
            log2 = log1
        else:
            # The logger changed midway
            log1.check_line(-1, self.check_encap, "inferior")
            log2.check_line(0, self.check_decap, "inferior")
        log2.check_line(-1, self.check_unref, "ctx")

    def test_no_user_logger(self):
        """Test the default behaviour (log to stderr)."""
        with StderrLog() as stderr:
            self.ctx_new(syslog.LOG_DEBUG)
        self.__check_flowtest_result(stderr)

    def test_initial_logger(self):
        """Test behaviour with a logger passed to py8x_ctx_new."""
        logger = UserLogger()
        with StderrLog() as stderr:
            self.ctx_new(syslog.LOG_DEBUG, logger)
        self.assertTrue(stderr.is_empty)
        self.__check_flowtest_result(logger)

    def test_late_enable(self):
        """Test behaviour when a logger is specified later."""
        logger = UserLogger()
        with StderrLog() as stderr:
            ctx = self.ctx_new(syslog.LOG_DEBUG)
            inf = py8x.inf_new(ctx)
            py8x.ctx_set_log_fn(ctx, logger)
            del ctx, inf
        self.__check_flowtest_result(stderr, logger)

    def test_reset_to_default(self):
        """Test behaviour when an initial logger is removed."""
        logger = UserLogger()
        with StderrLog() as stderr:
            ctx = self.ctx_new(syslog.LOG_DEBUG, logger)
            inf = py8x.inf_new(ctx)
            py8x.ctx_set_log_fn(ctx, None)
            del ctx, inf
        self.__check_flowtest_result(logger, stderr)

    # Helpers for tests that check logger failures.

    @property
    def BADLOG_TESTS(self):
        # Uncallable object set as logger
        yield (5, TypeError)
        # User loggers with wrong numbers of arguments
        yield (lambda: None, TypeError)
        yield (lambda a: None, TypeError)
        yield (lambda a, b: None, TypeError)
        yield (lambda a, b, c: None, TypeError)
        yield (lambda a, b, c, d: None, TypeError)
        yield (lambda a, b, c, d, e, f: None, TypeError)
        yield (lambda a, b, c, d, e, f, g: None, TypeError)
        yield (lambda a, b, c, d, e, f, g, h: None, TypeError)
        yield (lambda a, b, c, d, e, f, g, h, i: None, TypeError)
        # User logger raises exception
        class Error(Exception):
            pass
        def logger(*args):
            raise Error
        yield (logger, Error)

    def __badlog_test(self, test, func, *args):
        """Test a function with a bad logger."""
        logger, expect = test
        testing_ctx_new = func == self.ctx_new

        # Install the bad logger
        if testing_ctx_new:
            self.assertEqual(args, ())
            args = (syslog.LOG_DEBUG, logger)
        else:
            ctx = py8x.ob_get_ctx(args[0])
            saved_logger =  py8x.ctx_get_log_fn(ctx)
            py8x.ctx_set_log_fn(ctx, logger)

        # Call the function
        try:
            with StderrLog() as stderr:
                self.assertRaises(expect, func, *args)
        finally:
            # Put the good logger back
            if not testing_ctx_new:
                py8x.ctx_set_log_fn(ctx, saved_logger)

        # Check messages got logged to stderr
        if testing_ctx_new:
            self.__check_flowtest_result(stderr)
        else:
            self.__check_badlog_test_result(func, stderr)

    def __check_badlog_test_result(self, testfunc, stderr):
        if testfunc == py8x.ctx_import_bytecode:
            # Special case
            stderr.check_line(0, self.check_new, "note")
            stderr.check_line(-1, self.check_avail, "example::factorial(i)i")
        elif testfunc == py8x.ctx_import_native:
            # Special case
            stderr.check_line(0, self.check_new, "type")
            stderr.check_line(-1, self.check_avail, "test::func()")
        else:
            if testfunc is py8x.ctx_get_funcref:
                # Special case
                bookends = "type", "funcref"
            else:
                # General case
                self.assertTrue(stderr.has_messages)
                cls = stderr.messages[0][1].split(None, 1)[0]
                bookends = cls, cls

            stderr.check_line(0, self.check_new, bookends[0])
            stderr.check_line(-1, self.check_unref, bookends[1])

    def __badlog_ctx(self):
        """Context for __badlog_test tests."""
        return self.ctx_new(syslog.LOG_DEBUG, UserLogger())

    # Logger failure tests for all functions that return new references.

    def test_badlog_ctx_new(self):
        """Test py8x_ctx_new with bad loggers."""
        for test in self.BADLOG_TESTS:
            self.__badlog_test(test, self.ctx_new)

    def test_badlog_ctx_get_funcref(self):
        """Test py8x_ctx_get_funcref with bad loggers."""
        for test in self.BADLOG_TESTS:
            self.__badlog_test(test,
                               py8x.ctx_get_funcref,
                               self.__badlog_ctx(),
                               "test", "func", "Fi(oo)i", "iop")

    def test_badlog_ctx_import_bytecode(self):
        """Test py8x_ctx_import_bytecode with bad loggers."""
        for test in self.BADLOG_TESTS:
            self.__badlog_test(test,
                               py8x.ctx_import_bytecode,
                               self.__badlog_ctx(), self.GOOD_NOTE,
                               "testnote", 0)

    def test_badlog_ctx_import_native(self):
        """Test py8x_ctx_import_native with bad loggers."""
        for test in self.BADLOG_TESTS:
            self.__badlog_test(test,
                               py8x.ctx_import_native,
                               self.__badlog_ctx(), "test", "func",
                               "", "", None)

    def test_badlog_inf_new(self):
        """Test py8x_inf_new with bad loggers."""
        for test in self.BADLOG_TESTS:
            self.__badlog_test(test,
                               py8x.inf_new,
                               self.__badlog_ctx())

    def test_badlog_xctx_new(self):
        """Test py8x_xctx_new with bad loggers."""
        for test in self.BADLOG_TESTS:
            self.__badlog_test(test,
                               py8x.xctx_new,
                               self.__badlog_ctx(), 512)
