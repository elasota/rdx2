/*
 * Copyright (C) 2011-2013 Eric Lasota
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __RDX_OPCODES_HPP__
#define __RDX_OPCODES_HPP__

#include "rdx_coretypes.hpp"

enum rdxEOpArgs
{
	rdxOPARGS_INT1 = 1,
	rdxOPARGS_INT2 = 2,
	rdxOPARGS_INTVAR = 4,
	rdxOPARGS_RES = 8,
	rdxOPARGS_STR = 16,
};

struct rdxSOpcodeInfo
{
	rdxUInt8 paramFlags;
};

typedef rdxUInt8 rdxInstructionOpcode;

enum rdxEOpcode
{
	rdxOP_startbarrier,
	rdxOP_endbarrier,
	rdxOP_throw,
	rdxOP_try,
	rdxOP_catch,
	rdxOP_trycast,
	rdxOP_jump,
	rdxOP_jumpif,
	rdxOP_jumpifnot,
	rdxOP_jumpiftrue,
	rdxOP_jumpiffalse,
	rdxOP_jumpifnull,
	rdxOP_jumpifnotnull,
	rdxOP_jumpifequal,
	rdxOP_jumpifnotequal,
	rdxOP_call,
	rdxOP_callvirtual,
	rdxOP_calldelegate,
	rdxOP_alloclocal,
	rdxOP_createlocal,
	rdxOP_removelocal,
	rdxOP_pushempty,
	rdxOP_newinstance,
	rdxOP_newinstanceset,
	rdxOP_null,
	rdxOP_pinlocal,
	rdxOP_tovarying,
	rdxOP_arrayindex,
	rdxOP_property,
	rdxOP_move,
	rdxOP_load,
	rdxOP_clone,
	rdxOP_pop,
	rdxOP_cast,
	rdxOP_localref,
	rdxOP_return,
	rdxOP_hash,
	rdxOP_res,
	rdxOP_constant,
	rdxOP_constant_str,
	rdxOP_switch,
	rdxOP_iteratearray,

	rdxOP_Count,
};

static const rdxSOpcodeInfo RDX_OPCODE_INFO[] =
{
	//OP_startbarrier,
	{ rdxOPARGS_INT1 },

	//OP_endbarrier,
	{ 0 },

	//OP_throw,
	{ 0 },

	//OP_try,
	{ rdxOPARGS_INT1 | rdxOPARGS_INT2 },

	//OP_catch,
	{ rdxOPARGS_RES },

	//OP_trycast,
	{ rdxOPARGS_INT1 | rdxOPARGS_RES },

	//OP_jump,
	{ rdxOPARGS_INT1 },

	//OP_jumpif,
	{ rdxOPARGS_INT1 | rdxOPARGS_RES },

	//OP_jumpifnot,
	{ rdxOPARGS_INT1 | rdxOPARGS_RES  },

	//OP_jumpiftrue,
	{ rdxOPARGS_INT1 },

	//OP_jumpiffalse,
	{ rdxOPARGS_INT1 },

	//OP_jumpifnull,
	{ rdxOPARGS_INT1 },

	//OP_jumpifnotnull,
	{ rdxOPARGS_INT1 },

	//OP_jumpifequal,
	{ rdxOPARGS_INT1 },

	//OP_jumpifnotequal,
	{ rdxOPARGS_INT1 },

	//OP_call,
	{ rdxOPARGS_RES },

	//OP_callvirtual,
	{ rdxOPARGS_RES },

	//OP_calldelegate,
	{ rdxOPARGS_RES },

	//OP_alloclocal,
	{ rdxOPARGS_RES },

	//OP_createlocal,
	{ rdxOPARGS_RES },

	//OP_removelocal,
	{ 0 },

	//OP_pushempty,
	{ rdxOPARGS_RES },

	//OP_newinstance,
	{ rdxOPARGS_INT1 | rdxOPARGS_RES },

	//OP_newinstanceset,
	{ rdxOPARGS_INT1 | rdxOPARGS_INTVAR | rdxOPARGS_RES },

	//OP_null,
	{ 0 },
	
	//OP_pinlocal,
	{ 0 },

	//OP_tovarying,
	{ 0 },

	//OP_arrayindex,
	{ rdxOPARGS_INT1 },
	
	//OP_property,
	{ rdxOPARGS_INT1 },

	//OP_move,
	{ 0 },

	//OP_load,
	{ 0 },
	
	//OP_clone,
	{ rdxOPARGS_INT1 | rdxOPARGS_INT2 },

	//OP_pop,
	{ 0 },

	//OP_cast,
	{ rdxOPARGS_RES },

	//OP_localref,
	{ rdxOPARGS_INT1 },

	//OP_return,
	{ rdxOPARGS_INT1 },

	//OP_hash,
	{ 0 },

	//OP_res,
	{ rdxOPARGS_RES },

	//OP_constant,
	{ rdxOPARGS_RES | rdxOPARGS_INT1 },

	//OP_constant_str,
	{ rdxOPARGS_RES | rdxOPARGS_STR },

	//OP_switch,
	{ rdxOPARGS_RES | rdxOPARGS_INT1 },

	//OP_iteratearray
	{ rdxOPARGS_INT1 | rdxOPARGS_INT2 },
};

static const char *rdxOPCODE_NAMES[] =
{
	"OP_startbarrier",
	"OP_endbarrier",
	"OP_throw",
	"OP_try",
	"OP_catch",
	"OP_trycast",
	"OP_jump",
	"OP_jumpif",
	"OP_jumpifnot",
	"OP_jumpiftrue",
	"OP_jumpiffalse",
	"OP_jumpifnull",
	"OP_jumpifnotnull",
	"OP_jumpifequal",
	"OP_jumpifnotequal",
	"OP_call",
	"OP_callvirtual",
	"OP_calldelegate",
	"OP_alloclocal",
	"OP_createlocal",
	"OP_removelocal",
	"OP_pushempty",
	"OP_newinstance",
	"OP_newinstanceset",
	"OP_null",
	"OP_pinlocal",
	"OP_tovarying",
	"OP_arrayindex",
	"OP_property",
	"OP_move",
	"OP_load",
	"OP_clone",
	"OP_pop",
	"OP_cast",
	"OP_localref",
	"OP_return",
	"OP_hash",
	"OP_res",
	"OP_constant",
	"OP_constant_str",
	"OP_switch",
	"OP_iteratearray",
};

#endif
