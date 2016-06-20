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

#include <stdio.h>
#include <string.h>
#include "libi8x-private.h"
#include "interp-private.h"
#include "archspec.h"
#include "optable.c"

static struct i8x_func *
i8x_code_get_func (struct i8x_code *code)
{
  return (struct i8x_func *)
    i8x_ob_get_parent ((struct i8x_object *) code);
}

static struct i8x_note *
i8x_code_get_note (struct i8x_code *code)
{
  return i8x_func_get_note (i8x_code_get_func (code));
}

struct i8x_list *
i8x_code_get_relocs (struct i8x_code *code)
{
  return code->relocs;
}

i8x_err_e
i8x_code_error (struct i8x_code *code, i8x_err_e err,
		struct i8x_instr *ip)
{
  return i8x_note_error (i8x_code_get_note (code),
			 err, ip_to_bcp (code, ip));
}

void
i8x_code_reset_is_visited (struct i8x_code *code)
{
  struct i8x_instr *op;

  i8x_code_foreach_op (code, op)
    op->is_visited = false;
}

static i8x_err_e
i8x_code_unpack_info (struct i8x_code *code, struct i8x_funcref *ref)
{
  struct i8x_note *note = i8x_code_get_note (code);
  size_t num_params = i8x_funcref_get_num_params (ref);
  struct i8x_chunk *chunk;
  struct i8x_readbuf *rb;
  const char *location;
  uint16_t archspec;
  i8x_err_e err;

  err = i8x_note_get_unique_chunk (note, I8_CHUNK_CODEINFO,
				   false, &chunk);
  if (err != I8X_OK)
    return err;

  if (chunk == NULL)
    {
      i8x_assert (code->wordsize == 0);
      code->max_stack = num_params;

      return I8X_OK;
    }

  if (i8x_chunk_get_version (chunk) != 1)
    return i8x_chunk_version_error (chunk);

  err = i8x_rb_new_from_chunk (chunk, &rb);
  if (err != I8X_OK)
    return err;

  /* Read the architecture specifier.  */
  location = i8x_rb_get_ptr (rb);
  i8x_rb_set_byte_order (rb, I8X_BYTE_ORDER_NATIVE);
  err = i8x_rb_read_uint16_t (rb, &archspec);
  if (err != I8X_OK)
    goto cleanup;

  i8x_assert (code->wordsize == 0);
  for (int wordsize = 32; wordsize <= __WORDSIZE; wordsize += 32)
    {
      for (int is_swapped = 0; is_swapped <= 1; is_swapped++)
	{
	  if (archspec == ARCHSPEC (wordsize, is_swapped))
	    {
	      code->wordsize = wordsize;
	      code->byte_order = is_swapped ?
		I8X_BYTE_ORDER_REVERSED : I8X_BYTE_ORDER_NATIVE;

	      break;
	    }
	}

      if (code->wordsize != 0)
	break;
    }

  if (code->wordsize == 0)
    {
      err = i8x_rb_error (rb, I8X_NOTE_UNHANDLED, location);
      goto cleanup;
    }

  /* Read max_stack.  */
  location = i8x_rb_get_ptr (rb);
  err = i8x_rb_read_uleb128 (rb, &code->max_stack);
  if (err != I8X_OK)
    goto cleanup;

  if (code->max_stack < num_params)
    {
      err = i8x_rb_error (rb, I8X_NOTE_INVALID, location);
      goto cleanup;
    }

 cleanup:
  rb = i8x_rb_unref (rb);

  return err;
}

static i8x_err_e
i8x_code_read_opcode (struct i8x_readbuf *rb, i8x_opcode_t *opcode)
{
  const char *location = i8x_rb_get_ptr (rb);
  uintptr_t tmp;
  uint8_t byte;
  i8x_opcode_t result;
  i8x_err_e err;

  err = i8x_rb_read_uint8_t (rb, &byte);
  if (err != I8X_OK)
    return err;

  if (byte == DW_OP_GNU_wide_op)
    {
      uintptr_t wide;

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
    return i8x_rb_error (rb, I8X_NOTE_UNHANDLED, location);

  *opcode = result;

  return I8X_OK;
}

static i8x_err_e
i8x_code_read_operand (struct i8x_readbuf *rb,
		       struct i8x_code *code,
		       i8x_operand_type_e type,
		       union i8x_value *operand)
{
  const char *location = i8x_rb_get_ptr (rb);
  intptr_t signed_result;
  uintptr_t unsigned_result;
  union i8x_value result;
  bool is_signed;
  i8x_err_e err;

  if (type == I8X_OPR_ADDRESS)
    {
      if (code->wordsize > __WORDSIZE)
	return i8x_rb_error (rb, I8X_NOTE_UNHANDLED, location);

      switch (code->wordsize)
	{
	case 32:
	  type = I8X_OPR_UINT32;
	  break;

	case 64:
	  type = I8X_OPR_UINT64;
	  break;

	default:
	  return i8x_rb_error (rb, I8X_NOTE_UNHANDLED, location);
	}
    }

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
      return i8x_rb_error (rb, I8X_NOTE_UNHANDLED, location);
    }

  if (err != I8X_OK)
    return err;

  /* Check for overflow.  */
  if (is_signed)
    {
      result.i = signed_result;
      if (result.i != signed_result)
	return i8x_rb_error (rb, I8X_NOTE_UNHANDLED, location);
    }
  else
    {
      result.u = unsigned_result;
      if (result.u != unsigned_result)
	return i8x_rb_error (rb, I8X_NOTE_UNHANDLED, location);
    }

  *operand = result;

  return I8X_OK;
}

static i8x_err_e
i8x_code_unpack_bytecode (struct i8x_code *code)
{
  struct i8x_note *note = i8x_code_get_note (code);
  struct i8x_ctx *ctx = i8x_note_get_ctx (note);
  struct i8x_chunk *chunk;
  size_t itable_size;
  struct i8x_readbuf *rb;
  struct i8x_instr *op;
  i8x_err_e err;

  /* Make sure IT_EMPTY_SLOT does not clash with any defined
     operation.  */
  i8x_assert (optable[IT_EMPTY_SLOT].name == NULL);

  /* Allocating the itable with calloc ensures every entry is
     initialized to IT_EMPTY_SLOT iff IT_EMPTY_SLOT == 0.  If
     this gets redefined then we need to initialize things.  */
  i8x_assert (IT_EMPTY_SLOT == 0);

  err = i8x_note_get_unique_chunk (note, I8_CHUNK_BYTECODE,
				   false, &chunk);
  if (err != I8X_OK)
    return err;

  if (chunk != NULL)
    {
      if (i8x_chunk_get_version (chunk) != 3)
	return i8x_chunk_version_error (chunk);

      code->code_size = i8x_chunk_get_encoded_size (chunk);
      code->code_start = i8x_chunk_get_encoded (chunk);
    }

  itable_size = code->code_size + 1;  /* For I8X_OP_return.  */
  code->itable = calloc (itable_size, sizeof (struct i8x_instr));
  if (code->itable == NULL)
    return i8x_out_of_memory (ctx);

  code->itable_limit = code->itable + itable_size;

  /* Functions return by jumping one instruction past the end of the
     bytecode.  Create a real instruction at that location for jumps
     to land at.  */
  op = code->itable + code->code_size;
  op->code = I8X_OP_return;
  op->desc = &optable[op->code];
  i8x_assert (op->desc != NULL && op->desc->name != NULL);

  if (chunk == NULL)
    return I8X_OK;

  err = i8x_list_new (ctx, true, &code->relocs);
  if (err != I8X_OK)
    return err;

  err = i8x_rb_new_from_chunk (chunk, &rb);
  if (err != I8X_OK)
    return err;

  i8x_rb_set_byte_order (rb, code->byte_order);

  while (i8x_rb_bytes_left (rb) > 0)
    {
      op = bcp_to_ip (code, i8x_rb_get_ptr (rb));

      /* Read the opcode and operands.  */
      err = i8x_code_read_opcode (rb, &op->code);
      if (err != I8X_OK)
	break;

      if (op->code <= MAX_OPCODE)
	op->desc = &optable[op->code];

      if (op->desc == NULL || op->desc->name == NULL)
	{
	  notice (ctx, "opcode " LHEX " not in optable\n", op->code);
	  err = i8x_code_error (code, I8X_NOTE_UNHANDLED, op);
	  break;
	}

      err = i8x_code_read_operand (rb, code, op->desc->arg1, &op->arg1);
      if (err != I8X_OK)
	break;

      err = i8x_code_read_operand (rb, code, op->desc->arg2, &op->arg2);
      if (err != I8X_OK)
	break;

      /* Set up the next instruction pointers.  */
      op->fall_through = bcp_to_ip (code, i8x_rb_get_ptr (rb));

      if (op->code == DW_OP_skip)
	op->fall_through += op->arg1.i;
      else if (op->code == DW_OP_bra)
	op->branch_next = op->fall_through + op->arg1.i;
    }

  rb = i8x_rb_unref (rb);

  if (err == I8X_OK)
    i8x_code_dump_itable (code, __FUNCTION__);

  return err;
}

/* True if the only thing this instruction does is continue
   onwards to exactly one other instruction.  */

#define i8x_op_is_fall_through_only(op) \
  ((op)->code == DW_OP_skip || (op)->code == DW_OP_nop)

/* Check and optimize one next instruction pointer of one
   instruction (or the entry point, in which case OP will
   be NULL). */

static i8x_err_e
i8x_code_setup_flow_1 (struct i8x_code *code,
		       struct i8x_instr *op,
		       struct i8x_instr **result)
{
  struct i8x_instr *next_op = *result;

  i8x_code_reset_is_visited (code);

  while (true)
    {
      /* Validate the branch that got us here.  */
      if (next_op < code->itable
	  || next_op >= code->itable_limit
	  || next_op->code == IT_EMPTY_SLOT)
	return i8x_code_error (code, I8X_NOTE_INVALID, op);

      /* If the next instruction does anything more than
	 fall through then we're done.  */
      if (!i8x_op_is_fall_through_only (next_op))
	break;

      /* Continue through this null instruction.  */
      op = next_op;
      next_op = op->fall_through;

      /* Avoid trivial infinite loops.  */
      if (op->is_visited)
	return i8x_code_error (code, I8X_NOTE_INVALID, op);

      op->is_visited = true;
    }

  *result = next_op;

  return I8X_OK;
}

/* Set up the function entry point, then check and optimize the next
   instruction pointers of all instructions.  A successful result
   indicates that the entry point and all next instruction pointers
   except I8X_OP_return point to a valid location in the instruction
   table, and that all "skip" and "nop" instructions have been
   removed.  */

static i8x_err_e
i8x_code_setup_flow (struct i8x_code *code)
{
  struct i8x_instr *op;
  i8x_err_e err;

  /* Set up the entry point.  */
  code->entry_point = code->itable;
  err = i8x_code_setup_flow_1 (code, NULL, &code->entry_point);
  if (err != I8X_OK)
    return err;

  /* Set up the instruction table.  */
  i8x_code_foreach_op (code, op)
    {
      if (op->code == IT_EMPTY_SLOT || op->code == I8X_OP_return)
	continue;

      if (op->code == DW_OP_bra)
	{
	  err = i8x_code_setup_flow_1 (code, op, &op->branch_next);
	  if (err != I8X_OK)
	    return err;
	}

      err = i8x_code_setup_flow_1 (code, op, &op->fall_through);
      if (err != I8X_OK)
	return err;
    }

  /* Lose all the now-unreachable fall-through-only instructions.  */
  i8x_code_foreach_op (code, op)
    if (i8x_op_is_fall_through_only (op))
      op->code = IT_EMPTY_SLOT;

  i8x_code_dump_itable (code, __FUNCTION__);

  return I8X_OK;
}

static i8x_err_e
i8x_code_get_reloc (struct i8x_code *code, uintptr_t unrelocated,
		    struct i8x_reloc **relocp)
{
  struct i8x_listitem *li;
  struct i8x_reloc *reloc;
  i8x_err_e err;

  /* If we have this relocation already then return it.  */
  i8x_list_foreach (code->relocs, li)
    {
      reloc = i8x_listitem_get_reloc (li);

      if (i8x_reloc_get_unrelocated (reloc) == unrelocated)
	{
	  *relocp = i8x_reloc_ref (reloc);

	  return I8X_OK;
	}
    }

  /* It's a new relocation that needs creating.  */
  err = i8x_reloc_new (code, unrelocated, &reloc);
  if (err != I8X_OK)
    return err;

  err = i8x_list_append_reloc (code->relocs, reloc);
  if (err != I8X_OK)
    {
      reloc = i8x_reloc_unref (reloc);

      return err;
    }

  *relocp = reloc;

  return I8X_OK;
}

static void
i8x_code_rewrite_op (struct i8x_instr *op, i8x_opcode_t new_opcode)
{
  i8x_assert (new_opcode <= MAX_OPCODE);

  op->code = new_opcode;
  op->desc = &optable[new_opcode];

  i8x_assert (op->desc->name != NULL);
}

static i8x_err_e
i8x_code_rewrite_pre_validate (struct i8x_code *code)
{
  struct i8x_func *func = i8x_code_get_func (code);
  struct i8x_list *externals = i8x_func_get_externals (func);
  struct i8x_instr *op;
  i8x_err_e err;

  i8x_code_foreach_op (code, op)
    {
      switch (op->code)
	{
	case DW_OP_const1u:
	case DW_OP_const1s:
	case DW_OP_const2u:
	case DW_OP_const2s:
	case DW_OP_const4u:
	case DW_OP_const4s:
	case DW_OP_const8u:
	case DW_OP_const8s:
	case DW_OP_constu:
	case DW_OP_consts:
	  /* These are all the same.  */
	  i8x_code_rewrite_op (op, I8X_OP_const);
	  break;

	case DW_OP_addr:
	  /* Create a relocation and store at op->addr1.  */
	  err = i8x_code_get_reloc (code, op->arg1.u, &op->addr1);
	  if (err != I8X_OK)
	    return err;
	  break;

	case I8_OP_load_external:
	  /* Put the indicated function reference at op->ext1.  */
	  if (op->arg1.u == 0)
	    op->ext1 = i8x_funcref_ref (i8x_func_get_funcref (func));
	  else
	    {
	      struct i8x_listitem *li;

	      li = i8x_list_get_item_by_index (externals,
					       op->arg1.u - 1);
	      if (li == NULL)
		return i8x_code_error (code, I8X_NOTE_INVALID, op);

	      op->ext1
		= i8x_funcref_ref (i8x_listitem_get_funcref (li));
	    }
	  break;
	}
    }

  i8x_code_dump_itable (code, __FUNCTION__);

  return I8X_OK;
}

static i8x_err_e
i8x_code_remove_casts (struct i8x_code *code)
{
  struct i8x_instr *op1, *op2;

  i8x_code_foreach_op (code, op1)
    {
      if (op1->code != I8_OP_cast_int2ptr
	  && op1->code != I8_OP_cast_ptr2int)
	continue;

      i8x_code_foreach_op (code, op2)
	{
	  if (op2->branch_next == op1)
	    op2->branch_next = op1->fall_through;

	  if (op2->fall_through == op1)
	    op2->fall_through = op1->fall_through;
	}

      op1->code = IT_EMPTY_SLOT;
      op1->is_visited = false;
    }

  i8x_code_dump_itable (code, __FUNCTION__);

  return I8X_OK;
}

static int
i8x_log2 (int x)
{
  int y = 0;

  while (x >>= 1)
    y++;

  return y;
}

static i8x_err_e
i8x_code_rewrite_derefs (struct i8x_code *code)
{
  struct i8x_instr *op;
  int size, shift;

  i8x_code_foreach_op (code, op)
    {
      bool is_signed = false;
      bool is_swapped = false;

      switch (op->code)
	{
	case DW_OP_deref:
	  size = code->wordsize;
	  break;

	case I8_OP_deref_int:
	  size = op->arg1.i;

	  if (size == 0)
	    size = code->wordsize;
	  else if (size < 0)
	    {
	      size = -size;
	      is_signed = true;
	    }
	  break;

	default:
	  continue;
	}

      shift = i8x_log2 (size);
      if (shift < 3
	  || (1 << shift) != size
	  || size > code->wordsize)
	return i8x_code_error (code, I8X_NOTE_UNHANDLED, op);

      shift -= 3;
      i8x_assert (shift >= 0 && shift <= 3);

      if (shift > 0)
	{
	  if (code->byte_order == I8X_BYTE_ORDER_REVERSED)
	    is_swapped = true;
	  else if (code->byte_order != I8X_BYTE_ORDER_NATIVE)
	    return i8x_code_error (code, I8X_NOTE_INVALID, op);
	}

      i8x_code_rewrite_op (op,
			   I8X_OP_deref_u8
			   | ((shift & 3)  << 2)
			   | ((is_signed ? 1 : 0) << 1)
			   | (is_swapped ? 1 : 0));
    }

  i8x_code_dump_itable (code, __FUNCTION__);

  return I8X_OK;
}

static i8x_err_e
i8x_code_setup_dispatch (struct i8x_code *code)
{
  struct i8x_ctx *ctx = i8x_code_get_ctx (code);
  void **dispatch_std, **dispatch_dbg;
  void *std_unhandled;
  struct i8x_instr *op;

  i8x_code_dump_itable (code, __FUNCTION__);

  /* Get the dispatch tables.  */
  i8x_ctx_get_dispatch_tables (ctx, &dispatch_std, &dispatch_dbg);
  std_unhandled = dispatch_std[IT_EMPTY_SLOT];

  i8x_code_foreach_op (code, op)
    {
      i8x_assert (op->code <= MAX_OPCODE);

      op->impl_std = dispatch_std[op->code];
      op->impl_dbg = dispatch_dbg[op->code];

      /* op->is_visited was set by the validator.  */
      if (op->is_visited)
	{
	  if (op->impl_std == std_unhandled)
	    {
	      notice (ctx, "%s not implemented in interpreter\n",
		      op->desc->name);
	      return i8x_code_error (code, I8X_NOTE_UNHANDLED, op);
	    }
	}
      else
	i8x_assert (op->impl_std == std_unhandled);
    }

  return I8X_OK;
}

static i8x_err_e
i8x_code_init (struct i8x_code *code)
{
  struct i8x_func *func = i8x_code_get_func (code);
  struct i8x_funcref *ref = i8x_func_get_funcref (func);
  struct i8x_ctx *ctx = i8x_funcref_get_ctx (ref);
  i8x_err_e err;

  if (i8x_ctx_get_log_priority (ctx) >= LOG_INFO)
    {
      struct i8x_note *note = i8x_func_get_note (func);

      info (ctx, "%s[" LHEX "]: %s\n",
	    i8x_note_get_src_name (note),
	    i8x_note_get_src_offset (note),
	    i8x_funcref_get_fullname (ref));
    }

  err = i8x_code_unpack_info (code, ref);
  if (err != I8X_OK)
    return err;

  err = i8x_code_unpack_bytecode (code);
  if (err != I8X_OK)
    return err;

  err = i8x_code_setup_flow (code);
  if (err != I8X_OK)
    return err;

  err = i8x_code_rewrite_pre_validate (code);
  if (err != I8X_OK)
    return err;

  err = i8x_code_validate (code, ref);
  if (err != I8X_OK)
    return err;

  err = i8x_code_rewrite_derefs (code);
  if (err != I8X_OK)
    return err;

  err = i8x_code_remove_casts (code);
  if (err != I8X_OK)
    return err;

  err = i8x_code_setup_dispatch (code);
  if (err != I8X_OK)
    return err;

  return I8X_OK;
}

static void
i8x_code_unlink (struct i8x_object *ob)
{
  struct i8x_code *code = (struct i8x_code *) ob;
  struct i8x_instr *op;

  if (code->itable != NULL)
    i8x_code_foreach_op (code, op)
      {
	op->addr1 = i8x_reloc_unref (op->addr1);
	op->ext1 = i8x_funcref_unref (op->ext1);
      }

  code->relocs = i8x_list_unref (code->relocs);
}

static void
i8x_code_free (struct i8x_object *ob)
{
  struct i8x_code *code = (struct i8x_code *) ob;

  if (code->itable != NULL)
    free (code->itable);
}

const struct i8x_object_ops i8x_code_ops =
  {
    "code",			/* Object name.  */
    sizeof (struct i8x_code),	/* Object size.  */
    i8x_code_unlink,		/* Unlink function.  */
    i8x_code_free,		/* Free function.  */
  };

i8x_err_e
i8x_code_new (struct i8x_func *func, struct i8x_code **code)
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

/* Convert an instruction pointer to a note source offset.  */

size_t
ip_to_so (struct i8x_code *code, struct i8x_instr *ip)
{
  if (ip == NULL)
    return 0;

  struct i8x_note *note = i8x_code_get_note (code);
  const char *mem_base = i8x_note_get_encoded (note);
  ssize_t src_base = i8x_note_get_src_offset (note);

  if (src_base < 0)
    src_base = 0;

  return ip_to_bcp (code, ip) - mem_base + src_base;
}

void
i8x_code_dump_itable (struct i8x_code *code, const char *where)
{
  struct i8x_ctx *ctx = i8x_code_get_ctx (code);
  struct i8x_instr *op;

  if (i8x_ctx_get_log_priority (ctx) < LOG_INFO)
    return;

  info (ctx, "%s:\n", where);
  i8x_code_foreach_op (code, op)
    {
      char arg1[32] = "";  /* Operand 1.  */
      char arg2[32] = "";  /* Operand 2.  */
      char fnext[32] = ""; /* Fall through next.  */
      char bnext[32] = ""; /* Branch next.  */
      const char *fname;   /* Function name.  */
      char insn[128];

      if (op->code == IT_EMPTY_SLOT)
	continue;

      if (op->desc->arg1 != I8X_OPR_NONE)
	snprintf (arg1, sizeof (arg1), " " LDEC "", op->arg1.u);

      if (op->desc->arg2 != I8X_OPR_NONE)
	snprintf (arg2, sizeof (arg2), ", " LDEC "", op->arg2.u);

      snprintf (insn, sizeof (insn),
		"%s%s%s", op->desc->name, arg1, arg2);

      if (op->code != I8X_OP_return)
	snprintf (fnext, sizeof (fnext), "=> " LHEX "",
		  ip_to_so (code, op->fall_through));

      if (op->code == DW_OP_bra)
	snprintf (bnext, sizeof (bnext), ", " LHEX "",
		  ip_to_so (code, op->branch_next));

      if (op->ext1 != NULL)
	{
	  fname = i8x_funcref_get_fullname (op->ext1);
	  strncpy (bnext, " / ", sizeof (bnext));
	}
      else
	fname = "";

      info (ctx, "  " LHEX ": %-24s %s%s%s\n",
	    ip_to_so (code, op), insn, fnext, bnext, fname);
    }
  info (ctx, "\n");
}
