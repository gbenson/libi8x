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
#include "interp-private.h"

#define NOTE_NOT_VALID()						\
  do {									\
    return i8x_code_error (code, I8X_NOTE_INVALID, op);			\
  } while (0)

#define ALLOCATE_STACK(result)						\
  do {									\
    result = calloc (code->max_stack, sizeof (struct i8x_type *));	\
    if (result == NULL)							\
      return i8x_out_of_memory (i8x_code_get_ctx (code));		\
  } while (0)

#define COPY_STACK(dst, src)						\
  do {									\
    size_t stack_depth = STACK_DEPTH ();				\
    if (stack_depth != 0)						\
      memcpy (dst, src, stack_depth * sizeof (struct i8x_type *));	\
  } while (0)

/* Most operations ENSURE_DEPTH or ADJUST_STACK or both.  Either
   will fail if code->max_stack is zero because stack, stack_ptr
   and stack_limit will all be NULL in that case.  */

#define STACK_DEPTH() ((size_t) (stack_ptr - stack))

#define ENSURE_DEPTH(nslots)						\
  do {									\
    if (STACK_DEPTH () < nslots)					\
      NOTE_NOT_VALID ();						\
  } while (0)

#define ADJUST_STACK(nslots)						\
  do {									\
    stack_ptr += (nslots);						\
									\
    if (stack_ptr < stack || stack_ptr > stack_limit)			\
      NOTE_NOT_VALID ();						\
  } while (0)

#define STACK(slot) stack_ptr[- 1 - (slot)]

#define TYPES_MATCH(a, b)					\
  (((a) == (b))							\
   || ((a) == int_or_ptr && ((b) == inttype || (b) == ptrtype))	\
   || ((b) == int_or_ptr && ((a) == inttype || (a) == ptrtype)))

#define ENSURE_TYPE(slot, type)						\
  do {									\
    struct i8x_type *et_tmp1 = STACK(slot);				\
    struct i8x_type *et_tmp2 = (type);					\
									\
    if (!TYPES_MATCH (et_tmp1, et_tmp2))				\
      {									\
	notice (ctx, "stack[%d]: %s != %s\n", (slot),			\
		i8x_type_get_encoded (et_tmp1),				\
		i8x_type_get_encoded (et_tmp2));			\
	NOTE_NOT_VALID ();						\
      }									\
  } while (0)

#define SLOT_TO_STR(slot)			\
  ((STACK_DEPTH () > (slot))			\
   ? i8x_type_get_encoded (STACK(slot))		\
   : "-")

static i8x_err_e
i8x_code_validate_1 (struct i8x_code *code, struct i8x_funcref *ref,
		     struct i8x_instr *op, struct i8x_type **stack,
		     struct i8x_type **stack_limit,
		     struct i8x_type **stack_ptr)
{
  struct i8x_ctx *ctx = i8x_code_get_ctx (code);
  struct i8x_type *inttype = i8x_ctx_get_integer_type (ctx);
  struct i8x_type *ptrtype = i8x_ctx_get_pointer_type (ctx);
  struct i8x_type *int_or_ptr = i8x_ctx_get_int_or_ptr_type (ctx);
  const char *trace_prefix = NULL;
  struct i8x_type **saved_sp, *tmp;
  struct i8x_list *types;
  struct i8x_listitem *li;
  i8x_err_e err;

  if (i8x_ctx_get_log_priority (ctx) > LOG_DEBUG)
    trace_prefix = i8x_funcref_get_fullname (ref);

  while (true)
    {
      if (trace_prefix != NULL)
	{
	  trace (ctx, "%s\t0x%lx\t%-20s [%ld]\t%-16s%-16s\n",
		 trace_prefix, ip_to_so (code, op), op->desc->name,
		 STACK_DEPTH (), SLOT_TO_STR (0), SLOT_TO_STR (1));
	}

      if (op->code == I8X_OP_return)
	{
	  /* Function is returning.  */

	  int slot = 0;

	  ENSURE_DEPTH (i8x_funcref_get_num_returns (ref));

	  types = i8x_funcref_get_rtypes (ref);
	  i8x_list_foreach (types, li)
	    {
	      ENSURE_TYPE (slot, i8x_listitem_get_type (li));
	      slot++;
	    }

	  op->is_visited = true;

	  if (trace_prefix != NULL)
	    dbg (ctx, "\n");

	  return I8X_OK;
	}

      if (!op->is_visited)
	{
	  /* Copy the stack we arrived at this instruction with.  */
	  if (stack != NULL)
	    {
	      ALLOCATE_STACK (op->entry_stack);
	      COPY_STACK (op->entry_stack, stack);
	    }

	  op->is_visited = true;
	}
      else
	{
	  /* Flows merge: check the stacks match.  */
	  if (stack != NULL)
	    {
	      size_t total_slots = stack_limit - stack;
	      size_t dead_slots = stack_limit - stack_ptr;

	      if (dead_slots != 0)
		memset (stack_ptr, 0,
			dead_slots * sizeof (struct i8x_type *));

	      for (size_t i = 0; i < total_slots; i++)
		{
		  if (stack[i] == NULL)
		    {
		      if (op->entry_stack[i] != NULL)
			NOTE_NOT_VALID ();

		      break;
		    }

		  if (op->entry_stack[i] == NULL)
		    NOTE_NOT_VALID ();

		  if (!TYPES_MATCH (stack[i], op->entry_stack[i]))
		    NOTE_NOT_VALID ();
		}
	    }

	  return I8X_OK;
	}

      switch (op->code)
	{
	case IT_EMPTY_SLOT:
	  NOTE_NOT_VALID ();

	case DW_OP_addr:
	  ADJUST_STACK (1);
	  STACK(0) = ptrtype;
	  break;

	case DW_OP_deref:
	  ENSURE_DEPTH (1);
	  ENSURE_TYPE (0, ptrtype);
	  STACK(0) = ptrtype;
	  break;

	case DW_OP_dup:
	  ENSURE_DEPTH (1);
	  ADJUST_STACK (1);
	  STACK(0) = STACK(1);
	  break;

	case DW_OP_drop:
	  ENSURE_DEPTH (1);
	  ADJUST_STACK (-1);
	  break;

	case DW_OP_pick:
	  ENSURE_DEPTH (op->arg1.u + 1);
	  ADJUST_STACK (1);
	  STACK(0) = STACK(op->arg1.u + 1);
	  break;

	case DW_OP_swap:
	  ENSURE_DEPTH (2);
	  tmp = STACK(0);
	  STACK(0) = STACK(1);
	  STACK(1) = tmp;
	  break;

	case DW_OP_rot:
	  ENSURE_DEPTH (3);
	  tmp = STACK(0);
	  STACK(0) = STACK(1);
	  STACK(1) = STACK(2);
	  STACK(2) = tmp;
	  break;

	case DW_OP_abs:
	case DW_OP_neg:
	case DW_OP_not:
	  ENSURE_DEPTH (1);
	  ENSURE_TYPE (0, inttype);
	  break;

	case DW_OP_and:
	case DW_OP_div:
	case DW_OP_mod:
	case DW_OP_mul:
	case DW_OP_or:
	case DW_OP_shl:
	case DW_OP_shr:
	case DW_OP_shra:
	case DW_OP_xor:
	  ENSURE_DEPTH (2);
	  ENSURE_TYPE (0, inttype);
	  ENSURE_TYPE (1, inttype);
	  ADJUST_STACK (-1);
	  STACK(0) = inttype;
	  break;

	case DW_OP_minus:
	  ENSURE_DEPTH (2);
	  ENSURE_TYPE (0, inttype);
	  ENSURE_TYPE (1, int_or_ptr);
	  ADJUST_STACK (-1);
	  if (STACK(0) != ptrtype)
	    STACK(0) = inttype;
	  break;

	case DW_OP_plus_uconst:
	  ENSURE_DEPTH (1);
	  ENSURE_TYPE (0, int_or_ptr);
	  break;

	case DW_OP_bra:
	  ENSURE_DEPTH (1);
	  ENSURE_TYPE (0, int_or_ptr);
	  ADJUST_STACK (-1);
	  saved_sp = stack_ptr;
	  err = i8x_code_validate_1 (code, ref, op->branch_next,
				     stack, stack_limit, stack_ptr);
	  if (err != I8X_OK)
	    return err;
	  stack_ptr = saved_sp;
	  COPY_STACK (stack, op->entry_stack);
	  break;

	case DW_OP_eq:
	case DW_OP_ge:
	case DW_OP_gt:
	case DW_OP_le:
	case DW_OP_lt:
	case DW_OP_ne:
	  ENSURE_DEPTH (2);
	  ENSURE_TYPE (0, int_or_ptr);
	  ENSURE_TYPE (1, STACK(0));
	  ADJUST_STACK (-1);
	  STACK(0) = inttype;
	  break;

	case DW_OP_lit0:
	  ADJUST_STACK (1);
	  STACK(0) = int_or_ptr;
	  break;

	case DW_OP_lit1:
	case DW_OP_lit2:
	case DW_OP_lit3:
	case DW_OP_lit4:
	case DW_OP_lit5:
	case DW_OP_lit6:
	case DW_OP_lit7:
	case DW_OP_lit8:
	case DW_OP_lit9:
	case DW_OP_lit10:
	case DW_OP_lit11:
	case DW_OP_lit12:
	case DW_OP_lit13:
	case DW_OP_lit14:
	case DW_OP_lit15:
	case DW_OP_lit16:
	case DW_OP_lit17:
	case DW_OP_lit18:
	case DW_OP_lit19:
	case DW_OP_lit20:
	case DW_OP_lit21:
	case DW_OP_lit22:
	case DW_OP_lit23:
	case DW_OP_lit24:
	case DW_OP_lit25:
	case DW_OP_lit26:
	case DW_OP_lit27:
	case DW_OP_lit28:
	case DW_OP_lit29:
	case DW_OP_lit30:
	case DW_OP_lit31:
	  ADJUST_STACK (1);
	  STACK(0) = inttype;
	  break;

	case I8_OP_call:
	  ENSURE_DEPTH (1);
	  tmp = STACK(0);
	  if (!i8x_type_is_functype (tmp))
	    NOTE_NOT_VALID ();
	  ADJUST_STACK (-1);

	  types = i8x_type_get_ptypes (tmp);
	  i8x_list_foreach_reversed (types, li)
	    {
	      ENSURE_DEPTH (1);
	      ENSURE_TYPE (0, i8x_listitem_get_type (li));
	      ADJUST_STACK (-1);
	    }

	  types = i8x_type_get_rtypes (tmp);
	  i8x_list_foreach_reversed (types, li)
	    {
	      ADJUST_STACK (1);
	      STACK(0) = i8x_listitem_get_type (li);
	    }
	  break;

	case I8_OP_load_external:
	  ADJUST_STACK (1);
	  STACK(0) = i8x_funcref_get_type (op->ext1);
	  break;

	case I8_OP_deref_int:
	  ENSURE_DEPTH (1);
	  ENSURE_TYPE (0, ptrtype);
	  STACK(0) = inttype;
	  break;

	case I8X_OP_const:
	  ADJUST_STACK (1);
	  STACK(0) = inttype;
	  break;

	default:
	  notice (ctx, "%s not implemented in validator\n",
		  op->desc->name);
	  return i8x_code_error (code, I8X_NOTE_UNHANDLED, op);
	}

      op = op->fall_through;
    }
}

i8x_err_e
i8x_code_validate (struct i8x_code *code, struct i8x_funcref *ref)
{
  struct i8x_type **stack = NULL;
  struct i8x_type **stack_limit;
  struct i8x_type **stack_ptr;
  struct i8x_list *types;
  struct i8x_listitem *li;
  struct i8x_instr *op = code->entry_point;
  i8x_err_e err;

  if (code->max_stack != 0)
    ALLOCATE_STACK (stack);

  stack_ptr = stack;
  stack_limit = stack + code->max_stack;

  /* Push the arguments.  */
  types = i8x_funcref_get_ptypes (ref);
  i8x_list_foreach (types, li)
    {
      ADJUST_STACK (1);
      STACK(0) = i8x_listitem_get_type (li);
    }

  /* Walk the code.  */
  i8x_code_reset_is_visited (code);
  err = i8x_code_validate_1 (code, ref, op, stack, stack_limit, stack_ptr);

  /* Free everything we allocated.  */
  if (stack != NULL)
    free (stack);

  i8x_code_foreach_op (code, op)
    if (op->entry_stack != NULL)
      free (op->entry_stack);

  if (err != I8X_OK)
    return err;

  /* Remove any unreachable (unvalidated) code.  */
  i8x_code_foreach_op (code, op)
    if (!op->is_visited)
      op->code = IT_EMPTY_SLOT;

  i8x_code_dump_itable (code, __FUNCTION__);

  return I8X_OK;
}
