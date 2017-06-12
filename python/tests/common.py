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

import sys
import unittest

if sys.version_info < (3,):
    bytes = lambda seq: "".join(chr(c).decode("iso-8859-1")
                                for c in seq).encode("iso-8859-1")

class TestCase(unittest.TestCase):

    # 32-bit little-endian version of I8C's factorial.i8 example.
    GOOD_NOTE = bytes((
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

    # Corrupt version of GOOD_NOTE.
    CORRUPT_NOTE = GOOD_NOTE[:-1]
