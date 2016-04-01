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

#include <i8x/libi8x.h>
#include "libi8x-private.h"

struct i8x_note
{
  I8X_OBJECT_FIELDS;

  char *srcname;	/* Name of the file this note came from.  */
  ssize_t srcoffset;	/* Offset in the file this note came from.  */

  size_t encoded_size;	/* Size of encoded data, in bytes.  */
  char *encoded;	/* Encoded data.  */
};

static i8x_err_e
i8x_note_init (struct i8x_note *note, const char *buf, size_t bufsiz,
	       const char *srcname, ssize_t srcoffset)
{
  if (srcname != NULL)
    {
      note->srcname = strdup (srcname);

      if (note->srcname == NULL)
	return i8x_out_of_memory (i8x_note_get_ctx (note));
    }

  note->srcoffset = srcoffset;

  note->encoded_size = bufsiz;
  note->encoded = malloc (bufsiz);
  if (note->encoded == NULL)
    return i8x_out_of_memory (i8x_note_get_ctx (note));

  memcpy (note->encoded, buf, bufsiz);

  return I8X_OK;
}

static void
i8x_note_free (struct i8x_object *ob)
{
  struct i8x_note *note = (struct i8x_note *) ob;

  if (note->srcname != NULL)
    free (note->srcname);

  if (note->encoded != NULL)
    free (note->encoded);
}

const struct i8x_object_ops i8x_note_ops =
  {
    "note",			/* Object name.  */
    sizeof (struct i8x_note),	/* Object size.  */
    NULL,			/* Unlink function.  */
    i8x_note_free,		/* Free function.  */
  };

I8X_EXPORT i8x_err_e
i8x_note_new_from_buf (struct i8x_ctx *ctx, const char *buf,
		       size_t bufsiz, const char *srcname,
		       ssize_t srcoffset, struct i8x_note **note)
{
  struct i8x_note *n;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_note_ops, &n);
  if (err != I8X_OK)
    return err;

  err = i8x_note_init (n, buf, bufsiz, srcname, srcoffset);
  if (err != I8X_OK)
    {
      i8x_note_unref (n);

      return err;
    }

  *note = n;

  return I8X_OK;
}

I8X_EXPORT const char *
i8x_note_get_src_name (struct i8x_note *note)
{
  return note->srcname;
}

I8X_EXPORT ssize_t
i8x_note_get_src_offset (struct i8x_note *note)
{
  return note->srcoffset;
}

I8X_EXPORT size_t
i8x_note_get_encoded_size (struct i8x_note *note)
{
  return note->encoded_size;
}

I8X_EXPORT const char *
i8x_note_get_encoded (struct i8x_note *note)
{
  return note->encoded;
}
