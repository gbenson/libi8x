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

#include <errno.h>
#include <fcntl.h>
#include <libi8x.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#define NUM_TESTS 2
static const char *tests[NUM_TESTS][2] =
  {
    {"iterative", "corpus/i8c/0.0.2/test_loops/test_basic_0001"},
    {"recursive", "corpus/i8c/0.0.2/test_debug_code/test_loggers_0001"},
  };

#define NUM_VARIANTS 2
static const char *variants[NUM_VARIANTS] =
  {
    "el",  /* Little endian.  */
    "be",  /* Big endian.  */
  };

#define STACK_SIZE 512

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
#if __WORDSIZE == 64
    6227020800,
    87178291200,
    1307674368000,
    20922789888000,
    355687428096000,
    6402373705728000,
    121645100408832000,
    2432902008176640000,
#endif // __WORDSIZE == 64
  };

static i8x_err_e
load_and_register (struct i8x_ctx *ctx, const char *filename,
		   struct i8x_func **funcp)
{
  struct stat sb;
  char *buf;
  int fd;
  struct i8x_note *note;
  struct i8x_func *func;
  i8x_err_e err;

  fd = open (filename, O_RDONLY);
  if (fd == -1)
    exit (EXIT_FAILURE);

  if (fstat (fd, &sb) == -1)
    exit (EXIT_FAILURE);

  buf = mmap (0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (buf == MAP_FAILED)
    exit (EXIT_FAILURE);

  if (close (fd) == -1)
    exit (EXIT_FAILURE);

  err = i8x_note_new_from_buf (ctx, buf, sb.st_size, filename, 0, &note);
  if (err != I8X_OK)
    exit (EXIT_FAILURE);

  if (munmap (buf, sb.st_size) == -1)
    exit (EXIT_FAILURE);

  err = i8x_func_new_from_note (note, &func);
  i8x_note_unref (note);
  if (err != I8X_OK)
    exit (EXIT_FAILURE);

  err = i8x_ctx_register_func (ctx, func);
  i8x_func_unref (func);
  if (err != I8X_OK)
    exit (EXIT_FAILURE);

  *funcp = i8x_func_ref (func);

  return I8X_OK;
}

static void
do_test (struct i8x_ctx *ctx, struct i8x_xctx *xctx,
	 struct i8x_inferior *inf, const char *test_name,
	 const char *test_note, const char *note_variant,
	 bool use_debug_interpreter)
{
  char filename[BUFSIZ];
  struct i8x_func *func;
  struct i8x_funcref *ref;
  i8x_err_e err;

  fprintf (stderr, " %s: %s,%s,%s ...",
	   program_invocation_short_name,
	   test_name, note_variant,
	   use_debug_interpreter ? "dbg" : "std");
  fflush (stderr);

  snprintf (filename, sizeof (filename),
	    "%s/%s/0001", test_note, note_variant);
  filename[sizeof (filename) - 1] = '\0';

  err = load_and_register (ctx, filename, &func);
  if (err != I8X_OK)
    exit (EXIT_FAILURE);

  ref = i8x_func_get_funcref (func);
  if (strcmp (i8x_funcref_get_fullname (ref), "test::factorial(i)i"))
    exit (EXIT_FAILURE);

  i8x_xctx_set_use_debug_interpreter (xctx, use_debug_interpreter);

  for (int i = 0; i < NUM_FACTORIALS; i++)
    {
      union i8x_value args[1], rets[1];

      args[0].i = i;
      err = i8x_xctx_call (xctx, ref, inf, args, rets);
      if (err != I8X_OK)
	exit (EXIT_FAILURE);

      if (rets[0].i != factorials[i])
	exit (EXIT_FAILURE);
    }

  i8x_ctx_unregister_func (ctx, func);
  i8x_func_unref (func);

  fprintf (stderr, "ok\n");
}

int
main (int argc, char *argv[])
{
  struct i8x_ctx *ctx;
  struct i8x_xctx *xctx;
  struct i8x_inferior *inf;
  i8x_err_e err;

  err = i8x_ctx_new (I8X_DBG_MEM, NULL, &ctx);
  if (err != I8X_OK)
    exit (EXIT_FAILURE);

  err = i8x_xctx_new (ctx, STACK_SIZE, &xctx);
  if (err != I8X_OK)
    exit (EXIT_FAILURE);

  inf = NULL;

  for (int di = 1; di >= 0; di--)
    {
      bool use_debug_interpreter = (di != 0);

      for (int t = 0; t < NUM_TESTS; t++)
	{
	  const char *test_name = tests[t][0];
	  const char *test_note = tests[t][1];

	  for (int v = 0; v < NUM_VARIANTS; v++)
	    {
	      const char *note_variant = variants[v];

	      do_test (ctx, xctx, inf, test_name, test_note,
		       note_variant, use_debug_interpreter);
	    }
	}
    }

  i8x_xctx_unref (xctx);
  i8x_ctx_unref (ctx);

  exit (EXIT_SUCCESS);
}
