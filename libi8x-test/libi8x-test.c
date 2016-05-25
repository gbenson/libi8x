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

#include <stdio.h>
#include <byteswap.h>
#include <libi8x-test.h>

void
__i8x_test_fail (const char *file, int line,
		 struct i8x_ctx *ctx, i8x_err_e err,
		 const char *format, ...)
{
  char buf[BUFSIZ];
  va_list args;

  fprintf (stderr, "%s:%d: ", file, line);

  if (err != I8X_OK)
    fputs (i8x_ctx_strerror_r (ctx, err, buf, sizeof (buf)), stderr);
  else
    {
      va_start (args, format);
      vfprintf (stderr, format, args);
      va_end (args);
    }

  fputc ('\n', stderr);

  exit (EXIT_FAILURE);
}

const char *
i8x_byte_order_name (bool bytes_reversed)
{
  static const char *names[2] = {"be", "el"};
  uint16_t tmp = 1;

  if (bytes_reversed)
    tmp = bswap_16 (tmp);

  return names[*((unsigned char *) &tmp)];
}

void
i8x_execution_test_main (void)
{
  /* Run the test with the debug allocator and interpreter, to
     catch any assertion failures or reference-counting errors,
     and then run it with the regular setup.  */
  for (int is_debug = 1; is_debug >= 0; is_debug--)
    {
      struct i8x_ctx *ctx;
      struct i8x_xctx *xctx;
      struct i8x_inf *inf;
      const int stack_size = 512;
      i8x_err_e err;

      err = i8x_ctx_new (is_debug == 1 ? I8X_DBG_MEM : 0, NULL, &ctx);
      CHECK_CALL (NULL, err);

      err = i8x_xctx_new (ctx, stack_size, &xctx);
      CHECK_CALL (ctx, err);
      i8x_xctx_set_use_debug_interpreter (xctx, is_debug);

      err = i8x_inf_new (ctx, &inf);
      CHECK_CALL (ctx, err);

      /* Run each test in 32-bit mode first, and then 64-bit
	 mode if supported.  */
      for (int wordsize = 32; wordsize <= __WORDSIZE; wordsize += 32)
	{
	  /* Run each test in native byte order first, and then
	     in reversed byte order to catch any missing swaps.  */
	  for (int is_reversed = 0; is_reversed <= 1; is_reversed++)
	    i8x_execution_test (ctx, xctx, inf, wordsize, is_reversed);
	}

      i8x_inf_unref (inf);
      i8x_xctx_unref (xctx);
      i8x_ctx_unref (ctx);
    }

  exit (EXIT_SUCCESS);
}
