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

/* Instruction description.  */

struct i8x_idesc
{
  const char *name;			/* Name, for tracing etc.  */
  i8x_operand_type_e arg1, arg2;	/* Operand types.  */
};

/* Instruction.  */

struct i8x_instr
{
  i8x_opcode_t code;		/* Opcode.  */
  const struct i8x_idesc *desc;	/* Description.  */
  union i8x_value arg1, arg2;	/* Operands.  */
};

/* Unpacked bytecode of one note.  */

struct i8x_code
{
  I8X_OBJECT_FIELDS;

  i8x_byte_order_e byte_order;	/* The byte order of the code chunk.  */
  size_t code_size;		/* Size of undecoded bytecode, in bytes.  */
  const char *code_start;	/* First byte of undecoded bytecode.  */

  struct i8x_instr *itable;	/* Decoded bytecode.  */

  size_t max_stack;		/* Maximum stack this function uses.  */
};

/* Interpreter private functions.  */

i8x_err_e i8x_code_error (struct i8x_code *code, i8x_err_e err,
			  struct i8x_instr *ip);

/* Convert a bytecode pointer to an instruction pointer.  */

static inline struct i8x_instr * __attribute__ ((always_inline))
bcp_to_ip (struct i8x_code *code, const char *bcp)
{
  size_t bci = bcp - code->code_start;

  return code->itable + bci;
}

/* Convert an instruction pointer to a bytecode pointer.  */

static inline const char * __attribute__ ((always_inline))
ip_to_bcp (struct i8x_code *code, struct i8x_instr *ip)
{
  size_t bci = ip - code->itable;

  return code->code_start + bci;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _INTERP_PRIVATE_H_ */
