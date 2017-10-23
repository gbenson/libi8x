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

/* Smoke test.  Attempt to execute every note in tests/corpus.  */

#include "execution-test.h"

#include <string.h>
#include <ftw.h>

static struct i8x_func *dummy_function_Fp_i;
static struct i8x_func *dummy_function_Fi_po;

static void
set_dummy_values (struct i8x_list *types, union i8x_value *value)
{
  struct i8x_listitem *li;

  i8x_list_foreach (types, li)
    {
      struct i8x_type *type = i8x_listitem_get_type (li);
      const char *encoded = i8x_type_get_encoded (type);

      switch (encoded[0])
	{
	case 'i':
	  value->i = 23;
	  break;

	case 'p':
	  value->u = 0xBAD00001;
	  break;

	case 'o':
	  value->u = 0xBAD00002;
	  break;

	case 'F':
	  if (strcmp (encoded, "Fp(i)") == 0)
	    {
	      value->f = i8x_func_get_funcref (dummy_function_Fp_i);
	      break;
	    }
	  if (strcmp (encoded, "Fi(po)") == 0)
	    {
	      value->f = i8x_func_get_funcref (dummy_function_Fi_po);
	      break;
	    }
	  /* fall through */

	default:
	  FAIL ("unhandled type '%s'", encoded);
	}

      value++;
    }
}

static i8x_err_e
relocate_addr (struct i8x_inf *inf, struct i8x_reloc *reloc,
	       uintptr_t *result)
{
  *result = i8x_reloc_get_unrelocated (reloc);

  return I8X_OK;
}

static i8x_err_e
read_memory (struct i8x_inf *inf, uintptr_t addr, size_t len,
	     void *result)
{
  memset (result, 0, len);

  return I8X_OK;
}

static i8x_err_e
call_unresolved (struct i8x_xctx *xctx, struct i8x_inf *inf,
		 struct i8x_func *func, union i8x_value *args,
		 union i8x_value *rets)
{
  if (func != dummy_function_Fp_i && func != dummy_function_Fi_po)
    {
      struct i8x_funcref *ref = i8x_func_get_funcref (func);

      set_dummy_values (i8x_funcref_get_rtypes(ref), rets);
    }

  return I8X_OK;
}

static void
resolve_and_execute (struct i8x_ctx *ctx, struct i8x_xctx *xctx,
		     struct i8x_inf *inf, const char *filename,
		     struct i8x_func *functions)
{
  CHECK (functions != NULL);

  /* Make sure every function is resolved.  */
  for (struct i8x_func *func = functions;
       func != NULL;
       func = (struct i8x_func *) i8x_func_get_userdata (func))
    {
      struct i8x_funcref *func_ref = i8x_func_get_funcref (func);

      if (i8x_funcref_is_resolved (func_ref))
	continue;

      /* Find the first unresolved function.  */
      struct i8x_listitem *li;
      struct i8x_funcref *ext_ref = NULL;

      i8x_list_foreach (i8x_func_get_externals (func), li)
	{
	  ext_ref = i8x_listitem_get_funcref (li);
	  if (!i8x_funcref_is_resolved (ext_ref))
	    break;
	}

      CHECK (ext_ref != NULL);
      CHECK (!i8x_funcref_is_resolved (ext_ref));

      /* Create and register a dummy function with that signature.  */
      struct i8x_func *ext_func;
      i8x_err_e err
	= i8x_func_new_native (ctx, ext_ref, call_unresolved,
			       &ext_func);
      CHECK_CALL (ctx, err);

      i8x_func_set_userdata (ext_func, functions, NULL);

      err = i8x_func_register (ext_func);
      CHECK_CALL (ctx, err);

      /* Try again.  */
      resolve_and_execute (ctx, xctx, inf, filename, ext_func);

      i8x_func_unregister (ext_func);
      i8x_func_unref (ext_func);

      return;
    }

  /* Call every function.  */
  for (struct i8x_func *func = functions;
       func != NULL;
       func = (struct i8x_func *) i8x_func_get_userdata (func))
    {
      struct i8x_funcref *ref = i8x_func_get_funcref (func);
      CHECK (i8x_funcref_is_resolved (ref));

      union i8x_value *args
	= alloca (i8x_funcref_get_num_params (ref)
		  * sizeof (union i8x_value));
      union i8x_value *rets
	= alloca (i8x_funcref_get_num_returns (ref)
		  * sizeof (union i8x_value));

      set_dummy_values (i8x_funcref_get_ptypes(ref), args);

      i8x_err_e expect_err
	= (strcmp (i8x_funcref_get_signature (ref),
		   "test::fold_load_test()") == 0
	   ? I8X_STACK_OVERFLOW : I8X_OK);

      i8x_err_e err = i8x_xctx_call (xctx, ref, inf, args, rets);

      if (expect_err != I8X_OK)
	CHECK (err == expect_err);
      else
	CHECK_CALL (ctx, err);
    }
}

static void
load_and_execute_1 (struct i8x_ctx *ctx, struct i8x_xctx *xctx,
		    struct i8x_inf *inf, char *filename,
		    struct i8x_func *next_func)
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

  /* Build a linked list of loaded notes.  */
  i8x_func_set_userdata (func, next_func, NULL);

  char *dash = strrchr (filename, '-');
  CHECK (dash != NULL);
  CHECK (strlen (dash) == 5);
  int note_index = atoi (dash + 1);
  CHECK (note_index > 0);

  if (note_index > 1)
    {
      /* Recursively load all lower-numbered notes.  */
      sprintf (dash, "-%04d", note_index - 1);
      load_and_execute_1 (ctx, xctx, inf, filename, func);
    }
  else
    {
      /* All notes loaded, time to run the test.  */
      resolve_and_execute (ctx, xctx, inf, filename, func);
    }

  i8x_func_unregister (func);
  i8x_func_unref (func);
}

static void
load_and_execute (struct i8x_ctx *ctx, struct i8x_xctx *xctx,
		  struct i8x_inf *inf, const char *_filename)
{
  char *filename = strdup (_filename);
  CHECK (filename != NULL);

  load_and_execute_1 (ctx, xctx, inf, filename, NULL);

  free (filename);
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

  i8x_inf_set_read_mem_fn (inf, read_memory);
  i8x_inf_set_relocate_fn (inf, relocate_addr);

  i8x_err_e err = i8x_ctx_import_native (ctx,
					 "smoketest::dummy_function(i)p",
					 call_unresolved,
					 &dummy_function_Fp_i);
  CHECK_CALL (ctx, err);

  err = i8x_ctx_import_native (ctx,
			       "smoketest::dummy_function(po)i",
			       call_unresolved,
			       &dummy_function_Fi_po);
  CHECK_CALL (ctx, err);

  ftw_ctx = ctx;
  ftw_xctx = xctx;
  ftw_inf = inf;

  CHECK (ftw (i8x_test_srcfile ("corpus"), ftw_callback, 16) == 0);

  i8x_func_unref (dummy_function_Fp_i);
  i8x_func_unref (dummy_function_Fi_po);
}
