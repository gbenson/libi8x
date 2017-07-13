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

class ChildObject(Object):
    """Base class for all objects except contexts."""

    @property
    def context(self):
        """Context that created this object."""
        return py8x.ob_get_ctx(self)

class ExecutionContext(ChildObject):
    pass

class Function(ChildObject):
    @property
    def ref(self):
        """This function's reference."""
        return py8x.func_get_funcref(self)

class FunctionReference(ChildObject):
    pass

class Inferior(ChildObject):
    pass

class Context(Object):
    def __init__(self, flags=0, log_fn=None):
        check = py8x.ctx_new(self.__new_context, flags, log_fn)
        assert check is self
        py8x.ctx_set_object_factory(self, self.__new_child)

    # Python wrappers for the underlying objects in C libi8x.
    # May be overridden to provide your own implementations.
    # Note that they must be overridden in the *class*, not
    # in individual context objects.
    EXECUTION_CONTEXT_CLASS = ExecutionContext
    FUNCTION_CLASS = Function
    FUNCTION_REFERENCE_CLASS = FunctionReference
    INFERIOR_CLASS = Inferior

    # Map short classnames from C libi8x to the above names.
    __LONG_CLASSNAMES = {
        "func": "FUNCTION",
        "funcref": "FUNCTION_REFERENCE",
        "xctx": "EXECUTION_CONTEXT",
        }

    def __new_context(self, clsname):
        """Object factory used for contexts."""
        assert clsname == "ctx"
        return self

    @classmethod
    def __new_child(cls, clsname):
        """Object factory used for everything but contexts."""
        assert clsname != "ctx"
        clsname = cls.__LONG_CLASSNAMES.get(clsname, clsname)
        return getattr(cls, clsname.upper() + "_CLASS")()

    @property
    def log_priority(self):
        """Logging priority."""
        return py8x.ctx_get_log_priority(self)

    @log_priority.setter
    def log_priority(self, value):
        return py8x.ctx_set_log_priority(self, value)

    def new_xctx(self, stack_slots=512):
        """Create a new execution context."""
        return py8x.xctx_new(self, stack_slots)

    def new_inferior(self):
        """Create a new inferior."""
        return py8x.inf_new(self)

    def import_bytecode(self, buf, srcname=None, srcoffset=-1):
        """Load and register a bytecode function."""
        return py8x.ctx_import_bytecode(self, buf, srcname, srcoffset)

    def import_native(self, provider, name, ptypes, rtypes, impl):
        """Load and register a native function."""
        return py8x.ctx_import_native(self, provider, name, ptypes,
                                      rtypes, impl)
