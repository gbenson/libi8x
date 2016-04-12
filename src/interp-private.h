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

#ifndef _INTERP_PRIVATE_H_
#define _INTERP_PRIVATE_H_

#include "libi8x-private.h"
#include "opcodes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Opcodes.  */

typedef uint_fast16_t i8x_opcode_t;

/* Operand types.  */

typedef enum
{
  I8X_OPR_NONE = 0,
  I8X_OPR_ADDRESS,
  I8X_OPR_INT8,
  I8X_OPR_UINT8,
  I8X_OPR_INT16,
  I8X_OPR_UINT16,
  I8X_OPR_INT32,
  I8X_OPR_UINT32,
  I8X_OPR_INT64,
  I8X_OPR_UINT64,
  I8X_OPR_SLEB128,
  I8X_OPR_ULEB128,
}
i8x_operand_type_e;

/* Operator descriptions.  */

struct i8x_opdesc
{
  const char *name;		/* Name, for tracing.  */
  i8x_operand_type_e op1, op2;	/* Operand types.  */
};

/* An unpacked instruction.  */

struct i8x_instr
{
  i8x_opcode_t code;		/* The opcode.  */

  const struct i8x_opdesc *desc;/* Description (name, operand types).  */

  union i8x_value op1, op2;	/* Operands.  */
};

/* The unpacked bytecode of one note.  */

struct i8x_code
{
  I8X_OBJECT_FIELDS;

  i8x_byte_order_e byte_order;	/* The byte order of the code chunk.  */

  const char *code_start;	/* First byte of undecoded bytecode.  */
  size_t code_size;		/* Size of bytecode, in bytes.  */
  struct i8x_instr **itable;	/* Sparse table of instructions.  */

  size_t max_stack;		/* The maximum stack this bytecode uses.  */
};

/* Private functions.  */

struct i8x_func *i8x_code_get_func (struct i8x_code *code);
struct i8x_note *i8x_code_get_note (struct i8x_code *code);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _INTERP_PRIVATE_H_ */
