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
#include "xctx-private.h"

static i8x_err_e
i8x_xctx_init (struct i8x_xctx *xctx, size_t nslots)
{
  struct i8x_ctx *ctx = i8x_xctx_get_ctx (xctx);

  xctx->use_debug_interpreter =
    i8x_ctx_get_log_priority (ctx) >= LOG_DEBUG;

  xctx->stack_base = calloc (nslots, sizeof (union i8x_value));
  if (xctx->stack_base == NULL)
    return i8x_out_of_memory (ctx);

  xctx->stack_limit = xctx->stack_base + nslots;

  xctx->vsp = xctx->stack_base;
  xctx->csp = xctx->stack_limit;

  return I8X_OK;
}

static void
i8x_xctx_unlink (struct i8x_object *ob)
{
  struct i8x_xctx *xctx = (struct i8x_xctx *) ob;

  if (xctx->bytecode_count)
    dbg (i8x_xctx_get_ctx (xctx), "%d bytecodes executed\n",
	 xctx->bytecode_count);
}

static void
i8x_xctx_free (struct i8x_object *ob)
{
  struct i8x_xctx *xctx = (struct i8x_xctx *) ob;

  if (xctx->stack_base != NULL)
    free (xctx->stack_base);
}

const struct i8x_object_ops i8x_xctx_ops =
  {
    "xctx",			/* Object name.  */
    sizeof (struct i8x_xctx),	/* Object size.  */
    i8x_xctx_unlink,		/* Unlink function.  */
    i8x_xctx_free,		/* Free function.  */
  };

I8X_EXPORT i8x_err_e
i8x_xctx_new (struct i8x_ctx *ctx, size_t stack_slots,
	      struct i8x_xctx **xctx)
{
  struct i8x_xctx *x;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_xctx_ops, &x);
  if (err != I8X_OK)
    return err;

  err = i8x_xctx_init (x, stack_slots);
  if (err != I8X_OK)
    {
      x = i8x_xctx_unref (x);

      return err;
    }

  dbg (ctx, "stack_slots=" LDEC "\n", stack_slots);

  *xctx = x;

  return I8X_OK;
}

I8X_EXPORT bool
i8x_xctx_get_use_debug_interpreter (struct i8x_xctx *xctx)
{
  return xctx->use_debug_interpreter;
}

I8X_EXPORT void
i8x_xctx_set_use_debug_interpreter (struct i8x_xctx *xctx,
				    bool use_debug_interpreter)
{
  xctx->use_debug_interpreter = use_debug_interpreter;
}

I8X_EXPORT int
i8x_xctx_get_wordsize (struct i8x_xctx *xctx)
{
  return xctx->wordsize;
}

I8X_EXPORT i8x_byte_order_e
i8x_xctx_get_byte_order (struct i8x_xctx *xctx)
{
  return xctx->byte_order;
}
