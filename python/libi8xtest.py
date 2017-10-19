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

import libi8x
import os
import sys
import syslog
import unittest

if sys.version_info < (3,):
    def compat_bytes(iterable_of_ints):
        encoding = "iso-8859-1"
        return "".join(chr(value).decode(encoding)
                       for value in iterable_of_ints).encode(encoding)

    def compat_all(*items):
        return tuple(item.encode("us-ascii") for item in items)
else:
    compat_bytes = bytes
    compat_all = lambda *items: items

__all__ = compat_all(
    "APITestCase",
    "BaseTestCase",
    "compat_all",
    "compat_bytes",
    "Libi8xTestCase",
)

class APITestCase(unittest.TestCase):
    PY_TOPDIR = os.path.dirname(__file__)

    _py8x_testfile_fmt = os.path.join(PY_TOPDIR, "tests", "lo", "test_%s.py")

    def _has_testcase(self, function):
        """Return True if the specified function has an API testcase."""
        fmt = getattr(self, "_%s_testfile_fmt" % function.split("_", 1)[0])
        filename = fmt % function
        if os.path.exists(filename):
            return True

        # Allow get and set tests to be combined.
        dirname, basename = os.path.split(filename)
        for try_combine in ("_get_", "_set_"):
            if basename.find(try_combine) == -1:
                continue
            combined = basename.replace(try_combine, "_get_set_")
            return os.path.exists(os.path.join(dirname, combined))
        return False

    def assertAllTested(self, functions):
        """Raise AssertionError unless all functions have testcases."""
        all_checked = True
        for func in sorted(functions):
            if not self._has_testcase(func):
                print("UNCHECKED:", func)
                all_checked = False
        self.assertTrue(all_checked)

class BaseTestCase(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(BaseTestCase, self).__init__(*args, **kwargs)
        self._libi8xtest_user_setUp = self.setUp
        self.setUp = self._libi8xtest_setUp
        self._libi8xtest_user_tearDown = self.tearDown
        self.tearDown = self._libi8xtest_tearDown

    def do_not_call(self, *args):
        """Callable object to be passed to things that need them."""
        self.fail("crowbar called")

    # 32-bit little-endian version of I8C's factorial.i8 example.
    GOOD_NOTE = compat_bytes((
        # 0x00..0x0f
        0x05, 0x01, 0x03, 0x18, 0x49, 0x03, 0x02, 0x03,   # |....I...|
        0x13, 0x12, 0x31, 0x2b, 0x28, 0x04, 0x00, 0x31,   # |..1+(..1|

        # 0x10..0x1f
        0x2f, 0x09, 0x00, 0x12, 0x31, 0x1c, 0xff, 0x01,   # |/...1...|
        0x00, 0xff, 0x00, 0x1e, 0x01, 0x02, 0x04, 0x0a,   # |........|

        # 0x20..0x2f
        0x00, 0x12, 0x12, 0x04, 0x01, 0x14, 0x66, 0x61,   # |......fa|
        0x63, 0x74, 0x6f, 0x72, 0x69, 0x61, 0x6c, 0x00,   # |ctorial.|

        # 0x30..0x39
        0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x00,   # |example.|
        0x69, 0x00))                                      # |i.|

    # Version of GOOD_NOTE that should be rejected as corrupt
    # (the string table chunk is truncated by one byte).
    CORRUPT_NOTE = GOOD_NOTE[:-1]
    CORRUPT_NOTE_ERROR_OFFSET = 0x26

    # Version of GOOD_NOTE that should be rejected as unhandled
    # (the third instruction is an operation we don't support).
    UNHANDLED_NOTE = GOOD_NOTE[:11] + b"\x97" + GOOD_NOTE[12:]
    UNHANDLED_NOTE_ERROR_OFFSET = 11

    # Version of GOOD_NOTE that should be rejected as invalid
    # (a "dup" has been replaced with a "swap", messing up the
    # stack).
    INVALID_NOTE = GOOD_NOTE[:9] + b"\x16" + GOOD_NOTE[10:]
    INVALID_NOTE_ERROR_OFFSET = 9

    # Version of GOOD_NOTE with a very long name.
    @property
    def LONG_NAME_NOTE(self):
        offset = 0x1c  # of start of signature chunk.
        self.assertEqual(self.GOOD_NOTE[offset:offset + 3],
                         compat_bytes((0x01,    # I8_CHUNK_SIGNATURE
                                       0x02,    # version 2
                                       0x04)))  # size (4 bytes)
        return (self.GOOD_NOTE[:offset]
                + compat_bytes((0x01,        # I8_CHUNK_SIGNATURE
                                0x02,        # version 2
                                0x04,        # size (4 bytes)
                                0x02,        #   provider_offset
                                0x02,        #   name_offset
                                0x00,        #   rtypes_offset
                                0x00,        #   ptypes_offset

                                0x04,        # I8_CHUNK_STRINGS
                                0x01,        # version 1
                                0xf1, 0x03,  # size (497 bytes)
                                0x69,        # 'i'
                                0x00))       # '\0'
                + b"_".join((b"longname",) * 55) + b"\0")

    # 32-bit little-endian note that divides by zero.
    #
    # define test::DZ returns int
    #   load 0
    #   dup
    #   div
    DIVIDE_BY_ZERO_NOTE = compat_bytes((
        # 0x00..0x0f
        0x05, 0x01, 0x03, 0x18, 0x49, 0x02, 0x02, 0x03,   # |....I...|
        0x03, 0x30, 0x12, 0x1b, 0x01, 0x02, 0x04, 0x00,   # |.0......|

        # 0x10..0x1f
        0x05, 0x04, 0x08, 0x04, 0x01, 0x0a, 0x74, 0x65,   # |......te|
        0x73, 0x74, 0x00, 0x44, 0x5a, 0x00, 0x69, 0x00))  # |st.DZ.i.|
    DIVIDE_BY_ZERO_NOTE_ERROR_OFFSET = 11

    # 32-bit little-endian note that overflows the stack.
    #
    # define test::overflow
    #   call overflow
    STACK_OVERFLOW_NOTE = compat_bytes((
        # 0x00..0x0f
        0x05, 0x01, 0x03, 0x18, 0x49, 0x01, 0x02, 0x03,   # |....I...|
        0x05, 0xff, 0x01, 0x00, 0xff, 0x00, 0x01, 0x02,   # |........|

        # 0x10..0x1f
        0x04, 0x09, 0x00, 0x08, 0x08, 0x04, 0x01, 0x0e,   # |........|
        0x6f, 0x76, 0x65, 0x72, 0x66, 0x6c, 0x6f, 0x77,   # |overflow|

        # 0x20..0x25
        0x00, 0x74, 0x65, 0x73, 0x74, 0x00))              # |.test.|
    STACK_OVERFLOW_NOTE_ERROR_OFFSET = 9

    # 32-bit little-endian note to test pointer dereferencing.
    #
    # define test::deref returns ptr
    #   argument ptr x
    #   deref ptr
    DEREF_NOTE = compat_bytes((
        # 0x00..0x0f
        0x05, 0x01, 0x03, 0x18, 0x49, 0x01, 0x02, 0x03,   # |....I...|
        0x01, 0x06, 0x01, 0x02, 0x04, 0x06, 0x00, 0x0b,   # |........|

        # 0x10..0x1f
        0x0b, 0x04, 0x01, 0x0d, 0x64, 0x65, 0x72, 0x65,   # |....dere|
        0x66, 0x00, 0x74, 0x65, 0x73, 0x74, 0x00, 0x70,   # |f.test.p|

        # 0x20..0x20
        0x00))                                            # |.|

    # 32-bit little-endian note to test relocation.
    #
    # extern ptr extptr
    # define test::extern returns ptr
    #   load extptr
    RELOC_NOTE = compat_bytes((
        # 0x00..0x0f
        0x05, 0x01, 0x03, 0x18, 0x49, 0x01, 0x02, 0x03,   # |....I...|
        0x05, 0x03, 0x6b, 0x63, 0x75, 0x66, 0x01, 0x02,   # |........|

        # 0x10..0x1f
        0x04, 0x07, 0x00, 0x06, 0x0c, 0x04, 0x01, 0x0e,   # |........|
        0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x00, 0x74,   # |extern.t|

        # 0x20..0x25
        0x65, 0x73, 0x74, 0x00, 0x70, 0x00))              # |est.p.|

    # 32-bit little-endian note that takes a function as an argument.
    # It returns f(x*2 + 1).
    #
    # define test::func_arg returns int
    #   argument func int (int) f
    #   argument int x
    #   mul 2
    #   add 1
    #   swap
    #   call
    FUNC_ARG_NOTE = compat_bytes((
        # 0x00..0x0f
        0x05, 0x01, 0x03, 0x18, 0x49, 0x03, 0x02, 0x03,   # |....I...|
        0x07, 0x32, 0x1e, 0x23, 0x01, 0x16, 0xff, 0x00,   # |.2.#....|

        # 0x10..0x1f
        0x01, 0x02, 0x04, 0x10, 0x00, 0x09, 0x0e, 0x04,   # |........|
        0x01, 0x15, 0x66, 0x75, 0x6e, 0x63, 0x5f, 0x61,   # |..func_a|

        # 0x20..0x2e
        0x72, 0x67, 0x00, 0x46, 0x69, 0x28, 0x69, 0x29,   # |rg.Fi(i)|
        0x69, 0x00, 0x74, 0x65, 0x73, 0x74, 0x00))        # |i.test.|

    # 32-bit little-endian note that returns a function.
    #
    # extern func int (int) example::factorial
    # define test::func_ret returns func int (int)
    #   load example::factorial
    FUNC_RET_NOTE = compat_bytes((
        # 0x00..0x0f
        0x05, 0x01, 0x03, 0x18, 0x49, 0x01, 0x02, 0x03,   # |....I...|
        0x03, 0xff, 0x01, 0x01, 0x01, 0x02, 0x04, 0x21,   # |.......!|

        # 0x10..0x1f
        0x0a, 0x09, 0x1b, 0x03, 0x02, 0x04, 0x13, 0x00,   # |........|
        0x26, 0x26, 0x04, 0x01, 0x28, 0x66, 0x61, 0x63,   # |&&..(fac|

        # 0x20..0x2f
        0x74, 0x6f, 0x72, 0x69, 0x61, 0x6c, 0x00, 0x66,   # |torial.f|
        0x75, 0x6e, 0x63, 0x5f, 0x72, 0x65, 0x74, 0x00,   # |unc_ret.|

        # 0x30..0x3f
        0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x00,   # |example.|
        0x46, 0x69, 0x28, 0x69, 0x29, 0x00, 0x74, 0x65,   # |Fi(i).te|

        # 0x40..0x44
        0x73, 0x74, 0x00, 0x69, 0x00))                    # |st.i.|

    # 32-bit big-endian note with multiple arguments and returns.
    #
    # define test::quad returns int, int, int
    #   argument int a
    #   argument int b
    #   load b
    #   load a
    #   mul
    #   add
    #   load 1
    MULTI_ARGSRETS_NOTE = compat_bytes((
        # 0x00..0x0f
        0x05, 0x01, 0x03, 0x49, 0x18, 0x04, 0x02, 0x03,   # |...I....|
        0x06, 0x12, 0x15, 0x02, 0x1e, 0x22, 0x31, 0x01,   # |....."1.|

        # 0x10..0x1f
        0x02, 0x04, 0x05, 0x00, 0x0b, 0x0a, 0x04, 0x01,   # |........|
        0x0e, 0x71, 0x75, 0x61, 0x64, 0x00, 0x74, 0x65,   # |.quad.te|

        # 0x20..0x26
        0x73, 0x74, 0x00, 0x69, 0x69, 0x69, 0x00))        # |st.iii.|

class Libi8xTestCase(BaseTestCase):
    """Base class for testcases using the high-level layer."""

    def ctx_new(self, flags=None, logger=None, klass=libi8x.Context):
        """Standard way to create a context for testing.

        If flags is None (the default) then some extra checks will
        be enabled.  Most tests should use take advantage of these.
        """

        # Enable memory checking where possible.
        if flags is None:
            self.assertIsNone(logger)
            flags = syslog.LOG_DEBUG | libi8x.DBG_MEM
            logger = self._libi8xtest_logger

        # Don't randomly log to stderr.
        self.assertTrue(flags & 15 == 0 or logger is not None)

        # Store what we did so that tests that require specific
        # setup can check they got it.
        self.ctx_new_flags = flags
        self.ctx_new_logger = logger

        return klass(flags, logger)

    def _libi8xtest_setUp(self):
        self._i8xlog = []
        self._libi8xtest_user_setUp()

    def _libi8xtest_logger(self, *args):
        self._i8xlog.append(args)

    def _libi8xtest_tearDown(self):
        self._libi8xtest_user_tearDown()

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
            if not msg:
                continue
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
