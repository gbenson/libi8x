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
from .. import common
import weakref

class TestObject(object):
    def __init__(self, type):
        self.type = type

    def __str__(self):
        return "<%s>" % self.type

class TestCase(common.TestCase):
    def setUp(self):
        self.__objects = []

    def __new_i8xobj(self, type):
        result = TestObject(type)
        self.__objects.append(weakref.ref(result))
        return result

    def tearDown(self):
        # Delete any objects we're referencing.
        keys = [key
                for key, value in self.__dict__.items()
                if isinstance(value, TestObject)]
        try:
            del value
        except UnboundLocalError:
            pass
        for key in keys:
            self.__dict__.pop(key)

        # Check everything got released.
        objects = [ob
                   for ob in (ref() for ref in self.__objects)
                   if ob is not None]
        self.assertEqual(list(map(str, objects)), [])

    def ctx_new(self, flags=0, log_fn=None):
        """Standard way to create an i8x_ctx for tests."""
        return py8x.ctx_new(self.__new_i8xobj, flags | py8x.I8X_DBG_MEM, log_fn)

class PopulatedTestCase(TestCase):
    """A testcase with a context and a loaded function."""

    TESTNOTE = TestCase.GOOD_NOTE

    def setUp(self):
        super(PopulatedTestCase, self).setUp()
        self.ctx = self.ctx_new()
        self.func = py8x.ctx_import_bytecode(self.ctx, self.TESTNOTE,
                                             "testnote", 0)
        self.xctx = py8x.xctx_new(self.ctx, 512)
        self.inf = py8x.inf_new(self.ctx)
        self.funcref = py8x.func_get_funcref(self.func)
