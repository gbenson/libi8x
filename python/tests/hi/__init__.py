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

from libi8xtest import *
import libi8x
import weakref

__all__ = compat_all(
    "libi8x",
    "TestCase",
)

class TestCase(Libi8xTestCase):
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

    def _test_persistence(self, attr):
        """Test getattr(self, attr).is_persistent."""
        obj = getattr(self, attr, None)
        self.assertIsNotNone(obj)
        self.assertTrue(hasattr(obj, "is_persistent"))

        # Ensure we have the only reference.
        delattr(self, attr)

        # Check objects aren't persistent by default.
        self.assertFalse(obj.is_persistent)

        # Test making the object persistent.
        obj.is_persistent = True
        self.assertTrue(obj.is_persistent)

        # Check the object persists.
        ref = weakref.ref(obj)
        del obj
        obj = ref()
        self.assertIsNotNone(obj)

        # Test making the object transient.
        obj.is_persistent = False
        self.assertFalse(obj.is_persistent)

        # Check the object now gets freed.  If it didn't then we
        # didn't have the only reference and the above persistence
        # check was invalid.
        del obj
        obj = ref()
        self.assertIsNone(obj)
