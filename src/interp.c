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

/* Note that this file is compiled twice: once directly, and once
   #include'd from dbg-interp.c with #define DEBUG_INTERPRETER.  The
   direct compile builds the standard interpreter i8x_xctx_call, and
   the indirect one builds the debug interpreter i8x_xctx_call_dbg.
   Both interpreters rely on the validator having checked the code
   but the debug interpreter has assertions and tracing code that are
   compiled out of the standard interpreter.  */

#include <string.h>
#include "libi8x-private.h"
#include "interp-private.h"
#include "funcref-private.h"
#include "inferior-private.h"
#include "symref-private.h"
#include "xctx-private.h"

#ifdef DEBUG_INTERPRETER
# define INTERPRETER i8x_xctx_call_dbg
# define DEBUG_ONLY(expr) expr
# define NOT_DEBUG(expr)
#else
# define INTERPRETER i8x_xctx_call
# define DEBUG_ONLY(expr)
# define NOT_DEBUG(expr) expr
# undef i8x_assert
# undef dbg
# define i8x_assert(expr)
# define dbg(ctx, arg...)
# define i8x_xctx_trace(...)
#endif

/* Value stack macros.  */

#define STACK_DEPTH() ((size_t) (vsp - vsp_floor))

#define ENSURE_DEPTH(nslots) \
  i8x_assert (STACK_DEPTH() >= nslots)

#define ADJUST_STACK(nslots)				\
  do {							\
    vsp += (nslots);					\
    i8x_assert (vsp >= vsp_floor);			\
    i8x_assert (vsp <= vsp_floor + code->max_stack);	\
  } while (0)

#define STACK(slot) vsp[-1 - (slot)]

/* Layout of call stack frames.  */

enum
  {
    CS_CALLER,		/* Caller funcref (NULL for entry frames).  */
    CS_CALLSITE,	/* Instruction in caller that caused this call.  */
    CS_VSPFLOOR,	/* Caller's vsp_floor.  */
    CS_FRAME_SIZE	/* Number of slots in a call stack frame.  */
  };

/* Call stack macros.  */

#define SETUP_BYTECODE_UNCHECKED(callee_ref)	\
  do {						\
    code = (callee_ref)->interp_impl;		\
  } while (0)

#define SETUP_BYTECODE(callee_ref)		\
  do {						\
    SETUP_BYTECODE_UNCHECKED (callee_ref);	\
    i8x_assert (code != NULL);			\
  } while (0)

#define SETUP_CALL(caller_ref, caller_op, callee_ref, _callee_vspfloor) \
  do {									\
    union i8x_value *callee_vspfloor = (_callee_vspfloor);		\
									\
    /* Make space for the call frame.  */				\
    csp -= CS_FRAME_SIZE;						\
									\
    /* Check we have enough stack.  */					\
    if (__i8x_unlikely (callee_vspfloor + code->max_stack > csp))	\
      {									\
	err = i8x_code_error (code, I8X_STACK_OVERFLOW,			\
			      code->entry_point);			\
	goto unwind_and_return;						\
      }									\
									\
    /* Fill in the call frame.  */					\
    csp[CS_CALLER].f = (caller_ref);					\
    csp[CS_CALLSITE].p = (caller_op);					\
    csp[CS_VSPFLOOR].p = vsp_floor;					\
									\
    /* Switch to the new function.  */					\
    ref = (callee_ref);							\
    vsp_floor = callee_vspfloor;					\
  } while (0)

#define RETURN_FROM_CALL()						\
  do {									\
    struct i8x_funcref *caller_ref;					\
									\
    /* Check enough stuff was returned.  */				\
    ENSURE_DEPTH (ref->num_rets);					\
									\
    /* If this an entry frame we can unwind immediately.  */		\
    i8x_assert (xctx->stack_limit - csp >= CS_FRAME_SIZE);		\
    caller_ref = csp[CS_CALLER].f;	\
    if (caller_ref == NULL)						\
      goto unwind_and_return_values;					\
									\
    /* Remove any extra stuff on the value stack.  */			\
    size_t want_slots = ref->num_rets;					\
    size_t have_slots = STACK_DEPTH ();					\
									\
    i8x_assert (have_slots >= want_slots);				\
    if (have_slots != want_slots)					\
      {									\
	memmove (vsp - have_slots, vsp - want_slots,			\
		 sizeof (union i8x_value) * want_slots);		\
									\
	ADJUST_STACK (want_slots - have_slots);				\
      }									\
									\
    /* Switch back to the caller.  */					\
    ref = caller_ref;							\
    op = (struct i8x_instr *) csp[CS_CALLSITE].p;			\
    vsp_floor = (union i8x_value *) csp[CS_VSPFLOOR].p;			\
									\
    /* Drop the call frame.  */						\
    csp += CS_FRAME_SIZE;						\
  } while (0)

/* Dispatch macros.  */

#ifdef DEBUG_INTERPRETER
# define IMPL_FIELD impl_dbg
#else
# define IMPL_FIELD impl_std
#endif

#define DISPATCH(next_op)			\
  do {						\
    op = (next_op);				\
    i8x_assert (op != NULL);			\
    i8x_assert (op->IMPL_FIELD != NULL);	\
    i8x_xctx_trace (xctx, ref, code, op,	\
		    vsp, vsp_floor, csp);	\
    goto *op->IMPL_FIELD;			\
  } while (0)

#define CONTINUE  DISPATCH (op->fall_through)

/* Operations table.  */

#define OPERATION(code) op_ ## code

#define DTABLE_ADD(code) \
  dtable[code] = &&op_ ## code

#define DTABLE_ADD_OPS(dtable)		\
  do {					\
    DTABLE_ADD (DW_OP_dup);		\
    DTABLE_ADD (DW_OP_drop);		\
    DTABLE_ADD (DW_OP_swap);		\
    DTABLE_ADD (DW_OP_rot);		\
    DTABLE_ADD (DW_OP_and);		\
    DTABLE_ADD (DW_OP_minus);		\
    DTABLE_ADD (DW_OP_mul);		\
    DTABLE_ADD (DW_OP_or);		\
    DTABLE_ADD (DW_OP_plus);		\
    DTABLE_ADD (DW_OP_shl);		\
    DTABLE_ADD (DW_OP_shr);		\
    DTABLE_ADD (DW_OP_xor);		\
    DTABLE_ADD (DW_OP_bra);		\
    DTABLE_ADD (DW_OP_eq);		\
    DTABLE_ADD (DW_OP_ge);		\
    DTABLE_ADD (DW_OP_gt);		\
    DTABLE_ADD (DW_OP_le);		\
    DTABLE_ADD (DW_OP_lt);		\
    DTABLE_ADD (DW_OP_ne);		\
    DTABLE_ADD (DW_OP_lit0);		\
    DTABLE_ADD (DW_OP_lit1);		\
    DTABLE_ADD (DW_OP_lit2);		\
    DTABLE_ADD (DW_OP_lit3);		\
    DTABLE_ADD (DW_OP_lit4);		\
    DTABLE_ADD (DW_OP_lit5);		\
    DTABLE_ADD (DW_OP_lit6);		\
    DTABLE_ADD (DW_OP_lit7);		\
    DTABLE_ADD (DW_OP_lit8);		\
    DTABLE_ADD (DW_OP_lit9);		\
    DTABLE_ADD (DW_OP_lit10);		\
    DTABLE_ADD (DW_OP_lit11);		\
    DTABLE_ADD (DW_OP_lit12);		\
    DTABLE_ADD (DW_OP_lit13);		\
    DTABLE_ADD (DW_OP_lit14);		\
    DTABLE_ADD (DW_OP_lit15);		\
    DTABLE_ADD (DW_OP_lit16);		\
    DTABLE_ADD (DW_OP_lit17);		\
    DTABLE_ADD (DW_OP_lit18);		\
    DTABLE_ADD (DW_OP_lit19);		\
    DTABLE_ADD (DW_OP_lit20);		\
    DTABLE_ADD (DW_OP_lit21);		\
    DTABLE_ADD (DW_OP_lit22);		\
    DTABLE_ADD (DW_OP_lit23);		\
    DTABLE_ADD (DW_OP_lit24);		\
    DTABLE_ADD (DW_OP_lit25);		\
    DTABLE_ADD (DW_OP_lit26);		\
    DTABLE_ADD (DW_OP_lit27);		\
    DTABLE_ADD (DW_OP_lit28);		\
    DTABLE_ADD (DW_OP_lit29);		\
    DTABLE_ADD (DW_OP_lit30);		\
    DTABLE_ADD (DW_OP_lit31);		\
    DTABLE_ADD (I8_OP_call);		\
    DTABLE_ADD (I8X_OP_return);		\
    DTABLE_ADD (I8X_OP_loadext_func);	\
    DTABLE_ADD (I8X_OP_loadext_sym);	\
  } while (0)

/* Call into the interpreter with the magic sequence to make
   it emit its dispatch table.  */

#ifdef DEBUG_INTERPRETER
i8x_err_e
i8x_ctx_init_dispatch_table (struct i8x_ctx *ctx, void **table,
			     size_t table_size, bool is_debug)
{
  struct i8x_funcref r;
  struct i8x_xctx x;

  dbg (ctx, "populating %s dispatch table\n",
       is_debug == false ? "standard" : "debug");

  memset (&r, 0, sizeof (r));
  memset (&x, 0, sizeof (x));

  x.use_debug_interpreter = is_debug;
  x.dispatch_table_to_init = table;
  x.dispatch_table_size = table_size;

  return i8x_xctx_call (&x, &r, NULL, NULL, NULL);
}
#endif /* DEBUG_INTERPRETER */

/* The interpreter itself, aka

I8X_EXPORT i8x_err_e
i8x_xctx_call (struct i8x_xctx *xctx, struct i8x_funcref *ref,
	       struct i8x_inferior *inf, union i8x_value *args,
	       union i8x_value *rets)  */

NOT_DEBUG(I8X_EXPORT) i8x_err_e
INTERPRETER (struct i8x_xctx *xctx, struct i8x_funcref *ref,
	     struct i8x_inferior *inf, union i8x_value *args,
	     union i8x_value *rets)
{
  /* If this function is native then we're in the wrong place.  */
  if (ref->native_impl != NULL)
    return ref->native_impl (xctx, inf, args, rets);

  /* Likewise if we should be in the debug interpreter but aren't.  */
#ifndef DEBUG_INTERPRETER
  if (__i8x_unlikely (xctx->use_debug_interpreter))
    return i8x_xctx_call_dbg (xctx, ref, inf, args, rets);
#endif

  /* Are we being asked to emit our dispatch table?  */
  if (__i8x_unlikely (xctx->dispatch_table_to_init != NULL))
    {
      void **dtable = xctx->dispatch_table_to_init;
      size_t dtable_size = xctx->dispatch_table_size;

      for (size_t i = 0; i < dtable_size; i++)
	dtable[i] = &&unhandled_operation;

      DTABLE_ADD_OPS ();

      return I8X_OK;
    }

  /* Get the bytecode.  */
  struct i8x_code *code;

  SETUP_BYTECODE_UNCHECKED (ref);
  if (__i8x_unlikely (code == NULL))
    return i8x_invalid_argument (i8x_xctx_get_ctx (xctx));

  /* Pull the stack pointers into local variables.  */
  union i8x_value *vsp = xctx->vsp;
  union i8x_value *csp = xctx->csp;

  i8x_assert (xctx->stack_base <= vsp);
  i8x_assert (vsp <= csp);
  i8x_assert (csp <= xctx->stack_limit);

  /* Keep our original stack pointers so we can reset them.  */
  union i8x_value *const saved_vsp = vsp;
  union i8x_value *const saved_csp = csp;

  /* Push an entry frame (with CS_CALLER = NULL) onto the call
     stack, then set vsp_floor for the function we're entering.  */
  union i8x_value *vsp_floor = NULL;
  i8x_err_e err = I8X_OK;

  SETUP_CALL (NULL, NULL, ref, vsp);

  /* Copy the arguments into the value stack.  */
  i8x_assert (code->max_stack >= ref->num_args);
  ADJUST_STACK (ref->num_args);
  memcpy (vsp_floor, args, sizeof (union i8x_value) * ref->num_args);

  /* Start executing.  */
  struct i8x_instr *op;
  DISPATCH (code->entry_point);

  OPERATION (DW_OP_dup):
    ENSURE_DEPTH (1);
    ADJUST_STACK (1);
    STACK(0) = STACK(1);
    CONTINUE;

  OPERATION (DW_OP_drop):
    ENSURE_DEPTH (1);
    ADJUST_STACK (-1);
    CONTINUE;

  OPERATION (DW_OP_swap):
    {
      union i8x_value tmp;

      ENSURE_DEPTH (2);
      tmp = STACK(0);
      STACK(0) = STACK(1);
      STACK(1) = tmp;
      CONTINUE;
    }

  OPERATION (DW_OP_rot):
    {
      union i8x_value tmp;

      ENSURE_DEPTH (3);
      tmp = STACK(0);
      STACK(0) = STACK(1);
      STACK(1) = STACK(2);
      STACK(2) = tmp;
      CONTINUE;
    }

#define OPERATION_DW_binary_op(name, operator)		\
  OPERATION (DW_OP_ ## name):				\
    ENSURE_DEPTH (2);					\
    STACK(1).u = STACK(1).u operator STACK(0).u;	\
    ADJUST_STACK (-1);					\
    CONTINUE

  OPERATION_DW_binary_op (and,   &);
  OPERATION_DW_binary_op (minus, -);
  OPERATION_DW_binary_op (mul,   *);
  OPERATION_DW_binary_op (or,    |);
  OPERATION_DW_binary_op (plus,  +);
  OPERATION_DW_binary_op (shl,  <<);
  OPERATION_DW_binary_op (shr,  >>);
  OPERATION_DW_binary_op (xor,   ^);

#undef OPERATION_DW_binary_op

  OPERATION (DW_OP_bra):
    ENSURE_DEPTH (1);
    ADJUST_STACK (-1);
    if (STACK(-1).i != 0)
      DISPATCH (op->branch_next);
    CONTINUE;

#define OPERATION_DW_cmp_op(name, operator)	\
  OPERATION (DW_OP_ ## name):			\
    ENSURE_DEPTH (2);				\
    if (STACK(1).i operator STACK(0).i)		\
      STACK(0).i = 1;				\
    else					\
      STACK(0).i = 0;				\
    ADJUST_STACK (-1);				\
    CONTINUE

  OPERATION_DW_cmp_op (eq, ==);
  OPERATION_DW_cmp_op (ge, >=);
  OPERATION_DW_cmp_op (gt, >);
  OPERATION_DW_cmp_op (le, <=);
  OPERATION_DW_cmp_op (lt, <);
  OPERATION_DW_cmp_op (ne, !=);

#undef OPERATION_DW_cmp_op

#define OPERATION_DW_OP_lit(n)	\
  OPERATION (DW_OP_lit ## n):	\
    ADJUST_STACK (1);		\
    STACK(0).i = n;		\
    CONTINUE

  OPERATION_DW_OP_lit (0);
  OPERATION_DW_OP_lit (1);
  OPERATION_DW_OP_lit (2);
  OPERATION_DW_OP_lit (3);
  OPERATION_DW_OP_lit (4);
  OPERATION_DW_OP_lit (5);
  OPERATION_DW_OP_lit (6);
  OPERATION_DW_OP_lit (7);
  OPERATION_DW_OP_lit (8);
  OPERATION_DW_OP_lit (9);
  OPERATION_DW_OP_lit (10);
  OPERATION_DW_OP_lit (11);
  OPERATION_DW_OP_lit (12);
  OPERATION_DW_OP_lit (13);
  OPERATION_DW_OP_lit (14);
  OPERATION_DW_OP_lit (15);
  OPERATION_DW_OP_lit (16);
  OPERATION_DW_OP_lit (17);
  OPERATION_DW_OP_lit (18);
  OPERATION_DW_OP_lit (19);
  OPERATION_DW_OP_lit (20);
  OPERATION_DW_OP_lit (21);
  OPERATION_DW_OP_lit (22);
  OPERATION_DW_OP_lit (23);
  OPERATION_DW_OP_lit (24);
  OPERATION_DW_OP_lit (25);
  OPERATION_DW_OP_lit (26);
  OPERATION_DW_OP_lit (27);
  OPERATION_DW_OP_lit (28);
  OPERATION_DW_OP_lit (29);
  OPERATION_DW_OP_lit (30);
  OPERATION_DW_OP_lit (31);

#undef DO_DW_OP_lit

  OPERATION (I8_OP_call):
    {
      struct i8x_funcref *callee;

      ENSURE_DEPTH (1);
      callee = STACK(0).f;
      ADJUST_STACK (-1);

      if (callee->native_impl == NULL)
	{
	  /* We don't recurse to call an interpreted function.
	     Instead, we push our current setup onto the call
	     stack, update ref, code and vsp_floor for the new
	     function, and continue, now in the callee.  */
	  SETUP_BYTECODE (callee);
	  SETUP_CALL (ref, op, callee, vsp - callee->num_args);
	  DISPATCH (code->entry_point);
	}

      i8x_not_implemented ();
      CONTINUE;
    }

  OPERATION (I8X_OP_return):
    RETURN_FROM_CALL ();
    SETUP_BYTECODE (ref);
    CONTINUE;

  OPERATION (I8X_OP_loadext_func):
    ADJUST_STACK (1);
    STACK(0).f = (struct i8x_funcref *) op->ext1;
    CONTINUE;

  OPERATION (I8X_OP_loadext_sym):
    {
      struct i8x_symref *sym = (struct i8x_symref *) op->ext1;

      /* See comments in symref-private.h about the limitations of
	 this cache.  */
      if (__i8x_unlikely (sym->cached_from != inf))
	{
	  struct i8x_func *func = (struct i8x_func *) code->_ob.parent;
	  uintptr_t value = I8X_POISON_BAD_SYM_RESOLVER;

	  err = inf->resolve_sym_fn (xctx, inf, func, sym->name, &value);

	  if (__i8x_unlikely (err != I8X_OK))
	    {
	      /* The resolver is user code and has no access to
		 i8x_ctx_set_error, so we augment whatever they
		 returned with a note and location.  */
	      err = i8x_code_error (code, err, op);
	      goto unwind_and_return;
	    }

	  sym->cached_value = value;
	  sym->cached_from = inf;
	}

      ADJUST_STACK (1);
      STACK(0).u = sym->cached_value;
      CONTINUE;
    }

 unhandled_operation:
  i8x_internal_error (__FILE__, __LINE__, __FUNCTION__,
		      _("%s: Not implemented."),
		      op->desc->name);

 unwind_and_return_values:
  memcpy (rets, vsp - ref->num_rets,
	  sizeof (union i8x_value) * ref->num_rets);

 unwind_and_return:
  xctx->vsp = saved_vsp;
  xctx->csp = saved_csp;

  return err;
}
