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
#if 0
#include <stdio.h>

#include "rdx_programmability.hpp"
#include "rdx_objectmanagement.hpp"
#include "rdx_ilcomp.hpp"

const char *RDX_OPCODE_NAMES[] =
{
	"ILOP_invalid",

	"ILOP_debuginfo",

	"ILOP_move",
			
	"ILOP_tovarying",
	"ILOP_tovarying_static",
	"ILOP_pinl",
	"ILOP_pinp",
	"ILOP_incptr",
	"ILOP_objinterior",
	"ILOP_objinterior_notnull",
	"ILOP_objinterior_notnull_persist",

	"ILOP_immediate",
	"ILOP_immediate_ptr",
	"ILOP_immediate_rtp",

	"ILOP_arrayindex",
	"ILOP_call",
	"ILOP_calldelegatebp",
	"ILOP_calldelegateprv",
	"ILOP_callvirtual",
	"ILOP_callinterface",

	"ILOP_verifynotnull",
	"ILOP_zero",
	"ILOP_newinstance",
	"ILOP_newinstance_local",

	"ILOP_exit",

	"ILOP_jump",
	"ILOP_jtrue",
	"ILOP_jfalse",
	"ILOP_jnullo",
	"ILOP_jnotnullo",
	"ILOP_jnulli",
	"ILOP_jnotnulli",

	"ILOP_jinherits",
	
	"ILOP_tick",
	"ILOP_assertenum",
	"ILOP_assertinherits",
	"ILOP_rcast_otoi_direct",
	"ILOP_rcast_otoi",
	"ILOP_rcast_itoi",
	"ILOP_rcast_itoo",

#ifdef RDX_ALLOW_HASH_OPS
	"ILOP_hash_f",
	"ILOP_hash_p",
#endif

	"ILOP_jeq_f",
	"ILOP_jeq_p",
	"ILOP_jne_f",
	"ILOP_jne_p",

	"ILOP_xnullref",
	"ILOP_catch",
	"ILOP_fatal",
	"ILOP_throw",

	"ILOP_hardenstack",

	"ILOP_switch",
	"ILOP_switch_ptr",
	"ILOP_iteratearray",
	"ILOP_iteratearraysub",
	
	"ILOP_iadd",
	"ILOP_isub",
	"ILOP_imul",
	"ILOP_idiv",
	"ILOP_imod",
	"ILOP_ineg",
	"ILOP_isx",
	"ILOP_itof",
			
	"ILOP_ilt",
	"ILOP_igt",
	"ILOP_ile",
	"ILOP_ige",
	"ILOP_ieq",
	"ILOP_ine",

	"ILOP_fadd",
	"ILOP_fsub",
	"ILOP_fmul",
	"ILOP_fdiv",
	"ILOP_fneg",
	"ILOP_ftoi",
			
	"ILOP_flt",
	"ILOP_fgt",
	"ILOP_fle",
	"ILOP_fge",
	"ILOP_feq",
	"ILOP_fne",

	"ILOP_jccp",
	"ILOP_jccf",
};

static const char *rdxFlagsToMoveMode(rdxLargeInt flags)
{
	static char moveMode[7];

	if(flags & RDX_ILOP_MOVE_SRC_PARENT_FRAME)
		moveMode[0] = 'p';
	else if(flags & RDX_ILOP_MOVE_SRC_TYPE_DEFAULT)
		moveMode[0] = 'd';
	else
		moveMode[0] = 'l';

	if(flags & RDX_ILOP_MOVE_SRC_DEREF)
		moveMode[1] = '*';
	else
		moveMode[1] = 'v';

	moveMode[2] = '-';
	moveMode[3] = '>';
	
	if(flags & RDX_ILOP_MOVE_DEST_PARENT_FRAME)
		moveMode[4] = 'p';
	else
		moveMode[4] = 'l';

	if(flags & RDX_ILOP_MOVE_DEST_DEREF)
		moveMode[5] = '*';
	else
		moveMode[5] = 'v';
	moveMode[6] = '\0';

	return moveMode;
}

void rdxDisassembleMethod(void *vf, rdxHandle<const rdxCMethod> m)
{
	FILE *f = static_cast<FILE *>(vf);
	rdxHandle<const rdxSILInstruction> instructions = m->m_native.ilinstructions.DirectCast<const rdxSILInstruction>();
	rdxLargeInt numInstructions = instructions.ObjectInfo()->numElements;

	fwprintf(f, L"Disassembly for method %s\n", m.ObjectInfo()->gstSymbol->AsChars());
	fwprintf(f, L"Stack capacity: %i\n", m->m_native.frameCapacity);
	fwprintf(f, L"Journal:\n");
	if(m->m_native.numJournals)
	{
		rdxSOffsetRef<rdxHandle, const rdxUInt8> cj = m->m_native.compactedJournals + 0;
		rdxLargeInt nBytes = cj.ObjectInfo()->numElements;

		for(rdxLargeInt i=0;i<m->m_native.numJournals;i++)
		{
			rdxSStackJournal j;
			j.Decompress(&cj, NULL);
			fwprintf(f, L"j %i%s: Instr range %i - %i   Offset %i   Type %s (%s)   p: %i  nna: %i\n", i, (j.isParameter ? L" (Param)" : L""), j.startInstruction, j.endInstruction, j.offset,
				j.vType.IsNotNull() ? j.vType.ObjectInfo()->gstSymbol->AsChars().RawData() : RDX_STATIC_STRING("NULL"), (j.isPointer ? L"Pointer" : L"Value"), j.isPointer ? j.pointerSource : -1, j.isNotNull ? j.notNullInstruction : -1 );
		}
	}
			
	fwprintf(f, L"Exception handlers:\n");
	if(m->m_native.translation1.IsNotNull())
	{
		for(rdxLargeInt i=0;i<m->m_native.exHandlers.ObjectInfo()->numElements;i++)
		{
			fwprintf(f, L"eh %i: Instr %i - %i  --> %i\n", i, m->m_native.exHandlers[i].startInstruction,
				m->m_native.exHandlers[i].endInstruction, m->m_native.exHandlers[i].handlerInstruction);
		}
	}

	rdxLargeInt numLargeArgs = m->m_native.largeArgs.ObjectInfo()->numElements;
	rdxLargeInt numCompactArgs = m->m_native.compactArgs.ObjectInfo()->numElements;
			
	fwprintf(f, L"Instructions:\n");
	for(rdxLargeInt i=0;i<numInstructions;i++)
	{
		rdxSOffsetRef<rdxHandle, const rdxSILInstruction> ili = instructions + i;
		bool resumeFlag = ((m->m_native.ilResumeFlags[i / 8] & (1 << (i & 7))) != 0);

		fprintf(f, "%s i %i: %s ", resumeFlag ? "***" : "   ", i, RDX_OPCODE_NAMES[ili->opcode]);

		if(ili->arg.ca >= (m->m_native.compactArgs.DirectCast<const rdxUILOpCompactArg>() + 0) && ili->arg.ca < m->m_native.compactArgs.DirectCast<const rdxUILOpCompactArg>() + numCompactArgs)
		{
			rdxLargeInt numArgs = m->m_native.compactArgs.DirectCast<const rdxUILOpCompactArg>() + numCompactArgs - ili->arg.ca;
			for(rdxLargeInt j=i+1;j<numInstructions;j++)
			{
				rdxSOffsetRef<rdxHandle, const rdxSILInstruction> ili2 = instructions + j;
				if(ili2->arg.ca >= m->m_native.compactArgs.DirectCast<const rdxUILOpCompactArg>() + 0 && ili2->arg.ca < m->m_native.compactArgs.DirectCast<const rdxUILOpCompactArg>() + numCompactArgs)
				{
					numArgs = ili2->arg.ca - ili->arg.ca;
					break;
				}
			}
					
			fprintf(f, "CA:");
			for(rdxLargeInt a=0;a<numArgs;a++)
				fprintf(f, "\t%i", ili->arg.ca[a].li);
		}
		else if(ili->arg.la >= m->m_native.largeArgs.DirectCast<const rdxUILOpLargeArg>() + 0 && ili->arg.la < m->m_native.largeArgs.DirectCast<const rdxUILOpLargeArg>() + numLargeArgs)
		{
			rdxLargeInt numArgs = m->m_native.largeArgs.DirectCast<const rdxUILOpLargeArg>() + numLargeArgs - ili->arg.la;
			for(rdxLargeInt j=i+1;j<numInstructions;j++)
			{
				rdxSOffsetRef<rdxHandle, const rdxSILInstruction> ili2 = instructions + j;
				if(ili2->arg.la >= m->m_native.largeArgs.DirectCast<const rdxUILOpLargeArg>() + 0 && ili2->arg.la < m->m_native.largeArgs.DirectCast<const rdxUILOpLargeArg>() + numLargeArgs)
				{
					numArgs = ili2->arg.la - ili->arg.la;
					break;
				}
			}

			fprintf(f, "LA:");
			for(rdxLargeInt a=0;a<numArgs;a++)
				fprintf(f, "\t%i:%i", ili->arg.la[a].ca[0].li, ili->arg.la[a].ca[1].li);
		}
		fprintf(f, "\n");

		if(ili->opcode == rdxILOP_move)
		{
			fprintf(f, "    (%s)\n", rdxFlagsToMoveMode(ili->arg.ca[4].li));
		}
	}
}
#endif
