/* Copyright (C) 2016 Red Hat, Inc.
   This file is part of the Infinity Note Execution Library.

   The Infinity Note Execution Library is free software; you can
   redistribute it and/or modify it under the terms of the GNU Lesser
   General Public License as published by the Free Software
   Foundation; either version 2.1 of the License, or (at your option)
   any later version.

   The Infinity Note Execution Library is distributed in the hope that
   it will be useful, but WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the GNU Lesser General Public License for more
   details.

   You should have received a copy of the GNU Lesser General Public
   License along with the Infinity Note Execution Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _LIBI8X_EXECUTION_TEST_H_
#define _LIBI8X_EXECUTION_TEST_H_

#include <libi8x-test.h>

#ifdef __cplusplus
extern "C" {
#endif

int
main (int argc, char *argv[])
{
  i8x_execution_test_main ();
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBI8X_EXECUTION_TEST_H_ */
