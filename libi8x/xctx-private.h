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

#ifndef _XCTX_PRIVATE_H_
#define _XCTX_PRIVATE_H_

#include <libi8x.h>

/* Execution context.  */

struct i8x_xctx
{
  I8X_OBJECT_FIELDS;

  /* If true, use the interpreter with assertions etc.  */
  bool use_debug_interpreter;

  /* Magic fields used by i8x_ctx_init_dispatch_table to get the
     interpreters to emit their dispatch tables.  */
  void **dispatch_table_to_init;
  size_t dispatch_table_size;

  /* Execution stack.  Actually two stacks in one: the value stack
     grows upwards from stack_base, and the call stack grows down
     from stack_limit.  */
  union i8x_value *stack_base, *stack_limit;

  /* The slot after the last slot in the value stack.
     Slots in the value stack are stack_base <= SLOT < vsp.
     The top slot in the value stack is *(vsp - 1).  */
  union i8x_value *vsp;

  /* The "top" slot in the call stack.
     Slots in the call stack are csp <= SLOT < stack_limit.  */
  union i8x_value *csp;

  /* Wordsize of the current interpreter frame, or 0 if unknown.  */
  int wordsize;

  /* Byte order of the current frame.  */
  i8x_byte_order_e byte_order;
};

#endif /* _XCTX_PRIVATE_H_ */
