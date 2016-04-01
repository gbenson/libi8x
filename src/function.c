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

#include <i8x/libi8x.h>
#include "libi8x-private.h"

struct i8x_func
{
  I8X_OBJECT_FIELDS;

  struct i8x_funcsig *sig;	/* The function's signature.  */
  i8x_impl_fn_t *impl_fn;	/* The function's implementation.  */

  struct i8x_note *note;	/* The note, or NULL if native.  */
};

static i8x_err_e
i8x_bcf_unpack_signature (struct i8x_func *func)
{
  struct i8x_note *note = func->note;
  struct i8x_readbuf *rb;
  struct i8x_chunk *chunk;
  i8x_err_e err;

  err = i8x_note_get_unique_chunk (note, I8_CHUNK_SIGNATURE,
				   I8X_NOTE_UNHANDLED,
				   I8X_NOTE_UNHANDLED, &chunk);
  if (err != I8X_OK)
    return err;

  if (i8x_chunk_get_version (chunk) != 2)
    return i8x_chunk_version_error (chunk);

  err = i8x_rb_new_from_chunk (chunk, &rb);
  if (err != I8X_OK)
    return err;

  err = i8x_fs_new_from_readbuf (rb, &func->sig);

  i8x_rb_unref (rb);

  if (err == I8X_OK)
    dbg (i8x_func_get_ctx (func),
	 "func %p is %s\n", func, i8x_fs_get_fullname (func->sig));

  return err;
}

static i8x_err_e
i8x_bcf_init (struct i8x_func *func)
{
  i8x_err_e err;

  err = i8x_bcf_unpack_signature (func);
  if (err != I8X_OK)
    return err;

  return I8X_OK;
}

static void
i8x_func_unlink (struct i8x_object *ob)
{
  struct i8x_func *func = (struct i8x_func *) ob;

  i8x_fs_unref (func->sig);
  i8x_note_unref (func->note);
}

const struct i8x_object_ops i8x_func_ops =
  {
    "func",			/* Object name.  */
    sizeof (struct i8x_func),	/* Object size.  */
    i8x_func_unlink,		/* Unlink function.  */
    NULL,			/* Free function.  */
  };

I8X_EXPORT i8x_err_e
i8x_func_new_from_note (struct i8x_note *note, struct i8x_func **func)
{
  struct i8x_ctx *ctx = i8x_note_get_ctx (note);
  struct i8x_func *f;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_func_ops, &f);
  if (err != I8X_OK)
    return err;

  f->note = i8x_note_ref (note);

  err = i8x_bcf_init (f);
  if (err != I8X_OK)
    {
      i8x_func_unref (f);

      return err;
    }

  f->impl_fn = NULL; // XXX!

  *func = f;

  return I8X_OK;
}

I8X_EXPORT i8x_err_e
i8x_func_new_native (struct i8x_ctx *ctx, struct i8x_funcsig *sig,
		     i8x_impl_fn_t *impl_fn, struct i8x_func **func)
{
  struct i8x_func *f;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_func_ops, &f);
  if (err != I8X_OK)
    return err;

  dbg (ctx, "func %p is %s\n", f, i8x_fs_get_fullname (sig));

  f->sig = i8x_fs_ref (sig);
  f->impl_fn = impl_fn;

  *func = f;

  return I8X_OK;
}

I8X_EXPORT struct i8x_funcsig *
i8x_func_get_signature (struct i8x_func *func)
{
  return func->sig;
}

I8X_EXPORT struct i8x_note *
i8x_func_get_note (struct i8x_func *func)
{
  return func->note;
}
