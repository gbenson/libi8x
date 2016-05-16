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

/* NB this file is included directly by code.c.  */

static struct i8x_idesc optable[] =
{
  {NULL},
  {NULL},
  {NULL},
  {"DW_OP_addr", I8X_OPR_ADDRESS},
  {NULL},
  {NULL},
  {"DW_OP_deref"},
  {NULL},
  {"DW_OP_const1u", I8X_OPR_UINT8},
  {"DW_OP_const1s", I8X_OPR_INT8},
  {"DW_OP_const2u", I8X_OPR_UINT16},
  {"DW_OP_const2s", I8X_OPR_INT16},
  {"DW_OP_const4u", I8X_OPR_UINT32},
  {"DW_OP_const4s", I8X_OPR_INT32},
  {"DW_OP_const8u", I8X_OPR_UINT64},
  {"DW_OP_const8s", I8X_OPR_INT64},

  /* 0x10..0x1f */
  {"DW_OP_constu", I8X_OPR_ULEB128},
  {"DW_OP_consts", I8X_OPR_SLEB128},
  {"DW_OP_dup"},
  {"DW_OP_drop"},
  {"DW_OP_over"},
  {"DW_OP_pick", I8X_OPR_UINT8},
  {"DW_OP_swap"},
  {"DW_OP_rot"},
  {"DW_OP_xderef"},
  {"DW_OP_abs"},
  {"DW_OP_and"},
  {"DW_OP_div"},
  {"DW_OP_minus"},
  {"DW_OP_mod"},
  {"DW_OP_mul"},
  {"DW_OP_neg"},

  /* 0x20..0x2f */
  {"DW_OP_not"},
  {"DW_OP_or"},
  {"DW_OP_plus"},
  {"DW_OP_plus_uconst", I8X_OPR_ULEB128},
  {"DW_OP_shl"},
  {"DW_OP_shr"},
  {"DW_OP_shra"},
  {"DW_OP_xor"},
  {"DW_OP_bra", I8X_OPR_INT16},
  {"DW_OP_eq"},
  {"DW_OP_ge"},
  {"DW_OP_gt"},
  {"DW_OP_le"},
  {"DW_OP_lt"},
  {"DW_OP_ne"},
  {"DW_OP_skip", I8X_OPR_INT16},

  /* 0x30..0x3f */
  {"DW_OP_lit0"},  {"DW_OP_lit1"},  {"DW_OP_lit2"},  {"DW_OP_lit3"},
  {"DW_OP_lit4"},  {"DW_OP_lit5"},  {"DW_OP_lit6"},  {"DW_OP_lit7"},
  {"DW_OP_lit8"},  {"DW_OP_lit9"},  {"DW_OP_lit10"}, {"DW_OP_lit11"},
  {"DW_OP_lit12"}, {"DW_OP_lit13"}, {"DW_OP_lit14"}, {"DW_OP_lit15"},

  /* 0x40..0x4f */
  {"DW_OP_lit16"}, {"DW_OP_lit17"}, {"DW_OP_lit18"}, {"DW_OP_lit19"},
  {"DW_OP_lit20"}, {"DW_OP_lit21"}, {"DW_OP_lit22"}, {"DW_OP_lit23"},
  {"DW_OP_lit24"}, {"DW_OP_lit25"}, {"DW_OP_lit26"}, {"DW_OP_lit27"},
  {"DW_OP_lit28"}, {"DW_OP_lit29"}, {"DW_OP_lit30"}, {"DW_OP_lit31"},

  /* 0x50..0x5f */
  {"DW_OP_reg0"},  {"DW_OP_reg1"},  {"DW_OP_reg2"},  {"DW_OP_reg3"},
  {"DW_OP_reg4"},  {"DW_OP_reg5"},  {"DW_OP_reg6"},  {"DW_OP_reg7"},
  {"DW_OP_reg8"},  {"DW_OP_reg9"},  {"DW_OP_reg10"}, {"DW_OP_reg11"},
  {"DW_OP_reg12"}, {"DW_OP_reg13"}, {"DW_OP_reg14"}, {"DW_OP_reg15"},

  /* 0x60..0x6f */
  {"DW_OP_reg16"}, {"DW_OP_reg17"}, {"DW_OP_reg18"}, {"DW_OP_reg19"},
  {"DW_OP_reg20"}, {"DW_OP_reg21"}, {"DW_OP_reg22"}, {"DW_OP_reg23"},
  {"DW_OP_reg24"}, {"DW_OP_reg25"}, {"DW_OP_reg26"}, {"DW_OP_reg27"},
  {"DW_OP_reg28"}, {"DW_OP_reg29"}, {"DW_OP_reg30"}, {"DW_OP_reg31"},

  /* 0x70..0x7f */
  {"DW_OP_breg0", I8X_OPR_SLEB128},  {"DW_OP_breg1", I8X_OPR_SLEB128},
  {"DW_OP_breg2", I8X_OPR_SLEB128},  {"DW_OP_breg3", I8X_OPR_SLEB128},
  {"DW_OP_breg4", I8X_OPR_SLEB128},  {"DW_OP_breg5", I8X_OPR_SLEB128},
  {"DW_OP_breg6", I8X_OPR_SLEB128},  {"DW_OP_breg7", I8X_OPR_SLEB128},
  {"DW_OP_breg8", I8X_OPR_SLEB128},  {"DW_OP_breg9", I8X_OPR_SLEB128},
  {"DW_OP_breg10", I8X_OPR_SLEB128}, {"DW_OP_breg11", I8X_OPR_SLEB128},
  {"DW_OP_breg12", I8X_OPR_SLEB128}, {"DW_OP_breg13", I8X_OPR_SLEB128},
  {"DW_OP_breg14", I8X_OPR_SLEB128}, {"DW_OP_breg15", I8X_OPR_SLEB128},

  /* 0x80..0x8f */
  {"DW_OP_breg16", I8X_OPR_SLEB128}, {"DW_OP_breg17", I8X_OPR_SLEB128},
  {"DW_OP_breg18", I8X_OPR_SLEB128}, {"DW_OP_breg19", I8X_OPR_SLEB128},
  {"DW_OP_breg20", I8X_OPR_SLEB128}, {"DW_OP_breg21", I8X_OPR_SLEB128},
  {"DW_OP_breg22", I8X_OPR_SLEB128}, {"DW_OP_breg23", I8X_OPR_SLEB128},
  {"DW_OP_breg24", I8X_OPR_SLEB128}, {"DW_OP_breg25", I8X_OPR_SLEB128},
  {"DW_OP_breg26", I8X_OPR_SLEB128}, {"DW_OP_breg27", I8X_OPR_SLEB128},
  {"DW_OP_breg28", I8X_OPR_SLEB128}, {"DW_OP_breg29", I8X_OPR_SLEB128},
  {"DW_OP_breg30", I8X_OPR_SLEB128}, {"DW_OP_breg31", I8X_OPR_SLEB128},

  /* 0x90..0x9f */
  {"DW_OP_regx", I8X_OPR_ULEB128},
  {"DW_OP_fbreg", I8X_OPR_SLEB128},
  {"DW_OP_bregx", I8X_OPR_ULEB128, I8X_OPR_SLEB128},
  {"DW_OP_piece", I8X_OPR_ULEB128},
  {"DW_OP_deref_size", I8X_OPR_UINT8},
  {"DW_OP_xderef_size", I8X_OPR_UINT8},
  {"DW_OP_nop"},
  {"DW_OP_push_object_address"},
  {"DW_OP_call2"},
  {"DW_OP_call4"},
  {"DW_OP_call_ref"},
  {"DW_OP_form_tls_address"},
  {"DW_OP_call_frame_cfa"},
  {"DW_OP_bit_piece"},
  {"DW_OP_implicit_value"},
  {"DW_OP_stack_value"},

  /* 0xa0..0xaf */
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},

  /* 0xb0..0xbf */
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},

  /* 0xc0..0xcf */
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},

  /* 0xd0..0xdf */
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},

  /* 0xe0..0xef */
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},

  /* 0xf0..0xff */
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},

  /* 0x100..0x10f */
  {"I8_OP_call"},
  {"I8_OP_load_external", I8X_OPR_ULEB128},
  {"I8_OP_deref_int", I8X_OPR_INT8},
  {NULL},
  {NULL}, {NULL}, {NULL}, {NULL},
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},

  /* 0x110..0x11f */
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},

  /* 0x120..0x12f */
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},

  /* 0x130..0x13f */
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
  {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL},

  /* 0x140..0x14f */
  {"I8X_OP_return"},
};

#define NUM_OPCODES (sizeof (optable) / sizeof (struct i8x_idesc))
#define MAX_OPCODE  (NUM_OPCODES - 1)

size_t
i8x_ctx_get_dispatch_table_size (struct i8x_ctx *ctx)
{
  return NUM_OPCODES;
}
