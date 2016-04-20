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
	.uleb128 3	/* max stack */
#define I8_CHUNK_BYTECODE 0x02
6:	.uleb128 I8_CHUNK_BYTECODE
	.uleb128 2	/* chunk version */
	.uleb128 8f-7f	/* chunk size */
#define DW_OP_dup 0x12
7:	.byte DW_OP_dup	/* rfact.i8:7 */
#define DW_OP_lit1 0x31
	.byte DW_OP_lit1	/* rfact.i8:8 */
#define DW_OP_gt 0x2b
	.byte DW_OP_gt	/* rfact.i8:8 */
#define DW_OP_bra 0x28
	.byte DW_OP_bra	/* rfact.i8:8 */
	.2byte 10f-11f
11:	.byte DW_OP_lit1	/* rfact.i8:9 */
#define DW_OP_skip 0x2f
	.byte DW_OP_skip	/* rfact.i8:10 */
	.2byte 9f-12f
12:
10:	.byte DW_OP_dup	/* rfact.i8:13 */
	.byte DW_OP_lit1	/* rfact.i8:14 */
#define DW_OP_minus 0x1c
	.byte DW_OP_minus	/* rfact.i8:14 */
#define DW_OP_GNU_wide_op 0xff
	.byte DW_OP_GNU_wide_op	/* rfact.i8:15 */
#define I8_OP_load_external 0x101
	.uleb128 I8_OP_load_external - 0x100
	.uleb128 0
	.byte DW_OP_GNU_wide_op	/* rfact.i8:15 */
#define I8_OP_call 0x100
	.uleb128 I8_OP_call - 0x100
#define DW_OP_mul 0x1e
	.byte DW_OP_mul	/* rfact.i8:16 */
9:
#define I8_CHUNK_SIGNATURE 0x01
8:	.uleb128 I8_CHUNK_SIGNATURE
	.uleb128 2	/* chunk version */
	.uleb128 17f-16f	/* chunk size */
16:	.uleb128 14f-13f	/* provider offset */
	.uleb128 0	/* name offset */
	.uleb128 15f-13f	/* param types offset */
	.uleb128 15f-13f	/* return types offset */
#define I8_CHUNK_EXTERNALS 0x03
17:	.uleb128 I8_CHUNK_EXTERNALS
	.uleb128 1	/* chunk version */
	.uleb128 19f-18f	/* chunk size */
#define I8_EXT_FUNCTION 'f'
18:	.byte I8_EXT_FUNCTION	/* extern 0 type */
	.uleb128 14f-13f	/* extern 0 provider offset */
	.uleb128 0	/* extern 0 name offset */
	.uleb128 15f-13f	/* extern 0 ptypes offset */
	.uleb128 15f-13f	/* extern 0 rtypes offset */
#define I8_CHUNK_STRINGS 0x04
19:	.uleb128 I8_CHUNK_STRINGS
	.uleb128 1	/* chunk version */
	.uleb128 21f-20f	/* chunk size */
20:
13:	.string "factorial"
14:	.string "test"
15:	.string "i"
21:
4:	.balign 4
