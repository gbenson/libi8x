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

#include <string.h>
#include "libi8x-private.h"
#include "reloc-private.h"

void
i8x_reloc_invalidate_for_inferior (struct i8x_reloc *reloc,
				   struct i8x_inf *inf)
{
  if (reloc->cached_from != inf)
    return;

  /* Set cached_from to something that definitely isn't an
     inferior, and poison the cached value too while we're
     at it.  */
  reloc->cached_from = (struct i8x_inf *) reloc;
  reloc->cached_value = I8X_POISON_BAD_CACHED_RELOC;

  dbg (i8x_reloc_get_ctx (reloc),
       "invalidated reloc %p value for inferior %p\n", reloc, inf);
}

static i8x_err_e
i8x_reloc_init (struct i8x_reloc *reloc, uintptr_t unrelocated)
{
  reloc->unrelocated = unrelocated;

  i8x_reloc_invalidate_for_inferior (reloc, reloc->cached_from);
  i8x_assert (reloc->cached_from == (struct i8x_inf *) reloc);

  return I8X_OK;
}

const struct i8x_object_ops i8x_reloc_ops =
  {
    "reloc",			/* Object name.  */
    sizeof (struct i8x_reloc),	/* Object size.  */
    NULL,			/* Unlink function.  */
    NULL,			/* Free function.  */
  };

i8x_err_e
i8x_reloc_new (struct i8x_code *code, uintptr_t unrelocated,
	       struct i8x_reloc **reloc)
{
  struct i8x_reloc *r;
  i8x_err_e err;

  err = i8x_ob_new (code, &i8x_reloc_ops, &r);
  if (err != I8X_OK)
    return err;

  err = i8x_reloc_init (r, unrelocated);
  if (err != I8X_OK)
    {
      r = i8x_reloc_unref (r);

      return err;
    }

  dbg (i8x_code_get_ctx (code), "reloc %p is 0x%lx\n", r, unrelocated);

  *reloc = r;

  return I8X_OK;
}

I8X_EXPORT uintptr_t
i8x_reloc_get_unrelocated (struct i8x_reloc *reloc)
{
  return reloc->unrelocated;
}
