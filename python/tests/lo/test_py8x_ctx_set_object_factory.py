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
import syslog

class TestPy8xCtxSetObjectFactory(common.TestCase):
    def test_basic(self):
        """Test py8x_ctx_set_object_factory."""
        ctx = self.ctx_new()
        objects = []
        def obf_func(*args):
            objects.append(args)
            return common.TestObject(*args)
        py8x.ctx_set_object_factory(ctx, obf_func)
        py8x.inf_new(ctx) # create an object
        self.assertGreater(len(objects), 0)
