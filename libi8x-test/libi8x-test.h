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

#ifndef _LIBI8X_TEST_H_
#define _LIBI8X_TEST_H_

#include <libi8x.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHECK(expr) \
  ((void) ((expr) ? 0 : (FAIL (#expr), 0)))

#define FAIL(...) \
  __i8x_test_fail (__FILE__, __LINE__, NULL, I8X_OK, __VA_ARGS__)

#define CHECK_CALL(ctx, err)						\
  ((void) ((err) == I8X_OK ? 0 :					\
	   (__i8x_test_fail (__FILE__, __LINE__,			\
			     (ctx), (err), "err != I8X_OK"), 0)))

void __i8x_test_fail (const char *file, int line,
		      struct i8x_ctx *ctx, i8x_err_e err,
		      const char *format, ...)
  __attribute__ ((__noreturn__, format (printf, 5, 6)));

const char *i8x_byte_order_name (i8x_byte_order_e byte_order);

void i8x_execution_test_main (void) __attribute__ ((__noreturn__));

void i8x_execution_test (struct i8x_ctx *ctx, struct i8x_xctx *xctx,
			 struct i8x_inferior *inf,
			 i8x_byte_order_e byte_order);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBI8X_TEST_H_ */
