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
import copy
import re
import sys

class _TestBadArgTypes(common.TestCase):
    def setUp(self):
        super(_TestBadArgTypes, self).setUp()

        # libi8x types.
        self.tv_ctx = self.ctx_new()
        self.tv_xctx = py8x.xctx_new(self.tv_ctx, 512)
        self.tv_func = py8x.ctx_import_bytecode(self.tv_ctx,
                                                self.RELOC_NOTE,
                                                "testnote", 0)
        self.tv_funcref = py8x.func_get_funcref(self.tv_func)
        self.tv_note = py8x.func_get_note(self.tv_func)
        self.tv_chunk = py8x.note_get_unique_chunk(self.tv_note,
                                                   py8x.I8_CHUNK_BYTECODE,
                                                   True)
        self.tv_list = py8x.func_get_relocs(self.tv_func)
        self.tv_listitem = py8x.list_get_first(self.tv_list)
        self.tv_reloc = py8x.listitem_get_object(self.tv_listitem)
        self.tv_inf = py8x.inf_new(self.tv_ctx)

        # Special i8x type used as a placeholder for any i8x object.
        # It could be anything but it shouldn't be the same as any
        # of the above.
        self.tv_object = py8x.note_get_unique_chunk(self.tv_note,
                                                    py8x.I8_CHUNK_SIGNATURE,
                                                    True)
        # Non-i8x types.
        self.tv_integer = 5
        self.tv_string = ""
        self.tv_sequence = ()
        if sys.version_info >= (3,):
            self.tv_buffer = b""
        self.tv_callable = lambda *args: common.TestObject("BATobj")

        # Types used for testing but not required for probing.
        self.tv_none = None
        self.tv_pylist = []
        self.tv_dict = {}
        self.tv_pyobj = object()

        # Some functions require special handling.
        self.expect_special = {
            (py8x.ctx_new, 2): self.OPTIONAL_CALLABLE,
            (py8x.ctx_set_log_fn, 1): self.OPTIONAL_CALLABLE,
            (py8x.ctx_import_bytecode, 2): self.OPTIONAL_STRING,
            (py8x.xctx_call, 3): self.ZERO_LENGTH_SEQUENCE,
        }

        # Reverse map used for debug printing.
        self.typenames = {
            repr(self.tv_string): repr(self.tv_string),
            repr(self.tv_pyobj): "<object>",
            repr(self.tv_callable): "<callable>",
        }
        for attr in dir(self):
            if not attr.startswith("tv_"):
                continue
            value = repr(getattr(self, attr))
            if isinstance(value, common.TestObject):
                assert not value in self.typenames
                self.typenames[value] = "i8x_" + attr[3:]

    def value_of_type(self, type):
        if type.startswith("i8x_"):
            type = type[4:]
        return getattr(self, "tv_" + type)

    def _BAT_runTest(self, func):
        print("Checking py8x_" + func.__name__)
        valid_args = self.get_valid_arguments(func)
        print("  valid_args =", self.__strargs(valid_args))
        print()

        test_args = [getattr(self, attr)
                     for attr in dir(self)
                     if (attr.startswith("tv_")
                         and attr != "tv_object")]

        for index, orig_value in enumerate(valid_args):
            args = list(valid_args)
            for test_value in test_args:
                if test_value is orig_value:
                    continue

                args[index] = test_value
                print("trying py8x_%s%s" % (func.__name__,
                                            self.__strargs(args)))

                # Figure out what to expect.
                if (orig_value is self.tv_object
                    and isinstance(test_value, common.TestObject)):
                    # A tv_object argument means any i8x object is ok.
                    expect = None
                else:
                    special = self.expect_special.get((func, index), None)
                    if special is not None:
                        expect = special(test_value)
                    else:
                        expect = TypeError

                # Call the function.
                if expect is None:
                    func(*args)
                    continue

                with self.assertRaises(expect) as cm:
                    func(*args)

                if expect is not TypeError:
                    continue

                err_index, wanted, got = self.decode_typeerror(cm.exception)
                self.assertEqual(isinstance(orig_value, common.TestObject),
                                 wanted.startswith("i8x_"))
                self.assertIn(err_index, (None, index))

    # Argument type probing.  Uses regular expressions to parse
    # TypeError messages.  Does Python have localized exception
    # messages?  Because this is going to fail if it does.

    WRONG_ARGC = re.compile(
        r"^function takes exactly (\d+) arguments? \(0 given\)$")

    X_REQUIRED_1 = re.compile(r"^an? (\w+) is required(?: \(got (.+)\))?$")
    X_REQUIRED_2 = re.compile(r"^an? (.+) is required, not (.+)$")
    MUST_BE_X = re.compile(r"^(?:argument (\d+) )?must be (.+), not (.+)$")
    NOT_INTEGER = re.compile(
        r"^'(.+)' object cannot be interpreted as an integer$")
    NOT_BUFFER = re.compile(
        r"^'(.+)' does not support the buffer interface$")
    NOT_CALLABLE = re.compile(r"^'(.+)' object is not callable$")

    def get_valid_arguments(self, func):
        """Return a tuple of valid arguments for func."""
        argc = self.probe_argcount(func)
        print("  argc =", argc)
        return tuple(self.probe_argtypes(func, argc))

    def probe_argcount(self, func):
        try:
            func()
        except TypeError as e:
            msg = e.args[0]
            m = self.WRONG_ARGC.match(msg)
            self.assertIsNotNone(m, msg)
            return int(m.group(1))

    class GotValidArguments(Exception):
        pass

    __probe_object = object()

    def probe_argtypes(self, func, argc):
        args = [None] * argc
        try:
            while True:
                # What's wrong currently?
                problem = self.__probe_argtypes(func, args)
                index, wants, got = problem

                # If we have an index we're pretty much done.
                if index is not None:
                    print("  arg[%d] => %s" % (index, wants))
                    args[index] = self.value_of_type(wants)
                    continue

                # Need to play "guess the index".
                if got is None:
                    sub = self.value_of_type(wants)
                else:
                    sub = self.__probe_object
                for index in range(len(args)):
                    if args[index] is not None:
                        continue
                    tmp = copy.copy(args)
                    tmp[index] = sub
                    result = self.__probe_argtypes(func, tmp)
                    if result == problem:
                        continue

                    # Something changed, and now we know the index.
                    index_now, wants_now, got_now = result
                    self.assertIn(index_now, (index, None))
                    if got_now == wants:
                        # We've caused a new problem, skip it.
                        continue
                    elif wants_now != wants:
                        # We fixed the original problem and hit another.
                        if sub is self.__probe_object:
                            continue
                    elif sub is self.__probe_object:
                        self.assertIn(got_now, (None, "object"))
                        sub = self.value_of_type(wants)

                    print("  arg[%d] => %s" % (index, wants))
                    args[index] = sub
                    break
                else:
                    self.fail("can't determine index")

        except self.GotValidArguments:
            result = self.__valid_args
            del self.__valid_args
            return result
        self.fail("shouldn't get here")

    def __probe_argtypes(self, func, args):
        try:
            try:
                func(*args)
            except StopIteration:
                # We got into py8x_list_get_*
                self.assertIn(func, (py8x.list_get_first,
                                     py8x.list_get_next))
            except AttributeError as e:
                # We got into py8x_xctx_call
                self.assertIs(func, py8x.xctx_call)
                self.assertGreaterEqual(e.args[0].find("relocate_address"), 0)
            except ValueError as e:
                # We got into py8x_ctx_get_funcref or py8x_ctx_import_native
                self.assertIn(func, (py8x.ctx_get_funcref,
                                     py8x.ctx_import_native))
            except py8x.UnhandledNoteError as e:
                # We got into py8x_ctx_import_bytecode
                self.assertIs(func, py8x.ctx_import_bytecode)
            self.__valid_args = args
            raise self.GotValidArguments
        except TypeError as e:
            return self.decode_typeerror(e)

    def decode_typeerror(self, te):
        raw = self.__parse_typeerror_msg(te.args[0])
        if len(raw) == 3:
            index = raw[0]
            if index is not None:
                index = int(index) - 1
            raw = raw[1:]
        else:
            index = None
        return (index,) + tuple(map(self.__parse_argtype, raw))

    def __parse_typeerror_msg(self, msg):
        m = self.X_REQUIRED_1.match(msg)
        if m is None:
            m = self.X_REQUIRED_2.match(msg)
        if m is None:
            m = self.MUST_BE_X.match(msg)
        if m is not None:
            return m.groups()

        m = self.NOT_INTEGER.match(msg)
        if m is not None:
            return "integer", m.group(1)

        m = self.NOT_BUFFER.match(msg)
        if m is not None:
            return "buffer", m.group(1)

        m = self.NOT_CALLABLE.match(msg)
        if m is not None:
            return "callable", m.group(1)

        raise NotImplementedError(msg)

    def __parse_argtype(self, type):
        if type is None:
            return type
        type = type.strip("'")
        if type.startswith("str"):
            type = "string"
        elif type.find("int") >= 0:
            type = "integer"
        elif type.startswith("bytes"):
            type = "buffer"
        elif type.endswith("NoneType"):
            type = "None"
        elif type.startswith("type "):
            type = type[5:]
        return type

    # Helpers for self.expect_special.

    def OPTIONAL_CALLABLE(self, test_value):
        if test_value is not self.tv_callable:
            return TypeError

    def OPTIONAL_STRING(self, test_value):
        return (test_value is self.tv_string
                and py8x.UnhandledNoteError
                or TypeError)

    def ZERO_LENGTH_SEQUENCE(self, test_value):
        try:
            if len(test_value) == 0 and test_value is not self.tv_dict:
                return AttributeError
        except:
            pass
        return TypeError

    # Debug printing.

    def __strargs(self, args):
        return "(%s)" % ", ".join(
            self.typenames.get(repr(arg), str(arg))
            for arg in args)


# Generate a TestBadArgTypes class with individual test_py8x_*
# methods. Is there a better way to do this? I tried this with
# a TestSuite but the Nose output was so ugly :(
def _setup():
    return """\
class TestBadArgTypes(_TestBadArgTypes):
""" + "\n".join("""
    def test_py8x_%s(self):
        "Test py8x_%s with bad argument types."
        self._BAT_runTest(py8x.%s)""" % (attr, attr, attr)
                for attr in dir(py8x)
                if attr[:2] not in ("__", "I8")
                   and type(getattr(py8x, attr)) not in (type, type(0)))
exec(_setup())
del _setup
