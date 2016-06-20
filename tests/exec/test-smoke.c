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

/* Smoke test.  Attempt to execute every note in tests/corpus.  */

#include "execution-test.h"

#include <ftw.h>

static void
load_and_execute (struct i8x_ctx *ctx, struct i8x_xctx *xctx,
		  struct i8x_inf *inf, const char *filename)
{
  struct i8x_sized_buf buf;
  i8x_test_mmap (filename, &buf);

  struct i8x_func *func;
  i8x_err_e err = i8x_ctx_import_bytecode (ctx, buf.ptr, buf.size,
					   filename, 0, &func);
  i8x_test_munmap (&buf);

  /* tests/valid/test-corpus will catch any real failures here.  */
  if (err != I8X_OK)
    return;

  i8x_ctx_unregister_func (ctx, func);
  i8x_func_unref (func);
}

static struct i8x_ctx *ftw_ctx;
static struct i8x_xctx *ftw_xctx;
static struct i8x_inf *ftw_inf;

static int
ftw_callback (const char *fpath, const struct stat *sb, int typeflag)
{
  if (typeflag == FTW_D)
    return 0;
  CHECK (typeflag == FTW_F);

  /* Unlike test-corpus, we don't check for non-note files here.
     We're going to try loading them.  */

  load_and_execute (ftw_ctx, ftw_xctx, ftw_inf, fpath);

  return 0;
}

void
i8x_execution_test (struct i8x_ctx *ctx, struct i8x_xctx *xctx,
		    struct i8x_inf *inf, int wordsize,
		    bool bytes_reversed)
{
  /* Most execution tests build or load specific notes with the
     specified wordsize and byte order.  This test just loads
     whatever notes it finds, so we only need to run once.  */
  if (bytes_reversed || wordsize != __WORDSIZE)
    return;

  ftw_ctx = ctx;
  ftw_xctx = xctx;
  ftw_inf = inf;

  CHECK (ftw ("corpus", ftw_callback, 16) == 0);
}
