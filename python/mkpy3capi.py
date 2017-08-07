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

import os
import pycparser
import sys
import subprocess

class API(object):
    def __init__(self, header, include_path):
        with open(header) as fp:
            src = fp.read()
        self.__constants = {}
        self.__extract_cpp_constants(src)
        self.__types = {}
        self.__functions = {}
        self.__extract_everything_else(src, include_path)

    def add_constant(self, name):
        assert name not in self.__constants
        self.__constants[name] = True

    def emit_constants(self, fp):
        for name in sorted(self.__constants):
            if name.startswith("I8X_"):
                pyname = name[4:]
            else:
                pyname = name
            print('  PyModule_AddIntConstant (m, "%s", %s);' % (pyname,
                                                                name),
                  file=fp)

    def add_type(self, name):
        self.__types[name] = True

    TYPE_PREFIXES = {"object": "ob", "readbuf": "rb"}
    CLASSNAMES = {"inf": '"inferior"', "object": "NULL"}

    def emit_object_functions(self, fp):
        for name in sorted(self.__types):
            print("PY8X_OBJECT_FUNCTIONS (%s, %s, %s);"
                  % (name,
                     self.TYPE_PREFIXES.get(name, name),
                     self.CLASSNAMES.get(name, '"%s"' % name)),
                  file=fp)

    def add_function(self, type, name, params):
        assert name not in self.__functions
        self.__functions[name] = (type, params)

    def emit_functions(self, fp):
        self.__ftable = []
        for name, type_and_params in sorted(self.__functions.items()):
            self.__emit_function(fp, name, *type_and_params)

    def emit_function_table(self, fp):
        for name in sorted(self.__ftable):
            assert name.startswith("i8x_")
            print("  PY8X_FUNCTION (%s)," % name[4:], file=fp)

    def __emit_function(self, fp, name, rtype, params):
        try:
            rtype = PyType.from_ctype(rtype)
        except NotImplementedError:
            return
        try:
            params = [(PyType.from_ctype(t), n) for t, n in params]
        except NotImplementedError:
            return

        assert name.startswith("i8x_")
        tmp = name[4:]
        if not os.path.exists(self.__testfmt % tmp):
            for what in ("_get_", "_set_"):
                if tmp.find(what) >= 0:
                    tmp = tmp.replace(what, "_get_set_")
                    if os.path.exists(self.__testfmt % tmp):
                        break
            else:
                return

        self.__ftable.append(name)

        if (isinstance(rtype, I8xError)
              and isinstance(params[-1][0], I8xObjPtrPtr)):
            rtype.opp_return, retname = params.pop()
            rtype.opp_return.retname = retname
            rtype.opp_parent = params[0][0].argname(params[0][1])

        print("""\
/* Python binding for %s.  */

static PyObject *
py%s (PyObject *self, PyObject *args)
{""" % (name, name[1:]), file=fp)

        fmts, args = [], []
        for ptype, pname in params:
            tmp = ptype.argtype
            if not tmp.endswith(" *"):
                tmp += " "
            print("  %s%s;" % (tmp, ptype.argname(pname)), file=fp)
            tmp = ptype.converter
            if tmp is not None:
                args.append(ptype.converter)
            args.append("&" + ptype.argname(pname))
            fmts.append(ptype.argfmt)
        print("""
  if (!PyArg_ParseTuple (args, "%s", %s))
    return NULL;
""" % ("".join(fmts), ", ".join(args)), file=fp)

        if rtype.opp_return is not None:
            params.append((rtype.opp_return, "&" + rtype.opp_return.retname))

        for ptype, pname in params:
            unwrap = ptype.unwrap_arg(pname)
            if unwrap is not None:
                unwraps = True
                print("  %s;" % unwrap, file=fp)

        tmp = rtype.ctype
        if tmp == "void":
            tmp = ""
        else:
            if not tmp.endswith(" *"):
                tmp += " "
            tmp += rtype.retname
            tmp += " = "
        print("  %s%s (%s);" % (
            tmp, name,
            ", ".join(pname for ptype, pname in params)), file=fp)
        print("\n  %s;\n" % rtype.do_check_result(), file=fp);
        print("  %s;\n}\n" % rtype.do_return(), file=fp)

    def __parse_pragma(self, line, prefix="libi8x_api_"):
        if line.startswith("#"):
            line = line[1:].strip().split()
            if (len(line) == 2
                  and line[0] == "pragma"
                  and line[1].startswith(prefix)):
                return line[1][len(prefix):]

    def __emit_boilerplate(self, template, output, fp):
        print("/* %s generated by %s from %s. */"
              % (os.path.basename(output),
                 os.path.basename(sys.argv[0]),
                 os.path.basename(template)), file=fp)
        print("/* DO NOT EDIT THIS FILE, "
              + "EDIT THE TEMPLATE AND REGENERATE. */", file=fp)
        print(file=fp)

    def emit(self, template, output, testfmt):
        self.__testfmt = testfmt
        with open(template) as ifp:
            with open(output, "w") as ofp:
                self.__emit_boilerplate(template, output, ofp)
                for line in ifp.readlines():
                    what = self.__parse_pragma(line)
                    if what is None:
                        ofp.write(line)
                    else:
                        getattr(self, "emit_" + what)(ofp)

    def __gcc(self, args, input):
        gcc = subprocess.Popen(("gcc",) + args + ("-",),
                               stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE)
        output, check = gcc.communicate(input.encode("utf-8"))
        assert check is None
        if gcc.returncode != 0:
            sys.exit(1)
        return output.decode("utf-8")

    def __strip_comments(self, src):
        return self.__gcc(("-E", "-fpreprocessed", "-dD"), src)

    def __extract_cpp_constants(self, src):
        """Extract #define constants lost in preprocessing."""
        for line in self.__strip_comments(src).split("\n"):
            if not line.startswith("#"):
                continue
            line = line.split()
            if (len(line) == 3
                  and line[0] == "#define"
                  and "(" not in line[1]):
                self.add_constant(line[1])

    def __parse(self, src, include_path):
        src = src.replace("__attribute__ ((always_inline))", "")
        src = self.__gcc(("-E", "-I" + include_path), src)
        return pycparser.CParser().parse(src)

    def __extract_everything_else(self, src, include_path):
        ASTVisitor(self).visit(self.__parse(src, include_path))

class PyType(object):
    retname = "result"

    @classmethod
    def __cinit(cls):
        cls.CLASSES = {"bool": CBool,
                       "const char *": CString,
                       "i8x_err_e": I8xError,
                       "void": CVoid}
        for ctype in CInt.CTYPES:
            cls.CLASSES[ctype] = CInt

    @classmethod
    def from_ctype(cls, ctype):
        if not hasattr(cls, "CLASSES"):
            cls.__cinit()
        tmp = cls.CLASSES.get(ctype, None)
        if tmp is None:
            for tmp in (I8xObjPtr, I8xObjPtrPtr):
                if ctype.startswith(tmp.PREFIX) and ctype.endswith(tmp.SUFFIX):
                    break
            else:
                tmp = None
        if tmp is None:
            raise NotImplementedError(ctype)
        cls = tmp
        return cls(ctype)

    def __init__(self, ctype):
        self.ctype = ctype

    @property
    def argtype(self):
        return self.ctype

    def argname(self, name):
        return name

    converter = None

    def unwrap_arg(self, name):
        pass

    opp_return = None

    def do_check_result(self):
        return "PY8X_CHECK_UNCAUGHT ()"

class CBool(PyType):
    argfmt = "i" # XXX use "p" for PY3K?

    def __init__(self, ctype):
        super(CBool, self).__init__("int")

    def do_return(self):
        return """\
if (result)
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE"""

class CInt(PyType):
    __ops = {"int":   ("i", "Int"),
             "size_t": ("O&", "Long"),
             "ssize_t": ("n", "Long"),
             "uintptr_t": ("O&", "Long")}

    __ops["i8x_byte_order_e"] = __ops["int"]

    CTYPES = list(__ops.keys())

    @property
    def argfmt(self):
        return self.__ops[self.ctype][0]

    @property
    def converter(self):
        if self.argfmt == "O&":
            return "py8x_pylong_AsUintptr_t"

    @property
    def __creator(self):
        return "Py%s_FromLong" % self.__ops[self.ctype][1]

    def do_return(self):
        return "return %s (%s)" % (self.__creator, self.retname)

class CString(PyType):
    argfmt = "s"

    def do_return(self):
        return "return PyStr_FromString (%s)" % self.retname

class CVoid(PyType):
    def do_return(self):
        return "Py_RETURN_NONE";

class I8xError(PyType):
    retname = "err"

    def do_check_result(self):
        if self.opp_return is None:
            return "PY8X_CHECK_CALL (ctx, err)"
        else:
            return ("PY8X_CHECK_CALL_DEREF (ctx, err, %s)"
                    % self.opp_return.retname)

    def do_return(self):
        if self.opp_return is None:
            return "Py_RETURN_NONE"
        else:
            return "return py8x_encapsulate (%s)" % self.opp_return.retname
        return result

class I8xObject(PyType):
    PREFIX = "struct i8x_"

    SHORT_PREFIXES = {"readbuf": "rb", "object": "ob"}

    @property
    def func_prefix(self):
        prefix = self.ctype[len(self.PREFIX):-len(self.SUFFIX)]
        prefix = self.SHORT_PREFIXES.get(prefix, prefix)
        return prefix

class I8xObjPtr(I8xObject):
    SUFFIX = " *"

    argtype = "PyObject *"
    argfmt = "O"

    def argname(self, name):
        return name + "c"

    def unwrap_arg(self, name):
        result = "%s%s = PY8X_FROM_CAPSULE" % (self.ctype, name)
        if name == self.func_prefix:
            result += " ("
        else:
            result += "_2 (%s, " % self.func_prefix
        return result + name + ")"

    def do_return(self):
        return ("return py8x_encapsulate (i8x_%s_ref (result))"
                % self.func_prefix)

class I8xObjPtrPtr(I8xObject):
    SUFFIX = " **"

    def unwrap_arg(self, name):
        return self.ctype[:-1] + self.retname

class ASTVisitor(pycparser.c_ast.NodeVisitor):
    def __init__(self, api):
        self.api = api

    def visit_Enum(self, node):
        names = [e.name for e in node.values.enumerators]
        if "I8X_OK" not in names:
            list(map(self.api.add_constant, names))

    def visit_Decl(self, node):
        name = node.name
        if (name is None
            or (name.startswith("i8x_")
                and not ((name != "i8x_ob_get_ctx"
                          and name.startswith("i8x_ob_"))
                         or name.endswith("_ref")
                         or name.endswith("_unref")
                         or name.endswith("_userdata")
                         or name.endswith("_fn")
                         or name.endswith("_cb")
                         or name.startswith("i8x_ctx_import_")
                         or name.startswith("i8x_rb_read_")
                         or name.startswith("i8x_list_get_")
                         or name in ("i8x_ctx_new",
                                     "i8x_inf_new",
                                     "i8x_note_get_unique_chunk",
                                     "i8x_chunk_get_encoded")))):
            TopLevelDeclVisitor(self.api).visit(node.type)

class TopLevelDeclVisitor(pycparser.c_ast.NodeVisitor):
    def __init__(self, api):
        self.api = api

    def visit_Struct(self, node, PREFIX="i8x_"):
        if node.name.startswith(PREFIX):
            self.api.add_type(node.name[len(PREFIX):])

    def visit_FuncDecl(self, node):
        plv = ParamListVisitor()
        plv.visit(node.args)
        tv = TypeVisitor()
        tv.visit(node.type)
        self.api.add_function(tv.type, tv.name, plv.params)

class ParamListVisitor(pycparser.c_ast.NodeVisitor):
    def visit_ParamList(self, node):
        assert not hasattr(self, "params")
        self.params = []
        for param in node.params:
            self.visit(param)

    def visit_Decl(self, node):
        tv = TypeVisitor()
        tv.visit(node.type)
        self.params.append((tv.type, tv.name))

class TypeVisitor(pycparser.c_ast.NodeVisitor):
    def __init__(self):
        self.__is_const = False
        self.__nonatomic = None # struct or union
        self.__basetype = None
        self.__starcount = 0
        self.name = None

    @property
    def type(self):
        result = []
        if self.__is_const:
            result.append("const")
        if self.__nonatomic is not None:
            result.append(self.__nonatomic)
        assert self.__basetype is not None
        result.append(self.__basetype)
        if self.__starcount:
            result.append("*" * self.__starcount)
        return " ".join(result)

    def generic_visit(self, node):
        node.show()
        print(dir(node))
        raise NotImplementedError

    def visit_PtrDecl(self, node):
        self.__starcount += 1
        assert not node.quals
        self.visit(node.type)

    def visit_TypeDecl(self, node):
        assert self.name is None
        self.name = node.declname
        assert self.name is not None
        if len(node.quals) == 1:
            assert node.quals[0] == "const"
            assert not self.__is_const
            self.__is_const = True
        else:
            assert not node.quals
        self.visit(node.type)

    def visit_IdentifierType(self, node):
        assert self.__nonatomic is None
        assert self.__basetype is None
        assert len(node.names) == 1
        [self.__basetype] = node.names
        assert self.__basetype is not None

    def visit_Struct(self, node):
        self.__visit_Struct_or_Union(node, "struct")

    def visit_Union(self, node):
        self.__visit_Struct_or_Union(node, "union")

    def __visit_Struct_or_Union(self, node, type):
        assert self.__nonatomic is None
        assert self.__basetype is None
        assert node.decls is None
        self.__basetype = node.name
        assert self.__basetype is not None
        self.__nonatomic = type

def main():
    if len(sys.argv) != 2:
        print("""\
usage: %s FAKE_INCLUDE_PATH

The pycparser source contains some "fake" standard include files,
but these aren't in the packages pip installs (and the license
may not allow them to be bundled here) so you'll need to
  git clone https://github.com/eliben/pycparser.git
and supply /path/to/pycparser/utils/fake_libc_include as the
argument to this script.""" % sys.argv[0], file=sys.stderr)
        sys.exit(1)
    fake_include_path = sys.argv[1]

    curdir = os.path.dirname(os.path.realpath(sys.argv[0]))
    topdir = os.path.dirname(curdir)
    header = os.path.join(topdir, "libi8x", "libi8x.h")
    output = os.path.join(curdir, "libi8x.c")
    template = output + ".in"
    testdir = os.path.join(topdir, "python", "tests", "lo")
    testfmt = os.path.join(testdir, "test_py8x_%s.py")

    API(header, fake_include_path).emit(template, output, testfmt)

if __name__ == "__main__":
    main()
