	.section .note.infinity, "", "note"
	.balign 4

	/* test::factorial */
	.4byte 2f-1f	/* namesz */
	.4byte 4f-3f	/* descsz */
#define NT_GNU_INFINITY 0x05
	.4byte NT_GNU_INFINITY
1:	.string "GNU"
2:	.balign 4
#define I8_CHUNK_CODEINFO 0x05
3:	.uleb128 I8_CHUNK_CODEINFO
	.uleb128 1	/* chunk version */
	.uleb128 6f-5f	/* chunk size */
#define I8_BYTE_ORDER_MARK 0x6938
5:	.2byte I8_BYTE_ORDER_MARK
	.uleb128 4	/* max stack */
#define I8_CHUNK_BYTECODE 0x02
6:	.uleb128 I8_CHUNK_BYTECODE
	.uleb128 2	/* chunk version */
	.uleb128 8f-7f	/* chunk size */
#define DW_OP_lit1 0x31
7:	.byte DW_OP_lit1	/* ifact.i8:7 */
#define DW_OP_swap 0x16
	.byte DW_OP_swap	/* ifact.i8:8 */
#define DW_OP_dup 0x12
10:	.byte DW_OP_dup	/* ifact.i8:20 */
	.byte DW_OP_lit1	/* ifact.i8:21 */
#define DW_OP_gt 0x2b
	.byte DW_OP_gt	/* ifact.i8:22 */
#define DW_OP_bra 0x28
	.byte DW_OP_bra	/* ifact.i8:22 */
	.2byte 11f-12f
#define DW_OP_drop 0x13
12:	.byte DW_OP_drop	/* ifact.i8:23 */
#define DW_OP_skip 0x2f
	.byte DW_OP_skip	/* ifact.i8:23 */
	.2byte 9f-13f
13:
11:	.byte DW_OP_dup	/* ifact.i8:12 */
#define DW_OP_rot 0x17
	.byte DW_OP_rot	/* ifact.i8:13 */
#define DW_OP_mul 0x1e
	.byte DW_OP_mul	/* ifact.i8:14 */
	.byte DW_OP_swap	/* ifact.i8:15 */
	.byte DW_OP_lit1	/* ifact.i8:16 */
#define DW_OP_minus 0x1c
	.byte DW_OP_minus	/* ifact.i8:17 */
	.byte DW_OP_skip	/* ifact.i8:17 */
	.2byte 10b-14f
14:
9:
#define I8_CHUNK_SIGNATURE 0x01
8:	.uleb128 I8_CHUNK_SIGNATURE
	.uleb128 2	/* chunk version */
	.uleb128 19f-18f	/* chunk size */
18:	.uleb128 16f-15f	/* provider offset */
	.uleb128 0	/* name offset */
	.uleb128 17f-15f	/* param types offset */
	.uleb128 17f-15f	/* return types offset */
#define I8_CHUNK_STRINGS 0x04
19:	.uleb128 I8_CHUNK_STRINGS
	.uleb128 1	/* chunk version */
	.uleb128 21f-20f	/* chunk size */
20:
15:	.string "factorial"
16:	.string "test"
17:	.string "i"
21:
4:	.balign 4
