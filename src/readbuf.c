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

#include <byteswap.h>
#include "libi8x-private.h"

struct i8x_readbuf
{
  I8X_OBJECT_FIELDS;

  i8x_byte_order_e byte_order;	/* Byte order for multibyte values.  */

  const char *start;	/* Pointer to first byte of buffer.  */
  const char *limit;	/* Pointer to byte after last byte of buffer.  */
  const char *ptr;	/* Pointer to next byte to be read.  */
};

const struct i8x_object_ops i8x_readbuf_ops =
  {
    "readbuf",				/* Object name.  */
    sizeof (struct i8x_readbuf),	/* Object size.  */
    NULL,				/* Unlink function.  */
    NULL,				/* Free function.  */
  };

static i8x_err_e
i8x_rb_new (struct i8x_note *note,
	    const char *buf, size_t bufsiz,
	    struct i8x_readbuf **rb)
{
  struct i8x_readbuf *b;
  i8x_err_e err;

  err = i8x_ob_new (note, &i8x_readbuf_ops, &b);
  if (err != I8X_OK)
    return err;

  b->start = b->ptr = buf;
  b->limit = buf + bufsiz;

  *rb = b;

  return I8X_OK;
}

I8X_EXPORT i8x_err_e
i8x_rb_new_from_note (struct i8x_note *note, struct i8x_readbuf **rb)
{
  return i8x_rb_new (note, i8x_note_get_encoded (note),
		     i8x_note_get_encoded_size (note), rb);
}

I8X_EXPORT i8x_err_e
i8x_rb_new_from_chunk (struct i8x_chunk *chunk, struct i8x_readbuf **rb)
{
  return i8x_rb_new (i8x_chunk_get_note (chunk),
		     i8x_chunk_get_encoded (chunk),
		     i8x_chunk_get_encoded_size (chunk), rb);
}

I8X_EXPORT struct i8x_note *
i8x_rb_get_note (struct i8x_readbuf *rb)
{
  return (struct i8x_note *)
    i8x_ob_get_parent ((struct i8x_object *) rb);
}

I8X_EXPORT i8x_byte_order_e
i8x_rb_get_byte_order (struct i8x_readbuf *rb)
{
  return rb->byte_order;
}

I8X_EXPORT void
i8x_rb_set_byte_order (struct i8x_readbuf *rb, i8x_byte_order_e order)
{
  rb->byte_order = order;
}

const char *
i8x_rb_get_ptr (struct i8x_readbuf *rb)
{
  return rb->ptr;
}

static i8x_err_e
i8x_rb_error (i8x_err_e code, struct i8x_readbuf *rb)
{
  struct i8x_note *note = i8x_rb_get_note (rb);

  return i8x_note_error (note, code, rb->ptr);
}

I8X_EXPORT size_t
i8x_rb_bytes_left (struct i8x_readbuf *rb)
{
  return rb->limit - rb->ptr;
}

I8X_EXPORT i8x_err_e
i8x_rb_read_byte_order_mark (struct i8x_readbuf *rb)
{
  const char *saved_ptr = rb->ptr;
  i8x_byte_order_e saved_order = rb->byte_order;
  uint16_t new_order;
  i8x_err_e err;

  rb->byte_order = I8X_BYTE_ORDER_STANDARD;
  err = i8x_rb_read_uint16_t (rb, &new_order);
  rb->byte_order = saved_order;
  if (err != I8X_OK)
    return err;

  if (new_order != I8X_BYTE_ORDER_STANDARD
      && new_order != I8X_BYTE_ORDER_REVERSED)
    return i8x_note_error (i8x_rb_get_note (rb),
			   I8X_NOTE_INVALID, saved_ptr);

  rb->byte_order = new_order;

  return I8X_OK;
}

I8X_EXPORT i8x_err_e
i8x_rb_read_int8_t (struct i8x_readbuf *rb, int8_t *result)
{
  if (i8x_rb_bytes_left (rb) < sizeof (int8_t))
    return i8x_rb_error (I8X_NOTE_CORRUPT, rb);

  *result = *(uint8_t *) rb->ptr;
  rb->ptr += sizeof (int8_t);

  return I8X_OK;
}

I8X_EXPORT i8x_err_e
i8x_rb_read_uint8_t (struct i8x_readbuf *rb, uint8_t *result)
{
  if (i8x_rb_bytes_left (rb) < sizeof (uint8_t))
    return i8x_rb_error (I8X_NOTE_CORRUPT, rb);

  *result = *(uint8_t *) rb->ptr;
  rb->ptr += sizeof (uint8_t);

  return I8X_OK;
}

#define I8X_RB_READ_FIXED_MULTI_1(TYPE, BSWAP)				\
  I8X_EXPORT i8x_err_e							\
  i8x_rb_read_ ## TYPE (struct i8x_readbuf *rb, TYPE *result)		\
  {									\
    TYPE tmp;								\
									\
    if (i8x_rb_bytes_left (rb) < sizeof (TYPE))				\
      return i8x_rb_error (I8X_NOTE_CORRUPT, rb);			\
									\
    tmp = *(TYPE *) rb->ptr;						\
    rb->ptr += sizeof (TYPE);						\
									\
    if (rb->byte_order == I8X_BYTE_ORDER_REVERSED)			\
      tmp = BSWAP (tmp);						\
    else								\
      i8x_assert (rb->byte_order == I8X_BYTE_ORDER_STANDARD);		\
									\
    *result = tmp;							\
									\
    return I8X_OK;							\
  }

#define I8X_RB_READ_FIXED_MULTI(SIZE)					\
  I8X_RB_READ_FIXED_MULTI_1 (int ## SIZE ## _t, bswap_ ## SIZE)		\
  I8X_RB_READ_FIXED_MULTI_1 (uint ## SIZE ## _t, bswap_ ## SIZE)

I8X_RB_READ_FIXED_MULTI (16)
I8X_RB_READ_FIXED_MULTI (32)
I8X_RB_READ_FIXED_MULTI (64)

I8X_EXPORT i8x_err_e
i8x_rb_read_sleb128 (struct i8x_readbuf *rb, intmax_t *rp)
{
  i8x_not_implemented ();
}

I8X_EXPORT i8x_err_e
i8x_rb_read_uleb128 (struct i8x_readbuf *rb, uintmax_t *rp)
{
  uintmax_t result = 0;
  int shift = 0;

  while (1)
    {
      uint8_t byte;
      int err;

      err = i8x_rb_read_uint8_t (rb, &byte);
      if (err != I8X_OK)
	return err;

      result |= ((byte & 127) << shift);

      if ((byte & 128) == 0)
	break;

      shift += 7;
    }

  *rp = result;

  return I8X_OK;
}

I8X_EXPORT i8x_err_e
i8x_rb_read_bytes (struct i8x_readbuf *rb, size_t nbytes,
		   const char **result)
{
  if (i8x_rb_bytes_left (rb) < nbytes)
    return i8x_rb_error (I8X_NOTE_CORRUPT, rb);

  *result = rb->ptr;
  rb->ptr += nbytes;

  return I8X_OK;
}

i8x_err_e
i8x_rb_read_funcref (struct i8x_readbuf *rb, struct i8x_funcref **ref)
{
  const char *provider, *name, *ptypes, *rtypes;
  i8x_err_e err;

  err = i8x_rb_read_offset_string (rb, &provider);
  if (err != I8X_OK)
    return err;

  err = i8x_rb_read_offset_string (rb, &name);
  if (err != I8X_OK)
    return err;

  err = i8x_rb_read_offset_string (rb, &ptypes);
  if (err != I8X_OK)
    return err;

  err = i8x_rb_read_offset_string (rb, &rtypes);
  if (err != I8X_OK)
    return err;

  err = i8x_ctx_get_funcref (i8x_rb_get_ctx (rb),
			     provider, name, ptypes, rtypes, ref);

  return err;
}
