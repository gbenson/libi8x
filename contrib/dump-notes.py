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

# Extract all notes from the specified ELF(s) and write them out

import elfhack
import os
import sys

def main():
    if len(sys.argv) < 3:
        print("usage: dump-notes ELF... OUTDIR", file=sys.stderr)
        sys.exit(1)
    outdir = sys.argv[-1]
    index_s = 0
    for srcfile in sys.argv[1:-1]:
        index_s += 1
        with open(srcfile, "rb") as fp:
            elf = fp.read()
        index_n = 0
        for note in elfhack.extract_notes(elf):
            index_n += 1
            dstfile = os.path.join(outdir, "%04d-%04d" % (index_s, index_n))
            if not os.path.exists(outdir):
                os.makedirs(outdir)
            with open(dstfile, "wb") as fp:
                fp.write(note)

if __name__ == "__main__":
    main()
