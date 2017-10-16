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

/* Attempt to validate every note in tests/corpus.  */

#include "validation-test.h"

#include <ftw.h>
#include <string.h>

static bool
do_test (struct i8x_ctx *ctx, const char *filename)
{
  i8x_err_e err;

  struct i8x_sized_buf buf;
  i8x_test_mmap (filename, &buf);

  struct i8x_note *note;
  err = i8x_note_new (ctx, buf.ptr, buf.size, filename, 0, &note);
  i8x_test_munmap (&buf);
  CHECK_CALL (ctx, err);

  /* Peek into the note and check we have the wordsize for it.  */
  if (__WORDSIZE == 32)
    {
      struct i8x_chunk *chunk;
      err = i8x_note_get_unique_chunk (note, I8_CHUNK_CODEINFO,
				       false, &chunk);
      CHECK_CALL (ctx, err);

      if (chunk != NULL)
	{
	  CHECK (i8x_chunk_get_encoded_size (chunk) >= 1);

	  char first_byte = *i8x_chunk_get_encoded (chunk);
	  if (first_byte != 0x49 && first_byte != 0x18)
	    {
	      i8x_note_unref (note);

	      return true;
	    }
	}
    }

  i8x_err_e expect_err = I8X_OK;

  /* test_deref/0003-0001 and test_deref_offset_base/0001-0001
     both contain "I8_OP_deref_int 0" which is no longer valid.  */
  if (strstr (filename, "/i8c/0.0.3/") != NULL
      && (strstr (filename, "/test_deref/test_deref/0003-0001") != NULL
	  || strstr (filename, "/test_deref_offset_base/0001-0001") != NULL))
    {
      expect_err = I8X_NOTE_UNHANDLED;
    }

  /* test_deref/0011-0001 and test_deref/0015-0001 contain
     64-bit dereferences which we do not support on 32-bit.  */
  if (__WORDSIZE == 32
      && (strstr (filename, "/32be/") != NULL
	  || strstr (filename, "/32el/") != NULL)
      && (strstr (filename, "/test_deref/0011-0001") != NULL
	  || strstr (filename, "/test_deref/0015-0001") != NULL))
    {
	expect_err = I8X_NOTE_UNHANDLED;
    }

  /* Various test_load_constant tests contain constants overflow.  */
  if (strstr (filename, "/test_load_constant/test_output/") != NULL)
    {
      if (strstr (filename, "/0015-0001") != NULL
	  || strstr (filename, "/0028-0001") != NULL
	  || (__WORDSIZE == 32
	      && (strstr (filename, "/0011-0001") != NULL
		  || strstr (filename, "/0012-0001") != NULL
		  || strstr (filename, "/0013-0001") != NULL
		  || strstr (filename, "/0014-0001") != NULL
		  || strstr (filename, "/0024-0001") != NULL
		  || strstr (filename, "/0025-0001") != NULL
		  || strstr (filename, "/0026-0001") != NULL
		  || strstr (filename, "/0027-0001") != NULL)))
	{
	  expect_err = I8X_NOTE_UNHANDLED;
	}
    }

  struct i8x_func *func;
  err = i8x_func_new_bytecode (note, &func);
  i8x_note_unref (note);
  if (err == I8X_OK)
    i8x_func_unref (func);

  if (err != expect_err)
    {
      if (err == I8X_OK)
	fprintf (stderr, "%s: Should not validate\n", filename);
      else
	{
	  char msg[BUFSIZ];

	  fprintf (stderr, "%s\n",
		   i8x_ctx_strerror_r (ctx, err, msg, sizeof (msg)));
	}

      return false;
    }

  return true;
}

static struct i8x_ctx *ftw_ctx;
static int ftw_passcount;
static int ftw_failcount;

static int
ftw_callback (const char *fpath, const struct stat *sb, int typeflag)
{
  if (typeflag == FTW_D)
    return 0;
  CHECK (typeflag == FTW_F);

  if (strstr (fpath, "Makefile") != NULL
      || strstr (fpath, "README") != NULL)
    return 0;

  if (do_test (ftw_ctx, fpath))
    ftw_passcount++;
  else
    ftw_failcount++;

  return 0;
}

void
i8x_validation_test (struct i8x_ctx *ctx)
{
  ftw_ctx = ctx;
  ftw_passcount = 0;
  ftw_failcount = 0;

  CHECK (ftw (i8x_test_srcfile ("corpus"), ftw_callback, 16) == 0);
  CHECK (ftw_passcount + ftw_failcount == 1528);
  CHECK (ftw_failcount == 0);
}
