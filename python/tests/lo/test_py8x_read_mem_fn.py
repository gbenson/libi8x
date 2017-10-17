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
import struct
import sys

TEST_MEMORY = b"HeLlOmUmXoXoX"
TEST_OFFSET = 3

class TestPy8xReadMemFn_32be(common.PopulatedTestCase):
    WORDSIZE, BIGENDIAN = 32, True

    @staticmethod
    def __readmem(inf, addr, len):
        """A correct read_memory function."""
        return TEST_MEMORY[addr:addr + len]

    @property
    def EXPECT_RESULT(self):
        fmt = ((b"<", b">")[self.BIGENDIAN]
               + {32: b"I", 64: b"Q"}[self.WORDSIZE])
        len = struct.calcsize(fmt)
        return struct.unpack(fmt, TEST_MEMORY[TEST_OFFSET:TEST_OFFSET+len])

    def __archspec(self, wordsize, bigendian):
        base = "i8"
        if not bigendian:
            base = reversed(base)
        return common.compat_bytes(ord(c) ^ wordsize for c in base)

    @property
    def TESTNOTE(self):
        want_archspec = self.__archspec(self.WORDSIZE, self.BIGENDIAN)

        note = self.DEREF_NOTE
        archspec_start = 3
        archspec_limit = archspec_start + len(want_archspec)
        # Validate archspec_start before we rewrite the note.
        check_preamble = b"\5\1\3" # I8_CHUNK_CODEINFO, version 1, size 3
        check_start = archspec_start - len(check_preamble)
        self.assertEqual(note[check_start:archspec_limit],
                         check_preamble
                         + self.__archspec(32, False))
        # Rewrite the note with the desired archspec.
        return note[:archspec_start] + want_archspec + note[archspec_limit:]


    def __do_test(self):
        result = py8x.xctx_call(self.xctx, self.funcref, self.inf,
                                (TEST_OFFSET,))
        print(self.WORDSIZE, self.BIGENDIAN)
        print("expect", hex(self.EXPECT_RESULT[0]),
              "actual", hex(result[0]))
        self.assertEqual(result, self.EXPECT_RESULT)

    def test_no_readmem_func(self):
        """Test py8x_read_mem_fn with no memory reader function."""
        self.assertRaises(AttributeError, self.__do_test)

    def test_object_readmem_func(self):
        """Test py8x_read_mem_fn with the function set in the object."""
        self.inf.read_memory = self.__readmem
        self.__do_test()

    def test_class_readmem_func(self):
        """Test py8x_read_mem_fn with the function set in the class."""
        cls = common.TestObject
        try:
            cls.read_memory = self.__readmem
            self.__do_test()
        finally:
            del cls.read_memory

    def test_bad_argc(self):
        """Check py8x_read_mem_fn catches wrong numbers of arguments."""
        for readmem in (lambda: None,
                        lambda a: None,
                        lambda a, b: None,
                        lambda a, b, c, d: None):
            self.inf.read_memory = readmem
            self.assertRaises(TypeError, self.__do_test)

    def test_bad_result_type(self):
        """Check py8x_read_mem_fn catches results of the wrong type."""
        for result in (None, 0, [1, 2, 3, 4], (0, 1, 2, 3), (4,), []):
            def readmem(inf, addr, len):
                return result
            self.inf.read_memory = readmem
            self.assertRaises(TypeError, self.__do_test)

    def test_bad_result_length(self):
        """Check py8x_read_mem_fn catches results of the wrong length."""
        expect_size = self.WORDSIZE >> 3
        for size in range(10):
            if size != expect_size:
                def readmem(inf, addr, len):
                    return b"HeLlOmUmXyXyX"[:size]
                self.inf.read_memory = readmem
                with self.assertRaises(ValueError) as cm:
                    self.__do_test()
                self.assertEqual(str(cm.exception),
                                 "read_memory returned bad length"
                                 + " (expected %d, got %d)" % (
                                     expect_size, size))

    def test_exception(self):
        """Check py8x_read_mem_fn propagates exceptions."""
        class Error(Exception):
            pass
        def readmem(inf, addr, len):
            raise Error("boom")
        self.inf.read_memory = readmem
        self.assertRaises(Error, self.__do_test)

class TestPy8xReadMemFn_32el(TestPy8xReadMemFn_32be):
    BIGENDIAN = False

if sys.maxsize >> 32:
    class TestPy8xReadMemFn_64be(TestPy8xReadMemFn_32be):
        WORDSIZE = 64

    class TestPy8xReadMemFn_64el(TestPy8xReadMemFn_64be):
        BIGENDIAN = False
