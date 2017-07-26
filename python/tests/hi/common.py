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

from .. import common
import libi8x
import syslog

class TestCase(common.TestCase):
    def setUp(self):
        self._i8xlog = []

    def _logger(self, *args):
        self._i8xlog.append(args)

    def ctx_new(self, flags=None, logger=None, klass=libi8x.Context):
        """Standard way to create a Context for testing.

        If flags is None (the default) then some extra checks will
        be enabled.  Most tests should use take advantage of these."""

        # Enable memory checking where possible.
        if flags is None:
            self.assertIsNone(logger)
            flags = syslog.LOG_DEBUG | libi8x.DBG_MEM
            logger = self._logger

        # Don't randomly log to stderr.
        self.assertTrue(flags & 15 == 0 or logger is not None)

        # Store what we did so that tests that require specific
        # setup can check they got it.
        self.ctx_new_flags = flags
        self.ctx_new_logger = logger

        return klass(flags, logger)

    def tearDown(self):
        # Delete any objects we're referencing.
        keys = [key
                for key, value in self.__dict__.items()
                if isinstance(value, libi8x.Object)]
        try:
            del value
        except UnboundLocalError:
            pass
        for key in sorted(keys):
            self.__dict__.pop(key)

        # Ensure every object we created was released.
        SENSES = {"created": 1, "released": -1}
        counts = {}
        for entry in self._i8xlog:
            priority, filename, line, function, msg = entry
            if priority != syslog.LOG_DEBUG:
                continue
            msg = msg.rstrip().split()
            sense = SENSES.get(msg.pop(), None)
            if sense is None:
                continue
            if msg[-1] == "capsule":
                continue
            what = " ".join(msg)
            count = counts.get(what, 0) + sense
            if count == 0:
                counts.pop(what)
            else:
                counts[what] = count
        self.assertEqual(list(counts.keys()), [],
                         "test completed with unreleased objects")

    def assertIsSequence(self, seq):
        """Check that the object is a sequence."""
        def check(func):
            try:
                func(seq)
                return
            except TypeError as e:
                reason = str(e)
            self.fail(reason)

        def loop(seq):
            for item in seq:
                pass

        check(len)
        check(loop)
