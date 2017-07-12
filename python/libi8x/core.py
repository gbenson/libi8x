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

class Object(object):
    """Base class for all libi8x objects."""

class Context(Object):
    def __init__(self, flags=0, log_fn=None):
        self.__in_context_init = True
        check = py8x.ctx_new(self.__new_object, flags, log_fn)
        self.__in_context_init = False
        assert check is self

    def __new_object(self, clsname):
        if clsname == "ctx":
            assert self.__in_context_init
            return self
        assert not self.__in_context_init
        raise NotImplementedError(clsname)
