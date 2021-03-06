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
#include "funcref-private.h"
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

#ifdef DEBUG_INTERPRETER
# define STORE_VSP_LIMITS()					\
  union i8x_value *vsp_floor = saved_vsp;			\
  union i8x_value *vsp_limit = saved_vsp + code->max_stack
#else
# define STORE_VSP_LIMITS()
#endif

#define STACK_DEPTH() (vsp - vsp_floor)

#define ENSURE_DEPTH(nslots) \
  i8x_assert (STACK_DEPTH() >= nslots)

#define ADJUST_STACK(nslots)		\
  do {					\
    vsp += (nslots);			\
    i8x_assert (vsp >= vsp_floor);	\
    i8x_assert (vsp <= vsp_limit);	\
  } while (0)

#define STACK(slot) vsp[-1 - (slot)]

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
		    vsp, vsp_floor, vsp_limit,	\
		    csp);			\
    goto *op->IMPL_FIELD;			\
  } while (0)

#define CONTINUE  DISPATCH (op->fall_through)

/* Operations table.  */

#define OPERATION(code) op_ ## code

#define DTABLE_ADD(code) \
  dtable[code] = &&op_ ## code

#define DTABLE_ADD_OPS(dtable)	\
  do {				\
    DTABLE_ADD (DW_OP_dup);	\
    DTABLE_ADD (DW_OP_drop);	\
    DTABLE_ADD (DW_OP_swap);	\
    DTABLE_ADD (DW_OP_rot);	\
    DTABLE_ADD (DW_OP_and);	\
    DTABLE_ADD (DW_OP_minus);	\
    DTABLE_ADD (DW_OP_mul);	\
    DTABLE_ADD (DW_OP_or);	\
    DTABLE_ADD (DW_OP_plus);	\
    DTABLE_ADD (DW_OP_shl);	\
    DTABLE_ADD (DW_OP_shr);	\
    DTABLE_ADD (DW_OP_xor);	\
    DTABLE_ADD (DW_OP_bra);	\
    DTABLE_ADD (DW_OP_eq);	\
    DTABLE_ADD (DW_OP_ge);	\
    DTABLE_ADD (DW_OP_gt);	\
    DTABLE_ADD (DW_OP_le);	\
    DTABLE_ADD (DW_OP_lt);	\
    DTABLE_ADD (DW_OP_ne);	\
    DTABLE_ADD (DW_OP_lit0);	\
    DTABLE_ADD (DW_OP_lit1);	\
    DTABLE_ADD (DW_OP_lit2);	\
    DTABLE_ADD (DW_OP_lit3);	\
    DTABLE_ADD (DW_OP_lit4);	\
    DTABLE_ADD (DW_OP_lit5);	\
    DTABLE_ADD (DW_OP_lit6);	\
    DTABLE_ADD (DW_OP_lit7);	\
    DTABLE_ADD (DW_OP_lit8);	\
    DTABLE_ADD (DW_OP_lit9);	\
    DTABLE_ADD (DW_OP_lit10);	\
    DTABLE_ADD (DW_OP_lit11);	\
    DTABLE_ADD (DW_OP_lit12);	\
    DTABLE_ADD (DW_OP_lit13);	\
    DTABLE_ADD (DW_OP_lit14);	\
    DTABLE_ADD (DW_OP_lit15);	\
    DTABLE_ADD (DW_OP_lit16);	\
    DTABLE_ADD (DW_OP_lit17);	\
    DTABLE_ADD (DW_OP_lit18);	\
    DTABLE_ADD (DW_OP_lit19);	\
    DTABLE_ADD (DW_OP_lit20);	\
    DTABLE_ADD (DW_OP_lit21);	\
    DTABLE_ADD (DW_OP_lit22);	\
    DTABLE_ADD (DW_OP_lit23);	\
    DTABLE_ADD (DW_OP_lit24);	\
    DTABLE_ADD (DW_OP_lit25);	\
    DTABLE_ADD (DW_OP_lit26);	\
    DTABLE_ADD (DW_OP_lit27);	\
    DTABLE_ADD (DW_OP_lit28);	\
    DTABLE_ADD (DW_OP_lit29);	\
    DTABLE_ADD (DW_OP_lit30);	\
    DTABLE_ADD (DW_OP_lit31);	\
    DTABLE_ADD (I8X_OP_return);	\
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
  struct i8x_code *code;
  union i8x_value *vsp, *saved_vsp;
  union i8x_value *csp, *saved_csp;
  struct i8x_instr *op;
  union i8x_value tmp;
  i8x_err_e err = I8X_OK;

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

  /* Get the code.  */
  code = ref->interp_impl;
  i8x_assert (code != NULL);

  /* Pull the stack pointers into local variables.  */
  vsp = saved_vsp = xctx->vsp;
  csp = saved_csp = xctx->csp;
  i8x_assert (xctx->stack_base <= vsp);
  i8x_assert (vsp <= csp);
  i8x_assert (csp <= xctx->stack_limit);

  /* XXX push the dummy frame  */

  /* Check we have enough stack for this function.  */
  if (__i8x_unlikely (vsp + code->max_stack > csp))
    {
      err = i8x_code_error (code, I8X_STACK_OVERFLOW, code->entry_point);
      goto unwind_and_return;
    }

  /* Copy the arguments into the value stack.  */
  i8x_assert (code->max_stack >= (size_t) code->num_args);
  STORE_VSP_LIMITS ();
  ADJUST_STACK (code->num_args);
  memcpy (saved_vsp, args, sizeof (union i8x_value) * code->num_args);

  /* Start executing.  */
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
    ENSURE_DEPTH (2);
    tmp = STACK(0);
    STACK(0) = STACK(1);
    STACK(1) = tmp;
    CONTINUE;

  OPERATION (DW_OP_rot):
    ENSURE_DEPTH (3);
    tmp = STACK(0);
    STACK(0) = STACK(1);
    STACK(1) = STACK(2);
    STACK(2) = tmp;
    CONTINUE;

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

  OPERATION (I8X_OP_return):
    ENSURE_DEPTH (code->num_rets);
    goto unwind_and_return_values;

 unhandled_operation:
  i8x_internal_error (__FILE__, __LINE__, __FUNCTION__,
		      _("%s: Not implemented."),
		      op->desc->name);

 unwind_and_return_values:
  for (int i = 0; i < code->num_rets; i++)
    rets[i] = STACK(i);

 unwind_and_return:
  xctx->vsp = saved_vsp;
  xctx->csp = saved_csp;

  return err;
}
