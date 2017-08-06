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

class TestPy8xCtxGetFuncref(common.PopulatedTestCase):
    # Note that test_existing and test_creating test the same code
    # in py8x_ctx_get_funcref, but they're testing different paths
    # in py8x_encapsulate_2.

    def test_existing(self):
        """Test py8x_ctx_get_funcref returning an existing funcref."""
        ref = py8x.ctx_get_funcref(self.ctx, "example", "factorial", "i", "i")
        self.assertIsNotNone(ref)

    def test_creating(self):
        """Test py8x_ctx_get_funcref creating a new funcref."""
        ref = py8x.ctx_get_funcref(self.ctx, "not", "registered", "p", "o")
        self.assertIsNotNone(ref)

    def test_failure(self):
        """Test py8x_ctx_get_funcref failing."""
        self.assertRaises(ValueError,
                          py8x.ctx_get_funcref,
                          self.ctx, "3xample", "factorial", "i", "i")
