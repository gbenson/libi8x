/* Copyright (C) 2016-17 Red Hat, Inc.
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

#include "execution-test.h"

#define NUM_TESTS 2
static const char *tests[NUM_TESTS] = {
  /* Iterative */
  "corpus/i8c/0.0.3/%d%s/test_loops/test_basic/0001-0001",

  /* Recursive */
  "corpus/i8c/0.0.3/%d%s/test_debug_code/test_loggers/0001-0001",
};

#define NUM_FACTORIALS (sizeof (factorials) / sizeof (intptr_t))
static intptr_t factorials[] =
  {
    1,
    1,
    2,
    6,
    24,
    120,
    720,
    5040,
    40320,
    362880,
    3628800,
    39916800,
    479001600,
#if __WORDSIZE >= 64
    6227020800,
    87178291200,
    1307674368000,
    20922789888000,
    355687428096000,
    6402373705728000,
    121645100408832000,
    2432902008176640000,
#endif // __WORDSIZE >= 64
  };

static void
load_and_register (struct i8x_ctx *ctx, const char *filename,
		   struct i8x_func **func)
{
  struct i8x_sized_buf buf;
  i8x_test_mmap (filename, &buf);

  i8x_err_e err = i8x_ctx_import_bytecode (ctx, buf.ptr, buf.size,
					   filename, 0, func);
  CHECK_CALL (ctx, err);

  i8x_test_munmap (&buf);
}

static void
do_test (struct i8x_ctx *ctx, struct i8x_xctx *xctx,
	 struct i8x_inf *inf, const char *test_note,
	 int wordsize, const char *byte_order_name)
{
  char filename[BUFSIZ];
  struct i8x_func *func;
  struct i8x_funcref *ref;
  int max_input = wordsize == 64 ? 20 : 12;
  i8x_err_e err;
  int len;

  len = snprintf (filename, sizeof (filename),
		  test_note, wordsize, byte_order_name);
  CHECK ((size_t) len < sizeof (filename));

  load_and_register (ctx, filename, &func);

  ref = i8x_func_get_funcref (func);
  for (int i = 0; i <= max_input; i++)
    {
      union i8x_value args[1], rets[1];

      args[0].i = i;
      err = i8x_xctx_call (xctx, ref, inf, args, rets);
      CHECK_CALL (ctx, err);

      CHECK (rets[0].i == factorials[i]);
    }

  i8x_func_unregister (func);
  i8x_func_unref (func);
}

void
i8x_execution_test (struct i8x_ctx *ctx, struct i8x_xctx *xctx,
		    struct i8x_inf *inf, int wordsize,
		    bool bytes_reversed)
{
  const char *byte_order_name = i8x_byte_order_name (bytes_reversed);

  for (int i = 0; i < NUM_TESTS; i++)
    do_test (ctx, xctx, inf, tests[i], wordsize, byte_order_name);
}
