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

#include "libi8x-private.h"
#include "inferior-private.h"

static i8x_err_e
no_relocate_fn (struct i8x_xctx *xctx, struct i8x_inferior *inf,
		struct i8x_func *func, uintptr_t unresolved,
		uintptr_t *result)
{
  /* Don't use i8x_ctx_set_error, the interpreter does it for us.  */
  return I8X_NO_RELOCATE_FN;
}

static void
i8x_inferior_unlink (struct i8x_object *ob)
{
  struct i8x_inferior *inf = (struct i8x_inferior *) ob;

  i8x_inferior_invalidate_relocs (inf);
}

const struct i8x_object_ops i8x_inferior_ops =
  {
    "inferior",				/* Object name.  */
    sizeof (struct i8x_inferior),	/* Object size.  */
    i8x_inferior_unlink,		/* Unlink function.  */
    NULL,				/* Free function.  */
  };

I8X_EXPORT i8x_err_e
i8x_inferior_new (struct i8x_ctx *ctx, struct i8x_inferior **inf)
{
  struct i8x_inferior *i;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_inferior_ops, &i);
  if (err != I8X_OK)
    return err;

  i->relocate_fn = no_relocate_fn;

  *inf = i;

  return I8X_OK;
}

I8X_EXPORT void
i8x_inferior_set_relocate_fn (struct i8x_inferior *inf,
			      i8x_relocate_fn_t *relocate_fn)
{
  inf->relocate_fn = relocate_fn;
}