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

#include <stdlib.h>
#include <stdint.h>

#include <i8x/libi8x.h>
#include "libi8x-private.h"

struct i8x_chunk
{
  I8X_OBJECT_FIELDS;

  struct i8x_chunk *next;

  uintmax_t type_id;
  uintmax_t version;

  const char *version_ptr;	/* For i8x_chunk_version_error.  */

  size_t encoded_size;	/* Size of encoded data, in bytes.  */
  const char *encoded;	/* Encoded data.  */
};

static void
i8x_chunk_unlink (struct i8x_object *ob)
{
  struct i8x_chunk *chunk = (struct i8x_chunk *) ob;

  i8x_chunk_unref (chunk->next);
}

const struct i8x_object_ops i8x_chunk_ops =
  {
    "chunk",			/* Object name.  */
    sizeof (struct i8x_chunk),	/* Object size.  */
    i8x_chunk_unlink,		/* Unlink function.  */
    NULL,			/* Free function.  */
  };

static i8x_err_e
i8x_chunk_new_from_rb (struct i8x_readbuf *rb, struct i8x_chunk **chunk)
{
  struct i8x_note *note = i8x_rb_get_note (rb);
  struct i8x_ctx *ctx = i8x_note_get_ctx (note);
  uintmax_t type_id, version;
  const char *version_ptr;
  size_t encoded_size;
  const char *encoded;
  struct i8x_chunk *c;
  i8x_err_e err;

  err = i8x_rb_read_uleb128 (rb, &type_id);
  if (err != I8X_OK)
    return err;

  version_ptr = i8x_rb_get_ptr (rb);
  err = i8x_rb_read_uleb128 (rb, &version);
  if (err != I8X_OK)
    return err;

  err = i8x_rb_read_uleb128 (rb, &encoded_size);
  if (err != I8X_OK)
    return err;

  dbg (ctx, "found type_id %ld, version %ld, size %ld\n",
       type_id, version, encoded_size);

  /* Note consumers MUST treat chunks of zero size
     as if they were not present.  */
  if (encoded_size == 0)
    {
      *chunk = NULL;

      return I8X_OK;
    }

  err = i8x_rb_read_bytes (rb, encoded_size, &encoded);
  if (err != I8X_OK)
    return err;

  err = i8x_ob_new (note, &i8x_chunk_ops, &c);
  if (err != I8X_OK)
    return err;

  c->type_id = type_id;
  c->version = version;
  c->version_ptr = version_ptr;
  c->encoded_size = encoded_size;
  c->encoded = encoded;

  *chunk = c;

  return I8X_OK;
}

i8x_err_e
i8x_chunk_list_new_from_readbuf (struct i8x_readbuf *rb,
				 struct i8x_chunk **chunk_list)
{
  struct i8x_chunk *first_chunk = NULL;
  struct i8x_chunk **next_chunk = &first_chunk;

  while (i8x_rb_bytes_left (rb) > 0)
    {
      i8x_err_e err;

      err = i8x_chunk_new_from_rb (rb, next_chunk);
      if (err != I8X_OK)
	{
	  i8x_chunk_unref (first_chunk);
	  return err;
	}

      if (*next_chunk != NULL)
	next_chunk = &(*next_chunk)->next;
    }

  *chunk_list = first_chunk;

  return I8X_OK;
}

I8X_EXPORT struct i8x_note *
i8x_chunk_get_note (struct i8x_chunk *chunk)
{
  return (struct i8x_note *)
    i8x_ob_get_parent ((struct i8x_object *) chunk);
}

struct i8x_chunk *
i8x_chunk_get_next (struct i8x_chunk *chunk)
{
  return chunk->next;
}

I8X_EXPORT uintmax_t
i8x_chunk_get_type_id (struct i8x_chunk *chunk)
{
  return chunk->type_id;
}

I8X_EXPORT uintmax_t
i8x_chunk_get_version (struct i8x_chunk *chunk)
{
  return chunk->version;
}

i8x_err_e
i8x_chunk_version_error (struct i8x_chunk *chunk)
{
  return i8x_note_error (i8x_chunk_get_note (chunk),
			 I8X_NOTE_UNHANDLED,
			 chunk->version_ptr);
}

I8X_EXPORT size_t
i8x_chunk_get_encoded_size (struct i8x_chunk *chunk)
{
  return chunk->encoded_size;
}

I8X_EXPORT const char *
i8x_chunk_get_encoded (struct i8x_chunk *chunk)
{
  return chunk->encoded;
}
