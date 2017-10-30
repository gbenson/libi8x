Python bindings for libi8x
==========================

Infinity is a platform-independent system for executables and shared
libraries to expose functionality to debug, monitoring, and analysis
tooling.

In Infinity, executable and shared library files contain *Infinity
notes* in addition to their regular contents.  Each Infinity note
contains a function encoded in a platform-independent instruction
set that note-consuming tools can load and execute.

libi8x is a C library that allows programs to access and execute
Infinity functions.  This package provides Python bindings for the
libi8x C library.


Requirements
------------

You must have the libi8x C library installed on your system to build
and use this package.  Please see `libi8x’s README`_ for details.


Installation
------------

The easiest way to install libi8x-python is to use pip::

  pip install -U --user libi8x-python

or::

  sudo pip install -U libi8x-python

If you don’t have pip please refer to `installing pip`_.  Hint: try
one of these commands::

  sudo apt-get install python-pip
  sudo yum install python-pip

The latest development versions of libi8x and libi8x-python are
available from GitLab_.

libi8x-python requires Python 2.7 or newer.


License
-------

libi8x is licensed under the terms of the GNU Lesser General Public
License, either `version 2.1`_ of the License, or (at your option)
any later version.


Documentation
-------------

Right now there’s no documentation for libi8x aside from this
file.  This is being worked on!


Contributing
------------

The Infinity project homepage is https://infinitynotes.org/.  Future
work is planned and coordinated on https://infinitynotes.org/roadmap.
For help or to report bugs please email infinity@sourceware.org.


.. Links
.. _GitLab: https://gitlab.com/gbenson/libi8x/
.. _installing pip: https://pip.pypa.io/en/stable/installing/
.. _libi8x’s README: https://gitlab.com/gbenson/libi8x/blob/master/README
.. _version 2.1: http://gnu.org/licenses/lgpl-2.1.html
