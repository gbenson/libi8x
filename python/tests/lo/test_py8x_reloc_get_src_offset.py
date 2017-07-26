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

class TestPy8xRelocGetSrcOffset(common.TestCase):
    def __do_test(self, import_offset):
        if import_offset < 0:
            expect_offset = -1
        else:
            expect_offset = import_offset + 10
        ctx = self.ctx_new()
        func = py8x.ctx_import_bytecode(ctx, self.RELOC_NOTE,
                                        "testnote", import_offset)
        relocs = py8x.func_get_relocs(func)
        li = py8x.list_get_first(relocs)
        reloc = py8x.listitem_get_object(li)
        actual_offset = py8x.reloc_get_src_offset(reloc)
        self.assertEqual(actual_offset, expect_offset)

    def test_basic(self):
        """Test py8x_reloc_get_src_offset."""
        for import_offset in (-5, -1, 0, 1, 23):
            self.__do_test(import_offset)
