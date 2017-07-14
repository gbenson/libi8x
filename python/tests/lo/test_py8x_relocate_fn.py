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

class TestPy8xRelocateFn(common.PopulatedTestCase):
    TESTNOTE = common.PopulatedTestCase.RELOC_NOTE

    @staticmethod
    def __relocate(inf, reloc):
        """A correct relocate_address function."""
        return TestPy8xRelocateFn.EXPECT_RESULT

    EXPECT_RESULT = ((sys.maxsize << 1) | 1) & 0x9192939495969798

    def __do_test(self):
        result = py8x.xctx_call(self.xctx, self.funcref, self.inf, ())
        self.assertEqual(result, (self.EXPECT_RESULT,))

    def test_unset(self):
        """Test relocation when no relocation function is set."""
        self.assertRaises(py8x.I8XError, self.__do_test)

    def test_set_to_None(self):
        """Test relocation when relocation function is set to None."""
        py8x.inf_set_relocate_fn(self.inf, None)
        self.test_unset()

    def test_set(self):
        """Test relocation when a user relocation function is set."""
        py8x.inf_set_relocate_fn(self.inf, self.__relocate)
        self.__do_test()

    def test_bad_argc(self):
        """Check py8x_relocate_fn catches wrong numbers of arguments."""
        for relocate in (lambda: None,
                         lambda a: None,
                         lambda a, b, c: None,
                         lambda a, b, c, d: None):
            py8x.inf_set_relocate_fn(self.inf, relocate)
            self.assertRaises(TypeError, self.__do_test)

    def test_bad_result_type(self):
        """Check py8x_relocate_fn catches results of the wrong type."""
        for result in (None, b"hi", [1, 2, 3, 4], (0, 1, 2, 3), (4,), []):
            def relocate(inf, addr, len):
                return result
            py8x.inf_set_relocate_fn(self.inf, relocate)
            self.assertRaises(TypeError, self.__do_test)

    def test_exception(self):
        """Check py8x_relocate_fn propagates exceptions."""
        class Error(Exception):
            pass
        def relocate(inf, reloc):
            raise Error("boom")
        py8x.inf_set_relocate_fn(self.inf, relocate)
        self.assertRaises(Error, self.__do_test)
