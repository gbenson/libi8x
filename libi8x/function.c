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

#include "libi8x-private.h"

struct i8x_func
{
  I8X_OBJECT_FIELDS;

  struct i8x_funcref *ref;	/* The function's signature.  */

  i8x_nat_fn_t *native_impl;	/* Implementation, or NULL if bytecode.  */

  struct i8x_note *note;	/* The note, or NULL if native.  */
  struct i8x_list *externals;	/* List of external function references.  */
  struct i8x_code *code;	/* Compiled bytecode.  */

  bool observed_available;	/* The last observer we called.  */
};

I8X_EXPORT bool
i8x_func_is_native (struct i8x_func *func)
{
  return func->note == NULL;
}

struct i8x_code *
i8x_func_get_interp_impl (struct i8x_func *func)
{
  return func->code;
}

i8x_nat_fn_t *
i8x_func_get_native_impl (struct i8x_func *func)
{
  return func->native_impl;
}

static i8x_err_e
i8x_bcf_unpack_signature (struct i8x_func *func)
{
  struct i8x_note *note = func->note;
  struct i8x_readbuf *rb;
  struct i8x_chunk *chunk;
  i8x_err_e err;

  err = i8x_note_get_unique_chunk (note, I8_CHUNK_SIGNATURE,
				   true, &chunk);
  if (err != I8X_OK)
    return err;

  if (i8x_chunk_get_version (chunk) != 2)
    return i8x_chunk_version_error (chunk);

  err = i8x_rb_new_from_chunk (chunk, &rb);
  if (err != I8X_OK)
    return err;

  err = i8x_rb_read_funcref (rb, &func->ref);

  rb = i8x_rb_unref (rb);

  if (err == I8X_OK)
    dbg (i8x_func_get_ctx (func),
	 "func %p is %s\n", func,
	 i8x_funcref_get_signature (func->ref));

  return err;
}

static i8x_err_e
i8x_bcf_unpack_externals (struct i8x_func *func)
{
  struct i8x_chunk *chunk;
  struct i8x_readbuf *rb;
  i8x_err_e err;

  err = i8x_note_get_unique_chunk (i8x_func_get_note (func),
				   I8_CHUNK_EXTERNALS,
				   false, &chunk);
  if (err != I8X_OK || chunk == NULL)
    return err;

  if (i8x_chunk_get_version (chunk) != 2)
    return i8x_chunk_version_error (chunk);

  err = i8x_list_new (i8x_func_get_ctx (func), true, &func->externals);
  if (err != I8X_OK)
    return err;

  err = i8x_rb_new_from_chunk (chunk, &rb);
  if (err != I8X_OK)
    return err;

  while (i8x_rb_bytes_left (rb) > 0)
    {
      struct i8x_funcref *ref;

      err = i8x_rb_read_funcref (rb, &ref);
      if (err != I8X_OK)
	break;

      err = i8x_list_append_funcref (func->externals, ref);
      ref = i8x_funcref_unref (ref);
      if (err != I8X_OK)
	break;
    }

  rb = i8x_rb_unref (rb);

  return err;
}

static i8x_err_e
i8x_bcf_init (struct i8x_func *func)
{
  i8x_err_e err;

  err = i8x_bcf_unpack_signature (func);
  if (err != I8X_OK)
    return err;

  err = i8x_bcf_unpack_externals (func);
  if (err != I8X_OK)
    return err;

  err = i8x_code_new (func, &func->code);
  if (err != I8X_OK)
    return err;

  return I8X_OK;
}

static void
i8x_func_unlink (struct i8x_object *ob)
{
  struct i8x_func *func = (struct i8x_func *) ob;

  if (func->observed_available)
    i8x_ctx_update_availability (func, false);

  func->code = i8x_code_unref (func->code);
  func->ref = i8x_funcref_unref (func->ref);
  func->externals = i8x_list_unref (func->externals);
  func->note = i8x_note_unref (func->note);
}

static const struct i8x_object_ops i8x_func_ops =
  {
    "func",			/* Object name.  */
    sizeof (struct i8x_func),	/* Object size.  */
    i8x_func_unlink,		/* Unlink function.  */
    NULL,			/* Free function.  */
  };

I8X_EXPORT i8x_err_e
i8x_func_new_bytecode (struct i8x_note *note, struct i8x_func **func)
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
      f = i8x_func_unref (f);

      return err;
    }

  *func = f;

  return I8X_OK;
}

I8X_EXPORT i8x_err_e
i8x_func_new_native (struct i8x_ctx *ctx, struct i8x_funcref *ref,
		     i8x_nat_fn_t *impl_fn, struct i8x_func **func)
{
  struct i8x_func *f;
  i8x_err_e err;

  if (impl_fn == NULL)
    return i8x_invalid_argument (ctx);

  err = i8x_ob_new (ctx, &i8x_func_ops, &f);
  if (err != I8X_OK)
    return err;

  dbg (ctx, "func %p is %s\n", f, i8x_funcref_get_signature (ref));

  f->ref = i8x_funcref_ref (ref);
  f->native_impl = impl_fn;

  *func = f;

  return I8X_OK;
}

I8X_EXPORT struct i8x_funcref *
i8x_func_get_funcref (struct i8x_func *func)
{
  return func->ref;
}

I8X_EXPORT struct i8x_note *
i8x_func_get_note (struct i8x_func *func)
{
  return func->note;
}

I8X_EXPORT struct i8x_list *
i8x_func_get_externals (struct i8x_func *func)
{
  return func->externals;
}

I8X_EXPORT struct i8x_list *
i8x_func_get_relocs (struct i8x_func *func)
{
  if (func->code == NULL)
    return NULL;

  return i8x_code_get_relocs (func->code);
}

bool
i8x_func_all_deps_resolved (struct i8x_func *func)
{
  struct i8x_listitem *li;

  i8x_list_foreach (func->externals, li)
    {
      struct i8x_funcref *ref = i8x_listitem_get_funcref (li);

      if (!i8x_funcref_is_resolved (ref))
	return false;
    }

  return true;
}

void
i8x_func_update_availability (struct i8x_func *func)
{
  bool is_available = i8x_funcref_is_resolved (func->ref);

  if (is_available == func->observed_available)
    return;

  i8x_ctx_update_availability (func, is_available);

  func->observed_available = is_available;
}
