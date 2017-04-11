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

# Extract all notes from an ELF

import struct

def extract_notes(data):
    # Get the byte order from the header
    HDRFMT = b"4sBB"
    hdrlen = struct.calcsize(HDRFMT)
    magic, ei_class, ei_data = struct.unpack(HDRFMT, data[:hdrlen])
    assert magic == b"\x7fELF"
    byteorder = {1: b"<", 2: b">"}[ei_data]
    # This is not a real ELF parser!
    NOTENAME = b"GNU\0"
    NT_GNU_INFINITY = 8995 # XXX not final!
    markerfmt = byteorder + ("I%ds" % len(NOTENAME)).encode("utf-8")
    marker = struct.pack(markerfmt, NT_GNU_INFINITY, NOTENAME)
    hdrfmt = byteorder + b"2I"
    start = hdrsz = struct.calcsize(hdrfmt)
    while True:
        start = data.find(marker, start)
        if start < 0:
            break
        start -= hdrsz
        namesz, descsz = struct.unpack(hdrfmt, data[start:start + hdrsz])
        if namesz != len(NOTENAME):
            start += hdrsz + 1 # Spurious match
            continue
        descstart = start + hdrsz + struct.calcsize(b"I") + len(NOTENAME)
        desclimit = descstart + descsz
        yield data[descstart:desclimit]
        start = desclimit
