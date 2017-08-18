/* Copyright (C) 2016-17 Red Hat, Inc.
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
   compiled out of the standard interpreter.

   A principle of the interpreter is that any decision that can be
   made during preprocessing should be made during preprocessing,
   even if this means having 14 different versions of I8_OP_deref_int
   or wasting a bunch of memory somewhere.  */

#include <byteswap.h>
#include <string.h>
#include "libi8x-private.h"
#include "interp-private.h"
#include "funcref-private.h"
#include "inferior-private.h"
#include "reloc-private.h"
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
# undef trace
# define i8x_assert(expr)
# define dbg(ctx, arg...)
# define trace(ctx, arg...)
# define i8x_xctx_trace(...)
#endif

#if __WORDSIZE >= 64
# define IF_64BIT(expr) expr
#else
# define IF_64BIT(expr)
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

/* Macros to store and restore vsp and csp across native calls.  The
   non-debug version of RESTORE_VSP_CSP should allow the compiler to
   avoid maintaining its own copies of vsp and csp as they're dead
   across the call itself.  */

#define STORE_VSP_CSP()						\
  do {								\
    xctx->vsp = vsp;						\
    xctx->csp = csp;						\
  } while (0)

#ifdef DEBUG_INTERPRETER
# define RESTORE_VSP_CSP()					\
  do {								\
    i8x_assert (xctx->vsp == vsp);				\
    i8x_assert (xctx->csp == csp);				\
  } while (0)
#else
# define RESTORE_VSP_CSP()					\
  do {								\
    vsp = xctx->vsp;						\
    csp = xctx->csp;						\
  } while (0)
#endif

/* Native calls.  */

#define ENTER_NATIVE()						\
  int saved_wordsize = xctx->wordsize;				\
  i8x_byte_order_e saved_byte_order = xctx->byte_order;		\
								\
  xctx->wordsize = code->wordsize;				\
  xctx->byte_order = code->byte_order;				\
								\
  STORE_VSP_CSP()

#define LEAVE_NATIVE()						\
  RESTORE_VSP_CSP();						\
								\
  xctx->wordsize = saved_wordsize;				\
  xctx->byte_order = saved_byte_order

static i8x_err_e
call_native (struct i8x_xctx *xctx, struct i8x_funcref *ref,
	     struct i8x_inf *inf, union i8x_value *args,
	     union i8x_value *rets)
{
  trace (i8x_xctx_get_ctx (xctx), "%s: native call\n", ref->signature);

  i8x_assert (ref->resolved != NULL);
  i8x_assert (ref->native_impl != NULL);

  i8x_err_e err = ref->native_impl (xctx, inf, ref->resolved, args, rets);

  trace (i8x_xctx_get_ctx (xctx), "%s: native return\n", ref->signature);

  return err;
}

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

#define DTABLE_ADD_OPS(dtable)			\
  do {						\
    DTABLE_ADD (DW_OP_addr);			\
    DTABLE_ADD (DW_OP_dup);			\
    DTABLE_ADD (DW_OP_drop);			\
    DTABLE_ADD (DW_OP_over);			\
    DTABLE_ADD (DW_OP_pick);			\
    DTABLE_ADD (DW_OP_swap);			\
    DTABLE_ADD (DW_OP_rot);			\
    DTABLE_ADD (DW_OP_abs);			\
    DTABLE_ADD (DW_OP_and);			\
    DTABLE_ADD (DW_OP_div);			\
    DTABLE_ADD (DW_OP_minus);			\
    DTABLE_ADD (DW_OP_mod);			\
    DTABLE_ADD (DW_OP_mul);			\
    DTABLE_ADD (DW_OP_neg);			\
    DTABLE_ADD (DW_OP_not);			\
    DTABLE_ADD (DW_OP_or);			\
    DTABLE_ADD (DW_OP_plus);			\
    DTABLE_ADD (DW_OP_plus_uconst);		\
    DTABLE_ADD (DW_OP_shl);			\
    DTABLE_ADD (DW_OP_shr);			\
    DTABLE_ADD (DW_OP_shra);			\
    DTABLE_ADD (DW_OP_xor);			\
    DTABLE_ADD (DW_OP_bra);			\
    DTABLE_ADD (DW_OP_eq);			\
    DTABLE_ADD (DW_OP_ge);			\
    DTABLE_ADD (DW_OP_gt);			\
    DTABLE_ADD (DW_OP_le);			\
    DTABLE_ADD (DW_OP_lt);			\
    DTABLE_ADD (DW_OP_ne);			\
    DTABLE_ADD (DW_OP_lit0);			\
    DTABLE_ADD (DW_OP_lit1);			\
    DTABLE_ADD (DW_OP_lit2);			\
    DTABLE_ADD (DW_OP_lit3);			\
    DTABLE_ADD (DW_OP_lit4);			\
    DTABLE_ADD (DW_OP_lit5);			\
    DTABLE_ADD (DW_OP_lit6);			\
    DTABLE_ADD (DW_OP_lit7);			\
    DTABLE_ADD (DW_OP_lit8);			\
    DTABLE_ADD (DW_OP_lit9);			\
    DTABLE_ADD (DW_OP_lit10);			\
    DTABLE_ADD (DW_OP_lit11);			\
    DTABLE_ADD (DW_OP_lit12);			\
    DTABLE_ADD (DW_OP_lit13);			\
    DTABLE_ADD (DW_OP_lit14);			\
    DTABLE_ADD (DW_OP_lit15);			\
    DTABLE_ADD (DW_OP_lit16);			\
    DTABLE_ADD (DW_OP_lit17);			\
    DTABLE_ADD (DW_OP_lit18);			\
    DTABLE_ADD (DW_OP_lit19);			\
    DTABLE_ADD (DW_OP_lit20);			\
    DTABLE_ADD (DW_OP_lit21);			\
    DTABLE_ADD (DW_OP_lit22);			\
    DTABLE_ADD (DW_OP_lit23);			\
    DTABLE_ADD (DW_OP_lit24);			\
    DTABLE_ADD (DW_OP_lit25);			\
    DTABLE_ADD (DW_OP_lit26);			\
    DTABLE_ADD (DW_OP_lit27);			\
    DTABLE_ADD (DW_OP_lit28);			\
    DTABLE_ADD (DW_OP_lit29);			\
    DTABLE_ADD (DW_OP_lit30);			\
    DTABLE_ADD (DW_OP_lit31);			\
    DTABLE_ADD (I8_OP_call);			\
    DTABLE_ADD (I8_OP_load_external);		\
    DTABLE_ADD (I8_OP_warn);			\
    DTABLE_ADD (I8X_OP_return);			\
    DTABLE_ADD (I8X_OP_const);			\
    DTABLE_ADD (I8X_OP_deref_u8);		\
    DTABLE_ADD (I8X_OP_deref_i8);		\
    DTABLE_ADD (I8X_OP_deref_u16n);		\
    DTABLE_ADD (I8X_OP_deref_u16r);		\
    DTABLE_ADD (I8X_OP_deref_i16n);		\
    DTABLE_ADD (I8X_OP_deref_i16r);		\
    DTABLE_ADD (I8X_OP_deref_u32n);		\
    DTABLE_ADD (I8X_OP_deref_u32r);		\
    DTABLE_ADD (I8X_OP_deref_i32n);		\
    DTABLE_ADD (I8X_OP_deref_i32r);		\
    IF_64BIT (DTABLE_ADD (I8X_OP_deref_u64n));	\
    IF_64BIT (DTABLE_ADD (I8X_OP_deref_u64r));	\
    IF_64BIT (DTABLE_ADD (I8X_OP_deref_i64n));	\
    IF_64BIT (DTABLE_ADD (I8X_OP_deref_i64r));	\
  } while (0)

/* Call into the interpreter with the magic sequence to make
   it emit its dispatch table.  */

#ifdef DEBUG_INTERPRETER
i8x_err_e
i8x_ctx_init_dispatch_table (struct i8x_ctx *ctx, void **table,
			     size_t table_size, bool is_debug)
{
  struct i8x_xctx xctx;

  dbg (ctx, "populating dispatch_%s\n",
       is_debug == false ? "std" : "dbg");

  memset (&xctx, 0, sizeof (xctx));

  xctx.use_debug_interpreter = is_debug;
  xctx.dispatch_table_to_init = table;
  xctx.dispatch_table_size = table_size;

  return i8x_xctx_call (&xctx, NULL, NULL, NULL, NULL);
}
#endif /* DEBUG_INTERPRETER */

/* Callbacks are user code and have no access to i8x_ctx_set_error,
   so we decorate whatever they return with a note and location.  */

#define CALLBACK_ERROR_CHECK()			\
  do {						\
    if (__i8x_unlikely (err != I8X_OK))		\
      {						\
	err = i8x_code_error (code, err, op);	\
	goto unwind_and_return;			\
      }						\
  } while (0)

/* The interpreter itself, aka

I8X_EXPORT i8x_err_e
i8x_xctx_call (struct i8x_xctx *xctx, struct i8x_funcref *ref,
	       struct i8x_inf *inf, union i8x_value *args,
	       union i8x_value *rets)  */

NOT_DEBUG(I8X_EXPORT) i8x_err_e
INTERPRETER (struct i8x_xctx *xctx, struct i8x_funcref *ref,
	     struct i8x_inf *inf, union i8x_value *args,
	     union i8x_value *rets)
{
  /* Switch to the debug interpreter if requested.  */
#ifndef DEBUG_INTERPRETER
  if (__i8x_unlikely (xctx->use_debug_interpreter))
    return i8x_xctx_call_dbg (xctx, ref, inf, args, rets);
#endif

  /* Emit our dispatch table if requested.  */
  if (__i8x_unlikely (xctx->dispatch_table_to_init != NULL))
    {
      void **dtable = xctx->dispatch_table_to_init;
      size_t dtable_size = xctx->dispatch_table_size;

      for (size_t i = 0; i < dtable_size; i++)
	dtable[i] = &&unhandled_operation;

      DTABLE_ADD_OPS ();

      return I8X_OK;
    }

  /* If this is a native function then execute it.  */
  if (ref->native_impl != NULL)
    return call_native (xctx, ref, inf, args, rets);

  /* Get the bytecode.  */
  struct i8x_code *code;

  SETUP_BYTECODE_UNCHECKED (ref);
  if (__i8x_unlikely (code == NULL))
    return i8x_unresolved_function (i8x_xctx_get_ctx (xctx));

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

  OPERATION (DW_OP_addr):
    {
      struct i8x_reloc *reloc = op->addr1;

      /* See comments in reloc-private.h about the limitations
	 of this cache.  */
      if (__i8x_unlikely (reloc->cached_from != inf))
	{
	  uintptr_t value DEBUG_ONLY(= I8X_POISON_BAD_RELOCATE_FN);

	  err = inf->relocate_fn (inf, reloc, &value);
	  CALLBACK_ERROR_CHECK ();

	  reloc->cached_value = value;
	  reloc->cached_from = inf;
	}

      ADJUST_STACK (1);
      STACK(0).u = reloc->cached_value;
      CONTINUE;
    }

#define OPERATION_DW_OP_pick(name, slot)	\
  OPERATION (DW_OP_ ## name):			\
    ENSURE_DEPTH ((slot) + 1);			\
    ADJUST_STACK (1);				\
    STACK(0) = STACK((slot) + 1);		\
    CONTINUE

  OPERATION_DW_OP_pick(dup, 0);
  OPERATION_DW_OP_pick(over, 1);
  OPERATION_DW_OP_pick(pick, op->arg1.u);

#undef OPERATION_DW_OP_pick

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

  OPERATION (DW_OP_abs):
    ENSURE_DEPTH (1);
    STACK(0).i = abs (STACK(0).i);
    CONTINUE;

#define OPERATION_DW_binary_op(name, operator, type)	\
  OPERATION (DW_OP_ ## name):				\
    ENSURE_DEPTH (2);					\
    STACK(1).type = STACK(1).type operator STACK(0).u;	\
    ADJUST_STACK (-1);					\
    CONTINUE

  OPERATION_DW_binary_op (and,   &, u);
  OPERATION_DW_binary_op (minus, -, u);
  OPERATION_DW_binary_op (mul,   *, u);
  OPERATION_DW_binary_op (or,    |, u);
  OPERATION_DW_binary_op (plus,  +, u);
  OPERATION_DW_binary_op (shl,  <<, u);
  OPERATION_DW_binary_op (shr,  >>, u);
  OPERATION_DW_binary_op (shra, >>, i);
  OPERATION_DW_binary_op (xor,   ^, u);

#undef OPERATION_DW_binary_op

#define OPERATION_DW_divmod(name, operator, type)		\
  OPERATION (DW_OP_ ## name):					\
    ENSURE_DEPTH (2);						\
    if (__i8x_unlikely (STACK(0).type == 0))			\
      {								\
	err = i8x_code_error (code, I8X_DIVIDE_BY_ZERO, op);	\
	goto unwind_and_return;					\
      }								\
    STACK(1).type = STACK(1).type operator STACK(0).type;	\
    ADJUST_STACK (-1);						\
    CONTINUE

  OPERATION_DW_divmod(div, /, i);
  OPERATION_DW_divmod(mod, %, u);

#undef OPERATION_DW_divmod

  OPERATION (DW_OP_neg):
    ENSURE_DEPTH (1);
    STACK(0).i = -STACK(0).i;
    CONTINUE;

  OPERATION (DW_OP_not):
    ENSURE_DEPTH (1);
    STACK(0).u = ~STACK(0).u;
    CONTINUE;

  OPERATION (DW_OP_plus_uconst):
    ENSURE_DEPTH (1);
    STACK(0).u += op->arg1.u;
    CONTINUE;

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
      STACK(1).i = 1;				\
    else					\
      STACK(1).i = 0;				\
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

#undef OPERATION_DW_OP_lit

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

      size_t num_args = callee->num_args;
      size_t num_rets = callee->num_rets;

      union i8x_value *arg0 = vsp - num_args;

      /* Allocate return values array on the stack.  The spec
	 doesn't account for this in the function's declared
	 max_stack, but the validator will have increased it
	 for us if necessary.  */
      union i8x_value *ret0 = vsp;
      ADJUST_STACK (callee->num_rets);

      ENTER_NATIVE ();
      err = call_native (xctx, callee, inf, arg0, ret0);
      LEAVE_NATIVE ();
      CALLBACK_ERROR_CHECK ();

      if (__i8x_likely (num_args != 0))
	{
	  memmove (arg0, ret0, num_rets * sizeof (union i8x_value));
	  ADJUST_STACK (-num_args);
	}
      CONTINUE;
    }

  OPERATION (I8_OP_load_external):
    ADJUST_STACK (1);
    STACK(0).f = op->ext1;
    CONTINUE;

  OPERATION (I8_OP_warn):
    warn (i8x_xctx_get_ctx (xctx), "%s: %s\n",
	  i8x_funcref_get_signature (ref), (const char *) op->arg1.p);
    CONTINUE;

  OPERATION (I8X_OP_return):
    RETURN_FROM_CALL ();
    SETUP_BYTECODE (ref);
    CONTINUE;

  OPERATION (I8X_OP_const):
    ADJUST_STACK (1);
    STACK(0).u = op->arg1.u;
    CONTINUE;

#define OPERATION_I8X_OP_deref_2(name, type, process, result)		\
  OPERATION (I8X_OP_deref_ ## name):					\
    {									\
      type tmp;								\
									\
      ENSURE_DEPTH (1);							\
      err = inf->read_mem_fn (inf, STACK(0).u, sizeof (tmp), &tmp);	\
      CALLBACK_ERROR_CHECK ();						\
      tmp = process (tmp);						\
      STACK(0).result = tmp;						\
      CONTINUE;								\
    }

#define OPERATION_I8X_OP_deref_1(name, SIZE, process)			\
  OPERATION_I8X_OP_deref_2 (u ## name, uint ## SIZE ## _t, process, u)	\
  OPERATION_I8X_OP_deref_2 (i ## name, int ## SIZE ## _t, process, i)

#define OPERATION_I8X_OP_deref_8()					\
  OPERATION_I8X_OP_deref_1(8, 8, NOSWAP)

#define OPERATION_I8X_OP_deref(SIZE)					\
  OPERATION_I8X_OP_deref_1(SIZE ## n, SIZE, NOSWAP);			\
  OPERATION_I8X_OP_deref_1(SIZE ## r, SIZE, bswap_ ## SIZE)

#define NOSWAP

  OPERATION_I8X_OP_deref_8 ();
  OPERATION_I8X_OP_deref (16);
  OPERATION_I8X_OP_deref (32);
  IF_64BIT (OPERATION_I8X_OP_deref (64));

#undef OPERATION_I8X_OP_deref_2
#undef OPERATION_I8X_OP_deref_1
#undef OPERATION_I8X_OP_deref_8
#undef OPERATION_I8X_OP_deref
#undef NOSWAP

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
