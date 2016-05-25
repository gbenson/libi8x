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

#include "execution-test.h"

#include <libi8x.h>
#include <opcodes.h>
#include <string.h>

/* define test::test_deref returns int
          argument ptr address
          deref u64
 */
static uint8_t testnote[] = {
  /* 0x00..0x0f */
  0x05, 0x01, 0x03, 0x78, 0x29, 0x01, 0x02, 0x03,	/* |...x)...|	*/
  0x04, 0xff, 0x02, 0xc0, 0x00, 0x01, 0x02, 0x04,	/* |........|	*/

  /* 0x10..0x1f */
  0x0b, 0x00, 0x12, 0x10, 0x04, 0x01, 0x14, 0x74,	/* |.......t|	*/
  0x65, 0x73, 0x74, 0x5f, 0x64, 0x65, 0x72, 0x65,	/* |est_dere|	*/

  /* 0x20..0x2a */
  0x66, 0x00, 0x74, 0x65, 0x73, 0x74, 0x00, 0x69,	/* |f.test.i|	*/
  0x00, 0x70, 0x00,					/* |.p.|	*/
};

static uint16_t *testnote_byteorder = (uint16_t *) (testnote + 0x03);
static uint8_t *testnote_code = testnote + 0x09;
static char *testnote_return = (char *) (testnote + 0x27);

static void
check_offsets (void)
{
  CHECK (((char *) testnote_byteorder)[0] == 'x');
  CHECK (((char *) testnote_byteorder)[1] == ')');

  CHECK (testnote_code[0] == DW_OP_GNU_wide_op);
  CHECK (testnote_code[1] + 0x100 == I8_OP_deref_int);
  CHECK (testnote_code[2] == 0xc0); /* 1st byte of SLEB128 "64".  */
  CHECK (testnote_code[3] == 0x00); /* 2nd byte of SLEB128 "64".  */

  CHECK (*testnote_return == 'i');
}

/* TestCase ID, for switches.  */
#define TC_UNSIGNED	false
#define TC_SIGNED	true

#define TC_EL		false
#define TC_BE		true

#define TCID(size, is_signed, bytes_reversed) \
  ((size & 0xffff) | (is_signed << 16) | (bytes_reversed << 24))

static union i8x_value
expect_result (int size,  bool is_signed, bool bytes_reversed)
{
  uint16_t tmp = 1;
  bool expect_be = (*((uint8_t *) &tmp) == 0) ^ bytes_reversed;
  union i8x_value result;

  switch (TCID (size, is_signed, expect_be))
    {
      /* 8 bits */
    case TCID (8, TC_UNSIGNED, TC_BE):
    case TCID (8, TC_UNSIGNED, TC_EL):
      result.u = 0x80;
      break;

    case TCID (8, TC_SIGNED, TC_BE):
    case TCID (8, TC_SIGNED, TC_EL):
      result.i = -0x80;
      break;

      /* 16 bits */
    case TCID (16, TC_UNSIGNED, TC_BE):
      result.u = 0x8081;
      break;

    case TCID (16, TC_UNSIGNED, TC_EL):
      result.u = 0x8180;
      break;

    case TCID (16, TC_SIGNED, TC_BE):
      result.i = -0x7f7f;
      break;

    case TCID (16, TC_SIGNED, TC_EL):
      result.i = -0x7e80;
      break;

      /* 32 bits */
    case TCID (32, TC_UNSIGNED, TC_BE):
      result.u = 0x80818283;
      break;

    case TCID (32, TC_UNSIGNED, TC_EL):
      result.u = 0x83828180;
      break;

    case TCID (32, TC_SIGNED, TC_BE):
      result.i = -0x7f7e7d7d;
      break;

    case TCID (32, TC_SIGNED, TC_EL):
      result.i = -0x7c7d7e80;
      break;

      /* 64 bits */
#if __WORDSIZE >= 64
    case TCID (64, TC_UNSIGNED, TC_BE):
      result.u = 0x8081828384858687;
      break;

    case TCID (64, TC_UNSIGNED, TC_EL):
      result.u = 0x8786858483828180;
      break;

    case TCID (64, TC_SIGNED, TC_BE):
      result.i = -0x7f7e7d7c7b7a7979;
      break;

    case TCID (64, TC_SIGNED, TC_EL):
      result.i = -0x78797a7b7c7d7e80;
      break;
#endif /* __WORDSIZE >= 64 */

    default:
      FAIL ("bad switch");
    }

  return result;
}

#define TEST_ADDR 0x12345678

static void
do_test (struct i8x_ctx *ctx, struct i8x_xctx *xctx,
	 struct i8x_inf *inf, int size, bool is_signed,
	 bool bytes_reversed)
{
  union i8x_value expect = expect_result (size, is_signed,
					  bytes_reversed);
  union i8x_value actual, input;
  struct i8x_func *func;
  i8x_err_e err;

  err = i8x_ctx_import_bytecode (ctx, (const char *) testnote,
				 sizeof (testnote), "<testnote>",
				 0, &func);
  CHECK_CALL (ctx, err);

  input.u = TEST_ADDR;
  err = i8x_xctx_call (xctx, i8x_func_get_funcref (func), inf,
		       &input, &actual);

  i8x_ctx_unregister_func (ctx, func);
  i8x_func_unref (func);

  CHECK_CALL (ctx, err);
  CHECK (actual.u == expect.u);
}

static uint8_t test_memory[__WORDSIZE >> 3];

static i8x_err_e
read_memory (struct i8x_inf *inf, uintptr_t addr, size_t len,
	     void *result)
{
  CHECK (addr == TEST_ADDR);

  memcpy (result, test_memory, len);

  return I8X_OK;
}

/* Copied from libi8x/code.c.  */

#define ARCHSPEC_1(msb, lsb, wordsize)			\
  ((((msb) ^ (wordsize)) << 8) | ((lsb) ^ (wordsize)))

#define ARCHSPEC(wordsize, is_swapped)			\
  (!(is_swapped) ? ARCHSPEC_1 ('i', '8', wordsize)	\
		 : ARCHSPEC_1 ('8', 'i', wordsize))

void
i8x_execution_test (struct i8x_ctx *ctx, struct i8x_xctx *xctx,
		    struct i8x_inf *inf, int wordsize,
		    bool bytes_reversed)
{
  static bool is_setup = false;

  if (!is_setup)
    {
      check_offsets ();

      for (size_t i = 0; i < sizeof (test_memory); i++)
	test_memory[i] = 0x80 + i;

      is_setup = true;
    }

  i8x_inf_set_read_mem_fn (inf, read_memory);

  *testnote_byteorder = ARCHSPEC (wordsize, bytes_reversed);

  /* deref ptr */
  testnote_code[0] = DW_OP_deref;
  testnote_code[1] = DW_OP_nop;
  testnote_code[2] = DW_OP_nop;
  testnote_code[3] = DW_OP_nop;
  *testnote_return = 'p';

  do_test (ctx, xctx, inf, wordsize, false, bytes_reversed);

  /* unsized deref int */
  testnote_code[0] = DW_OP_GNU_wide_op;
  testnote_code[1] = I8_OP_deref_int - 0x100;
  testnote_code[2] = 0;
  *testnote_return = 'i';

  do_test (ctx, xctx, inf, wordsize, false, bytes_reversed);

  /* sized deref int */
  for (int size = 8; size <= wordsize; size <<= 1)
    {
      for (int is_signed = 0; is_signed <= 1; is_signed++)
	{
	  if (size == 64 && !is_signed)
	    {
	      /* 64 is two bytes in SLEB128.  */
	      testnote_code[2] = 0xc0;
	      testnote_code[3] = 0x00;
	    }
	  else
	    {
	      /* All the rest are a single byte.  */
	      testnote_code[2] = (is_signed ? -size : size) & 0x7f;
	      testnote_code[3] = DW_OP_nop;
	    }

	  do_test (ctx, xctx, inf, size, is_signed, bytes_reversed);
	}
    }
}
