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
import sys

class TestPy8xSetException(common.TestCase):
    def test_memory_error(self):
        """Test a function returning I8X_ENOMEM."""
        ctx = self.ctx_new()
        with self.assertRaises(MemoryError):
            py8x.xctx_new(ctx, sys.maxsize * 2 + 1)

    def test_invalid_argument(self):
        """Test a function returning I8X_EINVAL."""
        ctx = self.ctx_new()
        with self.assertRaises(ValueError):
            py8x.ctx_get_funcref(ctx, "", "", "", "")

    def __decorated_test(self, sname, soff):
        print(sname, soff)
        if soff < 0:
            expect_soff = None
        else:
            expect_soff = soff + 38

        ctx = self.ctx_new()
        with self.assertRaises(py8x.I8XError) as cm:
            py8x.ctx_import_bytecode(ctx, self.CORRUPT_NOTE, sname, soff)
        args = cm.exception.args

        self.assertEqual(len(args), 4)
        self.assertEqual(args[0], py8x.NOTE_CORRUPT)
        self.assertEqual(args[1], "Corrupt note")
        self.assertEqual(args[2], sname)
        self.assertEqual(args[3], expect_soff)

    def test_decorated(self):
        """Test a decorated I8XError (one caused by a note)."""
        for srcname in (None, "testnote"):
            for srcoffset in (-4, -1, 0, 5, 23):
                self.__decorated_test(srcname, srcoffset)
