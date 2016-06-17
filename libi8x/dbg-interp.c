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

static void i8x_xctx_trace (struct i8x_xctx *xctx,
			    struct i8x_funcref *ref,
			    struct i8x_code *code,
			    struct i8x_instr *op,
			    union i8x_value *vsp,
			    union i8x_value *vsp_floor,
			    union i8x_value *csp);

#define DEBUG_INTERPRETER
#include "interp.c"

#include <stdio.h>

#define SLOT_TO_STR(buf, slot)					\
  do {								\
    if (STACK_DEPTH () > slot)					\
      snprintf (buf, sizeof (buf),				\
		"0x%08" LONG_FMT "x", STACK (slot).u);		\
    else							\
      strncpy (buf, "----------", sizeof (buf));		\
    buf [sizeof (buf) - 1] = '\0';				\
  } while (0)

static void
i8x_xctx_trace (struct i8x_xctx *xctx,  struct i8x_funcref *ref,
		struct i8x_code *code, struct i8x_instr *op,
		union i8x_value *vsp, union i8x_value *vsp_floor,
		union i8x_value *csp)
{
  struct i8x_ctx *ctx = i8x_xctx_get_ctx (xctx);

  if (i8x_ctx_get_log_priority (ctx) < LOG_TRACE)
    return;

  char offset[32], depth[32], stack0[32], stack1[32];

  snprintf (offset, sizeof (offset), "" LHEX "", ip_to_so (code, op));
  snprintf (depth, sizeof (depth), "[" LDEC "]", STACK_DEPTH ());

  SLOT_TO_STR (stack0, 0);
  SLOT_TO_STR (stack1, 1);

  trace (ctx, "%-39s %10s\t%-23s %4s\t%-18s\t%-18s\n",
	 ref->fullname, offset, op->desc->name,
	 depth, stack0, stack1);
}
