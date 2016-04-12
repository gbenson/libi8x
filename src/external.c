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

struct i8x_ext
{
  I8X_OBJECT_FIELDS;

  struct i8x_funcref *funcref;	/* Non-NULL for function externals.  */
  struct i8x_symref *symref;	/* Non-NULL for symbol externals.  */
};

static void
i8x_ext_unlink (struct i8x_object *ob)
{
  struct i8x_ext *ext = (struct i8x_ext *) ob;

  ext->funcref = i8x_funcref_unref (ext->funcref);
  ext->symref = i8x_symref_unref (ext->symref);
}

const struct i8x_object_ops i8x_ext_ops =
  {
    "ext",			/* Object name.  */
    sizeof (struct i8x_ext),	/* Object size.  */
    i8x_ext_unlink,		/* Unlink function.  */
    NULL,			/* Free function.  */
  };

#define I8_EXT_FUNCTION 'f'
#define I8_EXT_SYMBOL 's'

i8x_err_e
i8x_ext_new_from_readbuf (struct i8x_readbuf *rb, struct i8x_ext **ext)
{
  struct i8x_note *note = i8x_rb_get_note (rb);
  struct i8x_ext *e;
  const char *error_ptr;
  uint8_t type_id;
  const char *name;
  i8x_err_e err;

  error_ptr = i8x_rb_get_ptr (rb);

  err = i8x_rb_read_uint8_t (rb, &type_id);
  if (err != I8X_OK)
    return err;

  err = i8x_ob_new (note, &i8x_ext_ops, &e);
  if (err != I8X_OK)
    return err;

  switch (type_id)
    {
    case I8_EXT_FUNCTION:
      err = i8x_rb_read_funcref (rb, &e->funcref);
      if (err != I8X_OK)
	break;

      dbg (i8x_note_get_ctx (note),
	   "ext %p is funcref %p\n", e, e->funcref);

      break;

    case I8_EXT_SYMBOL:
      err = i8x_rb_read_offset_string (rb, &name);
      if (err != I8X_OK)
	break;

      err = i8x_ctx_get_symref (i8x_note_get_ctx (note), name, &e->symref);
      if (err != I8X_OK)
	break;

      dbg (i8x_note_get_ctx (note),
	   "ext %p is symref %p\n", e, e->symref);

      break;

    default:
      err = i8x_note_error (note, I8X_NOTE_UNHANDLED, error_ptr);
    }

  if (err == I8X_OK)
    *ext = e;
  else
    i8x_ext_unref (e);

  return err;
}

struct i8x_funcref *
i8x_ext_as_funcref (struct i8x_ext *ext)
{
  return ext->funcref;
}

struct i8x_symref *
i8x_ext_as_symref (struct i8x_ext *ext)
{
  return ext->symref;
}
