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

class TestPy8xTypeIsFuncType(common.PopulatedTestCase):
    def test_is_functype(self):
        """Test py8x_type_is_functype on a function type."""
        functype = py8x.funcref_get_type(self.funcref)
        self.assertTrue(py8x.type_is_functype(functype))

    def test_not_functype(self):
        """Test py8x_type_is_functype on a non-function type."""
        functype = py8x.funcref_get_type(self.funcref)
        ptypes = py8x.type_get_ptypes(functype)
        li = py8x.list_get_first(ptypes)
        ptype = py8x.listitem_get_object(li)
        self.assertFalse(py8x.type_is_functype(ptype))
