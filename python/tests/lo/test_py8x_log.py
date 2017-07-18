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

    # Tests for where messages flow to.

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
