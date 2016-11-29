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
no_read_mem_fn (struct i8x_inf *inf, uintptr_t addr, size_t len,
		void *result)
{
  error (i8x_inf_get_ctx (inf),
	 "inferior %p has no read_mem function\n", inf);

  /* Don't use i8x_ctx_set_error, the interpreter decorates
     our returned error code in CALLBACK_ERROR_CHECK.  */
  return I8X_READ_MEM_FAILED;
}

static i8x_err_e
no_relocate_fn (struct i8x_inf *inf, struct i8x_note *note,
		uintptr_t unrelocated, uintptr_t *result)
{
  error (i8x_inf_get_ctx (inf),
	 "inferior %p has no relocate function\n", inf);

  /* Don't use i8x_ctx_set_error, the interpreter decorates
     our returned error code in CALLBACK_ERROR_CHECK.  */
  return I8X_RELOC_FAILED;
}

static void
i8x_inf_unlink (struct i8x_object *ob)
{
  struct i8x_inf *inf = (struct i8x_inf *) ob;

  i8x_inf_invalidate_relocs (inf);
}

const struct i8x_object_ops i8x_inf_ops =
  {
    "inferior",			/* Object name.  */
    sizeof (struct i8x_inf),	/* Object size.  */
    i8x_inf_unlink,		/* Unlink function.  */
    NULL,			/* Free function.  */
  };

I8X_EXPORT i8x_err_e
i8x_inf_new (struct i8x_ctx *ctx, struct i8x_inf **inf)
{
  struct i8x_inf *i;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_inf_ops, &i);
  if (err != I8X_OK)
    return err;

  i->read_mem_fn = no_read_mem_fn;
  i->relocate_fn = no_relocate_fn;

  *inf = i;

  return I8X_OK;
}

I8X_EXPORT void
i8x_inf_set_read_mem_fn (struct i8x_inf *inf,
			 i8x_read_mem_fn_t *read_mem_fn)
{
  inf->read_mem_fn = read_mem_fn;
}

I8X_EXPORT void
i8x_inf_set_relocate_fn (struct i8x_inf *inf,
			 i8x_relocate_fn_t *relocate_fn)
{
  inf->relocate_fn = relocate_fn;
}
