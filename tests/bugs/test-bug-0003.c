/* Copyright (C) 2017 Red Hat, Inc.
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

/* define test::bug_0003 returns int
       load -281474976710656
 */
static uint8_t testnote[] = {
  /* 0x00..0x0f */
  0x05, 0x01, 0x03, 0x78, 0x29, 0x01, 0x02, 0x03,	/* |...x)...|	*/
  0x08, 0x11, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,	/* |........|	*/

  /* 0x10..0x1f */
  0x40, 0x01, 0x02, 0x04, 0x09, 0x00, 0x08, 0x0e,	/* |@.......|	*/
  0x04, 0x01, 0x10, 0x62, 0x75, 0x67, 0x5f, 0x30,	/* |...bug_0|	*/

  /* 0x20..0x2a */
  0x30, 0x30, 0x33, 0x00, 0x74, 0x65, 0x73, 0x74,	/* |003.test|	*/
  0x00, 0x69, 0x00,					/* |.i.|	*/
};

static uint16_t *testnote_byteorder = (uint16_t *) (testnote + 0x03);

static void
check_offsets (void)
{
  CHECK (((char *) testnote_byteorder)[0] == 'x');
  CHECK (((char *) testnote_byteorder)[1] == ')');
}

void
i8x_execution_test (struct i8x_ctx *ctx, struct i8x_xctx *xctx,
		    struct i8x_inf *inf, int wordsize,
		    bool bytes_reversed)
{
  static bool is_setup = false;
  i8x_err_e err;

  if (!is_setup)
    {
      check_offsets ();
      is_setup = true;
    }

  *testnote_byteorder = ARCHSPEC (wordsize, bytes_reversed);

  err = i8x_ctx_import_bytecode (ctx, (const char *) testnote,
				 sizeof (testnote), "<testnote>",
				 0, NULL);
#if __WORDSIZE <= 32
  CHECK (err == I8X_NOTE_UNHANDLED);
#else
  CHECK_CALL (ctx, err);
#endif
}
