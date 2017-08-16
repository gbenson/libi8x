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

class TestExceptions(common.TestCase):
    # Note there's six exception classes, and for those raised from
    # notes there's four variants to consider (imported with srcname
    # or not, imported with srcoffset or not).  The low-level layer
    # tests all combinations; this file needs to test just enough to
    # ensure that __str__ and the accessors work.

    def test_corrupt_note(self):
        """Test CorruptNoteError, with no source name or offset."""
        ctx = self.ctx_new()
        with self.assertRaises(libi8x.CorruptNoteError) as cm:
            ctx.import_bytecode(self.CORRUPT_NOTE)
        e = cm.exception

        self.assertIs(e.srcname, None)
        self.assertIs(e.srcoffset, None)
        self.assertEqual(str(e), e.strerror)
        self.assertEqual(e.strerror, "Corrupt note")

    def test_invalid_note(self):
        """Test InvalidNoteError, with a source name but no offset."""
        ctx = self.ctx_new()
        with self.assertRaises(libi8x.InvalidNoteError) as cm:
            ctx.import_bytecode(self.INVALID_NOTE, __file__)
        e = cm.exception

        self.assertEqual(e.srcname, __file__)
        self.assertIs(e.srcoffset, None)
        self.assertEqual(str(e), __file__ + ": " + e.strerror)
        self.assertEqual(e.strerror, "Invalid note")

    def test_unhandled_note(self):
        """Test UnhandledNoteError, with a source offset but no name."""
        ctx = self.ctx_new()
        with self.assertRaises(libi8x.UnhandledNoteError) as cm:
            ctx.import_bytecode(self.UNHANDLED_NOTE, srcoffset=23)
        e = cm.exception

        self.assertIs(e.srcname, None)
        self.assertEqual(e.srcoffset, 23 + self.UNHANDLED_NOTE_ERROR_OFFSET)
        self.assertEqual(str(e), "[0x%x]: %s" % (e.srcoffset, e.strerror))
        self.assertEqual(e.strerror, "Unhandled note")

    def test_stack_overflow(self):
        """Test StackOverflowError, with a source offset but no name."""
        ctx = self.ctx_new()
        func = ctx.import_bytecode(self.STACK_OVERFLOW_NOTE, None, 17)
        ref = func.ref
        inf = ctx.new_inferior()
        xctx = ctx.new_xctx()
        with self.assertRaises(libi8x.StackOverflowError) as cm:
            xctx.call(ref, inf)
        e = cm.exception

        self.assertIs(e.srcname, None)
        self.assertEqual(e.srcoffset,
                         17 + self.STACK_OVERFLOW_NOTE_ERROR_OFFSET)
        self.assertEqual(str(e), "[0x%x]: %s" % (e.srcoffset, e.strerror))
        self.assertEqual(e.strerror, "Stack overflow")

    def test_divide_by_zero(self):
        """Test DivideByZeroError, with both source name and offset."""
        ctx = self.ctx_new()
        func = ctx.import_bytecode(self.DIVIDE_BY_ZERO_NOTE, __file__, 11)
        ref = func.ref
        inf = ctx.new_inferior()
        xctx = ctx.new_xctx()
        with self.assertRaises(libi8x.DivideByZeroError) as cm:
            xctx.call(ref, inf)
        e = cm.exception

        self.assertEqual(e.srcname, __file__)
        self.assertEqual(e.srcoffset,
                         11 + self.DIVIDE_BY_ZERO_NOTE_ERROR_OFFSET)
        self.assertEqual(str(e), "%s[0x%x]: %s" % (__file__, e.srcoffset,
                                                   e.strerror))
        self.assertEqual(e.strerror, "Division by zero")

    def test_unresolved_function(self):
        """Test UnresolvedFunctionError."""
        ctx = self.ctx_new()
        ref = ctx.get_funcref("x::x()")
        inf = ctx.new_inferior()
        xctx = ctx.new_xctx()
        with self.assertRaises(libi8x.UnresolvedFunctionError) as cm:
            xctx.call(ref, inf)
        e = cm.exception

        self.assertIs(e.srcname, None)
        self.assertIs(e.srcoffset, None)
        self.assertEqual(str(e), e.strerror)
        self.assertEqual(e.strerror, "Unresolved function")
