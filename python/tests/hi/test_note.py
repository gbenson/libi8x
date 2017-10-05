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

from . import *

class TestNote(TestCase):
    def setUp(self):
        self.ctx = self.ctx_new()
        self.func = self.ctx.import_bytecode(self.GOOD_NOTE)
        self.note = self.func.note

    def test_context(self):
        """Test Note.context."""
        self.assertIs(self.note.context, self.ctx)

    def test_is_persistent(self):
        """Test Note.is_persistent."""
        del self.ctx, self.func
        self._test_persistence("note")
