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

class TestPy8xFuncRefIsResolved(common.PopulatedTestCase):
    def test_resolved(self):
        """Test py8x_funcref_is_resolved on a resolved function."""
        ref = py8x.ctx_get_funcref (self.ctx, "example", "factorial", "i", "i")
        self.assertTrue(py8x.funcref_is_resolved(ref))

    def test_undefined(self):
        """Test py8x_funcref_is_resolved on an undefined function."""
        ref = py8x.ctx_get_funcref (self.ctx, "exmapel", "factorial", "i", "i")
        self.assertFalse(py8x.funcref_is_resolved(ref))

    def test_ambiguous(self):
        """Test py8x_funcref_is_resolved on a function defined twice."""
        py8x.ctx_import_native(self.ctx, "example", "factorial", "i", "i", None)
        ref = py8x.ctx_get_funcref (self.ctx, "example", "factorial", "i", "i")
        self.assertFalse(py8x.funcref_is_resolved(ref))
