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
import nose
import unittest

here = os.path.realpath(os.path.dirname(__file__))

def collector(*args):
    suite = unittest.TestSuite()
    cwd = os.getcwd()
    try:
        for root in (os.path.join(here, "tests", "lo"),
                     os.path.join(here, "tests", "hi"),
                     os.path.dirname(here)):
            os.chdir(root)
            suite.addTest(nose.collector())
    finally:
        os.chdir(cwd)
    return suite
