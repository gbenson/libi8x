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
            py8x.ctx_get_funcref(ctx, "")

    def test_corrupt_note(self):
        """Test a function returning I8X_NOTE_CORRUPT."""
        self.__test_I8XError(py8x.CorruptNoteError,
                             self.CORRUPT_NOTE,
                             self.CORRUPT_NOTE_ERROR_OFFSET)

    def test_unhandled_note(self):
        """Test a function returning I8X_NOTE_UNHANDLED."""
        self.__test_I8XError(py8x.UnhandledNoteError,
                             self.UNHANDLED_NOTE,
                             self.UNHANDLED_NOTE_ERROR_OFFSET)

    def test_invalid_note(self):
        """Test a function returning I8X_NOTE_INVALID."""
        self.__test_I8XError(py8x.InvalidNoteError,
                             self.INVALID_NOTE,
                             self.INVALID_NOTE_ERROR_OFFSET)

    def test_divide_by_zero(self):
        """Test a function returning I8X_DIVIDE_BY_ZERO."""
        self.__test_I8XError(py8x.DivideByZeroError,
                             self.DIVIDE_BY_ZERO_NOTE,
                             self.DIVIDE_BY_ZERO_NOTE_ERROR_OFFSET)

    def test_stack_overflow(self):
        """Test a function returning I8X_STACK_OVERFLOW."""
        self.__test_I8XError(py8x.StackOverflowError,
                             self.STACK_OVERFLOW_NOTE,
                             self.STACK_OVERFLOW_NOTE_ERROR_OFFSET)

    def test_unresolved_function(self):
        """Test a function returning I8X_UNRESOLVED_FUNCTION."""
        ctx = self.ctx_new()
        ref = py8x.ctx_get_funcref(ctx, "example::factorial(i)i")
        inf = py8x.inf_new(ctx)
        xctx = py8x.xctx_new(ctx, 512)
        with self.assertRaises(py8x.UnresolvedFunctionError):
            py8x.xctx_call(xctx, ref, inf, (5,))

    def test_natcall_bad_funcref_ret(self):
        """Test a function returning I8X_NATCALL_BAD_FUNCREF_RET."""
        ctx = self.ctx_new()
        sig = "test::func()Fi(i)"
        def impl(*args):
            return sig
        func = py8x.ctx_import_native(ctx, sig, impl)
        inf = py8x.inf_new(ctx)
        xctx = py8x.xctx_new(ctx, 512)
        with self.assertRaises(py8x.ReturnTypeError):
            py8x.xctx_call(xctx, sig, inf, ())

    # Helpers

    def __test_I8XError(self, exception, note, error_offset, *call_args):
        for srcname in (None, "testnote"):
            for srcoffset in (-4, -1, 0, 5, 23):
                if issubclass(exception, py8x.NoteError):
                    assert not call_args
                    call_args = None
                else:
                    assert issubclass(exception, py8x.ExecutionError)

                if srcoffset < 0:
                    expect_offset = None
                else:
                    expect_offset = srcoffset + error_offset

                self.__test_I8XError_1(exception, expect_offset,
                                       (note, srcname, srcoffset),
                                       call_args)

    def __test_I8XError_1(self, exception, expect_offset, import_args,
                          call_args):
        ctx = self.ctx_new()
        if call_args is None:
            # NoteError (raised on import).
            with self.assertRaises(exception) as cm:
                py8x.ctx_import_bytecode(ctx, *import_args)
        else:
            # ExecutionError (raised on execution).
            func = py8x.ctx_import_bytecode(ctx, *import_args)
            ref = py8x.func_get_funcref(func)
            inf = py8x.inf_new(ctx)
            xctx = py8x.xctx_new(ctx, 512)
            with self.assertRaises(exception) as cm:
                py8x.xctx_call(xctx, ref, inf, call_args)
        args = cm.exception.args

        self.assertEqual(len(args), 3)
        self.assertEqual(args[1], import_args[1])
        self.assertEqual(args[2], expect_offset)
