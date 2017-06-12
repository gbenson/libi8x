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

import os
import sys

def logalyze(fp):
    senses = {"created": 1, "released": -1}
    counts = {}
    for line in fp.readlines():
        line = line.rstrip().split()
        sense = senses.get(line[-1], None)
        if sense is not None:
            what = " ".join(line[-3:-1])
            counts[what] = counts.get(what, 0) + sense
    for item, count in sorted(counts.items()):
        if count != 0:
            print(item, count)

def main():
    if len(sys.argv) != 2:
        print("usage: %s LOGFILE" % os.path.basename(sys.argv[0]),
              fp=sys.stderr)
        sys.exit(1)
    with open(sys.argv[1]) as fp:
        logalyze(fp)

if __name__ == "__main__":
    main()
