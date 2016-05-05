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

/* Special opcode denoting slots in the instruction table that
   cannot be executed or jumped to, either because that location
   in the bytecode is mid-instruction or because an instruction
   has been eliminated.  */
#define IT_EMPTY_SLOT 0

/* Instruction.  */

struct i8x_instr
{
  i8x_opcode_t code;			/* Opcode or IT_EMPTY_SLOT.  */
  const struct i8x_idesc *desc;		/* Description.  */
  union i8x_value arg1, arg2;		/* Operands.  */

  /* For DW_OP_addr: the relocation referenced by arg1.  */
  struct i8x_reloc *addr1;

  /* For I8_OP_load_external: the external function referenced by arg1.  */
  struct i8x_funcref *ext1;

  /* Pointers to the next instruction for branch and non-branch
     cases.  NULL in either pointer is a return from this function.  */
  struct i8x_instr *branch_next;
  struct i8x_instr *fall_through;

  /* Pointers to the instruction's implementations in the
     standard and debug interpreters respectively.  */
  void *impl_std, *impl_dbg;

  /* Temporary variable used to avoid loops during i8x_code
     initialization.  */
  bool is_visited;

  /* Temporary variable used by the validator.  */
  struct i8x_type **entry_stack;
};

/* Unpacked bytecode of one note.  */

struct i8x_code
{
  I8X_OBJECT_FIELDS;

  int wordsize;		/* Wordsize of the code chunk, or 0 if unknown.  */
  bool bytes_swapped;	/* True if bytes are swapped (unknown if
			   wordsize == 0)  */

  size_t code_size;		/* Size of undecoded bytecode, in bytes.  */
  const char *code_start;	/* First byte of undecoded bytecode.  */

  struct i8x_instr *itable;		/* Decoded bytecode.  */
  struct i8x_instr *itable_limit;	/* The end of the above.  */
  struct i8x_instr *entry_point;	/* Function entry point.  */

  size_t max_stack;		/* Maximum stack this function uses.  */

  struct i8x_list *relocs;	/* List of interned relocations.  */
};

/* Interpreter private functions.  */

#ifdef __cplusplus
extern "C" {
#endif

i8x_err_e i8x_code_error (struct i8x_code *code, i8x_err_e err,
			  struct i8x_instr *ip);
size_t ip_to_so (struct i8x_code *code, struct i8x_instr *ip);
void i8x_code_dump_itable (struct i8x_code *code, const char *where);
void i8x_code_reset_is_visited (struct i8x_code *code);
i8x_err_e i8x_code_validate (struct i8x_code *code,
			     struct i8x_funcref *ref);
i8x_err_e i8x_xctx_call_dbg (struct i8x_xctx *xctx,
			     struct i8x_funcref *ref,
			     struct i8x_inferior *inf,
			     union i8x_value *args,
			     union i8x_value *rets);

#define i8x_code_foreach_op(code, op) \
  for (op = code->itable; op < code->itable_limit; op++)

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
