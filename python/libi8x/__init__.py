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
#  __future__.unicode_literals breaks "from libi8x import *"

from _libi8x import (
    __version__,
    CorruptNoteError,
    DBG_MEM,
    DivideByZeroError,
    ExecutionError,
    I8XError,
    InvalidNoteError,
    LOG_TRACE,
    NoteError,
    ReturnTypeError,
    StackOverflowError,
    UnhandledNoteError,
    UnresolvedFunctionError,
)
from .core import (
    Context,
    ExecutionContext,
    Function,
    FunctionReference,
    Inferior,
    Object,
    Relocation,
)

__all__ = (
    "Context",
    "CorruptNoteError",
    "DBG_MEM",
    "DivideByZeroError",
    "ExecutionContext",
    "ExecutionError",
    "Function",
    "FunctionReference",
    "I8XError",
    "Inferior",
    "InvalidNoteError",
    "LOG_TRACE",
    "NoteError",
    "Object",
    "Relocation",
    "ReturnTypeError",
    "StackOverflowError",
    "UnhandledNoteError",
    "UnresolvedFunctionError",
)

# Add accessor properties and __str__ to I8XError.

def _add_I8XError_properties(*args):
    for index, name_doc in enumerate(args):
        name, doc = name_doc
        exec("setattr(I8XError, "
             + repr(name)
             + ", property(lambda self: self.args[%d], " % index
             + "doc=%s))" % repr(doc))
_add_I8XError_properties(
    ("strerror",  "Description of the libi8x i8x_err_e error code."),
    ("srcname",   "Name of the note this error occurred in."),
    ("srcoffset", "Offset into the note where this error occurred."),
)
del _add_I8XError_properties

def _I8XError__str__(self):
    prefix = ""
    if self.srcname is not None:
        prefix += self.srcname
    if self.srcoffset is not None:
        prefix += "[0x%x]" % self.srcoffset
    if prefix:
        prefix += ": "
    return prefix + self.strerror

I8XError.__str__ = _I8XError__str__
del _I8XError__str__
