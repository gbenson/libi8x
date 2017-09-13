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
import threading
import sys

# Base classes

class Object(object):
    """Base class for all libi8x objects."""

class ChildObject(Object):
    """Base class for all objects except contexts."""

    @property
    def context(self):
        """Context that created this object."""
        return py8x.ob_get_ctx(self)

    @property
    def is_persistent(self):
        """Is this object protected from garbage collection?"""
        with _gc_protected_lock:
            return self in _gc_protected

    @is_persistent.setter
    def is_persistent(self, value):
        if value:
            with _gc_protected_lock:
                _gc_protected[self] = True
        else:
            with _gc_protected_lock:
                _gc_protected.pop(self, None)

_gc_protected_lock = threading.Lock()
with _gc_protected_lock:
    _gc_protected = {}

# Internal objects

class InternalObject(ChildObject):
    """Base class for all objects not fixed in the Python API.

    Some of these are things that might change in the future, for
    example i8x_list which could well be replaced by a hashtable.
    Some are classes internal to C libi8x that become exposed by
    i8x_ob_get_parent; Python instances of these are referenced by
    their children (in py8x_userdata->parent) but are not otherwise
    accessible from Python code.
    """

class _I8XCode(InternalObject):
    pass

class _I8XList(InternalObject):
    def __len__(self):
        return py8x.list_size(self)

    def __iter__(self):
        return _I8XListIterator(self)

class _I8XListIterator(object):
    def __init__(self, list):
        self.__list = list
        self.__item = None

    def __iter__(self):
        return self

    def __next__(self):
        if self.__item is None:
            self.__item = py8x.list_get_first(self.__list)
        else:
            self.__item = py8x.list_get_next(self.__list, self.__item)
        return self.__item.content

    if sys.version_info < (3,):
        next = __next__
        del __next__

class _I8XListItem(InternalObject):
    @property
    def content(self):
        return py8x.listitem_get_object(self)

# Public objects

class ExecutionContext(ChildObject):
    def call(self, ref_or_sig, inferior, *args):
        """Execute an Infinity function."""
        return py8x.xctx_call(self, ref_or_sig, inferior, args)

class Function(ChildObject):
    @property
    def ref(self):
        """This function's reference."""
        return py8x.func_get_funcref(self)

    @property
    def signature(self):
        """This function's signature."""
        return self.ref.signature

    @property
    def relocations(self):
        """This function's relocations."""
        return py8x.func_get_relocs(self) or ()

class FunctionReference(ChildObject):
    @property
    def signature(self):
        """This reference's function signature."""
        return py8x.funcref_get_signature(self)

    @property
    def is_global(self):
        """Does this reference a globally registered function?"""
        return py8x.funcref_is_global(self)

    @property
    def is_resolved(self):
        """Does this reference resolve to a callable function?"""
        return py8x.funcref_is_resolved(self)

class Inferior(ChildObject):
    def read_memory(self, address, nbytes):
        """Read an array of bytes from memory."""
        raise NotImplementedError

    def relocate_address(self, reloc):
        """Relocate an address."""
        raise NotImplementedError

class Relocation(ChildObject):
    @property
    def function(self):
        """The function this relocation is for."""
        return py8x.reloc_get_func(self)

    @property
    def source_offset(self):
        """This relocation's source offset, or -1 if unknown."""
        return py8x.reloc_get_src_offset(self)

class Context(Object):
    def __init__(self, flags=0, log_fn=None):
        check = py8x.ctx_new(self.__new_context, flags, log_fn)
        assert check is self
        py8x.ctx_set_object_factory(self, Context.__new_child)

    # Python wrappers for the underlying objects in C libi8x.
    # May be overridden to provide your own implementations.
    # Note that they must be overridden in the *class*, not
    # in individual context objects.
    EXECUTION_CONTEXT_CLASS = ExecutionContext
    FUNCTION_CLASS = Function
    FUNCTION_REFERENCE_CLASS = FunctionReference
    INFERIOR_CLASS = Inferior
    RELOCATION_CLASS = Relocation

    # Map short classnames from C libi8x to the above names.
    __LONG_CLASSNAMES = {
        "func": "FUNCTION",
        "funcref": "FUNCTION_REFERENCE",
        "reloc": "RELOCATION",
        "xctx": "EXECUTION_CONTEXT",
        }

    # Python wrappers for objects internal to either Python
    # or C libi8x.  See InternalObject.__doc__.  These are
    # not made available for extension/overriding.
    __INTERNAL_CLASSES = {
        "code": _I8XCode,
        "list": _I8XList,
        "listitem": _I8XListItem,
    }

    def __new_context(self, ctx, clsname):
        """Object factory used for contexts."""
        assert clsname == "ctx" and ctx is None
        return self

    def __new_child(self, clsname):
        """Object factory used for everything but contexts."""
        assert clsname != "ctx"
        klass = self.__INTERNAL_CLASSES.get(clsname, None)
        if klass is None:
            clsname = self.__LONG_CLASSNAMES.get(clsname, clsname)
            klass = getattr(self, clsname.upper() + "_CLASS")
        result = klass()
        if getattr(result, "__libi8x_persistent__", False):
            result.is_persistent = True
        return result

    @property
    def log_priority(self):
        """Logging priority."""
        return py8x.ctx_get_log_priority(self)

    @log_priority.setter
    def log_priority(self, value):
        return py8x.ctx_set_log_priority(self, value)

    @property
    def logger(self):
        """Function to log messages."""
        return py8x.ctx_get_log_fn(self)

    @logger.setter
    def logger(self, value):
        return py8x.ctx_set_log_fn(self, value)

    def new_xctx(self, stack_slots=512):
        """Create a new execution context."""
        return py8x.xctx_new(self, stack_slots)

    def new_inferior(self):
        """Create a new inferior."""
        return py8x.inf_new(self)

    def get_funcref(self, signature):
        """Return a reference to the specified function."""
        return py8x.ctx_get_funcref(self, signature)

    @property
    def functions(self):
        """Iterable of all currently registered functions."""
        return py8x.ctx_get_functions(self)

    def unregister(self, func):
        """Unregister a previously registered function."""
        return py8x.ctx_unregister_func(self, func)

    def import_bytecode(self, buf, srcname=None, srcoffset=-1):
        """Load and register a bytecode function."""
        return py8x.ctx_import_bytecode(self, buf, srcname, srcoffset)

    def import_native(self, signature, impl):
        """Load and register a native function."""
        return py8x.ctx_import_native(self, signature, impl)
