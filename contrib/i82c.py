#!/usr/bin/env python
# Copyright (C) 2016 Red Hat, Inc.
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

# Compile i8 source to object code and emit the notes as a C char array

import elfhack
import subprocess
import sys
import tempfile

def byte2char(byte):
    if byte >= 32 and byte < 127:
        return chr(byte)
    return "."

def main():
    if len(sys.argv) != 2:
        print("usage: i82c FILE.i8", file=sys.stderr)
        sys.exit(1)
    srcfile = sys.argv[1]

    with tempfile.NamedTemporaryFile(suffix=".o") as tf:
        subprocess.check_call(("i8c", "-c", srcfile, "-o", tf.name))
        with open(tf.name, "rb") as fp:
            elf = fp.read()

    with open(srcfile) as fp:
        print(("/* " + "   ".join(fp.readlines())).rstrip() + "\n */")

    for note in elfhack.extract_notes(elf):
        if sys.version_info < (3,):
            note = map(ord, note)
        print("static uint8_t testnote[] = {")
        start = 0
        while note:
            chunk, note = note[:16], note[16:]
            limit = start + len(chunk)
            if start != 0:
                print()
            print("  /* 0x%02x..0x%02x */" % (start, limit - 1))
            start = limit

            for row in chunk[:8], chunk[8:]:
                line = "  " + " ".join("0x%02x," % c for c in row)
                col = len(line)
                while col < 56:
                    col = ((col // 8) + 1) * 8
                    line += "\t"
                txt = "/* |%s|" % "".join(map(byte2char, row))
                line += txt
                col += len(txt)
                while col < 72:
                    col = ((col // 8) + 1) * 8
                    line += "\t"
                print(line + "*/")
        print("};")

if __name__ == "__main__":
    main()
