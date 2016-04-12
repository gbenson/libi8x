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
#include "interp-private.h"
#include "optable.c"

struct i8x_func *
i8x_code_get_func (struct i8x_code *code)
{
  return (struct i8x_func *)
    i8x_ob_get_parent ((struct i8x_object *) code);
}

struct i8x_note *
i8x_code_get_note (struct i8x_code *code)
{
  return i8x_func_get_note (i8x_code_get_func (code));
}

static i8x_err_e
i8x_code_unpack_info (struct i8x_code *code)
{
  struct i8x_note *note = i8x_code_get_note (code);
  struct i8x_chunk *chunk;
  struct i8x_readbuf *rb;
  i8x_err_e err;

  // XXX maybe doesn't need to exist?
  // (can infer max_stack from params list)
  err = i8x_note_get_unique_chunk (note, I8_CHUNK_CODEINFO,
				   true, &chunk);
  if (err != I8X_OK)
    return err;

  if (i8x_chunk_get_version (chunk) != 1)
    return i8x_chunk_version_error (chunk);

  err = i8x_rb_new_from_chunk (chunk, &rb);
  if (err != I8X_OK)
    return err;

  err = i8x_rb_read_byte_order_mark (rb);
  if (err != I8X_OK)
    goto cleanup;

  code->byte_order = i8x_rb_get_byte_order (rb);

  err = i8x_rb_read_uleb128 (rb, &code->max_stack);
  if (err != I8X_OK)
    goto cleanup;

 cleanup:
  rb = i8x_rb_unref (rb);

  return err;
}

static i8x_err_e
i8x_code_read_opcode (struct i8x_readbuf *rb, i8x_opcode_t *opcode)
{
  const char *location = i8x_rb_get_ptr (rb);
  uintmax_t tmp;
  uint8_t byte;
  i8x_opcode_t result;
  i8x_err_e err;

  err = i8x_rb_read_uint8_t (rb, &byte);
  if (err != I8X_OK)
    return err;

  if (byte == DW_OP_GNU_wide_op)
    {
      uintmax_t wide;

      err = i8x_rb_read_uleb128 (rb, &wide);
      if (err != I8X_OK)
	return err;

      tmp = 0x100 + wide;
    }
  else
    tmp = byte;

  /* Check for overflow.  */
  result = tmp;
  if (result != tmp)
    return i8x_note_error (i8x_rb_get_note(rb),
			   I8X_NOTE_UNHANDLED, location);

  *opcode = result;

  return I8X_OK;
}

static i8x_err_e
i8x_code_read_operand (struct i8x_readbuf *rb,
		       i8x_operand_type_e type,
		       union i8x_value *operand)
{
  const char *location = i8x_rb_get_ptr (rb);
  intmax_t signed_result;
  uintmax_t unsigned_result;
  union i8x_value result;
  bool is_signed;
  i8x_err_e err;

  switch (type)
    {
    case I8X_OPR_NONE:
      return I8X_OK;

#define I8X_READ_FIXED_1(LABEL, TYPE, IS_SIGNED, RESULT)	\
  case LABEL:							\
    is_signed = IS_SIGNED;					\
    {								\
      TYPE tmp;							\
								\
      err = i8x_rb_read_ ## TYPE (rb, &tmp);			\
      RESULT = tmp;						\
    }								\
    break

#define I8X_READ_FIXED(SIZE)					\
  I8X_READ_FIXED_1 (I8X_OPR_INT ## SIZE,			\
		    int ## SIZE ## _t,				\
		    true, signed_result);			\
  I8X_READ_FIXED_1 (I8X_OPR_UINT ## SIZE,			\
		    uint ## SIZE ## _t,				\
		    false, unsigned_result)

    I8X_READ_FIXED (8);
    I8X_READ_FIXED (16);
    I8X_READ_FIXED (32);
    I8X_READ_FIXED (64);

#undef I8X_READ_FIXED
#undef I8X_READ_FIXED_1

    case I8X_OPR_SLEB128:
      is_signed = true;
      err = i8x_rb_read_sleb128 (rb, &signed_result);
      break;

    case I8X_OPR_ULEB128:
      is_signed = false;
      err = i8x_rb_read_uleb128 (rb, &unsigned_result);
      break;

    default:
      return i8x_note_error (i8x_rb_get_note (rb),
			     I8X_NOTE_UNHANDLED, location);
    }

  if (err != I8X_OK)
    return err;

  /* Check for overflow.  */
  if (is_signed)
    {
      result.i = signed_result;
      if (result.i != signed_result)
	return i8x_note_error (i8x_rb_get_note(rb),
			       I8X_NOTE_UNHANDLED, location);
    }
  else
    {
      result.u = unsigned_result;
      if (result.u != unsigned_result)
	return i8x_note_error (i8x_rb_get_note(rb),
			       I8X_NOTE_UNHANDLED, location);
    }

  *operand = result;

  return I8X_OK;
}

static i8x_err_e
i8x_code_unpack_bytecode (struct i8x_code *code)
{
  struct i8x_note *note = i8x_code_get_note (code);
  struct i8x_chunk *chunk;
  struct i8x_readbuf *rb;
  i8x_err_e err;

  // XXX doesn't need to exist!
  err = i8x_note_get_unique_chunk (note, I8_CHUNK_BYTECODE,
				   true, &chunk);
  if (err != I8X_OK)
    return err;

  if (i8x_chunk_get_version (chunk) != 2)
    return i8x_chunk_version_error (chunk);

  code->code_size = i8x_chunk_get_encoded_size (chunk);
  code->code_start = i8x_chunk_get_encoded (chunk);

  code->itable = calloc (code->code_size, sizeof (struct i8x_instr *));
  if (code->itable == NULL)
    return i8x_out_of_memory (i8x_note_get_ctx (note));

  err = i8x_rb_new_from_chunk (chunk, &rb);
  if (err != I8X_OK)
    return err;

  i8x_rb_set_byte_order (rb, code->byte_order);

  while (i8x_rb_bytes_left (rb) > 0)
    {
      const char *ptr = i8x_rb_get_ptr (rb);
      struct i8x_instr *op;
      size_t bcp = ptr - code->code_start;

      op = code->itable[bcp] = calloc (1, sizeof (struct i8x_instr));
      if (op == NULL)
	{
	  err = i8x_out_of_memory (i8x_note_get_ctx (note));
	  break;
	}

      err = i8x_code_read_opcode (rb, &op->code);
      if (err != I8X_OK)
	break;

      if (op->code <= MAX_OPCODE)
	op->desc = &optable[op->code];

      if (op->desc == NULL || op->desc->name == NULL)
	{
	  notice (i8x_note_get_ctx (note),
		  "opcode 0x%lx not in optable\n", op->code);
	  err = i8x_note_error (note, I8X_NOTE_UNHANDLED, ptr);
	  break;
	}

      err = i8x_code_read_operand (rb, op->desc->op1, &op->op1);
      if (err != I8X_OK)
	break;

      err = i8x_code_read_operand (rb, op->desc->op2, &op->op2);
      if (err != I8X_OK)
	break;
    }

  rb = i8x_rb_unref (rb);

  return err;
}

static i8x_err_e
i8x_code_init (struct i8x_code *code)
{
  i8x_err_e err;

  err = i8x_code_unpack_info (code);
  if (err != I8X_OK)
    return err;

  err = i8x_code_unpack_bytecode (code);
  if (err != I8X_OK)
    return err;

  return I8X_OK;
}

static void
i8x_code_free (struct i8x_object *ob)
{
  struct i8x_code *code = (struct i8x_code *) ob;

  if (code->itable != NULL)
    {
      for (size_t bcp = 0; bcp < code->code_size; bcp++)
	{
	  struct i8x_instr *op = code->itable[bcp];

	  if (op != NULL)
	    free (op);
	}

      free (code->itable);
    }
}

const struct i8x_object_ops i8x_code_ops =
  {
    "code",			/* Object name.  */
    sizeof (struct i8x_code),	/* Object size.  */
    NULL,			/* Unlink function.  */
    i8x_code_free,		/* Free function.  */
  };

i8x_err_e
i8x_code_new_from_func (struct i8x_func *func, struct i8x_code **code)
{
  struct i8x_code *c;
  i8x_err_e err;

  err = i8x_ob_new (func, &i8x_code_ops, &c);
  if (err != I8X_OK)
    return err;

  err = i8x_code_init (c);
  if (err != I8X_OK)
    {
      c = i8x_code_unref (c);

      return err;
    }

  *code = c;

  return I8X_OK;
}
