/*
* Copyright (C) 2011-2014 Eric Lasota
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
// This validates RDXIL and converts it into MidIL
#include <stdio.h>

#include "rdx_ilcomp.hpp"
#include "rdx_objectmanagement.hpp"
#include "rdx_opcodes.hpp"
#include "rdx_constants.hpp"
#include "rdx_intrinsics.hpp"
#include "rdx_aliasing.hpp"
#include "rdx_builtins.hpp"
#include "rdx_varying.hpp"
#include "rdx_runtimestackvalue.hpp"
#include "rdx_coretypeattribs.hpp"
#include "../lz4/lz4.h"

RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSMILStackAction);
RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSExceptionRecoveryLocation);
RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSExceptionHandlerJournal);
RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSStackJournal);
RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSMarkupInstruction);
RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSILInstruction);
RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSILDebugInfo);
RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxUILOpCompactArg);
RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxUILOpLargeArg);
RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSILCallPoint);


struct rdxConstantTypes
{
	static rdxSObjectGUID coreInt;
	static rdxSObjectGUID coreUInt;
	static rdxSObjectGUID coreByte;
	static rdxSObjectGUID coreLargeInt;
	static rdxSObjectGUID coreLargeUInt;
	static rdxSObjectGUID coreFloat;
	static rdxSObjectGUID coreDouble;
	static rdxSObjectGUID coreBool;
	static rdxSObjectGUID coreString;
};

rdxSObjectGUID rdxConstantTypes::coreInt = rdxSObjectGUID::FromObjectName("Core", "int");
rdxSObjectGUID rdxConstantTypes::coreUInt = rdxSObjectGUID::FromObjectName("Core", "uint");
rdxSObjectGUID rdxConstantTypes::coreByte = rdxSObjectGUID::FromObjectName("Core", "byte");
rdxSObjectGUID rdxConstantTypes::coreLargeInt = rdxSObjectGUID::FromObjectName("Core", "largeint");
rdxSObjectGUID rdxConstantTypes::coreLargeUInt = rdxSObjectGUID::FromObjectName("Core", "largeuint");
rdxSObjectGUID rdxConstantTypes::coreFloat = rdxSObjectGUID::FromObjectName("Core", "float");
rdxSObjectGUID rdxConstantTypes::coreDouble = rdxSObjectGUID::FromObjectName("Core", "double");
rdxSObjectGUID rdxConstantTypes::coreBool = rdxSObjectGUID::FromObjectName("Core", "bool");
rdxSObjectGUID rdxConstantTypes::coreString = rdxSObjectGUID::FromObjectName("Core", "string");


#define FLUSH_RESUME_FLAGS	\
	do {\
		if(resumeFlagCount != 0)\
		{\
			RDX_PROTECT(ctx, instructionResumeFlags->Push(ctx, resumeFlags));\
			resumeFlags = 0;\
			resumeFlagCount = 0;\
		}\
	} while(0)

#define PUSH_RESUME_FLAG(ili)	\
	do {\
		if(makeResumable || OpcodeIsResumable(ili.opcode))\
			resumeFlags |= (1 << resumeFlagCount);\
		makeResumable = false;\
		resumeFlagCount++;\
		if(resumeFlagCount == 8)\
			FLUSH_RESUME_FLAGS;\
	} while (0)

#define PUSH_INSTR_CA_1(ili, a0) do {\
		ili.argOffs = compactArgs->count;\
		RDX_PROTECT(ctx, instructions->Push(ctx, ili) );\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a0));\
		instrNumIL++;\
		PUSH_RESUME_FLAG(ili);\
	} while(false)

#define PUSH_INSTR_CA_2(ili, a0, a1) do {\
		PUSH_INSTR_CA_1(ili, a0);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a1));\
	} while(false)

#define PUSH_INSTR_CA_3(ili, a0, a1, a2) do {\
		PUSH_INSTR_CA_2(ili, a0, a1);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a2));\
	} while(false)

#define PUSH_INSTR_CA_4(ili, a0, a1, a2, a3) do {\
		PUSH_INSTR_CA_3(ili, a0, a1, a2);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a3));\
	} while(false)

#define PUSH_INSTR_CA_5(ili, a0, a1, a2, a3, a4) do {\
		PUSH_INSTR_CA_4(ili, a0, a1, a2, a3);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a4));\
	} while(false)

#define PUSH_INSTR_CA_6(ili, a0, a1, a2, a3, a4, a5) do {\
		PUSH_INSTR_CA_5(ili, a0, a1, a2, a3, a4);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a5));\
	} while(false)

#define PUSH_INSTR_CA_7(ili, a0, a1, a2, a3, a4, a5, a6) do {\
		PUSH_INSTR_CA_6(ili, a0, a1, a2, a3, a4, a5);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a6));\
	} while(false)

#define PUSH_INSTR_CA_8(ili, a0, a1, a2, a3, a4, a5, a6, a7) do {\
		PUSH_INSTR_CA_7(ili, a0, a1, a2, a3, a4, a5, a6);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a7));\
	} while(false)

#define PUSH_INSTR_CA_9(ili, a0, a1, a2, a3, a4, a5, a6, a7, a8) do {\
		PUSH_INSTR_CA_8(ili, a0, a1, a2, a3, a4, a5, a6, a7);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a8));\
	} while(false)

#define PUSH_INSTR_CA_10(ili, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) do {\
		PUSH_INSTR_CA_9(ili, a0, a1, a2, a3, a4, a5, a6, a7, a8);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a9));\
	} while(false)


#define PUSH_INSTR_LA_1(ili, a0) do {\
		ili.argOffs = largeArgs->count;\
		RDX_PROTECT(ctx, instructions->Push(ctx, ili) );\
		RDX_PROTECT(ctx, largeArgs->Push(ctx, a0));\
		instrNumIL++;\
		PUSH_RESUME_FLAG(ili);\
	} while(false)

#define PUSH_INSTR_LA_2(ili, a0, a1) do {\
		PUSH_INSTR_LA_1(ili, a0);\
		RDX_PROTECT(ctx, largeArgs->Push(ctx, a1));\
	} while(false)

#define PUSH_INSTR_LA_3(ili, a0, a1, a2) do {\
		PUSH_INSTR_LA_2(ili, a0, a1);\
		RDX_PROTECT(ctx, largeArgs->Push(ctx, a2));\
	} while(false)

#define PUSH_INSTR(ili)	do \
	{\
		ili.argOffs = ~static_cast<rdxLargeUInt>(0);\
		RDX_PROTECT(ctx, instructions->Push(ctx, ili) );\
		instrNumIL++;\
		PUSH_RESUME_FLAG(ili);\
	} while(false)

/////////////////////////////////////////////////////////////////////
#define PUSH_MARKUP_CA_1(ili, a0) do {\
		ili.argOffs = compactArgs->count;\
		ili.instructionLink = instructions->count;\
		RDX_PROTECT(ctx, markupInstructions->Push(ctx, ili) );\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a0));\
		instrNumIL++;\
	} while(false)

#define PUSH_MARKUP_CA_2(ili, a0, a1) do {\
		PUSH_MARKUP_CA_1(ili, a0);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a1));\
	} while(false)

#define PUSH_MARKUP_CA_3(ili, a0, a1, a2) do {\
		PUSH_MARKUP_CA_2(ili, a0, a1);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a2));\
	} while(false)

#define PUSH_MARKUP_CA_4(ili, a0, a1, a2, a3) do {\
		PUSH_MARKUP_CA_3(ili, a0, a1, a2);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a3));\
	} while(false)

#define PUSH_MARKUP_CA_5(ili, a0, a1, a2, a3, a4) do {\
		PUSH_MARKUP_CA_4(ili, a0, a1, a2, a3);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a4));\
	} while(false)

#define PUSH_MARKUP_CA_6(ili, a0, a1, a2, a3, a4, a5) do {\
		PUSH_MARKUP_CA_5(ili, a0, a1, a2, a3, a4);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a5));\
	} while(false)

#define PUSH_MARKUP_CA_7(ili, a0, a1, a2, a3, a4, a5, a6) do {\
		PUSH_MARKUP_CA_6(ili, a0, a1, a2, a3, a4, a5);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a6));\
	} while(false)

#define PUSH_MARKUP_CA_8(ili, a0, a1, a2, a3, a4, a5, a6, a7) do {\
		PUSH_MARKUP_CA_7(ili, a0, a1, a2, a3, a4, a5, a6);\
		RDX_PROTECT(ctx, compactArgs->Push(ctx, a7));\
	} while(false)


#define PUSH_MARKUP_LA_1(ili, a0) do {\
		ili.argOffs = largeArgs->count;\
		ili.instructionLink = instructions->count - 1;\
		RDX_PROTECT(ctx, markupInstructions->Push(ctx, ili) );\
		RDX_PROTECT(ctx, largeArgs->Push(ctx, a0));\
		instrNumIL++;\
	} while(false)

#define PUSH_MARKUP_LA_2(ili, a0, a1) do {\
		PUSH_MARKUP_LA_1(ili, a0);\
		RDX_PROTECT(ctx, largeArgs->Push(ctx, a1));\
	} while(false)

#define PUSH_MARKUP(ili)	do \
	{\
		ili.argOffs = ~static_cast<rdxLargeUInt>(0);\
		ili.instructionLink = instructions->count - 1;\
		RDX_PROTECT(ctx, markupInstructions->Push(ctx, ili) );\
		instrNumIL++;\
	} while(false)


#define DECODE_INTEGER(T, dest) do	\
	{\
		T temp;\
		temp = static_cast<T>(decOp->hsi1);\
		if(static_cast<rdxHugeInt>(temp) != decOp->hsi1)\
		RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);\
		dest = temp;\
	} while(false)

#define DECODE_FLOAT(T, dest) do	\
	{\
		if(sizeof(T) == sizeof(rdxFloat32))\
		{\
			rdxInt32 ui;\
			rdxFloat32 f;\
			DECODE_INTEGER(rdxInt32, ui);\
			memcpy(&f, &ui, sizeof(rdxInt32));\
			dest = static_cast<T>(f);\
		}\
		else if(sizeof(T) == sizeof(rdxFloat64))\
		{\
			rdxInt64 ui;\
			rdxFloat64 f;\
			DECODE_INTEGER(rdxInt64, ui);\
			memcpy(&f, &ui, sizeof(ui));\
			dest = static_cast<T>(f);\
		}\
		else\
		{\
			RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);\
		}\
	} while(false)

#define SAFE_DECODE_LABEL(labelName, arg)									\
	rdxLargeUInt labelName;													\
	do																		\
	{																		\
		rdxLargeInt instrNumS;												\
		RDX_PROTECT_ASSIGN(ctx, instrNumS, rdxMakeSigned(ctx, instrNum));	\
		if(!rdxCheckAddOverflowS(instrNumS, decOp->arg))					\
			RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);					\
		rdxLargeInt targetInstructionS = instrNumS + decOp->arg;			\
		if(targetInstructionS < 0)											\
			RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);					\
		labelName = static_cast<rdxLargeUInt>(targetInstructionS);			\
		if(labelName >= numDecodedInstructions)								\
			RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);					\
	} while(0)

// Opcodes that execution can potentially stop on, used to determine safe resumes.  This should include any opcode that can stop execution or throw an exception
static const rdxEILOpcode RDX_RESUMABLE_OPCODES[] =
{
	rdxILOP_objproperty,
	rdxILOP_arrayindex,

	rdxILOP_call,
	rdxILOP_calldelegatebp,
	rdxILOP_calldelegateprv,
	rdxILOP_callvirtual,
	rdxILOP_callinterface,

	rdxILOP_verifynotnull,
	rdxILOP_newinstance,

	rdxILOP_jump,
	rdxILOP_jtrue,
	rdxILOP_jfalse,
	rdxILOP_jnullo,
	rdxILOP_jnotnullo,
	rdxILOP_jnulli,
	rdxILOP_jnotnulli,
	rdxILOP_jinherits,

	rdxILOP_tick,
	rdxILOP_assertenum,
	rdxILOP_assertinherits,

	rdxILOP_xnullref,
	rdxILOP_catch,
	rdxILOP_fatal,
	rdxILOP_throw,

	rdxILOP_iteratearray,
	rdxILOP_iteratearraysub,
};

inline rdxLargeUInt rdxMoveGrayFlagsForSource(const rdxIObjectManager *objm, rdxWeakRTRef(rdxCType) type)
{
	if(type.IsNull())
		return 0;

	// Conditionally mark gray if assigning a reference
	if(objm->TypeIsObjectReference(type))
		return RDX_ILOP_MOVE_SRC_IS_CLASS_REF;
	if(objm->TypeIsInterface(type))
		return RDX_ILOP_MOVE_SRC_IS_INTERFACE_REF;

	if(type->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType)
	{
		rdxWeakRTRef(rdxCStructuredType) st = type.StaticCast<rdxCStructuredType>();
		if(st->storageSpecifier == rdxSS_RefStruct ||
			st->storageSpecifier == rdxSS_ValStruct)
		{
			// Always mark target gray if assigning a structure that contains refs
			if(st->m_native.numContainedReferences > 0)
			{
				return RDX_ILOP_MOVE_SRC_CONTAINS_REFS;
			}
			if(st->m_native.nativeTypeInfo.IsNotNull())
			{
				rdxIfcTypeFuncs tf = st->m_native.nativeTypeInfo.TypeFuncs();
				if(tf.IsNotNull() && tf.GetVisitReferencesFunc() != RDX_CNULL)
					return RDX_ILOP_MOVE_SRC_CONTAINS_REFS;
			}
		}
	}
	return 0;
}

template<typename T>
struct rdxSPushList
{
	typename rdxArrayCRef(T) list;
	rdxLargeUInt count;
	rdxLargeUInt maximum;

	inline rdxSPushList()
	{
		count = 0;
		maximum = 0;
		list = rdxArrayCRef(T)::Null();
	}

	inline void Push(rdxSOperationContext *ctx, const T &ref)
	{
		if(list.IsNotNull())
			list->Element(count) = ref;
		if(!rdxCheckAddOverflowU(count, 1))
			RDX_LTHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

		count++;
		if(count > maximum)
			maximum = count;
	}

	inline void ReinsertBottom(rdxSOperationContext *ctx, rdxLargeUInt offset, const T &v)
	{
		if(list.IsNotNull())
			list->Element(offset) = v;
	}

	inline void ReinsertTop(rdxSOperationContext *ctx, rdxLargeUInt offset, const T &v)
	{
		if(list.IsNotNull())
			list->Element(count - 1 - offset) = v;
	}

	inline T RetrieveBottom(rdxSOperationContext *ctx, rdxLargeUInt offset)
	{
		if(offset >= count)
			RDX_LTHROWV(ctx, RDX_ERROR_IL_STACK_UNDERFLOW, T());
		if(list.IsNotNull())
			return list->Element(offset);
		T instance;
		memset(&instance, 0, sizeof(instance));
		return instance;
	}

	inline T RetrieveTop(rdxSOperationContext *ctx, rdxLargeUInt offset)
	{
		if(count <= offset)
			RDX_LTHROWV(ctx, RDX_ERROR_IL_STACK_UNDERFLOW, T());
		if(list.IsNotNull())
			return list->Element(count - 1 - offset);
		T instance;
		memset(&instance, 0, sizeof(instance));
		return instance;
	}

	inline T Pop(rdxSOperationContext *ctx)
	{
		if(!count)
			RDX_LTHROWV(ctx, RDX_ERROR_IL_STACK_UNDERFLOW, T());
		count--;
		if(list.IsNotNull())
			return list->Element(count);
		T instance;
		memset(&instance, 0, sizeof(instance));
		return instance;
	}
};

enum rdxEILCompilePass
{
	rdxPASS_CreateStacks,
	rdxPASS_GenerateCode,
	rdxPASS_StitchJumps,

	rdxPASS_Count,
};

enum rdxEVSTType
{
	rdxVST_Invalid,

	rdxVST_Value,
	rdxVST_Pointer,
	rdxVST_ConstPointer,
	rdxVST_Varying,
	rdxVST_LocalRef,
	rdxVST_Barrier,
	rdxVST_ValueShell,

	rdxVST_Indeterminate,	// Used for opstack checks that adjust by type
};

struct rdxSVStackValue
{
	rdxEVSTType vstType;
	rdxWeakHdl(rdxCType) vType;
	rdxLargeUInt barrierValueCount;
	rdxLargeUInt size;
	rdxLargeInt offset;
	rdxLargeUInt index;
	rdxLargeUInt stackAction;
	bool isFromLocal;							// Indicates that a pointer is definitely mutable (i.e. derived from a local)
	bool isNotNull;								// Indicates that the VStack can't contain a non-null
	bool demoted;
	rdxLargeUInt notNullInstruction;
	rdxLargeUInt creatingInstruction;			// For tracing barriers
	rdxLargeUInt sequentialID;
	rdxLargeInt pointerSource;					// For frame-local pointers, the location of the pointer that this was based on
	rdxEPointerSourceType pointerSourceType;

	bool isLoadedFromLocal;						// Indicates that a value was loaded from a local
	rdxLargeUInt loadedLocal;					// For local-loaded values, the index of the local that this was loaded from

	inline rdxSVStackValue()
	{
		vstType = rdxVST_Invalid;
		vType = rdxWeakHdl(rdxCType)::Null();
		barrierValueCount = 0;
		offset = 0;
		stackAction = 0;
		index = 0;
		isFromLocal = false;
		isNotNull = false;
		isLoadedFromLocal = false;
		loadedLocal = 0;
		notNullInstruction = 0;
		pointerSource = 0;
		pointerSourceType = rdxPST_Object;
		demoted = false;
	}
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSVStackValue);

RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSVStackValue);

struct rdxSVStackJournal
{
	rdxWeakHdl(rdxCType) vType;
	rdxLargeInt offset;
	rdxLargeUInt startInstruction;	// Mid-level IL instruction
	rdxEVSTType vstType;
	rdxLargeUInt sequentialID;
	rdxLargeInt pointerSource;
	rdxEPointerSourceType pointerSourceType;

	bool record;
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSVStackJournal);

RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSVStackJournal);

struct rdxSVLocal
{
	rdxWeakHdl(rdxCType) vType;
	bool isPointer;
	bool isParameter;
	bool isConstant;
	bool isNotNull;
	bool hasCreatingInstruction;

	// CAUTION: The local offset calculation code will pull the top local, so parameterOffset and offset need to be different to
	// avoid putting locals in a bad spot, i.e. by popping a parameter local and using its offset.
	rdxLargeInt offset;
	rdxLargeInt parameterOffset;
	rdxLargeUInt creatingInstruction;	// Used to determine valid jump targets
	rdxLargeUInt stackAction;
	rdxWeakHdl(rdxCString) name;

	inline rdxSVLocal()
	{
		isPointer = false;
		isParameter = false;
		isNotNull = false;
		isConstant = false;
		vType = rdxWeakHdl(rdxCType)::Null();
		offset = 0;
		parameterOffset = 0;
	}
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSVLocal);

RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSVLocal);

// Metadata for uncompiled instructions
// Most tracking is done in translation space, relative to the original instruction set rather than the emitted IL instructions
// Instructions are assumed to do 3 things in order: Intermediates, verifiers, and a single state modifying instruction.
struct rdxSInstructionMeta
{
	bool validJumpTarget;
	rdxLargeUInt translatedInstrNum;	// First IL instruction that executes the operation requested by the opcode, which may include validation.  Intermediates like ILOP_debuginfo and ILOP_tick are skipped.
	rdxLargeUInt numValidationInstrs;	// Number of instructions emitted preceding the state change causing instruction.  These are NOT skippable and may include validators.
	rdxLargeUInt minJumpRange;			// First instruction that can jump to this instruction
	rdxLargeUInt maxJumpRange;			// Last instruction that can jump to this instruction
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSInstructionMeta);

RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSInstructionMeta);

struct rdxSILDecodedOp
{
	rdxEOpcode opcode;
	rdxLargeInt sint1;
	rdxLargeInt sint2;
	rdxHugeInt hsi1;
	rdxLargeUInt intVarCount;
	rdxWeakOffsetHdl(rdxUInt8) intVarStart;
	rdxLargeUInt intVarBytes;
	rdxWeakHdl(rdxCObject) res;
	rdxWeakHdl(rdxCString) str;
};
RDX_DECLARE_SIMPLE_NATIVE_STRUCT(rdxSILDecodedOp);

RDX_IMPLEMENT_SIMPLE_NATIVE_STRUCT(rdxSILDecodedOp);

class rdxCILPassCompiler
{
public:
	rdxWeakHdl(rdxCMethod) method;
	rdxIObjectManager *objm;
	rdxSPushList<rdxSVStackValue> *opstack;
	rdxSPushList<rdxSVLocal> *localstack;
	rdxSPushList<rdxSILInstruction> *instructions;
	rdxSPushList<rdxSMarkupInstruction> *markupInstructions;
	rdxSPushList<rdxUInt8> *instructionResumeFlags;
	rdxSPushList<rdxUILOpCompactArg> *compactArgs;
	rdxSPushList<rdxUILOpLargeArg> *largeArgs;
	rdxSPushList<rdxSVStackJournal> *stackjournal;
	rdxSPushList<rdxSMILStackAction> *stackactions;
	rdxSPushList<rdxSStackJournal> *journal;
	rdxSPushList<rdxSExceptionHandlerJournal> *exHandlers;
	rdxSPushList<rdxSExceptionRecoveryLocation> *exRLs;
	rdxWeakArrayHdl(rdxSILDecodedOp) decOps;
	rdxWeakArrayHdl(rdxSInstructionMeta) metalist;
	rdxCRef(rdxCStructuredType) st_Exception;
	rdxEILCompilePass pass;

	rdxLargeInt lowestOffset;
	rdxLargeUInt slotMaximum;
	rdxLargeUInt instrNum;
	rdxLargeUInt instrNumIL;
	rdxLargeUInt sequentialID;

	static inline bool VSTIsPointer(int vst)
	{
		return vst == rdxVST_Pointer || vst == rdxVST_ConstPointer;
	}

	void AssertOpstackEmptyOrBarrier(rdxSOperationContext *ctx)
	{
		RDX_TRY(ctx)
		{
			if(opstack->count == 0)
				return;
			if(pass > rdxPASS_CreateStacks)
			{
				rdxSVStackValue vst;
				RDX_PROTECT_ASSIGN(ctx, vst, opstack->RetrieveTop(ctx, 0) );
				if(vst.vstType != rdxVST_Barrier)
					RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);
			}
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROW(ctx);
		}
		RDX_ENDTRY
	}

	void OpstackUsed(rdxSOperationContext *ctx, rdxEVSTType vstType, rdxWeakHdl(rdxCType) vType, rdxLargeUInt *outBytesUsed, rdxLargeUInt *outSlotsUsed)
	{
		RDX_TRY(ctx)
		{
			rdxLargeUInt sizeUsed = 0;

			switch(vstType)
			{
			case rdxVST_Varying:
				sizeUsed = sizeof(rdxSVarying);
				*outSlotsUsed = 1;
				break;
			case rdxVST_Pointer:
			case rdxVST_ConstPointer:
				sizeUsed = sizeof(rdxWeakTypelessOffsetRTRef);
				*outSlotsUsed = 1;
				break;
			case rdxVST_Value:
			case rdxVST_ValueShell:
				if(objm->TypeIsObjectReference(vType.ToWeakRTRef()))
				{
					sizeUsed = sizeof(rdxTracedRTRef(rdxCObject));
					*outSlotsUsed = 1;
				}
				else if(objm->TypeIsInterface(vType.ToWeakRTRef()))
				{
					sizeUsed = sizeof(rdxTracedTypelessIRef);
					*outSlotsUsed = 1;
				}
				else
				{
					if(vType->ObjectInfo()->containerType != objm->GetBuiltIns()->st_StructuredType)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

					sizeUsed = vType.StaticCast<rdxCStructuredType>()->m_native.size;
					*outSlotsUsed = 1;
					if(vType.StaticCast<rdxCStructuredType>()->m_native.alignment > rdxALIGN_RuntimeStackValue)
						RDX_STHROW(ctx, RDX_ERROR_IL_UNALIGNABLE_BYVAL);
				}
				break;
			case rdxVST_LocalRef:
			case rdxVST_Barrier:
				sizeUsed = 0;
				*outSlotsUsed = 0;
				break;
			default:
				RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
			};

			*outBytesUsed = sizeUsed;
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROW(ctx);
		}
		RDX_ENDTRY
	}

	rdxLargeInt OpstackBaseLocation(rdxSOperationContext *ctx) const
	{
		RDX_TRY(ctx)
		{
			if(opstack->count)
			{
				rdxSVStackValue opTop;
				RDX_PROTECT_ASSIGN(ctx, opTop, opstack->RetrieveTop(ctx, 0));
				return opTop.offset;
			}
			else if(localstack->count)
			{
				rdxSVLocal localTop;
				RDX_PROTECT_ASSIGN(ctx, localTop, localstack->RetrieveTop(ctx, 0));
				if(!localTop.isParameter)
					return localTop.offset;
			}
			return 0;
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROWV(ctx, 0);
		}
		RDX_ENDTRY
	}

	void AppendOpstack(rdxSOperationContext *ctx, rdxSVStackValue &vst)
	{
		RDX_TRY(ctx)
		{
			if(pass > rdxPASS_CreateStacks)
			{
				rdxLargeUInt sizeUsed;
				rdxLargeUInt slotsUsed;
				RDX_PROTECT(ctx, OpstackUsed(ctx, vst.vstType, vst.vType, &sizeUsed, &slotsUsed) );

				rdxLargeUInt opstackIndex = 0;
				rdxLargeInt opstackOffset = 0;

				RDX_PROTECT_ASSIGN(ctx, opstackOffset, OpstackBaseLocation(ctx));
				
				rdxLargeInt sizeUsedSigned;
				RDX_PROTECT_ASSIGN(ctx, sizeUsedSigned, rdxMakeSigned(ctx, sizeUsed));
				if(!rdxCheckAddOverflowS(-sizeUsedSigned, opstackOffset))
					RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
				opstackOffset -= sizeUsedSigned;
				opstackOffset &= ~(rdxALIGN_RuntimeStackValue - 1);

				opstackIndex += slotsUsed;
				if(opstackOffset < lowestOffset)
					lowestOffset = opstackOffset;
				if(opstackIndex > slotMaximum)
					slotMaximum = opstackIndex;

				vst.size = sizeUsed;
				vst.offset = opstackOffset;
				vst.stackAction = stackactions->count;
				vst.sequentialID = sequentialID++;

				//if(vst.vType == NULL)
				//	RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);
			}
			RDX_PROTECT(ctx, opstack->Push(ctx, vst) );

			rdxSVStackJournal sj;
			sj.startInstruction = instrNum;
			sj.record = false;
			sj.vType = vst.vType;
			sj.offset = vst.offset;
			sj.vstType = vst.vstType;
			sj.sequentialID = vst.sequentialID;
			sj.pointerSource = vst.pointerSource;
			sj.pointerSourceType = vst.pointerSourceType;
			RDX_PROTECT(ctx, stackjournal->Push(ctx, sj) );
			
			if(pass > rdxPASS_CreateStacks)
			{
				rdxSMILStackAction sa;
				sa.valueType = vst.vType;
				bool isValid = false;
				switch(vst.vstType)
				{
				case rdxVST_Value:
				case rdxVST_ValueShell:
					sa.actionType = rdxEMILSAT_PushOpstackValue;
					isValid = true;
					break;
				case rdxVST_Pointer:
				case rdxVST_ConstPointer:
					sa.actionType = rdxEMILSAT_PushOpstackPtr;
					isValid = true;
					break;
				case rdxVST_Varying:
					sa.actionType = rdxEMILSAT_PushOpstackVarying;
					isValid = true;
					break;
				case rdxVST_LocalRef:
				case rdxVST_Barrier:
					break;
				default:
					RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);	// ???
					break;
				};
				if(isValid)
					RDX_PROTECT(ctx, stackactions->Push(ctx, sa) );
			};
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROW(ctx);
		}
		RDX_ENDTRY
	}

	void LivenShell(rdxSOperationContext *ctx, rdxSVStackValue &vst)
	{
		if(vst.vstType == rdxVST_ValueShell)
		{
			vst.vstType = rdxVST_Value;

			// Update the journal, this value isn't valid until the next instruction
			if(stackjournal->list.IsNotNull())
			{
				for(rdxLargeUInt i=0;i<stackjournal->count;i++)
				{
					rdxSVStackJournal vsj = stackjournal->RetrieveBottom(ctx, i);
					if(vsj.sequentialID == vst.sequentialID)
					{
						vsj.startInstruction = instrNum;
						stackjournal->ReinsertBottom(ctx, i, vsj);
					}
				}
			}
		}
	}

	// Emits a journal entry that records a call-converted VStack value, then sets the to-be-emitted journal entry to use
	// the current instruction instead.  This is used to prevent journals from lower frames from being invalidated when
	// a higher frame overlaps it and potentially overwrites values with weaker-typed ones.  The journal can't simply be
	// changed type because a thread could potentially be restored to an earlier point in time while the journal exists, but
	// is used by a non-destructive operation like OP_clone
	// TBI: This will also handle interface parameter conversion
	void CallConvertOpstack(rdxSOperationContext *ctx, rdxLargeUInt topIndex, rdxWeakHdl(rdxCType) newType)
	{
		if(pass <= rdxPASS_CreateStacks)
			return;

		RDX_TRY(ctx)
		{
			rdxSVStackValue vst;
			RDX_PROTECT_ASSIGN(ctx, vst, opstack->RetrieveTop(ctx, topIndex) );

			rdxSVStackJournal vj;
			RDX_PROTECT_ASSIGN(ctx, vj, stackjournal->RetrieveTop(ctx, topIndex) );

			rdxSStackJournal sj;
			sj.startInstruction = vj.startInstruction;
			sj.endInstruction = instrNum - 1;	// Value stops existing prior to the current instruction

			if(sj.startInstruction != sj.endInstruction)	// Don't bother journaling this as a stricter type if it won't exist
			{
				sj.offset = vj.offset;
				sj.vType = vj.vType;
				sj.isPointer = VSTIsPointer(vj.vstType);
				sj.isConstant = (vj.vstType == rdxVST_ConstPointer);
				sj.isVarying = (vj.vstType == rdxVST_Varying);
				sj.isParameter = false;
				sj.isFromLocal = true;
				sj.isNotNull = vst.isNotNull;
				sj.notNullInstruction = vst.notNullInstruction;
				sj.name = rdxWeakHdl(rdxCString)::Null();
				sj.pointerSource = vj.pointerSource;
				sj.pointerSourceType = vj.pointerSourceType;
				if(vst.demoted)
					sj.vType = vst.vType;

				RDX_PROTECT(ctx, journal->Push(ctx, sj) );
			}

			vj.startInstruction = instrNum - 1;	// New value starts existing after the previous instruction
			RDX_PROTECT(ctx, stackjournal->ReinsertTop(ctx, topIndex, vj));

			// Demote the VStack value
			vst.demoted = true;
			vst.vType = newType;
			RDX_PROTECT(ctx, opstack->ReinsertTop(ctx, topIndex, vst) );
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROW(ctx);
		}
		RDX_ENDTRY
	}

	rdxSVStackValue PopOpstack(rdxSOperationContext *ctx, rdxEVSTType expectedType = rdxVST_Value)
	{
		RDX_TRY(ctx)
		{
			rdxSVStackValue vst;
			RDX_PROTECT_ASSIGN(ctx, vst, opstack->Pop(ctx) );

			rdxSVStackJournal vj;
			RDX_PROTECT_ASSIGN(ctx, vj, stackjournal->Pop(ctx) );
			if(pass > rdxPASS_CreateStacks)
			{
				if(expectedType != rdxVST_Indeterminate && !(expectedType == rdxVST_ConstPointer && vst.vstType == rdxVST_Pointer) && vst.vstType != expectedType)
					RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

				if(vj.record && vj.vstType != rdxVST_Barrier && vj.vstType != rdxVST_LocalRef)
				{
					rdxSStackJournal sj;
					sj.startInstruction = vj.startInstruction;
					sj.endInstruction = instrNum;
					sj.offset = vj.offset;
					sj.vType = vj.vType;
					sj.isPointer = VSTIsPointer(vj.vstType);
					sj.isConstant = (vj.vstType == rdxVST_ConstPointer);
					sj.isVarying = (vj.vstType == rdxVST_Varying);
					sj.isParameter = false;
					sj.isFromLocal = true;
					sj.isNotNull = vst.isNotNull;
					sj.notNullInstruction = vst.notNullInstruction;
					sj.name = rdxWeakHdl(rdxCString)::Null();
					sj.pointerSource = vj.pointerSource;
					sj.pointerSourceType = vj.pointerSourceType;

					if(vst.demoted)
						sj.vType = vst.vType;

					RDX_PROTECT(ctx, journal->Push(ctx, sj) );
				}

				switch(vst.vstType)
				{
				case rdxVST_Value:
				case rdxVST_ValueShell:
				case rdxVST_Pointer:
				case rdxVST_ConstPointer:
				case rdxVST_Varying:
					{
						rdxSMILStackAction sa;
						sa.valueType = rdxWeakHdl(rdxCType)::Null();
						sa.actionType = rdxEMILSAT_Pop;
						RDX_PROTECT(ctx, stackactions->Push(ctx, sa) );
					}
					break;
				case rdxVST_LocalRef:
				case rdxVST_Barrier:
					break;
				default:
					RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
					break;
				};
			}

			return vst;
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROWV(ctx, rdxSVStackValue());
		}
		RDX_ENDTRY
	}

	void AppendLocal(rdxSOperationContext *ctx, rdxWeakHdl(rdxCType) &t, rdxLargeUInt instruction, rdxWeakHdl(rdxCString) name, bool isPointer = false, bool isParameter = false, bool isConstant = false, bool isNotNull = false)
	{
		RDX_TRY(ctx)
		{
			rdxSVLocal vl;

			if(t == rdxWeakHdl(rdxCType)::Null())
				RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

			vl.vType = t;
			vl.isPointer = isPointer;
			vl.isParameter = isParameter;
			vl.creatingInstruction = instruction;
			vl.name = name;
			vl.isNotNull = isNotNull;
			vl.isConstant = isConstant;
			vl.stackAction = stackactions->count;

			rdxLargeUInt align = 1;
			rdxLargeUInt size = 0;
			if(vl.isPointer)
			{
				size = sizeof(rdxWeakOffsetRTRef(rdxCObject));
				align = rdxAlignOf(rdxWeakOffsetRTRef(rdxCObject));
			}
			else
			{
				RDX_PROTECT(ctx, objm->TypeValueSize(ctx, t, size, align) );
			}

			if(isParameter)
			{
				rdxLargeInt poffs = 0;
				bool overflowed = false;

				if(localstack->count)
					poffs = -localstack->RetrieveTop(ctx, 0).parameterOffset;
				rdxLargeUInt paddedSizeU = rdxPaddedSize(size, rdxALIGN_RuntimeStackValue, overflowed);
				if(overflowed)
					RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

				rdxLargeInt paddedSizeS;
				RDX_PROTECT_ASSIGN(ctx, paddedSizeS, rdxMakeSigned(ctx, paddedSizeU));

				if(!rdxCheckAddOverflowS(poffs, paddedSizeS))
					RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
				poffs += paddedSizeS;

				vl.parameterOffset = -poffs;
			}
			else
			{
				rdxLargeInt localOffset = 0;
				rdxLargeInt originalOffset;

				if(localstack->count)
					localOffset = originalOffset = localstack->RetrieveTop(ctx, 0).offset;

				rdxLargeInt sizeSigned;
				RDX_PROTECT_ASSIGN(ctx, sizeSigned, rdxMakeSigned(ctx, size));
				if(!rdxCheckAddOverflowS(localOffset, -sizeSigned))
					RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
				localOffset -= sizeSigned;
				
				rdxLargeInt alignSigned;
				RDX_PROTECT_ASSIGN(ctx, alignSigned, rdxMakeSigned(ctx, align));
				rdxLargeInt alignMask = ~(alignSigned - 1);

				localOffset &= alignMask;

				if(localOffset < lowestOffset)
					lowestOffset = localOffset;
				vl.offset = localOffset;

				RDX_VERBOSE( wprintf(L"Local %i: %s\n", vl.offset, GCInfo::From(vl.vType)->gstSymbol->_native.characters) );
			}


			rdxSVStackJournal sj;
			sj.startInstruction = instrNum;
			sj.record = false;
			sj.vType = t;
			sj.offset = vl.offset;
			sj.vstType = rdxVST_Value;
			sj.sequentialID = sequentialID++;

			rdxSMILStackAction sa;
			if(isParameter)
				sa.actionType = rdxEMILSAT_PushParentFrameParameter;
			else
				sa.actionType = rdxEMILSAT_PushLocal;
			sa.valueType = t;

			RDX_PROTECT(ctx, stackjournal->Push(ctx, sj) );
			RDX_PROTECT(ctx, localstack->Push(ctx, vl) );
			RDX_PROTECT(ctx, stackactions->Push(ctx, sa) );
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROW(ctx);
		}
		RDX_ENDTRY
	}

	rdxSVLocal PopLocal(rdxSOperationContext *ctx)
	{
		RDX_TRY(ctx)
		{
			rdxSVLocal vl;
			RDX_PROTECT_ASSIGN(ctx, vl, localstack->Pop(ctx) );

			rdxSVStackJournal vj;
			RDX_PROTECT_ASSIGN(ctx, vj, stackjournal->Pop(ctx) );
			
			rdxSMILStackAction sa;
			sa.actionType = rdxEMILSAT_Pop;
			sa.valueType = rdxWeakHdl(rdxCType)::Null();

			RDX_PROTECT(ctx, stackactions->Push(ctx, sa) );

			if(pass > rdxPASS_CreateStacks)
			{
				if(vj.record)
				{
					rdxSStackJournal sj;
					sj.startInstruction = vj.startInstruction;
					sj.endInstruction = instrNum;
					sj.offset = vj.offset;
					sj.vType = vj.vType;
					sj.isPointer = false;
					sj.isVarying = false;
					sj.isParameter = false;
					sj.isFromLocal = false;
					sj.isNotNull = false;
					sj.notNullInstruction = 0;
					sj.isConstant = false;
					sj.name = vl.name;
					sj.pointerSource = 0;
					sj.pointerSourceType = rdxPST_Invalid;

					RDX_PROTECT(ctx, journal->Push(ctx, sj) );
				}
			}

			return vl;
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROWV(ctx, rdxSVLocal());
		}
		RDX_ENDTRY
	}

	// Synchronizes the journal with the current opstack and localstack state.  Anything on the opstack at the time of call
	// can be traced.  This should be done on any instruction that can suspend the thread or throw an exception.
	void SyncJournal(rdxSOperationContext *ctx)
	{
		RDX_TRY(ctx)
		{
			if(stackjournal->list.IsNotNull())
			{
				for(rdxLargeUInt i=0;i<stackjournal->count;i++)
				{
					rdxSVStackJournal vj = stackjournal->RetrieveBottom(ctx, i);
					vj.record = true;
					stackjournal->ReinsertBottom(ctx, i, vj);
				}
			}
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROW(ctx);
		}
		RDX_ENDTRY
	}

	static bool TypeIsRefStruct(rdxIObjectManager *objm, rdxWeakHdl(rdxCType) t)
	{
		if(t.IsNull() || t->ObjectInfo()->containerType != objm->GetBuiltIns()->st_StructuredType)
			return false;
		return t.StaticCast<rdxCStructuredType>()->storageSpecifier == rdxSS_RefStruct;
	}

	rdxLargeUInt FindJumpTarget(rdxLargeUInt ilinstr)
	{
		if(pass < rdxPASS_StitchJumps)
			return ilinstr;

		rdxWeakOffsetHdl(rdxSILInstruction) instr = instructions->list->OffsetElementRTRef(ilinstr).ToHdl();
		// Skip debuginfo instructions
		while(instr->opcode == rdxILOP_debuginfo)
		{
			ilinstr++;
			instr++;
		}
		return ilinstr;
	}

	rdxLargeUInt StitchJump(rdxSOperationContext *ctx, rdxLargeUInt startInstr, rdxLargeUInt destInstr)
	{
		if(pass < rdxPASS_StitchJumps)
			return 0;

		RDX_TRY(ctx)
		{
			if(destInstr >= metalist->NumElements())
				RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_JUMP_TARGET);
			rdxWeakOffsetHdl(rdxSInstructionMeta) meta = metalist->OffsetElementRTRef(destInstr).ToHdl();
			if(startInstr < meta->minJumpRange || startInstr > meta->maxJumpRange || !meta->validJumpTarget)
				RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_JUMP_TARGET);

			return FindJumpTarget(meta->translatedInstrNum);
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROWV(ctx, 0);
		}
		RDX_ENDTRY
	}

	static bool OpcodeIsResumable(rdxEILOpcode opcode)
	{
		for(rdxLargeInt i=0;i<sizeof(RDX_RESUMABLE_OPCODES)/sizeof(RDX_RESUMABLE_OPCODES[0]);i++)
			if(RDX_RESUMABLE_OPCODES[i] == opcode)
				return true;
		return false;
	}

	template<class Ti>
	inline static bool DecodeInt(rdxWeakOffsetRTRef(rdxUInt8) &bytecode, rdxLargeUInt &bytesRemaining, Ti &out)
	{
		if(!bytesRemaining)
			return false;

		rdxUInt8 b = (bytecode++).Data()[0];
		bytesRemaining--;

		Ti v;
		v = (b & 0x7f);
		if(b & 0x40)
			v = (b & 0x3f) - 64;

		while(b & 0x80)
		{
			if(!bytesRemaining)
				return false;
			b = (bytecode++).Data()[0];
			bytesRemaining--;
			v = static_cast<Ti>((v << 7) | (b & 0x7f));
		}
		out = v;
		return true;
	}

	void CompilePass(rdxSOperationContext *ctx)
	{
		RDX_TRY(ctx)
		{
			rdxLargeUInt numDecodedInstructions = method->numInstructions;
			rdxLargeUInt currentEHLastInstruction = 0;		// The last instruction covered by an exception handler.  Used to determine if tail call optimizations are possible.
			bool ehLastInstructionSet = false;

			rdxLargeUInt numMethodParameters = 0;
			rdxLargeUInt numMethodReturnTypes = 0;
			rdxUInt8 resumeFlags = 0;
			int resumeFlagCount = 0;
			bool makeResumable = false;
			rdxLargeUInt lastDebugInfoInstruction = 0;
			bool lastDebugInfoInstructionSet = false;

			if(method->parameters.IsNotNull())
				numMethodParameters = method->parameters->NumElements();
			if(method->returnTypes.IsNotNull())
				numMethodReturnTypes = method->returnTypes->NumElements();

			for(rdxLargeUInt i=0;i<numMethodReturnTypes;i++)
			{
				if(method->returnTypes->Element(i) == objm->GetBuiltIns()->st_Varying)
					RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_VARYING_USAGE);
				if(method->returnTypes->Element(i).IsNull())
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				rdxSMILStackAction sa;
				sa.actionType = rdxEMILSAT_PushParentFrameReturnValue;
				sa.valueType = method->returnTypes->Element(i);
				RDX_PROTECT(ctx, stackactions->Push(ctx, sa));
			}

			// CAUTION: The local stack expects parameters to be first in numerous places, i.e. AppendLocal and tail call checks
			for(rdxLargeUInt i=0;i<numMethodParameters;i++)
			{
				rdxWeakOffsetRTRef(rdxSMethodParameter) param = method->parameters->OffsetElementRTRef(i);

				// This should be caught earlier, but be safe...
				if(param->type.IsNull())
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				bool isPointer = TypeIsRefStruct(objm, param->type.ToWeakHdl());

				if(param->type == objm->GetBuiltIns()->st_Varying)
					RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_VARYING_USAGE);

				// NotNull parameters must be constant
				if(param->isNotNull != rdxFalseValue && param->isConstant == rdxFalseValue)
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

				RDX_PROTECT(ctx, AppendLocal(ctx, param->type.ToWeakHdl(), 0, rdxWeakHdl(rdxCString)::Null(), isPointer, true, (param->isConstant != rdxFalseValue), (param->isNotNull != rdxFalseValue) ) );

				{
					rdxSStackJournal sj;
					sj.isParameter = true;
					sj.isFromLocal = false;
					sj.isVarying = false;
					sj.startInstruction = 0;
					sj.endInstruction = 0;
					sj.isPointer = isPointer;
					sj.offset = localstack->RetrieveTop(ctx, 0).parameterOffset;
					sj.pointerSource = sj.offset;
					sj.pointerSourceType = rdxPST_Parameter;
					sj.vType = param->type.ToWeakHdl();
					sj.name = rdxWeakHdl(rdxCString)::Null();
					sj.isConstant = (param->isConstant != rdxFalseValue);
					sj.isNotNull = (param->isNotNull != rdxFalseValue);
					sj.notNullInstruction = 0;

					RDX_PROTECT(ctx, journal->Push(ctx, sj) );

					if(i == method->thisParameterOffset - 1)
						method->m_native.thisParameterInvokeOffset = sj.offset;
				}
			}

			rdxLargeUInt numFileInfos = 0;
			rdxLargeUInt currentFileInfo = 0;
			rdxWeakArrayHdl(rdxSInstructionFileInfo) firstFileInfo = method->instructionFileInfos.ToWeakHdl();
			if(firstFileInfo.IsNotNull())
				numFileInfos = firstFileInfo->NumElements();

			instrNumIL = 0;
			for(instrNum=0;instrNum<numDecodedInstructions;instrNum++)
			{
				rdxWeakOffsetHdl(rdxSILDecodedOp) decOp = decOps->OffsetElementRTRef(instrNum).ToHdl();
				rdxWeakOffsetHdl(rdxSInstructionMeta) meta = metalist->OffsetElementRTRef(instrNum).ToHdl();

				{
					rdxSInstructionMeta *metaView = meta.Modify();

					metaView->translatedInstrNum = instructions->count;
					metaView->numValidationInstrs = 0;

					if(pass < rdxPASS_StitchJumps)
					{
						metaView->minJumpRange = 0;
						metaView->maxJumpRange = numDecodedInstructions - 1;
					}

					metaView->validJumpTarget = false;
					if(pass > rdxPASS_CreateStacks)
						if(opstack->count == 0 || opstack->RetrieveTop(ctx, 0).vstType == rdxVST_Barrier)
							metaView->validJumpTarget = true;
				}

				if(currentFileInfo != numFileInfos && firstFileInfo->Element(currentFileInfo).firstInstruction == instrNum)
				{
					if(instructions->count == lastDebugInfoInstruction && lastDebugInfoInstructionSet)
					{
						// Last instruction was a debuginfo, just remap it
						rdxUILOpCompactArg opArg0, opArg1;
						RDX_PROTECT_ASSIGN(ctx, opArg1, compactArgs->Pop(ctx));
						RDX_PROTECT_ASSIGN(ctx, opArg0, compactArgs->Pop(ctx));
						RDX_PROTECT(ctx, compactArgs->Push(ctx, opArg0));
						RDX_PROTECT(ctx, compactArgs->Push(ctx, opArg1));
					}
					else
					{
						rdxSILInstruction ili;
						bool prevResumable = makeResumable;
						ili.opcode = rdxILOP_debuginfo;

						rdxUILOpCompactArg opArg0, opArg1;

						opArg0.p = firstFileInfo->Element(currentFileInfo).filename.ToWeakHdl().GetPOD();
						opArg1.lui = firstFileInfo->Element(currentFileInfo).line;

						metalist->Element(instrNum).translatedInstrNum++;

						PUSH_INSTR_CA_2(ili, opArg0, opArg1);
						makeResumable = prevResumable;					// Carry through resumability
					}

					currentFileInfo++;
					lastDebugInfoInstruction = instructions->count;
					lastDebugInfoInstructionSet = true;
				}

				switch(decOp->opcode)
				{
				case rdxOP_startbarrier:
					{
						// Previous stack values must be shells or local pointers
						if(pass > rdxPASS_CreateStacks)
						{
							if(decOp->sint1 < 1)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							for(rdxLargeUInt i=0;i<static_cast<rdxLargeUInt>(decOp->sint1);i++)
							{
								rdxSVStackValue vst;
								RDX_PROTECT_ASSIGN(ctx, vst, opstack->RetrieveTop(ctx, i) );

								bool expectPointer = TypeIsRefStruct(objm, vst.vType);
								// Pointers need to be to locals, everything else must be a shell
								if( (expectPointer && (vst.vstType != rdxVST_Pointer || !vst.isFromLocal)) &&
									(!expectPointer && vst.vstType != rdxVST_ValueShell) )
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								// The validator won't consider these unshelled until the stack is flushed,
								// but midcode needs to treat them as unshelled immediately so the destinations
								// will be considered live.
								if(vst.vstType == rdxVST_ValueShell)
								{
									rdxSMarkupInstruction mi;
									mi.opcode = rdxMOP_unshell;

									rdxUILOpCompactArg opArg0;
									opArg0.li = vst.stackAction;

									PUSH_MARKUP_CA_1(mi, opArg0);
								}
							}
						}

						rdxSVStackValue vst;
						vst.vstType = rdxVST_Barrier;
						vst.vType = rdxWeakHdl(rdxCType)::Null();
						vst.barrierValueCount = static_cast<rdxLargeUInt>(decOp->sint1);
						vst.creatingInstruction = instrNum;
						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );
					}
					break;
				case rdxOP_endbarrier:
					{
						rdxSVStackValue vst;
						RDX_PROTECT_ASSIGN(ctx, vst, PopOpstack(ctx, rdxVST_Barrier) );

						if(pass > rdxPASS_CreateStacks)
						{
							if(instrNum == 0 || decOps->Element(instrNum-1).opcode != rdxOP_return)
								RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);

							{
								for(rdxLargeUInt i=vst.creatingInstruction;i<instrNum;i++)
								{
									rdxSInstructionMeta &meta = metalist->Element(i);
									if(meta.minJumpRange < vst.creatingInstruction)
										meta.minJumpRange = vst.creatingInstruction;
									if(meta.maxJumpRange > instrNum)
										meta.maxJumpRange = instrNum;
								}
							}

							// Emit a stack flush and deshell anything beyond the barrier
							{
								rdxSILInstruction ili;
								ili.opcode = rdxILOP_hardenstack;
								PUSH_INSTR(ili);
							}

							rdxSVStackValue rvVst;
							for(rdxLargeUInt i=0;i<vst.barrierValueCount;i++)
							{
								RDX_PROTECT_ASSIGN(ctx, rvVst, opstack->RetrieveTop(ctx, i) );
								if(rvVst.vstType == rdxVST_ValueShell)
								{
									RDX_PROTECT(ctx, LivenShell(ctx, rvVst) );
									RDX_PROTECT(ctx, opstack->ReinsertTop(ctx, i, rvVst) );
								}
							}
						}
					}
					break;
				case rdxOP_throw:
					{
						if(opstack->count != 1)
							RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);

						RDX_PROTECT(ctx, SyncJournal(ctx) );

						rdxSVStackValue vst;
						RDX_PROTECT_ASSIGN(ctx, vst, PopOpstack(ctx) );

						if(pass > rdxPASS_CreateStacks)
						{
							if(vst.vType.IsNull() || !objm->TypesCompatible(vst.vType.ToWeakRTRef(), st_Exception.ToWeakRTRef()))
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxSILInstruction ili;
							ili.opcode = rdxILOP_throw;

							rdxUILOpCompactArg opArg0;
							opArg0.lui = vst.stackAction;

							PUSH_INSTR_CA_1(ili, opArg0);
						}
					}
					break;
				case rdxOP_try:
					{
						// Catch label must be a "catch" that catches Core.Exception
						// Opstack must be empty and must not be within a barrier
						// Span must be positive and non-zero

						if(opstack->count)
							RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);

						rdxLargeInt instrNumS;
						RDX_PROTECT_ASSIGN(ctx, instrNumS, rdxMakeSigned(ctx, instrNum));

						if(!rdxCheckAddOverflowS(instrNumS, decOp->sint2))
							RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
						rdxLargeInt spanEndInstructionS = instrNumS + decOp->sint2;

						if(decOp->sint2 < 1)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						if(!rdxCheckAddOverflowS(instrNumS, decOp->sint1))
							RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
						rdxLargeInt catchInstructionS = instrNumS + decOp->sint1;

						if(spanEndInstructionS < 0 || catchInstructionS < 0)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
						rdxLargeUInt spanEndInstruction = static_cast<rdxLargeUInt>(spanEndInstructionS);
						rdxLargeUInt catchInstruction = static_cast<rdxLargeUInt>(catchInstructionS);

						if(spanEndInstruction >= numDecodedInstructions || catchInstruction >= numDecodedInstructions)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						if(pass > rdxPASS_CreateStacks)
						{
							if(decOps->Element(catchInstruction).opcode != rdxOP_catch ||
								decOps->Element(catchInstruction).res != this->st_Exception)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_JUMP_TARGET);
						}

						if(catchInstruction == numDecodedInstructions-1)
							RDX_STHROW(ctx, RDX_ERROR_IL_STACKS_UNBALANCED);	// This should get caught by the balancer?

						rdxSExceptionHandlerJournal ehj;
						ehj.startInstruction = instrNum + 1;
						ehj.endInstruction = spanEndInstruction - 1;
						ehj.handlerInstruction = catchInstruction + 1;	// The actual catch instruction is poisonous

						if(!ehLastInstructionSet || ehj.endInstruction > currentEHLastInstruction)
						{
							currentEHLastInstruction = ehj.endInstruction;
							ehLastInstructionSet = true;
						}

						// The catch and end both need to be reachable from here even if not used
						RDX_PROTECT(ctx, StitchJump(ctx, instrNum, spanEndInstruction) );
						RDX_PROTECT(ctx, StitchJump(ctx, instrNum, catchInstruction) );

						RDX_PROTECT(ctx, exHandlers->Push(ctx, ehj) );
					}
					break;
				case rdxOP_catch:
					{
						// Opstack must be empty and must not be within a barrier
						if(opstack->count)
							RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);

						rdxSVStackValue vst;
						vst.vstType = rdxVST_Value;
						
						if(decOp->res.IsNull() ||
							!objm->TypesCompatible(decOp->res->ObjectInfo()->containerType.ToWeakRTRef(), objm->GetBuiltIns()->st_Type.ToWeakRTRef()))
						RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxWeakHdl(rdxCType) t = decOp->res.StaticCast<rdxCType>();
						if(!objm->TypesCompatible(t.ToWeakRTRef(), st_Exception.ToWeakRTRef()))
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
						vst.vType = t;

						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

						rdxSILInstruction ili;
						ili.opcode = rdxILOP_catch;

						PUSH_INSTR(ili);

						rdxSExceptionRecoveryLocation erl;
						erl.instrNum = instrNum;
						erl.bpOffset = vst.offset;
						RDX_PROTECT(ctx, exRLs->Push(ctx, erl));
					}
					break;
				case rdxOP_trycast:
					{
						// Instruction at target label must be a "catch" of the same type as the trycast
						// Opstack must contain only the value being cast and must not be within a barrier

						if(opstack->count != 1)
							RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);

						RDX_PROTECT(ctx, SyncJournal(ctx) );

						SAFE_DECODE_LABEL(catchInstruction, sint1);

						if(decOp->res.IsNull() ||
							!objm->TypesCompatible(decOp->res->ObjectInfo()->containerType.ToWeakRTRef(), objm->GetBuiltIns()->st_Type.ToWeakRTRef()))
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxWeakHdl(rdxCType) ext = decOp->res.StaticCast<rdxCType>();
						if(!objm->TypesCompatible(ext.ToWeakRTRef(), st_Exception.ToWeakRTRef()))
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						if(pass > rdxPASS_CreateStacks)
						{
							if(decOps->Element(catchInstruction).opcode != rdxOP_catch ||
								decOps->Element(catchInstruction).res != ext)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_JUMP_TARGET);
						}

						if(catchInstruction == numDecodedInstructions-1)
							RDX_STHROW(ctx, RDX_ERROR_IL_STACKS_UNBALANCED);	// This should get caught by the balancer?

						rdxSVStackValue vst;
						RDX_PROTECT_ASSIGN(ctx, vst, opstack->RetrieveTop(ctx, 0) );

						if(pass > rdxPASS_CreateStacks)
						{
							if(vst.vstType != rdxVST_Value || vst.vType != st_Exception)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0;
							rdxUILOpCompactArg opArg1;
							rdxUILOpCompactArg opArg2;
							if(ext == st_Exception)		// Compiler's a bit stupid about this, optimize
							{
								ili.opcode = rdxILOP_jump;
							}
							else
							{
								ili.opcode = rdxILOP_jinherits;
								opArg2.p = ext.GetPOD();
							}

							// Catch needs to be reachable
							RDX_PROTECT(ctx, StitchJump(ctx, instrNum, catchInstruction) );

							RDX_PROTECT_ASSIGN(ctx, opArg0.lui, FindJumpTarget(metalist->Element(catchInstruction + 1).translatedInstrNum) );
							opArg1.lui = vst.stackAction;

							if(ext == st_Exception)
								PUSH_INSTR_CA_1(ili, opArg0);					// ILOP_jump
							else
								PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);	// ILOP_jinherits
						}
					}
					break;
				case rdxOP_jump:
					{
						if(pass > rdxPASS_CreateStacks)
						{
							// Stack must be empty or with only a barrier remaining
							if(opstack->count != 0 && opstack->RetrieveTop(ctx, 0).vstType != rdxVST_Barrier)
								RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);

							SAFE_DECODE_LABEL(targetInstruction, sint1);

							RDX_PROTECT(ctx, SyncJournal(ctx) );

							if(decOp->sint1 <= 0)
							{
								rdxSILInstruction tickILI;
#ifdef RDX_ENABLE_TIMESLICE_COUNTER
								tickILI.opcode = rdxILOP_tick;
#else
								tickILI.opcode = rdxILOP_hardenstack;
#endif
								PUSH_INSTR(tickILI);
								meta->translatedInstrNum++;
							}

							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0;
							ili.opcode = rdxILOP_jump;
							RDX_PROTECT_ASSIGN(ctx, opArg0.lui, StitchJump(ctx, instrNum, targetInstruction) );

							PUSH_INSTR_CA_1(ili, opArg0);
						}
					}
					break;
				case rdxOP_jumpif:
				case rdxOP_jumpifnot:
					{
						// NOTE: Demotion is not implemented for this because currently, no jumping intrinsics modify parameters
						if(decOp->res.IsNull() || decOp->res->ObjectInfo()->containerType != objm->GetBuiltIns()->st_Method)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxWeakHdl(rdxCMethod) cmethod = decOp->res.StaticCast<rdxCMethod>();
						cmethod->DetermineIntrinsicState();
						if(!cmethod->m_native.isBranching)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						if(decOp->sint1 <= 0)
						{
							rdxSILInstruction tickILI;
#ifdef RDX_ENABLE_TIMESLICE_COUNTER
							tickILI.opcode = rdxILOP_tick;
#else
							tickILI.opcode = rdxILOP_hardenstack;
#endif
							PUSH_INSTR(tickILI);
							meta->translatedInstrNum++;
						}

						RDX_PROTECT(ctx, SyncJournal(ctx) );	// Sync now before parameters get removed

						rdxWeakArrayHdl(rdxSMethodParameter) params = cmethod->parameters.ToWeakHdl();
						rdxLargeUInt numParams = 0;
						if(params.IsNotNull())
							numParams = params->NumElements();

						rdxLargeInt paramLocation = 0;
						rdxLargeUInt paramStackAction = 0;
						if(opstack->count)
						{
							paramLocation = opstack->RetrieveTop(ctx, 0).offset;
							paramStackAction = opstack->RetrieveTop(ctx, 0).stackAction;
						}

						// Method signature must match, values must be of the expected type (i.e. POINTER to STRUCT is not allowed)
						for(rdxLargeUInt i=0;i<numParams;i++)
						{
							rdxLargeUInt pi = numParams - 1 - i;
							rdxEVSTType expectedType = rdxVST_Value;
							if(params->Element(pi).type.IsNull())
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

							if(params->Element(pi).type == objm->GetBuiltIns()->st_Varying)
								expectedType = rdxVST_Varying;
							else if(TypeIsRefStruct(objm, params->Element(pi).type.ToWeakHdl()))
							{
								if(params->Element(pi).isConstant)
									expectedType = rdxVST_ConstPointer;
								else
									expectedType = rdxVST_Pointer;
							}

							rdxSVStackValue paramVST;
							RDX_PROTECT_ASSIGN(ctx, paramVST, PopOpstack(ctx, expectedType) );

							if(pass > rdxPASS_CreateStacks)
							{
								if(!objm->TypesCompatible(paramVST.vType.ToWeakRTRef(), params->Element(pi).type.ToWeakRTRef()))
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							}
						}

						// Stack needs to be clear or a barrier after params
						if(pass > rdxPASS_CreateStacks)
						{
							if(opstack->count != 0)
							{
								rdxSVStackValue stackTop = opstack->RetrieveTop(ctx, 0);
								if(stackTop.vstType != rdxVST_Barrier)
									RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);
							}
						}

						SAFE_DECODE_LABEL(targetInstruction, sint1);

						rdxSILInstruction ili;
						rdxUILOpCompactArg opArg0, opArg1, opArg2;
						ili.opcode = rdxILOP_jump;
						RDX_PROTECT_ASSIGN(ctx, opArg0.lui, StitchJump(ctx, instrNum, targetInstruction) );
						opArg1.lui = paramStackAction;
						opArg2.li = cmethod->m_native.p1;

						if(decOp->opcode == rdxOP_jumpif)
							ili.opcode = cmethod->m_native.opcode;
						else
							ili.opcode = cmethod->m_native.falseCheckOpcode;

						PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
					}
					break;
				case rdxOP_jumpiftrue:
				case rdxOP_jumpiffalse:
				case rdxOP_jumpifnull:
				case rdxOP_jumpifnotnull:
					{
						RDX_PROTECT(ctx, SyncJournal(ctx) );

						if(decOp->sint1 <= 0)
						{
							rdxSILInstruction tickILI;
#ifdef RDX_ENABLE_TIMESLICE_COUNTER
							tickILI.opcode = rdxILOP_tick;
#else
							tickILI.opcode = rdxILOP_hardenstack;
#endif
							PUSH_INSTR(tickILI);
							meta->translatedInstrNum++;
						}

						rdxSVStackValue bvVst;
						RDX_PROTECT_ASSIGN(ctx, bvVst, PopOpstack(ctx, rdxVST_Value) );

						if(pass > rdxPASS_CreateStacks)
						{
							if(opstack->count != 0 && opstack->RetrieveTop(ctx, 0).vstType != rdxVST_Barrier)
								RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);

							if(decOp->opcode == rdxOP_jumpiftrue || decOp->opcode == rdxOP_jumpiffalse)
							{
								if(bvVst.vType != objm->GetBuiltIns()->st_Bool)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							}
							else if(decOp->opcode == rdxOP_jumpifnull || decOp->opcode == rdxOP_jumpifnotnull)
							{
								if(!objm->TypeIsObjectReference(bvVst.vType) &&
									!objm->TypeIsInterface(bvVst.vType))
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							}

							SAFE_DECODE_LABEL(targetInstruction, sint1);

							rdxSILInstruction ili;
							if(decOp->opcode == rdxOP_jumpiftrue)
								ili.opcode = rdxILOP_jtrue;
							else if(decOp->opcode == rdxOP_jumpiffalse)
								ili.opcode = rdxILOP_jfalse;
							else if(decOp->opcode == rdxOP_jumpifnull)
							{
								if(objm->TypeIsObjectReference(bvVst.vType))
									ili.opcode = rdxILOP_jnullo;
								else if(objm->TypeIsInterface(bvVst.vType))
									ili.opcode = rdxILOP_jnulli;
								else
									RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
							}
							else if(decOp->opcode == rdxOP_jumpifnotnull)
							{
								if(objm->TypeIsObjectReference(bvVst.vType))
									ili.opcode = rdxILOP_jnotnullo;
								else if(objm->TypeIsInterface(bvVst.vType))
									ili.opcode = rdxILOP_jnotnulli;
								else
									RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
							}
							else
								RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);

							rdxUILOpCompactArg opArg0, opArg1;
							RDX_PROTECT_ASSIGN(ctx, opArg0.lui, StitchJump(ctx, instrNum, targetInstruction) );
							opArg1.lui = bvVst.stackAction;

							PUSH_INSTR_CA_2(ili, opArg0, opArg1);
						}
					}
					break;
				case rdxOP_jumpifequal:
				case rdxOP_jumpifnotequal:
					{
						RDX_PROTECT(ctx, SyncJournal(ctx) );

						rdxSVStackValue vst1, vst2;
						RDX_PROTECT_ASSIGN(ctx, vst1, PopOpstack(ctx, rdxVST_Indeterminate) );
						RDX_PROTECT_ASSIGN(ctx, vst2, PopOpstack(ctx, rdxVST_Indeterminate) );

						if(pass > rdxPASS_CreateStacks)
						{
							if(vst1.vstType != rdxVST_Value && !VSTIsPointer(vst1.vstType))
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							if(vst2.vstType != rdxVST_Value && !VSTIsPointer(vst2.vstType))
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							if(!objm->TypesCompatible(vst1.vType.ToWeakRTRef(), vst2.vType.ToWeakRTRef()) && !objm->TypesCompatible(vst2.vType.ToWeakRTRef(), vst1.vType.ToWeakRTRef()))
								RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_CONVERSION);

							bool expectRef = TypeIsRefStruct(objm, vst1.vType);
							if(expectRef && (!VSTIsPointer(vst1.vstType) || !VSTIsPointer(vst2.vstType)))
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							if(!expectRef && (vst1.vstType != rdxVST_Value || vst2.vstType != rdxVST_Value))
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							SAFE_DECODE_LABEL(targetInstruction, sint1);

							rdxLargeUInt size, align;
							RDX_PROTECT(ctx, objm->TypeValueSize(ctx, vst1.vType, size, align) );

							rdxSILInstruction ili;
							if(decOp->opcode == rdxOP_jumpifequal)
							{
								if(expectRef)
									ili.opcode = rdxILOP_jeq_p;
								else
									ili.opcode = rdxILOP_jeq_f;
							}
							else
							{
								if(expectRef)
									ili.opcode = rdxILOP_jne_p;
								else
									ili.opcode = rdxILOP_jne_f;
							}
							rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
							RDX_PROTECT_ASSIGN(ctx, opArg0.lui, StitchJump(ctx, instrNum, targetInstruction) );
							opArg1.lui = vst1.stackAction;
							opArg2.lui = vst2.stackAction;
							opArg3.p = vst1.vType.GetPOD();

							PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
						}
					}
					break;
				case rdxOP_callvirtual:
				case rdxOP_call:
				case rdxOP_calldelegate:
					{
						rdxWeakArrayHdl(rdxSMethodParameter) params = rdxWeakArrayHdl(rdxSMethodParameter)::Null();
						rdxWeakArrayHdl(rdxTracedRTRef(rdxCType)) returnTypes = rdxWeakArrayHdl(rdxTracedRTRef(rdxCType))::Null();
						rdxWeakHdl(rdxCMethod) calledMethod = rdxWeakHdl(rdxCMethod)::Null();
						rdxWeakHdl(rdxCDelegateType) dt = rdxWeakHdl(rdxCDelegateType)::Null();
						rdxWeakHdl(rdxCStructuredType) bdt = rdxWeakHdl(rdxCStructuredType)::Null();

						if(decOp->opcode == rdxOP_call || decOp->opcode == rdxOP_callvirtual)
						{
							calledMethod = decOp->res.StaticCast<rdxCMethod>();

							if(calledMethod.IsNull() || calledMethod->ObjectInfo()->containerType != objm->GetBuiltIns()->st_Method)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							calledMethod->DetermineIntrinsicState();
							if(!calledMethod->m_native.neverFails)
								RDX_PROTECT(ctx, SyncJournal(ctx) );	// Sync now before parameters get removed
							params = calledMethod->parameters.ToWeakHdl();
							returnTypes = calledMethod->returnTypes.ToWeakHdl();

							// Handle timeouts if this is not an intrinsic
							if(decOp->opcode != rdxOP_call || !calledMethod->m_native.isIntrinsic)
							{
								rdxSILInstruction tickILI;
								tickILI.opcode = rdxILOP_tick;
								PUSH_INSTR(tickILI);
								meta->translatedInstrNum++;
							}
						}
						else if(decOp->opcode == rdxOP_calldelegate)
						{
							// Delegate
							RDX_PROTECT(ctx, SyncJournal(ctx) );

							{
								rdxSILInstruction tickILI;
#ifdef RDX_ENABLE_TIMESLICE_COUNTER
								tickILI.opcode = rdxILOP_tick;
#else
								tickILI.opcode = rdxILOP_hardenstack;
#endif
								PUSH_INSTR(tickILI);
								meta->translatedInstrNum++;
							}

							if(decOp->res.IsNull())
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							if(decOp->res->ObjectInfo()->containerType == objm->GetBuiltIns()->st_DelegateType)
							{
								dt = decOp->res.StaticCast<rdxCDelegateType>();
								params = dt->parameters.ToWeakHdl();
								returnTypes = dt->returnTypes.ToWeakHdl();
							}
							else
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
						}
						else
							RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);

						rdxLargeUInt numParams = 0;
						if(params.IsNotNull())
							numParams = params->NumElements();

						rdxLargeUInt numReturns = 0;
						if(returnTypes.IsNotNull())
							numReturns = returnTypes->NumElements();

						rdxLargeInt paramLocation = 0;
						rdxLargeUInt paramStackAction = 0;
						if(opstack->count)
						{
							paramLocation = opstack->RetrieveTop(ctx, 0).offset;
							paramStackAction = opstack->RetrieveTop(ctx, 0).stackAction;
						}

						rdxLargeInt thisLocation = 0;
						rdxLargeUInt thisStackAction = 0;
						rdxWeakHdl(rdxCType) thisType = rdxWeakHdl(rdxCType)::Null();
						rdxLargeUInt vftOffset = 0;
						bool taggedThis = false;

						rdxSVStackValue thisVst;

						rdxLargeUInt parameterOpstackUsed = 0;

						if(pass > rdxPASS_CreateStacks)
						{
							for(rdxLargeUInt i=0;i<numParams;i++)
							{
								rdxSVStackValue paramVST;
								rdxLargeUInt paramStackOffset = numParams - 1 - i;
								RDX_PROTECT_ASSIGN(ctx, paramVST, opstack->RetrieveTop(ctx, paramStackOffset));

								if(params->Element(i).isNotNull && !paramVST.isNotNull)
								{
									if(!objm->TypeIsObjectReference(params->Element(i).type.ToWeakRTRef()) &&
										!objm->TypeIsInterface(params->Element(i).type.ToWeakRTRef()))
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

									paramVST.notNullInstruction = instrNumIL;
									paramVST.isNotNull = true;
									RDX_PROTECT(ctx, opstack->ReinsertTop(ctx, paramStackOffset, paramVST));

									rdxSILInstruction ili;
									ili.opcode = rdxILOP_verifynotnull;
									rdxUILOpCompactArg opArg0, opArg1;
									opArg0.lui = paramVST.stackAction;
									opArg1.lui = i;
									PUSH_INSTR_CA_2(ili, opArg0, opArg1);

									meta->numValidationInstrs++;
								}

								// If the type isn't an exact match, enforce compatibility and demote

								// Explicit demotion is necessary because otherwise, a value could be implicitly demoted when
								// passed to a function, and then promoted by the function to a subtype.  Because the journals
								// are still active in the parent frame, this can cause issues with the debug API and thread
								// deserialization.  We have to split the journal to prevent the value from being occupied earlier
								// with a weaker value earlier in its lifetime and then used by a different non-consuming operation
								// like OP_clone
								if(params->Element(i).type != paramVST.vType && params->Element(i).type != objm->GetBuiltIns()->st_Varying)
								{
									if(!objm->TypesCompatible(paramVST.vType.ToWeakRTRef(), params->Element(i).type.ToWeakRTRef()))
										RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_CONVERSION);

									RDX_PROTECT(ctx, CallConvertOpstack(ctx, paramStackOffset, params->Element(i).type.ToWeakHdl()));
								}
							}
						}

						bool paramsValidForTailCall = true;

						// Method signature must match, values must be of the expected type (i.e. POINTER to STRUCT is not allowed)
						for(rdxLargeUInt i=0;i<numParams;i++)
						{
							rdxLargeUInt pi = numParams - 1 - i;
							rdxEVSTType expectedType = rdxVST_Value;
							if(params->Element(pi).type.IsNull())
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

							if(params->Element(pi).type == objm->GetBuiltIns()->st_Varying)
								expectedType = rdxVST_Varying;
							else if(TypeIsRefStruct(objm, params->Element(pi).type.ToWeakHdl()))
							{
								if(params->Element(pi).isConstant)
									expectedType = rdxVST_ConstPointer;
								else
									expectedType = rdxVST_Pointer;
							}

							rdxSVStackValue paramVST;
							RDX_PROTECT_ASSIGN(ctx, paramVST, PopOpstack(ctx, expectedType) );

							if(calledMethod.IsNotNull() && pi == (calledMethod->thisParameterOffset - 1))
							{
								thisLocation = paramVST.offset;
								thisStackAction = paramVST.stackAction;
								thisType = paramVST.vType;
								taggedThis = true;
							}

							if(pass > rdxPASS_CreateStacks)
							{
								if(expectedType != rdxVST_Varying && paramVST.vType != params->Element(pi).type)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								parameterOpstackUsed += paramVST.size;
							}

#if defined(RDX_JOURNAL_ALL_VALUES) && defined(RDX_TAIL_CALL_OPTIMIZATION)
							if(calledMethod.IsNull() || calledMethod != method)
								paramsValidForTailCall = false;
							else
							{
								// If all values are journaled, then tail callable values are limited to ones that will be valid in the calling frame
								// "this" parameter needs to be pass-through because the parent may have invoked this as a virtual call and replacing it could cause the call point validator to reject it
								// Reference structs need to be pass-through because they may be anchored to a local otherwise
								if(pi == method->thisParameterOffset - 1 || TypeIsRefStruct(objm, params->Element(pi).type.ToWeakHdl()))
								{
									if(!paramVST.isLoadedFromLocal || paramVST.loadedLocal != pi)
										paramsValidForTailCall = false;
								}
							}
#endif
						}

						bool delegateFromParameter = false;
						rdxLargeInt delegateSourceOffset = 0;
						rdxLargeUInt delegateSourceStackAction = 0;
						if(decOp->opcode == rdxOP_calldelegate)
						{
							rdxSVStackValue delegateVST;
							RDX_PROTECT_ASSIGN(ctx, delegateVST, PopOpstack(ctx, rdxVST_LocalRef) );
							rdxSVLocal vl = localstack->RetrieveBottom(ctx, delegateVST.index);
							if(vl.isParameter)
							{
								delegateSourceOffset = vl.parameterOffset;
								delegateSourceStackAction = vl.stackAction;
								delegateFromParameter = true;
								if(vl.isPointer)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							}
							else
							{
								delegateSourceOffset = vl.offset;
								delegateFromParameter = false;
							}
							if(pass > rdxPASS_CreateStacks && vl.vType != decOp->res)
							{
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							}
						}

						// The PRV location must be accurate even if we have no return values.
						// This is to properly handle, for example, the following situation at the call site (assuming 8-byte RSV align):
						// <param: 4 bytes> <local: 4 bytes> <local: 8 bytes>
						//                                   ^--- Correct PRV location
						// The correct PRV location needs to be set so that -8 in the called function can locate the parameter at -8.
						rdxLargeInt rvLocation = 0;
						RDX_PROTECT_ASSIGN(ctx, rvLocation, OpstackBaseLocation(ctx));

						rvLocation += rdxALIGN_RuntimeStackValue - 1;
						rvLocation &= ~(rdxALIGN_RuntimeStackValue - 1);

						if(decOp->opcode == rdxOP_call)
						{
							if(calledMethod->m_native.isIntrinsic)
							{
								if(calledMethod->m_native.isBranching)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								rdxSILInstruction ili;
								rdxUILOpCompactArg opArg0, opArg1, opArg2;
								ili.opcode = calledMethod->m_native.opcode;
								opArg0.lui = paramStackAction;
								opArg1.li = calledMethod->m_native.p1;
								opArg2.li = calledMethod->m_native.p2;

								makeResumable = !calledMethod->m_native.neverFails;
								PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
								makeResumable = false;
							}
							else
							{
								bool isTailCall = false;

#ifdef RDX_TAIL_CALL_OPTIMIZATION
								// See if this is a tail call
								// TODO: Optimize this into tail calls for other methods, also make it optional
								// If this is done, then care will be needed to ensure it doesn't cause thread serialization safety checks to fail
								// This can't be done inside of an exception handler because the exception won't be able to check each frame.
								if(paramsValidForTailCall && calledMethod == method && instrNum != (numDecodedInstructions-1) && decOps->Element(instrNum+1).opcode == rdxOP_return && (instrNum > currentEHLastInstruction || !ehLastInstructionSet))
								{
									isTailCall = true;
									for(rdxLargeUInt i=0;i<numParams;i++)
									{
										rdxWeakHdl(rdxCType) t = method->parameters->Element(i).type.ToWeakHdl();

										if(t.IsNull())
											RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

										if(TypeIsRefStruct(objm, t))
										{
											isTailCall = false;
											break;
										}
									}
								}
#endif
								/*
								if(isTailCall)
								{
									{
										rdxSILInstruction ili;
										ili.opcode = rdxILOP_hardenstack;
										PUSH_INSTR(ili);
									}
									{
										rdxLargeInt parameterOpstackUsedS;
										RDX_PROTECT_ASSIGN(ctx, parameterOpstackUsedS, rdxMakeSigned(ctx, parameterOpstackUsed));

										rdxSILInstruction copyILI;
										rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;
										copyILI.opcode = rdxILOP_move;
										opArg0.li = paramLocation;
										opArg1.li = -parameterOpstackUsedS;
										opArg2.lui = parameterOpstackUsed;
										opArg3.li = rdxALIASINGTYPE_Block;
										opArg4.li = RDX_ILOP_MOVE_DEST_PARENT_FRAME | RDX_ILOP_MOVE_SRC_TRANSIENT;

										PUSH_INSTR_CA_5(copyILI, opArg0, opArg1, opArg2, opArg3, opArg4);
									}

									// Go back to the function start
									rdxSILInstruction ili;
									ili.opcode = rdxILOP_jump;
									rdxUILOpCompactArg opArg0;
									opArg0.li = 0;
									PUSH_INSTR_CA_1(ili, opArg0);
								}
								else
								*/
								{
									rdxSILInstruction ili;
									ili.opcode = rdxILOP_call;
									rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4, opArg5;
									opArg0.li = (paramLocation & (~static_cast<rdxLargeInt>(RDX_MAX_ALIGNMENT - 1)));
									opArg1.li = rvLocation;
									opArg2.p = calledMethod.GetPOD();
									opArg3.lui = paramStackAction;
									opArg4.lui = numParams;
									opArg5.lui = numReturns;
									PUSH_INSTR_CA_6(ili, opArg0, opArg1, opArg2, opArg3, opArg4, opArg5);
								}
							}
						}
						else if(decOp->opcode == rdxOP_callvirtual)
						{
							if(!taggedThis)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);


							if(thisType.IsNull())
							{
								rdxSILInstruction ili;
								ili.opcode = rdxILOP_xnullref;
								PUSH_INSTR(ili);
							}
							else
							{
								if(thisType->ObjectInfo()->containerType != objm->GetBuiltIns()->st_StructuredType)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								rdxWeakHdl(rdxCStructuredType) st = thisType.StaticCast<rdxCStructuredType>();

								rdxSILInstruction ili;
								rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4, opArg5, opArg6, opArg7, opArg8, opArg9;
								rdxLargeUInt argCount = 8;
								if(st->storageSpecifier == rdxSS_Class)
								{
									ili.opcode = rdxILOP_callvirtual;
								}
								else if(st->storageSpecifier == rdxSS_Interface)
								{
									// Make sure we can deduce this again in source export
									if(calledMethod->parameters->Element(calledMethod->thisParameterOffset - 1).type != thisType.ToWeakRTRef())
										RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);

									ili.opcode = rdxILOP_callinterface;
									opArg8.p = st.GetPOD();
									opArg9.p = calledMethod.GetPOD();
									argCount = 10;
								}
								else
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								// Can't call a non-virtual
								if(calledMethod->vftIndex <= 0)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								RDX_PROTECT(ctx, SyncJournal(ctx) );

								opArg0.li = (paramLocation & (~static_cast<rdxLargeInt>(RDX_MAX_ALIGNMENT - 1)));
								opArg1.li = rvLocation;
								opArg2.li = thisLocation;
								opArg3.lui = calledMethod->vftIndex - 1;
								opArg4.lui = paramStackAction;
								opArg5.lui = numParams;
								opArg6.lui = numReturns;
								opArg7.lui = thisStackAction;

								if(argCount == 10)
									PUSH_INSTR_CA_10(ili, opArg0, opArg1, opArg2, opArg3, opArg4, opArg5, opArg6, opArg7, opArg8, opArg9);
								else if(argCount == 8)
									PUSH_INSTR_CA_8(ili, opArg0, opArg1, opArg2, opArg3, opArg4, opArg5, opArg6, opArg7);
								else
									RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
							}
						}
						else if(decOp->opcode == rdxOP_calldelegate)
						{
							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4, opArg5, opArg6;
							opArg0.li = (paramLocation & (~static_cast<rdxLargeInt>(RDX_MAX_ALIGNMENT - 1)));
							opArg1.li = rvLocation;
							opArg2.li = delegateSourceOffset;
							opArg3.lui = paramStackAction;
							opArg4.lui = numParams;
							opArg5.lui = numReturns;
							opArg6.lui = delegateSourceStackAction;
							if(delegateFromParameter)
								ili.opcode = rdxILOP_calldelegateprv;
							else
								ili.opcode = rdxILOP_calldelegatebp;

							PUSH_INSTR_CA_7(ili, opArg0, opArg1, opArg2, opArg3, opArg4, opArg5, opArg6);
						}

						// Clean up the stack and make sure the return values match
						if(pass > rdxPASS_CreateStacks)
						{
							for(rdxLargeUInt i=0;i<numReturns;i++)
							{
								rdxLargeUInt ri = numReturns - 1 - i;

								rdxSVStackValue vst;
								RDX_PROTECT_ASSIGN(ctx, vst, opstack->RetrieveTop(ctx, i) );

								if(!objm->TypesCompatible(vst.vType.ToWeakRTRef(), returnTypes->Element(ri).ToWeakRTRef()))
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								if(vst.vType.IsNull())
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								if(TypeIsRefStruct(objm, vst.vType))
								{
									if(vst.vstType != rdxVST_Pointer)
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
									vst.vstType = rdxVST_ConstPointer;
								}
								else
								{
									if(vst.vstType != rdxVST_ValueShell)
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								}

								RDX_PROTECT(ctx, LivenShell(ctx, vst) );
								RDX_PROTECT(ctx, opstack->ReinsertTop(ctx, i, vst) );

								if(vst.vstType == rdxVST_ValueShell)
								{
									rdxSMarkupInstruction mi;
									mi.opcode = rdxMOP_unshell;
									rdxUILOpCompactArg opArg0;
									opArg0.lui = vst.stackAction;
									PUSH_MARKUP_CA_1(mi, opArg0);
								}
							}
						}
					}
					break;
				case rdxOP_alloclocal:
					{
						// OPSTACK must be empty
						if(opstack->count)
							RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);
						if(decOp->res.IsNull() ||
							!objm->ObjectCompatible(decOp->res.ToWeakRTRef(), objm->GetBuiltIns()->st_Type.ToWeakRTRef()))
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxWeakHdl(rdxCType) t = decOp->res.StaticCast<rdxCType>();
						if(t == objm->GetBuiltIns()->st_Varying.ToWeakHdl())
							RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_VARYING_USAGE);

						rdxLargeUInt size, align;
						RDX_PROTECT(ctx, objm->TypeValueSize(ctx, t, size, align) );

						RDX_PROTECT(ctx, AppendLocal(ctx, t, instrNum, decOp->str, false) );

						rdxLargeUInt instanceStackAction = localstack->RetrieveTop(ctx, 0).stackAction;

						if(!objm->TypeIsObjectReference(t.ToWeakRTRef()) && 
							!objm->TypeIsInterface(t.ToWeakRTRef()) &&
							t->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType &&
							!(t.StaticCast<rdxCStructuredType>()->m_native.flags & rdxCStructuredType::NativeProperties::STF_ZeroFill))
						{
							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4, opArg5;
							ili.opcode = rdxILOP_pushdefault;
							opArg0.lui = RDX_ILOP_MOVE_SRC_TYPE_DEFAULT;
							opArg1.p = t.StaticCast<rdxCStructuredType>()->m_native.currentDefaultValue.ToWeakHdl().GetPOD();
							opArg2.lui = instanceStackAction;
							opArg3.p = t.GetPOD();
							PUSH_INSTR_CA_6(ili, opArg0, opArg1, opArg2, opArg3, opArg4, opArg5);
						}
						else
						{
							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1;
							ili.opcode = rdxILOP_zero_local;
							opArg0.lui = instanceStackAction;
							opArg1.p = t.GetPOD();
							PUSH_INSTR_CA_2(ili, opArg0, opArg1);
						}
					}
					break;
				case rdxOP_createlocal:
					{
						// Type of the local and popped data must match exactly and it must be a value
						if(decOp->res.IsNull() || !objm->TypesCompatible(decOp->res->ObjectInfo()->containerType.ToWeakRTRef(), objm->GetBuiltIns()->st_Type.ToWeakRTRef()))
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
						rdxWeakHdl(rdxCType) t = decOp->res.StaticCast<rdxCType>();
						if(t == objm->GetBuiltIns()->st_Varying)
							RDX_STHROW(ctx, RDX_ERROR_FORBIDDEN_VARYING_USAGE);

						rdxSVStackValue vst;
						RDX_PROTECT_ASSIGN(ctx, vst, PopOpstack(ctx, rdxVST_Value) );
						RDX_PROTECT(ctx, AppendLocal(ctx, t, instrNum, decOp->str, false) );

						if(opstack->count != 0)
							RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);

						if(pass > rdxPASS_CreateStacks)
						{
							if(vst.vType != t)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
							ili.opcode = rdxILOP_move;
							opArg0.li = RDX_ILOP_MOVE_SRC_TRANSIENT | RDX_ILOP_MOVE_DEST_INITIALIZE;
							opArg1.li = vst.stackAction;
							opArg2.li = localstack->RetrieveTop(ctx, 0).stackAction;
							opArg3.p = t.GetPOD();

							PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
						}
					}
					break;
				case rdxOP_removelocal:
					{
						if(opstack->count != 0)
							RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);

						if(localstack->count == numMethodParameters)
							RDX_STHROW(ctx, RDX_ERROR_IL_STACK_UNDERFLOW);

						rdxSVLocal vl;
						RDX_PROTECT_ASSIGN(ctx, vl, PopLocal(ctx) );

						if(pass > rdxPASS_CreateStacks)
						{
							// Any instructions that exist while this local is active can only be jumped to within its lifespan
							for(rdxLargeUInt i=vl.creatingInstruction+1;i<instrNum;i++)
							{
								if(metalist->Element(i).minJumpRange < vl.creatingInstruction)
									metalist->Element(i).minJumpRange = vl.creatingInstruction;
								if(metalist->Element(i).maxJumpRange > instrNum)
									metalist->Element(i).maxJumpRange = instrNum;
							}

							{
								rdxSMarkupInstruction mi;
								rdxUILOpCompactArg opArg0;
								mi.opcode = rdxMOP_pop;
								opArg0.lui = vl.stackAction;
								PUSH_MARKUP_CA_1(mi, opArg0);
							}
						}
					}
					break;
				case rdxOP_pushempty:
					{
						if(decOp->res.IsNull() || !objm->TypesCompatible(decOp->res->ObjectInfo()->containerType.ToWeakRTRef(), objm->GetBuiltIns()->st_Type.ToWeakRTRef()))
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxSVStackValue vst;
						vst.vType = decOp->res.StaticCast<rdxCType>();
						if(TypeIsRefStruct(objm, vst.vType))
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
						vst.vstType = rdxVST_ValueShell;

						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

						{
							rdxSMarkupInstruction mi;
							mi.opcode = rdxMOP_addshell;
							rdxUILOpCompactArg opArg0;
							opArg0.p = vst.vType.GetPOD();
							PUSH_MARKUP_CA_1(mi, opArg0);
						}
					}
					break;
				case rdxOP_newinstance:
					{
						RDX_PROTECT(ctx, SyncJournal(ctx) );

						rdxSVStackValue vst;
						vst.vstType = rdxVST_Value;
						vst.vType = decOp->res.StaticCast<rdxCType>();
						if(vst.vType.IsNull())
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxLargeUInt numDimensions = 0;
						if(decOp->sint1 < 0)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxLargeUInt expectedDimensions = static_cast<rdxLargeUInt>(decOp->sint1);

						if(pass > rdxPASS_CreateStacks)
						{
							rdxWeakHdl(rdxCType) containerType = vst.vType->ObjectInfo()->containerType.ToWeakHdl();
							if(containerType == objm->GetBuiltIns()->st_StructuredType)
								numDimensions = 0;
							else if(containerType == objm->GetBuiltIns()->st_ArrayOfType)
								numDimensions = vst.vType.StaticCast<rdxCArrayOfType>()->numDimensions;
						}
						else
							numDimensions = expectedDimensions;

						if(numDimensions != expectedDimensions || numDimensions < 0)
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

						rdxLargeUInt dimensionStackAction = 0;

						for(rdxLargeUInt i=0;i<numDimensions;i++)
						{
							rdxSVStackValue dimVst;
							RDX_PROTECT_ASSIGN(ctx, dimVst, PopOpstack(ctx, rdxVST_Value) );
							if(pass > rdxPASS_CreateStacks && dimVst.vType != objm->GetBuiltIns()->st_LargeUInt)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							if(i == 0)
								dimensionStackAction = dimVst.stackAction;
						}

						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

						if(pass > rdxPASS_CreateStacks)
						{
							rdxLargeUInt size, align;
							rdxLargeUInt argCount = 0;
							bool isStructuredType = (vst.vType->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType);

							RDX_PROTECT(ctx, objm->TypeValueSize(ctx, vst.vType, size, align) );

							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4, opArg5;
							if(!objm->TypeIsObjectReference(vst.vType.ToWeakRTRef()) && 
								!objm->TypeIsInterface(vst.vType.ToWeakRTRef()) &&
								isStructuredType &&
								(vst.vType.StaticCast<rdxCStructuredType>()->m_native.flags & rdxCStructuredType::NativeProperties::STF_ZeroFill))
							{
								ili.opcode = rdxILOP_zero_op;
								opArg0.lui = vst.stackAction;
								opArg1.p = vst.vType.GetPOD();
								argCount = 2;
							}
							else
							{
								// Make sure this is a creatable type
								rdxWeakHdl(rdxCStructuredType) instanceCopy = rdxWeakHdl(rdxCStructuredType)::Null();

								if(vst.vType->ObjectInfo()->containerType == objm->GetBuiltIns()->st_ArrayOfType)
								{
								}
								else if(isStructuredType)
								{
									rdxWeakHdl(rdxCStructuredType) st = vst.vType.StaticCast<rdxCStructuredType>();
									switch(st->storageSpecifier)
									{
									case rdxSS_Enum:
									case rdxSS_Interface:
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
										break;
									case rdxSS_Class:
										if(st->isAbstract)
											RDX_STHROW(ctx, RDX_ERROR_CREATED_ABSTRACT_TYPE);
										break;
									case rdxSS_RefStruct:
										// Can't create refstructs
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
										break;
									case rdxSS_ValStruct:
										instanceCopy = st;
										break;
									default:
										RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);
									}
								}
								else
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								if(instanceCopy.IsNotNull())
								{
									ili.opcode = rdxILOP_pushdefault;
									opArg0.lui = RDX_ILOP_MOVE_SRC_TYPE_DEFAULT;
									opArg0.lui |= RDX_ILOP_MOVE_DEST_OPSTACK;
									opArg1.p = instanceCopy->m_native.currentDefaultValue.ToWeakHdl().GetPOD();
									opArg2.lui = vst.stackAction;
									opArg3.p = vst.vType.GetPOD();

									argCount = 4;
								}
								else
								{
									ili.opcode = rdxILOP_newinstance;
									opArg0.lui = vst.stackAction;
									opArg1.p = vst.vType.GetPOD();
									opArg2.lui = dimensionStackAction;
									opArg3.lui = numDimensions;
									argCount = 4;
								}
							}

							if(argCount == 2)
								PUSH_INSTR_CA_2(ili, opArg0, opArg1);
							else if(argCount == 4)
								PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
							else if(argCount == 5)
								PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
							else if(argCount == 6)
								PUSH_INSTR_CA_6(ili, opArg0, opArg1, opArg2, opArg3, opArg4, opArg5);
						}
					}
					break;
				case rdxOP_newinstanceset:
					{
						rdxWeakHdl(rdxCType) iType = decOp->res.StaticCast<rdxCType>();
						if(iType.IsNull())
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						if(iType->ObjectInfo()->containerType != objm->GetBuiltIns()->st_StructuredType)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxWeakHdl(rdxCStructuredType) st = iType.StaticCast<rdxCStructuredType>();
						if(objm->TypeIsObjectReference(iType.ToWeakRTRef()))
						{
							// Might throw an exception, i.e. memory allocation failure
							RDX_PROTECT(ctx, SyncJournal(ctx) );
						}

						if(decOp->sint1 < 0)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
						rdxLargeUInt localIndex = static_cast<rdxLargeUInt>(decOp->sint1);
						if(localIndex >= localstack->count)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxLargeUInt numProperties = decOp->intVarCount;

						bool isClass = false;
						switch(st->storageSpecifier)
						{
						case rdxSS_Enum:
						case rdxSS_Interface:
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							break;
						case rdxSS_Class:
							if(st->isAbstract)
								RDX_STHROW(ctx, RDX_ERROR_CREATED_ABSTRACT_TYPE);
							isClass = true;
							break;
						case rdxSS_RefStruct:
						case rdxSS_ValStruct:
							isClass = false;
							break;
						default:
							RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);
						}

						// If this is a class, create and dispose of a pointer.  This isn't a recorded value, since the sets are all forbidden.
						// This is necessary to make sure that the stack is the appropriate size though.
						rdxSVStackValue pointerVst;
						pointerVst.vstType = rdxVST_Pointer;
						pointerVst.vType = rdxWeakHdl(rdxCType)::Null();
						if(isClass)
						{
							RDX_PROTECT(ctx, AppendOpstack(ctx, pointerVst));
							RDX_PROTECT(ctx, PopOpstack(ctx, rdxVST_Pointer));
						}

						if(pass > rdxPASS_CreateStacks)
						{
							rdxSVLocal local;
							RDX_PROTECT_ASSIGN(ctx, local, localstack->RetrieveBottom(ctx, localIndex));

							if(!objm->TypesCompatible(st.ToWeakRTRef(), local.vType.ToWeakRTRef()))
								RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_CONVERSION);

							if(isClass)
							{
								rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
								rdxSILInstruction ili;
								ili.opcode = rdxILOP_newinstance_local;
								opArg0.lui = local.stackAction;
								opArg1.p = iType.GetPOD();
								opArg2.li = 0;
								opArg3.li = 0;
								PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
							}
							else
							{
								// Moves into the object aren't done using pins, which means they'll have aliasing problems with PCCM
								// Stack state needs to be flushed both before and after to allow aliasing
								// TODO: This probably isn't necessary any more...
								rdxSILInstruction ili;
								ili.opcode = rdxILOP_hardenstack;
								PUSH_INSTR(ili);
							}

							// It's technically possible for a struct localref that's already been initialized to be clobbered by this
							// We don't care, since it's just as legal to set it up using a property init, and native unrecoverability is not guaranteed on structs

							rdxLargeUInt numTypeProperties = 0;
							if(st->properties.IsNotNull())
								numTypeProperties = st->properties->NumElements();

							rdxLargeUInt lastIndex = 0;

							rdxWeakOffsetRTRef(rdxUInt8) argVar = decOp->intVarStart.ToRTRef();
							rdxLargeUInt argVarBytes = decOp->intVarBytes;

							for(rdxLargeUInt i=0;i<decOp->intVarCount;i++)
							{
								// Determine what property this is
								rdxLargeInt propIndexS;
								if(!DecodeInt<rdxLargeInt>(argVar, argVarBytes, propIndexS))
									RDX_STHROW(ctx, RDX_ERROR_IL_ARGUMENT_UNDERRUN);

								if(propIndexS < 0)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								rdxLargeUInt propIndex = static_cast<rdxLargeUInt>(propIndexS);
								if(propIndex >= numTypeProperties)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								rdxWeakOffsetHdl(rdxSProperty) prop = (st->properties->OffsetElementRTRef(propIndex).ToHdl());
								bool expectPointer = TypeIsRefStruct(objm, prop->type.ToWeakHdl());

								// Find the value
								rdxSVStackValue vst;
								RDX_PROTECT_ASSIGN(ctx, vst, opstack->RetrieveTop(ctx, numProperties - 1 - i));

								if( (expectPointer && !VSTIsPointer(vst.vstType)) || (!expectPointer && vst.vstType != rdxVST_Value))
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								if(!objm->TypesCompatible(vst.vType.ToWeakRTRef(), prop->type.ToWeakRTRef()))
									RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_CONVERSION);

								rdxLargeUInt vSize, vAlign;
								RDX_PROTECT(ctx, objm->TypeValueSize(ctx, prop->type.ToWeakHdl(), vSize, vAlign));

								if(isClass)
								{
									if(i == 0)
									{
										rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
										rdxSILInstruction ili;
										ili.opcode = rdxILOP_objproperty_notnull_persist;
										opArg0.lui = local.stackAction;
										opArg1.lui = pointerVst.stackAction;
										opArg2.p = st.GetPOD();
										opArg3.lui = propIndex;
										PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
									}
									else
									{
										rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
										rdxSILInstruction ili;
										ili.opcode = rdxILOP_changeproperty;
										opArg0.p = st.GetPOD();
										opArg1.lui = lastIndex;
										opArg2.lui = propIndex;
										opArg3.lui = pointerVst.stackAction;
										PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
									}

									lastIndex = propIndex;

									{
										// Move
										rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
										rdxSILInstruction ili;
										ili.opcode = rdxILOP_move;

										opArg0.lui = (RDX_ILOP_MOVE_DEST_DEREF | RDX_ILOP_MOVE_DEST_OBJECT | RDX_ILOP_MOVE_SRC_TRANSIENT);
										opArg1.lui = vst.stackAction;
										opArg2.lui = pointerVst.stackAction;
										opArg3.p = prop->type.ToWeakHdl().GetPOD();
										if(expectPointer)
											opArg0.li |= RDX_ILOP_MOVE_SRC_DEREF;
										PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
									}
								}
								else
								{
									{
										// Move
										rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
										rdxSILInstruction ili;
										ili.opcode = rdxILOP_move;
										opArg0.lui = RDX_ILOP_MOVE_SRC_TRANSIENT | RDX_ILOP_MOVE_DEST_DESTROY;
										opArg1.li = vst.stackAction;
										opArg2.li = local.stackAction;
										opArg3.p = prop->type.ToWeakHdl().GetPOD();
										if(expectPointer)
											opArg0.lui |= RDX_ILOP_MOVE_SRC_DEREF;
										PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
									}
									{
										// Emit property markup
										rdxUILOpCompactArg opArg0, opArg1;
										rdxSMarkupInstruction mi;
										opArg0.lui = propIndex;
										opArg1.p = st.GetPOD();
										mi.opcode = rdxMOP_moveprop;
										PUSH_MARKUP_CA_2(mi, opArg0, opArg1);
									}
								}
							}

							// Done with properties

							// Emit pop MOP so the VReg doesn't leak
							if(isClass)
							{
								rdxSMarkupInstruction mi;
								rdxUILOpCompactArg opArg0;
								mi.opcode = rdxMOP_pop;
								opArg0.lui = pointerVst.stackAction;
								PUSH_MARKUP_CA_1(mi, opArg0);
							}

							// Flush the stack state if this is a non-class
							if(!isClass)
							{
								rdxSILInstruction ili;
								ili.opcode = rdxILOP_hardenstack;
								PUSH_INSTR(ili);
							}
						}

						for(rdxLargeUInt os=0;os<numProperties;os++)
						{
							RDX_PROTECT(ctx, PopOpstack(ctx, rdxVST_Indeterminate));
						}
					}
					break;
				case rdxOP_null:
					{
						rdxSVStackValue vst;
						vst.vstType = rdxVST_Value;
						vst.vType = rdxWeakHdl(rdxCType)::Null();

						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

						rdxSILInstruction ili;
						rdxUILOpCompactArg opArg0, opArg1;
						ili.opcode = rdxILOP_zero_op;
						opArg0.li = vst.stackAction;
						opArg1.p = objm->GetBuiltIns()->st_Object.ToWeakHdl().GetPOD();

						PUSH_INSTR_CA_2(ili, opArg0, opArg1);
					}
					break;
				case rdxOP_pinlocal:
					{
						// Local must be a struct and must not be a pointer (i.e. byref struct parameter)
						rdxSVStackValue lref;
						RDX_PROTECT_ASSIGN(ctx, lref, PopOpstack(ctx, rdxVST_LocalRef) );

						rdxSVLocal vl = localstack->RetrieveBottom(ctx, lref.index);

						rdxSVStackValue vst;
						if(vl.isConstant)
							vst.vstType = rdxVST_ConstPointer;
						else
							vst.vstType = rdxVST_Pointer;

						vst.vType = vl.vType;
						vst.isFromLocal = true;
						if(vl.isParameter)
						{
							vst.pointerSourceType = rdxPST_PinnedParameter;
							vst.pointerSource = vl.parameterOffset;
						}
						else
						{
							vst.pointerSourceType = rdxPST_PinnedLocal;
							vst.pointerSource = vl.offset;
						}

						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

						if(pass > rdxPASS_CreateStacks)
						{
							if(vl.isPointer)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1;

							ili.opcode = rdxILOP_pinl;
							opArg0.li = vst.stackAction;
							opArg1.li = vl.stackAction;
							PUSH_INSTR_CA_2(ili, opArg0, opArg1);
						}
					}
					break;
				case rdxOP_tovarying:
					{
						rdxSVStackValue vst;
						RDX_PROTECT_ASSIGN(ctx, vst, PopOpstack(ctx, rdxVST_Indeterminate) );

						rdxEVSTType originalVstType = vst.vstType;

						rdxLargeUInt ptrOriginalAction = vst.stackAction;
						vst.vstType = rdxVST_Varying;
						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

						if(pass > rdxPASS_CreateStacks)
						{
							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1, opArg2;

							opArg0.li = vst.stackAction;
							opArg1.p = vst.vType.GetPOD();
							opArg2.li = ptrOriginalAction;

							if(originalVstType != rdxVST_Pointer && originalVstType == rdxVST_ConstPointer)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							ili.opcode = rdxILOP_tovarying;
							PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
						}
					}
					break;
				case rdxOP_arrayindex:
					{
						rdxSILInstruction ili;
						rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;

						// Must be largeuint list (size specified by int1) followed by an array with the same number of dimensions
						if(decOp->sint1 < 1)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
						rdxLargeUInt numDimensions = static_cast<rdxLargeUInt>(decOp->sint1);
						
						rdxSVStackValue vst;
						RDX_PROTECT_ASSIGN(ctx, vst, opstack->RetrieveTop(ctx, 0) );

						RDX_PROTECT(ctx, SyncJournal(ctx) );

						ili.opcode = rdxILOP_arrayindex;
						opArg0.li = vst.stackAction;

						for(rdxLargeUInt i=0;i<numDimensions;i++)
						{
							rdxSVStackValue dimVST;
							RDX_PROTECT_ASSIGN(ctx, dimVST, PopOpstack(ctx) );

							if(pass > rdxPASS_CreateStacks)
							{
								if(dimVST.vType != objm->GetBuiltIns()->st_LargeUInt)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							}
						}

						rdxSVStackValue arrayVST;
						rdxSVStackValue pointerVST;
						RDX_PROTECT_ASSIGN(ctx, arrayVST, PopOpstack(ctx) );

						opArg1.li = arrayVST.stackAction;

						if(pass > rdxPASS_CreateStacks)
						{
							rdxWeakHdl(rdxCType) t = arrayVST.vType;
							if(t.IsNull() || t->ObjectInfo()->containerType != objm->GetBuiltIns()->st_ArrayOfType)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							rdxWeakHdl(rdxCArrayOfType) aot = t.StaticCast<rdxCArrayOfType>();
							if(aot->numDimensions != numDimensions)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							if(aot->isConstant)
								pointerVST.vstType = rdxVST_ConstPointer;
							else
								pointerVST.vstType = rdxVST_Pointer;
							pointerVST.vType = aot->type.ToWeakHdl();
						}


						RDX_PROTECT(ctx, AppendOpstack(ctx, pointerVST) );
						opArg2.li = opstack->RetrieveTop(ctx, 0).stackAction;
						opArg3.li = numDimensions;

						PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
					}
					break;
				case rdxOP_property:
					{
						if(pass > rdxPASS_CreateStacks)
						{
							rdxSVStackValue vst;
							RDX_PROTECT_ASSIGN(ctx, vst, opstack->RetrieveTop(ctx, 0) );	// Retrieve so that the parameter is live when the journal syncs

							if(!VSTIsPointer(vst.vstType) &&
								vst.vstType != rdxVST_Value)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							if(vst.vstType == rdxVST_Value)
								RDX_PROTECT(ctx, SyncJournal(ctx) );

							// Pop for real now
							RDX_PROTECT(ctx, PopOpstack(ctx, vst.vstType) );

							// This value isn't pass-through any more
							vst.isLoadedFromLocal = false;

							rdxWeakHdl(rdxCType) t = vst.vType;
							if(t.IsNull() || t->ObjectInfo()->containerType != objm->GetBuiltIns()->st_StructuredType)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxWeakHdl(rdxCStructuredType) st = vst.vType.StaticCast<rdxCStructuredType>();

							rdxLargeUInt numProperties = 0;
							if(st->properties.IsNotNull())
								numProperties = st->properties->NumElements();

							if(decOp->sint1 < 0)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_PROPERTY);
							rdxLargeUInt propertyIndex = static_cast<rdxLargeUInt>(decOp->sint1);
							if(propertyIndex >= numProperties)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_PROPERTY);

							rdxWeakHdl(rdxCType) propertyType = st->properties->Element(propertyIndex).type.ToWeakHdl();

							if(propertyType.IsNull())
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

							rdxLargeUInt oldSA = vst.stackAction;

							if(VSTIsPointer(vst.vstType))
							{
								// Pointers to object references must be loaded first
								if(objm->TypeIsObjectReference(st.ToWeakRTRef()))
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								// Interface refs have no properties
								if(objm->TypeIsInterface(st.ToWeakRTRef()))
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);


								if(st->properties->Element(propertyIndex).isConstant)
									vst.vstType = rdxVST_ConstPointer;
								vst.vType = propertyType;
								RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

								rdxEOpcode prevOpcode = decOps->Element(instrNum-1).opcode;
								if(prevOpcode == rdxOP_property ||
									prevOpcode == rdxOP_pinlocal ||
									prevOpcode == rdxOP_arrayindex)
								{
									rdxSMarkupInstruction mi;
									mi.opcode = rdxMOP_incrprop;
									rdxUILOpCompactArg mopArg0, mopArg1, mopArg2, mopArg3;
									mopArg0.lui = propertyIndex;
									mopArg1.p = st.GetPOD();
									mopArg2.lui = oldSA;
									mopArg3.lui = vst.stackAction;
									PUSH_MARKUP_CA_4(mi, mopArg0, mopArg1, mopArg2, mopArg3);
								}
								else
								{
									rdxSILInstruction ili;
									rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
									ili.opcode = rdxILOP_ptrprop;
									opArg0.li = vst.stackAction;
									opArg1.p = st.GetPOD();
									opArg2.lui = propertyIndex;
									opArg3.lui = oldSA;
									PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
								}
							}
							else if(vst.vstType == rdxVST_Value)
							{
								rdxSILInstruction ili;
								rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
								ili.opcode = rdxILOP_objproperty;
								opArg0.li = vst.stackAction;

								if(vst.isNotNull)
									ili.opcode = rdxILOP_objproperty_notnull;

								RDX_PROTECT(ctx, SyncJournal(ctx) );

								if(st->storageSpecifier != rdxSS_Class)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								vst.vType = propertyType;

								if(st->properties->Element(propertyIndex).isConstant)
									vst.vstType = rdxVST_ConstPointer;
								else
									vst.vstType = rdxVST_Pointer;
								vst.isFromLocal = false;
								vst.isLoadedFromLocal = false;
								RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

								opArg1.li = vst.stackAction;
								opArg2.p = st.GetPOD();
								opArg3.lui = propertyIndex;

								PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
							}
							else
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
						}
					}
					break;
				case rdxOP_move:
					{
						rdxSVStackValue destVst;
						rdxSVStackValue sourceVst;

						RDX_PROTECT_ASSIGN(ctx, destVst, PopOpstack(ctx, rdxVST_Indeterminate) );
						RDX_PROTECT_ASSIGN(ctx, sourceVst, PopOpstack(ctx, rdxVST_Indeterminate) );

						if(pass > rdxPASS_CreateStacks)
						{
							if(!objm->TypesCompatible(sourceVst.vType.ToWeakRTRef(), destVst.vType.ToWeakRTRef()))
								RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_CONVERSION);

							rdxLargeUInt size, align;
							RDX_PROTECT(ctx, objm->TypeValueSize(ctx, sourceVst.vType, size, align) );

							if(destVst.vstType == rdxVST_Pointer)
							{
								rdxSILInstruction ili;
								rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
								ili.opcode = rdxILOP_move;
								opArg0.lui = RDX_ILOP_MOVE_DEST_DEREF | RDX_ILOP_MOVE_DEST_OPSTACK | RDX_ILOP_MOVE_DEST_DESTROY | RDX_ILOP_MOVE_DEST_TRANSIENT;
								opArg2.li = destVst.stackAction;
								opArg3.p = destVst.vType.GetPOD();

								if(!destVst.isFromLocal)
								{
									opArg0.lui |= RDX_ILOP_MOVE_DEST_OBJECT;
									opArg0.lui |= rdxMoveGrayFlagsForSource(objm, sourceVst.vType.ToWeakRTRef());
								}

								if(sourceVst.vstType == rdxVST_Value)
								{
									opArg0.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
									opArg1.li = sourceVst.stackAction;
								}
								else if(VSTIsPointer(sourceVst.vstType))
								{
									opArg0.lui |= RDX_ILOP_MOVE_SRC_DEREF;
									opArg0.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
									if(!sourceVst.isFromLocal)
										opArg0.lui |= RDX_ILOP_MOVE_SRC_OBJECT;

									opArg1.li = sourceVst.stackAction;
								}
								else if(sourceVst.vstType == rdxVST_LocalRef)
								{
									rdxSVLocal vl = localstack->RetrieveBottom(ctx, sourceVst.index);

									if(vl.isPointer)
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
									
									if(vl.isParameter)
										opArg0.lui |= RDX_ILOP_MOVE_SRC_PARENT_FRAME;

									opArg1.li = vl.stackAction;
								}
								else
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
							}
							else if(destVst.vstType == rdxVST_LocalRef)
							{
								rdxSILInstruction ili;
								rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
								ili.opcode = rdxILOP_move;
								bool destIsParameter;

								{
									rdxSVLocal vl = localstack->RetrieveBottom(ctx, destVst.index);

									if(vl.isPointer)
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
									destIsParameter = vl.isParameter;

									opArg2.li = vl.stackAction;
									opArg3.p = vl.vType.GetPOD();
								}

								opArg0.lui = RDX_ILOP_MOVE_DEST_OPSTACK | RDX_ILOP_MOVE_DEST_DESTROY;

								if(destIsParameter)
									opArg0.lui |= RDX_ILOP_MOVE_DEST_PARENT_FRAME;

								if(sourceVst.vstType == rdxVST_Value)
								{
									opArg1.li = sourceVst.stackAction;
									opArg0.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
								}
								else if(VSTIsPointer(sourceVst.vstType))
								{
									opArg0.lui |= RDX_ILOP_MOVE_SRC_DEREF;
									opArg0.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;

									opArg1.li = sourceVst.stackAction;
								}
								else if(sourceVst.vstType == rdxVST_LocalRef)
								{
									rdxSVLocal vl = localstack->RetrieveBottom(ctx, sourceVst.index);

									if(vl.isPointer)
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

									if(vl.isParameter)
									{
										opArg0.lui |= RDX_ILOP_MOVE_SRC_PARENT_FRAME;
										opArg1.li = vl.stackAction;
									}
									else
									{
										opArg1.li = vl.stackAction;
									}
								}
								else
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
							}
							else
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
						}
					}
					break;
				case rdxOP_clone:
					{
						rdxLargeInt offsetS = decOp->sint1;
						rdxLargeInt countS = decOp->sint2;

						if(offsetS < 0 || countS < 1)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxLargeUInt offset = static_cast<rdxLargeUInt>(offsetS);
						rdxLargeUInt count = static_cast<rdxLargeUInt>(countS);

						if(offset >= opstack->count || (opstack->count - offset) < count)
							RDX_STHROW(ctx, RDX_ERROR_IL_STACK_UNDERFLOW);

						rdxLargeUInt first = opstack->count - 1 - offset;
						rdxLargeUInt end = first + count;

						// Make sure none of these are shells and this doesn't cross a barrier boundary
						if(pass > rdxPASS_CreateStacks)
						{
							for(rdxLargeUInt i=first;i<opstack->count;i++)
							{
								rdxSVStackValue vst;
								RDX_PROTECT_ASSIGN(ctx, vst, opstack->RetrieveBottom(ctx, i));
								if(vst.vstType == rdxVST_Barrier)
									RDX_STHROW(ctx, RDX_ERROR_IL_STACK_UNDERFLOW);
								if(vst.vstType == rdxVST_ValueShell && i < end)
									RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);
							}
						}

						for(rdxLargeUInt i=0;i<count;i++)
						{
							rdxLargeUInt vstIndex = first + i;
							rdxSVStackValue vst;
							RDX_PROTECT_ASSIGN(ctx, vst, opstack->RetrieveBottom(ctx, vstIndex));
							rdxSVStackValue clonedVst = vst;
							RDX_PROTECT(ctx, AppendOpstack(ctx, clonedVst) );

							if(pass > rdxPASS_CreateStacks)
							{
								rdxLargeUInt size = 0;
								rdxLargeUInt dummyAlign = 1;
								switch(vst.vstType)
								{
								case rdxVST_Value:
								case rdxVST_Pointer:
								case rdxVST_Varying:
									{
										rdxSILInstruction ili;
										rdxUILOpCompactArg opArg0, opArg1;
										ili.opcode = rdxILOP_clone;
										opArg0.li = vst.stackAction;
										opArg1.li = clonedVst.stackAction;

										PUSH_INSTR_CA_2(ili, opArg0, opArg1);
									}
									break;
								case rdxVST_LocalRef:
									break;
								case rdxVST_Barrier:
								case rdxVST_ValueShell:
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
									break;
								}
							}
						}
					}
					break;
				case rdxOP_load:
					{
						rdxSVStackValue loadedVst;
						RDX_PROTECT_ASSIGN(ctx, loadedVst, PopOpstack(ctx, rdxVST_Indeterminate) );

						if(pass > rdxPASS_CreateStacks)
						{
							switch(loadedVst.vstType)
							{
							case rdxVST_ConstPointer:
							case rdxVST_Pointer:
								{
									rdxLargeUInt size, align;
									RDX_PROTECT(ctx, objm->TypeValueSize(ctx, loadedVst.vType, size, align) );

									if(TypeIsRefStruct(objm, loadedVst.vType))
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

									rdxSVStackValue createdVst;
									createdVst.vType = loadedVst.vType;
									createdVst.vstType = rdxVST_Value;

									RDX_PROTECT(ctx, AppendOpstack(ctx, createdVst) );

									rdxSILInstruction ili;
									rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
									ili.opcode = rdxILOP_move;
									opArg1.li = loadedVst.stackAction;
									opArg2.li = createdVst.stackAction;
									opArg3.p = createdVst.vType.GetPOD();
									opArg0.lui = 0;
									opArg0.lui |= RDX_ILOP_MOVE_SRC_DEREF;
									opArg0.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
									if(!loadedVst.isFromLocal)
									{
										opArg0.lui |= RDX_ILOP_MOVE_SRC_OBJECT;
										opArg0.lui |= rdxMoveGrayFlagsForSource(objm, loadedVst.vType.ToWeakRTRef());
									}
									opArg0.lui |= RDX_ILOP_MOVE_DEST_OPSTACK;


									PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
								}
								break;
							case rdxVST_LocalRef:
								{
									rdxSVLocal vl;
									RDX_PROTECT_ASSIGN(ctx, vl, localstack->RetrieveBottom(ctx, loadedVst.index) );
									if(vl.isParameter)
									{
										rdxLargeUInt size, align;
										rdxSVStackValue createdVst;
										createdVst.vType = vl.vType;
										createdVst.isLoadedFromLocal = true;
										createdVst.loadedLocal = loadedVst.index;

										if(vl.isPointer)
										{
											if(vl.isConstant)
												createdVst.vstType = rdxVST_ConstPointer;
											else
												createdVst.vstType = rdxVST_Pointer;

											createdVst.pointerSource = vl.parameterOffset;
											createdVst.pointerSourceType = rdxPST_OffsetParameter;
											size = sizeof(rdxWeakOffsetRTRef(rdxCObject));
											align = rdxAlignOf(rdxWeakOffsetRTRef(rdxCObject));
											RDX_PROTECT(ctx, AppendOpstack(ctx, createdVst) );
										}
										else
										{
											createdVst.vstType = rdxVST_Value;
											createdVst.isNotNull = vl.isNotNull;
											RDX_PROTECT(ctx, objm->TypeValueSize(ctx, vl.vType, size, align) );
											RDX_PROTECT(ctx, AppendOpstack(ctx, createdVst) );
										}

										rdxSILInstruction ili;
										rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
										ili.opcode = rdxILOP_move;
										opArg0.lui = RDX_ILOP_MOVE_DEST_OPSTACK;
										opArg0.lui |= RDX_ILOP_MOVE_SRC_PARENT_FRAME;
										if(vl.isPointer)
											opArg0.lui |= RDX_ILOP_MOVE_IS_RTP;
										opArg1.li = vl.stackAction;
										opArg2.li = createdVst.stackAction;
										opArg3.p = vl.vType.GetPOD();

										PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
									}
									else
									{
										rdxSVStackValue createdVst;
										createdVst.vstType = rdxVST_Value;
										createdVst.vType = vl.vType;
										createdVst.isNotNull = vl.isNotNull;
										createdVst.isLoadedFromLocal = true;
										createdVst.loadedLocal = loadedVst.index;
										RDX_PROTECT(ctx, AppendOpstack(ctx, createdVst) );

										rdxLargeUInt size, align;
										RDX_PROTECT(ctx, objm->TypeValueSize(ctx, vl.vType, size, align) );

										rdxSILInstruction ili;
										rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
										ili.opcode = rdxILOP_move;
										opArg0.lui = RDX_ILOP_MOVE_DEST_OPSTACK;
										opArg1.li = vl.stackAction;
										opArg2.li = createdVst.stackAction;
										opArg3.p = vl.vType.GetPOD();

										PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
									}

								}
								break;
							default:
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							};
						}
						else
						{
							rdxSVStackValue createdVst;
							RDX_PROTECT(ctx, AppendOpstack(ctx, createdVst) );
						}
					}
					break;
				case rdxOP_pop:
					{
						rdxSVStackValue vst;
						RDX_PROTECT_ASSIGN(ctx, vst, PopOpstack(ctx, rdxVST_Indeterminate) );
						bool poppedVReg = false;

						if(pass > rdxPASS_CreateStacks)
						{
							switch(vst.vstType)
							{
							case rdxVST_Value:
							case rdxVST_Pointer:
							case rdxVST_ConstPointer:
							case rdxVST_Varying:
							case rdxVST_ValueShell:
								{
									// VReg values
									rdxSMarkupInstruction mi;
									rdxUILOpCompactArg opArg0;
									mi.opcode = rdxMOP_pop;
									opArg0.lui = vst.stackAction;
									PUSH_MARKUP_CA_1(mi, opArg0);
								}
								break;
							case rdxVST_LocalRef:
								// Non-VReg, but still valid
								break;
							default:
								{
									// Invalid (i.e. a barrier)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								}
								break;
							};
						}
					}
					break;
				case rdxOP_cast:
					{
						RDX_PROTECT(ctx, SyncJournal(ctx) );	// TODO: Don't sync journals if this is emittable as otoi_direct

						rdxSVStackValue vst;
						RDX_PROTECT_ASSIGN(ctx, vst, PopOpstack(ctx, rdxVST_Value) );

						rdxSVStackValue newVST = vst;
						rdxWeakHdl(rdxCType) t;

						if(pass > rdxPASS_CreateStacks)
						{
							if(decOp->res.IsNull() || !objm->ObjectCompatible(decOp->res.ToWeakRTRef(), objm->GetBuiltIns()->st_Type.ToWeakRTRef()))
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							t = decOp->res.StaticCast<rdxCType>();
							if(!objm->TypeIsValid(t.ToWeakRTRef()))
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							newVST.vType = t;
						}

						// Append opstack to get the new SA
						RDX_PROTECT(ctx, AppendOpstack(ctx, newVST) );

						if(pass > rdxPASS_CreateStacks)
						{
							if(!objm->TypesCompatible(vst.vType.ToWeakRTRef(), t.ToWeakRTRef()))
							{
								// This will require a polymorphic cast
								if(!objm->TypesCompatiblePolymorphic(vst.vType.ToWeakRTRef(), t.ToWeakRTRef()))
								{
									int bp = 0;
									objm->TypesCompatiblePolymorphic(vst.vType.ToWeakRTRef(), t.ToWeakRTRef());
									RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_CONVERSION);
								}

								bool emitted = false;
								if(objm->TypeIsInterface(vst.vType))
								{
									if(objm->TypeIsClass(t))
									{
										rdxSILInstruction ili;
										rdxUILOpCompactArg opArg0, opArg1, opArg2;
										ili.opcode = rdxILOP_rcast_itoo;
										opArg0.li = vst.stackAction;
										opArg1.p = t.GetPOD();
										opArg2.li = newVST.stackAction;

										emitted = true;
										PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
									}
									else if(objm->TypeIsInterface(t))
									{
										rdxSILInstruction ili;
										rdxUILOpCompactArg opArg0, opArg1, opArg2;
										ili.opcode = rdxILOP_rcast_itoi;
										opArg0.li = vst.stackAction;
										opArg1.p = t.GetPOD();
										opArg2.li = newVST.stackAction;

										emitted = true;
										PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
									}
									else
										RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);
								}
								else if(vst.vType == objm->GetBuiltIns()->RDX_ENUM_INTERNAL_TYPE)
								{
									if(t->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType &&
										t.StaticCast<rdxCStructuredType>()->storageSpecifier == rdxSS_Enum)
									{
										if(emitted)
											RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);	// Don't emit multiple committing instrs per source instruction!
										emitted = true;

										rdxSILInstruction ili;
										rdxUILOpCompactArg opArg0, opArg1, opArg2;
										ili.opcode = rdxILOP_assertenum;
										opArg0.li = vst.stackAction;
										opArg1.p = t.GetPOD();
										opArg2.li = newVST.stackAction;
										PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
									}
									else
										RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);
								}
								else if(objm->TypeIsClass(vst.vType))
								{
									if(objm->TypeIsInterface(t))
									{
										// Determine if this is a direct conversion
										bool isDirect = false;
										rdxWeakRTRef(rdxCStructuredType) st = vst.vType.StaticCast<rdxCStructuredType>();
										if(st->interfaces.IsNotNull())
										{
											rdxLargeUInt nInterfaces = st->interfaces->NumElements();
											for(rdxLargeUInt ifcIndex=0;ifcIndex<nInterfaces;ifcIndex++)
											{
												rdxWeakRTRef(rdxCStructuredType) ifcType = st->interfaces->Element(ifcIndex).type.ToWeakRTRef();
												if(ifcType == t)
												{
													if(emitted)
														RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);	// Don't emit multiple committing instrs per source instruction!
													emitted = true;

													rdxSILInstruction ili;
													rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
													ili.opcode = rdxILOP_rcast_otoi_direct;
													opArg0.li = vst.stackAction;
													opArg1.p = t.GetPOD();
													opArg2.li = newVST.stackAction;
													opArg3.lui = st->m_native.interfaceOffsets->Element(ifcIndex);
													PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
													isDirect = true;
													break;
												}
											}
										}

										if(!isDirect)
										{
											if(emitted)
												RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);	// Don't emit multiple committing instrs per source instruction!
											emitted = true;

											rdxSILInstruction ili;
											rdxUILOpCompactArg opArg0, opArg1, opArg2;
											ili.opcode = rdxILOP_rcast_otoi;
											opArg0.li = vst.stackAction;
											opArg1.p = t.GetPOD();
											opArg2.li = newVST.stackAction;
											PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
										}
									}
									else if(objm->TypeIsClass(t.ToWeakRTRef()) || objm->TypeIsArray(t.ToWeakRTRef()))
									{
										if(emitted)
											RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);	// Don't emit multiple committing instrs per source instruction!
										emitted = true;

										rdxSILInstruction ili;
										rdxUILOpCompactArg opArg0, opArg1, opArg2;
										ili.opcode = rdxILOP_assertinherits;
										opArg0.li = vst.stackAction;
										opArg1.p = t.GetPOD();
										opArg2.li = newVST.stackAction;

										PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
									}
									else
										RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);
								}
								else
									RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);
							}
							else
							{
								rdxSMarkupInstruction mi;
								mi.opcode = rdxMOP_movesa;

								rdxUILOpCompactArg opArg0, opArg1;
								opArg0.li = vst.stackAction;
								opArg1.li = newVST.stackAction;

								PUSH_MARKUP_CA_2(mi, opArg0, opArg1);
							}
						}

					}
					break;
				case rdxOP_localref:
					{
						if(decOp->sint1 < 0)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxLargeUInt refIndex = static_cast<rdxLargeUInt>(decOp->sint1);
						if(refIndex >= localstack->count)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxSVStackValue vst;
						vst.vstType = rdxVST_LocalRef;
						vst.index = refIndex;

						vst.vType = localstack->RetrieveBottom(ctx, refIndex).vType;
						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );
					}
					break;
				case rdxOP_return:
					{
						// Opstack must match the expected return types.
						// Must be followed by a barrier or the end of the stack
						// Return values may be pointers, localrefs, or objectrefs, but must not require conversion to the destination types.

						rdxLargeInt numReturnValsS = decOp->sint1;
						if(numReturnValsS < 0)
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
						rdxLargeUInt numReturnVals = static_cast<rdxLargeUInt>(numReturnValsS);

						if(pass > rdxPASS_CreateStacks)
						{
							if(numReturnVals == opstack->count)
							{
								rdxWeakArrayHdl(rdxTracedRTRef(rdxCType)) returnTypes = method->returnTypes.ToWeakHdl();
								// Returning to the parent frame
								rdxLargeUInt numMethodReturns = 0;
								if(returnTypes.IsNotNull())
									numMethodReturns = returnTypes->NumElements();

								if(numReturnVals != numMethodReturns)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								for(rdxLargeUInt i=0;i<numReturnVals;i++)
								{
									rdxSVStackValue valueVst;
									rdxLargeUInt rvIndex = numReturnVals-1-i;

									RDX_PROTECT_ASSIGN(ctx, valueVst, PopOpstack(ctx, rdxVST_Indeterminate) );
									rdxWeakHdl(rdxCType) destinationReturnType = returnTypes->Element(rvIndex).ToWeakHdl();

									if(valueVst.vstType != rdxVST_LocalRef &&
										valueVst.vstType != rdxVST_Value &&
										!VSTIsPointer(valueVst.vstType))
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

									bool isRefStruct = TypeIsRefStruct(objm, valueVst.vType);
									if( !objm->TypesCompatible(valueVst.vType.ToWeakRTRef(), destinationReturnType.ToWeakRTRef()))
										RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_CONVERSION);

									rdxLargeUInt size, align;
									RDX_PROTECT(ctx, objm->TypeValueSize(ctx, valueVst.vType, size, align) );

									rdxSILInstruction ili;
									rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;

									ili.opcode = rdxILOP_move;
									opArg0.lui = 0;
									opArg0.lui |= RDX_ILOP_MOVE_DEST_PARENT_FRAME;

									if(valueVst.vstType == rdxVST_LocalRef)
									{
										rdxSVLocal vl;
										vl = localstack->RetrieveBottom(ctx, valueVst.index);
										if(vl.isParameter)
										{
											opArg0.lui |= RDX_ILOP_MOVE_SRC_PARENT_FRAME;

											if(vl.isPointer)
											{
												opArg0.lui |= RDX_ILOP_MOVE_SRC_DEREF;
												opArg0.lui |= RDX_ILOP_MOVE_DEST_DEREF;
												opArg0.lui |= RDX_ILOP_MOVE_DEST_DESTROY;
											}
										}
										else
										{
											if(isRefStruct)
											{
												opArg0.lui |= RDX_ILOP_MOVE_DEST_DEREF;
												opArg0.lui |= RDX_ILOP_MOVE_DEST_DESTROY;
											}
										}
										opArg1.li = vl.stackAction;
										opArg2.lui = rvIndex;		// RV index is the stack action index
									}
									else if(VSTIsPointer(valueVst.vstType))
									{
										opArg0.lui |= RDX_ILOP_MOVE_SRC_DEREF;
										opArg0.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
										if(isRefStruct)
										{
											opArg0.lui |= RDX_ILOP_MOVE_DEST_DEREF;
											opArg0.lui |= RDX_ILOP_MOVE_DEST_DESTROY;
										}
										if(!valueVst.isFromLocal)
											opArg0.lui |= RDX_ILOP_MOVE_SRC_OBJECT;
										opArg1.li = valueVst.stackAction;
										opArg2.li = rvIndex;
									}
									else if(valueVst.vstType == rdxVST_Value)
									{
										opArg0.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
										opArg1.li = valueVst.stackAction;
										opArg2.li = rvIndex;
									}
									opArg3.p = destinationReturnType.GetPOD();

									PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);

									rdxLargeUInt sizeUsed, slotsUsed;
									rdxEVSTType vstType = rdxVST_Value;
									if(TypeIsRefStruct(objm, destinationReturnType))
										vstType = rdxVST_Pointer;

									RDX_PROTECT(ctx, OpstackUsed(ctx, vstType, destinationReturnType, &sizeUsed, &slotsUsed) );

									{
										bool overflowed;
										sizeUsed = rdxPaddedSize(sizeUsed, rdxALIGN_RuntimeStackValue, overflowed);
										if(overflowed)
											RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
									}
								}

								{
									rdxSILInstruction ili;
									ili.opcode = rdxILOP_exit;
									PUSH_INSTR(ili);
								}
							}
							else
							{
								// Returning past a barrier in the current frame
								if(numReturnVals < 1 || numReturnVals > opstack->count)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								rdxLargeUInt targetInstruction;
								rdxLargeUInt barrierCount = 1;
								for(targetInstruction=instrNum;targetInstruction<numDecodedInstructions;targetInstruction++)
								{
									if(decOps->Element(targetInstruction).opcode == rdxOP_endbarrier)
										barrierCount--;
									else if(decOps->Element(targetInstruction).opcode == rdxOP_startbarrier)
										barrierCount++;
									if(!barrierCount)
										break;
								}

								if(barrierCount != 0)
									RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);

								// Make sure the barrier matches
								rdxSVStackValue vst;
								RDX_PROTECT_ASSIGN(ctx, vst, opstack->RetrieveTop(ctx, numReturnVals) );

								if(vst.vstType != rdxVST_Barrier || vst.barrierValueCount != numReturnVals)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								for(rdxLargeUInt i=0;i<numReturnVals;i++)
								{
									rdxSVStackValue valueVst;
									rdxSVStackValue destinationVst;
									RDX_PROTECT_ASSIGN(ctx, valueVst, PopOpstack(ctx, rdxVST_Indeterminate) );
									RDX_PROTECT_ASSIGN(ctx, destinationVst, opstack->RetrieveTop(ctx, numReturnVals) );		// This slides due to the above pop

									if(valueVst.vstType != rdxVST_LocalRef &&
										valueVst.vstType != rdxVST_Value &&
										!VSTIsPointer(valueVst.vstType))
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

									bool isRefStruct = TypeIsRefStruct(objm, valueVst.vType);
									if( (isRefStruct && destinationVst.vstType != rdxVST_Pointer) ||
										(!isRefStruct && destinationVst.vstType != rdxVST_ValueShell) ||
										!objm->TypesCompatible(valueVst.vType.ToWeakRTRef(), destinationVst.vType.ToWeakRTRef()))
										RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_CONVERSION);

									rdxLargeUInt size, align;
									RDX_PROTECT(ctx, objm->TypeValueSize(ctx, valueVst.vType, size, align) );

									if(destinationVst.vstType == rdxVST_ValueShell)
										RDX_PROTECT(ctx, LivenShell(ctx, destinationVst) );

									rdxSILInstruction ili;
									rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
									opArg0.lui = 0;
									ili.opcode = rdxILOP_move;

									if(valueVst.vstType == rdxVST_LocalRef)
									{
										rdxSVLocal vl;
										vl = localstack->RetrieveBottom(ctx, valueVst.index);
										if(vl.isParameter)
										{
											opArg0.lui |= RDX_ILOP_MOVE_SRC_PARENT_FRAME;

											if(vl.isPointer)
											{
												opArg0.lui |= RDX_ILOP_MOVE_SRC_DEREF;
												opArg0.lui |= RDX_ILOP_MOVE_DEST_DEREF;
												opArg0.lui |= RDX_ILOP_MOVE_DEST_DESTROY;
											}

											opArg1.li = vl.stackAction;
											opArg2.li = destinationVst.stackAction;
										}
										else
										{
											if(isRefStruct)
												opArg0.lui |= RDX_ILOP_MOVE_DEST_DEREF;

											opArg1.li = vl.stackAction;
											opArg2.li = destinationVst.stackAction;
										}
									}
									else if(valueVst.vstType == rdxVST_Pointer || valueVst.vstType == rdxVST_ConstPointer)
									{
										opArg0.lui |= RDX_ILOP_MOVE_SRC_DEREF;
										opArg0.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
										if(isRefStruct)
											opArg0.lui |= RDX_ILOP_MOVE_DEST_DEREF;
										opArg1.li = valueVst.stackAction;
										opArg2.li = destinationVst.stackAction;
									}
									else if(valueVst.vstType == rdxVST_Value)
									{
										opArg0.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
										opArg1.li = valueVst.stackAction;
										opArg2.li = destinationVst.stackAction;
									}
									else
										RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);

									opArg3.p = destinationVst.vType.GetPOD();

									PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
								}

								{
									rdxSILInstruction ili;
									rdxUILOpCompactArg opArg0;
									ili.opcode = rdxILOP_jump;
									RDX_PROTECT_ASSIGN(ctx, opArg0.lui, StitchJump(ctx, instrNum, targetInstruction) );

									PUSH_INSTR_CA_1(ili, opArg0);
								}
							}
						}
						else
						{
							for(rdxLargeUInt i=0;i<numReturnVals;i++)
								RDX_PROTECT(ctx, PopOpstack(ctx) );
						}
					}
					break;
				case rdxOP_res:
					{
						rdxSVStackValue vst;
						rdxSILInstruction ili;
						rdxUILOpLargeArg opArg0, opArg1;

						rdxWeakHdl(rdxCObject) res = decOp->res;
						if(res.IsNull())
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						rdxWeakHdl(rdxCType) resType = res->ObjectInfo()->containerType.ToWeakHdl();
						rdxWeakHdl(rdxCType) resTypeType = resType->ObjectInfo()->containerType.ToWeakHdl();

						if(resTypeType == objm->GetBuiltIns()->st_StructuredType)
						{
							switch(resType.StaticCast<rdxCStructuredType>()->storageSpecifier)
							{
							case rdxSS_RefStruct:
							case rdxSS_ValStruct:
							case rdxSS_Enum:
								{
									vst.vstType = rdxVST_Pointer;
									vst.vType = resType;

									if(res->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_ConstantStructure)
										vst.vstType = rdxVST_ConstPointer;

									ili.opcode = rdxILOP_immediate_rtp;
									rdxOffsetHdlPOD podRTP;
									podRTP.hdl = res.GetPOD();
									podRTP.offset = 0;
									opArg0.rtp = podRTP;
									opArg1.ca[1].p = resType.GetPOD();
								}
								break;
							case rdxSS_Interface:
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
								break;
								// TODO MUSTFIX: Resources must not be interfaces...

							case rdxSS_Class:
								{
									vst.vstType = rdxVST_Value;
									vst.vType = resType;

									ili.opcode = rdxILOP_immediate_ptr;
									opArg0.ca[0].p = res.GetPOD();
									opArg1.ca[1].p = resType.GetPOD();
								}
								break;
							default:
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
							};
						}
						else if(resTypeType == objm->GetBuiltIns()->st_ArrayOfType)
						{
							vst.vstType = rdxVST_Value;
							vst.vType = resType;

							ili.opcode = rdxILOP_immediate_ptr;
							opArg0.ca[0].p = res.GetPOD();
							opArg1.ca[1].p = resType.GetPOD();
						}
						else
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

						opArg1.ca[0].li = vst.stackAction;
						PUSH_INSTR_LA_2(ili, opArg0, opArg1);
					}
					break;
				case rdxOP_constant:
					{
						rdxSVStackValue vst;
						vst.vstType = rdxVST_Value;
						if(decOp->res.IsNull() || !objm->TypesCompatible(decOp->res->ObjectInfo()->containerType.ToWeakRTRef(), objm->GetBuiltIns()->st_Type.ToWeakRTRef()) )
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						vst.vType = decOp->res.StaticCast<rdxCType>();

						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

						if(pass > rdxPASS_CreateStacks)
						{
							rdxSILInstruction ili;
							rdxUILOpLargeArg opArg0, opArg1, opArg2;
							ili.opcode = rdxILOP_immediate;
							opArg1.ca[0].li = vst.stackAction;

							//rdxHugeInt hi1 = decOp->hsi1;

							if(vst.vType.IsNull())
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxSSerializationTag *serTag = vst.vType->ObjectInfo()->SerializationTag();
							if(!serTag)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							rdxSObjectGUID typeGUID = serTag->gstSymbol;

							opArg1.ca[1].p = vst.vType.GetPOD();

							if(vst.vType->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType &&
								vst.vType.StaticCast<rdxCStructuredType>()->storageSpecifier == rdxSS_Enum)
							{
								DECODE_INTEGER(rdxEnumValue, opArg0.ca[0].ev);
								if(!objm->EnumCompatible(opArg0.ca[0].ev, vst.vType.StaticCast<rdxCStructuredType>()->enumerants.ToWeakRTRef()))
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								opArg2.ca[0].lui = sizeof(rdxEnumValue);
							}
							else if(typeGUID == rdxConstantTypes::coreInt)
							{
								DECODE_INTEGER(rdxInt, opArg0.ca[0].si);
								opArg2.ca[0].lui = sizeof(rdxInt);
							}
							else if(typeGUID == rdxConstantTypes::coreUInt)
							{
								DECODE_INTEGER(rdxInt, opArg0.ca[0].si);
								opArg2.ca[0].lui = sizeof(rdxUInt);
							}
							else if(typeGUID == rdxConstantTypes::coreByte)
							{
								DECODE_INTEGER(rdxByte, opArg0.ca[0].b);
								opArg2.ca[0].lui = sizeof(rdxByte);
							}
							else if(typeGUID == rdxConstantTypes::coreLargeInt)
							{
								DECODE_INTEGER(rdxLargeInt, opArg0.ca[0].li);
								opArg2.ca[0].lui = sizeof(rdxLargeInt);
							}
							else if(typeGUID == rdxConstantTypes::coreLargeUInt)
							{
								// Using .li is intentional, since decodes are always signed
								DECODE_INTEGER(rdxLargeInt, opArg0.ca[0].li);
								opArg2.ca[0].lui = sizeof(rdxLargeUInt);
							}
							else if(typeGUID == rdxConstantTypes::coreFloat)
							{
								DECODE_FLOAT(rdxFloat, opArg0.ca[0].f);
								opArg2.ca[0].lui = sizeof(rdxFloat);
							}
							else if(typeGUID == rdxConstantTypes::coreDouble)
							{
								DECODE_FLOAT(rdxDouble, opArg0.dbl);
								opArg2.ca[0].lui = sizeof(rdxDouble);
							}
							else if(typeGUID == rdxConstantTypes::coreBool)
							{
								rdxInt bVal;
								DECODE_INTEGER(rdxInt, bVal);
								if(bVal != 0)
									opArg0.ca[0].bo = rdxTrueValue;
								else
									opArg0.ca[0].bo = rdxFalseValue;
								opArg2.ca[0].lui = sizeof(rdxBool);
							}
							else
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							PUSH_INSTR_LA_3(ili, opArg0, opArg1, opArg2);
						}
					}
					break;
				case rdxOP_constant_str:
					{
						rdxSVStackValue vst;
						vst.vstType = rdxVST_Value;
						vst.vType = decOp->res.StaticCast<rdxCType>();
						if(vst.vType.IsNull() || !objm->TypesCompatible(vst.vType->ObjectInfo()->containerType.ToWeakRTRef(), objm->GetBuiltIns()->st_Type.ToWeakRTRef()) )
							RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

						if(pass > rdxPASS_CreateStacks)
						{
							rdxSILInstruction ili;
							rdxUILOpLargeArg opArg0, opArg1;
							ili.opcode = rdxILOP_immediate_ptr;
							opArg1.ca[0].li = vst.stackAction;

							rdxSSerializationTag *serTag = vst.vType->ObjectInfo()->SerializationTag();
							if(!serTag)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxSObjectGUID typeGUID = serTag->gstSymbol;
							rdxWeakHdl(rdxCString) ivalue = decOp->str;
							
							if(typeGUID == rdxConstantTypes::coreString)
							{
								ili.opcode = rdxILOP_immediate_ptr;
								if(ivalue.IsNull())
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								opArg0.ca[0].p = ivalue.GetPOD();
								opArg1.ca[1].lui = sizeof(rdxTracedRTRef(rdxCString));
							}
							else
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							PUSH_INSTR_LA_2(ili, opArg0, opArg1);
						}
					}
					break;
				case rdxOP_switch:
					{
						rdxSVStackValue vst;
						RDX_PROTECT_ASSIGN(ctx, vst, PopOpstack(ctx, rdxVST_Indeterminate) );

						if(pass > rdxPASS_CreateStacks)
						{
							rdxEVSTType valueVSTType = vst.vstType;
							if(valueVSTType != rdxVST_Value && !VSTIsPointer(valueVSTType))
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							if(opstack->count != 0 && opstack->RetrieveTop(ctx, 0).vstType != rdxVST_Barrier)
								RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);

							if(vst.vType.IsNull())
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxWeakHdl(rdxCObject) searchArray = decOp->res;

							// Don't really care about overflows in this case because they'll just mismatch the array count check
							rdxLargeUInt numCases = static_cast<rdxLargeUInt>(decOp->sint1);

							if(searchArray.IsNull())
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							if((searchArray->ObjectInfo()->objectFlags & rdxGCInfo::GCOF_ConstantArray) == 0)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							// Make sure that the array is of the value type
							rdxGCInfo *searchArrayGCI = searchArray->ObjectInfo();
							if(searchArrayGCI->containerType.IsNull() || searchArrayGCI->containerType->ObjectInfo()->containerType != objm->GetBuiltIns()->st_ArrayOfType)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxWeakHdl(rdxCArrayOfType) searchArrayType = searchArrayGCI->containerType.StaticCast<rdxCArrayOfType>();

							if(searchArray.StaticCast<rdxCArrayContainer>()->NumElements() != numCases)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							if(!objm->TypesCompatible(vst.vType.ToWeakRTRef(), searchArrayType->type.ToWeakRTRef()))
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxLargeUInt size, align;
							RDX_PROTECT(ctx, objm->TypeValueSize(ctx, vst.vType, size, align) );

							if(numCases <= 0)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							// Make sure that there's an instruction after the case jumps
							if(pass > rdxPASS_CreateStacks && ((numDecodedInstructions - instrNum) < 2 || numDecodedInstructions - instrNum - 2 < numCases))
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							// Make sure that the next numCases instructions are all jumps that jump forward
							{
								for(rdxLargeUInt i=0;i<numCases;i++)
								{
									rdxLargeUInt caseInstrNum = instrNum + i + 1;
									if(caseInstrNum == numDecodedInstructions)
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

									if(decOps->Element(caseInstrNum).opcode != rdxOP_jump || decOps->Element(caseInstrNum).sint1 <= 0)
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								}
							}

							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;

							if(VSTIsPointer(valueVSTType))
								ili.opcode = rdxILOP_switch_ptr;
							else
								ili.opcode = rdxILOP_switch;
							opArg0.li = vst.stackAction;
							opArg1.p = decOp->res.GetPOD();
							opArg2.li = decOp->sint1;
							opArg3.p = vst.vType.GetPOD();

							PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
						}
					}
					break;
				case rdxOP_iteratearray:
					{
						RDX_PROTECT(ctx, SyncJournal(ctx) );

						if(decOp->sint1 <= 0)
						{
							rdxSILInstruction tickILI;
#ifdef RDX_ENABLE_TIMESLICE_COUNTER
							tickILI.opcode = rdxILOP_tick;
#else
							tickILI.opcode = rdxILOP_hardenstack;
#endif
							PUSH_INSTR(tickILI);
							meta->translatedInstrNum++;
						}
						rdxSVStackValue arrayVst;
						rdxSVStackValue indexVst;
						rdxSVStackValue destVst;
						rdxSVStackValue subIndexVst;
						RDX_PROTECT_ASSIGN(ctx, arrayVst, PopOpstack(ctx, rdxVST_LocalRef) );
						RDX_PROTECT_ASSIGN(ctx, indexVst, PopOpstack(ctx, rdxVST_LocalRef) );
						RDX_PROTECT_ASSIGN(ctx, destVst, PopOpstack(ctx, rdxVST_LocalRef) );

						if(decOp->sint2 != 0)
						{
							RDX_PROTECT_ASSIGN(ctx, subIndexVst, PopOpstack(ctx, rdxVST_LocalRef) );
						}

						if(pass > rdxPASS_CreateStacks)
						{
							if(opstack->count != 0 && opstack->RetrieveTop(ctx, 0).vstType != rdxVST_Barrier)
								RDX_STHROW(ctx, RDX_ERROR_IL_FORBIDDEN_OPERATION_STATE);

							rdxSVLocal arrayLocal = localstack->RetrieveBottom(ctx, arrayVst.index);
							rdxSVLocal indexLocal = localstack->RetrieveBottom(ctx, indexVst.index);
							rdxSVLocal destLocal = localstack->RetrieveBottom(ctx, destVst.index);
							rdxSVLocal subIndexLocal;

							if(arrayLocal.isParameter || arrayLocal.vType.IsNull() ||
								arrayLocal.vType->ObjectInfo()->containerType != objm->GetBuiltIns()->st_ArrayOfType)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxWeakHdl(rdxCArrayOfType) arrayLocalType = arrayLocal.vType.StaticCast<rdxCArrayOfType>();

							if(indexLocal.isParameter || indexVst.vType != objm->GetBuiltIns()->st_LargeUInt)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							if(decOp->sint2 != 0)
							{
								if(decOp->sint2 < 0)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								
								rdxLargeUInt numSubIndexes = static_cast<rdxLargeUInt>(decOp->sint2);
								if(arrayLocal.vType.StaticCast<rdxCArrayOfType>()->numDimensions != numSubIndexes)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								subIndexLocal = localstack->RetrieveBottom(ctx, subIndexVst.index);

								// Make sure each subindex local is a largeint
								for(rdxLargeUInt i=0;i<numSubIndexes;i++)
								{
									rdxSVLocal checkLocal;
									RDX_PROTECT_ASSIGN(ctx, checkLocal, localstack->RetrieveBottom(ctx, subIndexVst.index + i) );
									if(checkLocal.isParameter || checkLocal.vType != objm->GetBuiltIns()->st_LargeUInt)
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								}
							}

							if(destLocal.isParameter || destVst.vType.IsNull() || destVst.vType != arrayLocalType->type)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							SAFE_DECODE_LABEL(targetInstruction, sint1);

							rdxSILInstruction ili;

							rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4, opArg5, opArg6;
							opArg0.li = arrayLocal.stackAction;
							opArg1.li = indexLocal.stackAction;
							opArg2.li = destLocal.stackAction;
							RDX_PROTECT_ASSIGN(ctx, opArg3.lui, StitchJump(ctx, instrNum, targetInstruction) );

							opArg4.p = destVst.vType.GetPOD();

							if(decOp->sint2 == 0)
							{
								ili.opcode = rdxILOP_iteratearray;
								PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
							}
							else
							{
								ili.opcode = rdxILOP_iteratearraysub;
								opArg5.li = subIndexLocal.stackAction;
								opArg6.lui = static_cast<rdxLargeUInt>(decOp->sint2);
								PUSH_INSTR_CA_7(ili, opArg0, opArg1, opArg2, opArg3, opArg4, opArg5, opArg6);
							}
						}
					}
					break;
				default:
					RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
				};
			}

			// If this triggers and stackjournal->count != numMethodParameters, then the opstack was probably manipulated with
			// direct access instead of AppendOpstack and PopOpstack
			if(opstack->count != 0 || localstack->count != numMethodParameters || stackjournal->count != numMethodParameters)
				RDX_STHROW(ctx, RDX_ERROR_IL_STACKS_UNBALANCED);

			rdxSILInstruction ili;
			ili.opcode = rdxILOP_fatal;
			PUSH_INSTR(ili);

			FLUSH_RESUME_FLAGS;

			localstack->count = 0;
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROW(ctx);
		}
		RDX_ENDTRY
	}

	static void DecodeOps(rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) m, rdxIObjectManager *objm, rdxArrayCRef(rdxUInt8) &decompressedBytecodeArray, rdxWeakArrayHdl(rdxSILDecodedOp) decOps)
	{
		RDX_TRY(ctx)
		{
			if(m->bytecode->NumElements() == 0)
				RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

			rdxWeakOffsetRTRef(rdxUInt8) bytecode;
			rdxLargeUInt decompressedBytecodeSize;

			{
				// Decompress bytecode
				rdxWeakOffsetRTRef(rdxUInt8) bytecodeCursor = m->bytecode->OffsetElementRTRef(0);

				if(bytecodeCursor.Data()[0] == 0)
				{
					decompressedBytecodeSize = m->bytecode->NumElements() - 1;
					bytecode = bytecodeCursor + 1;
				}
				else
				{
					rdxLargeUInt compressedBytesRemaining = m->bytecode->NumElements();
					rdxLargeInt ssize;
					if(!DecodeInt<rdxLargeInt>(bytecodeCursor, compressedBytesRemaining, ssize))
						RDX_STHROW(ctx, RDX_ERROR_IL_ARGUMENT_UNDERRUN);
					if(ssize < 0)
						RDX_STHROW(ctx, RDX_ERROR_IL_ARGUMENT_UNDERRUN);
					decompressedBytecodeSize = static_cast<rdxLargeUInt>(ssize);
					if(decompressedBytecodeSize == 0 || decompressedBytecodeSize <= compressedBytesRemaining)
						RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);	// The compressor should not do this
					if(compressedBytesRemaining > RDX_COMPRESSED_BYTECODE_MAXSIZE)
						RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

					RDX_PROTECT_ASSIGN(ctx, decompressedBytecodeArray, rdxCInternalObjectFactory::Create1DArray<rdxUInt8>(ctx, objm, decompressedBytecodeSize));

					int decompressedSize = LZ4_decompress_safe(reinterpret_cast<const char*>(bytecodeCursor.Data()), reinterpret_cast<char*>(decompressedBytecodeArray->ArrayModify()), static_cast<int>(compressedBytesRemaining), static_cast<int>(decompressedBytecodeSize));
					if(decompressedSize < 0 || static_cast<rdxLargeUInt>(decompressedSize) != decompressedBytecodeSize)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

					bytecode = decompressedBytecodeArray->OffsetElementRTRef(0);
				}
			}

			rdxLargeUInt numInstructions = m->numInstructions;
			rdxLargeUInt bytesRemaining = decompressedBytecodeSize;

			for(rdxLargeUInt i=0;i<numInstructions;i++)
			{
				rdxSILDecodedOp decOp;
				memset(&decOp, 0, sizeof(decOp));

				rdxLargeInt opcode;
				if(!DecodeInt<rdxLargeInt>(bytecode, bytesRemaining, opcode))
					RDX_STHROW(ctx, RDX_ERROR_IL_ARGUMENT_UNDERRUN);

				if(opcode < 0 || opcode >= rdxOP_Count)
					RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPCODE);
				decOp.opcode = static_cast<rdxEOpcode>(opcode);

				// Decode arguments
				int argParamFlags = RDX_OPCODE_INFO[opcode].paramFlags;

				if(argParamFlags & rdxOPARGS_INT1)
				{
					if(!DecodeInt<rdxHugeInt>(bytecode, bytesRemaining, decOp.hsi1))
						RDX_STHROW(ctx, RDX_ERROR_IL_ARGUMENT_UNDERRUN);
					decOp.sint1 = static_cast<rdxLargeInt>(decOp.hsi1);
				}

				if(argParamFlags & rdxOPARGS_INT2)
				{
					if(!DecodeInt<rdxLargeInt>(bytecode, bytesRemaining, decOp.sint2))
						RDX_STHROW(ctx, RDX_ERROR_IL_ARGUMENT_UNDERRUN);
				}

				if(argParamFlags & rdxOPARGS_INTVAR)
				{
					rdxLargeInt intVarCountS;
					if(!DecodeInt(bytecode, bytesRemaining, intVarCountS))
						RDX_STHROW(ctx, RDX_ERROR_IL_ARGUMENT_UNDERRUN);
					if(intVarCountS < 0)
						RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
					decOp.intVarCount = static_cast<rdxLargeUInt>(intVarCountS);
					decOp.intVarStart = bytecode;

					rdxLargeInt dummy;
					for(rdxLargeUInt vi=0;vi<decOp.intVarCount;vi++)
						if(!DecodeInt(bytecode, bytesRemaining, dummy))
							RDX_STHROW(ctx, RDX_ERROR_IL_ARGUMENT_UNDERRUN);

					decOp.intVarBytes = static_cast<rdxLargeUInt>(bytecode - decOp.intVarStart.ToRTRef());
				}
				if(argParamFlags & rdxOPARGS_STR)
				{
					rdxLargeInt resIndexS;
					if(!DecodeInt(bytecode, bytesRemaining, resIndexS))
						RDX_STHROW(ctx, RDX_ERROR_IL_ARGUMENT_UNDERRUN);
					if(resIndexS < 0)
						RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
					rdxLargeUInt resIndex = static_cast<rdxLargeUInt>(resIndexS);
					
					if(m->resArgs.IsNull() || resIndex >= m->resArgs->NumElements())
						RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
					rdxWeakHdl(rdxCObject) strResArg = m->resArgs->Element(resIndex).ToWeakHdl();
					if(strResArg.IsNotNull() && strResArg->ObjectInfo()->containerType != objm->GetBuiltIns()->st_String)
						RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
					decOp.str = strResArg.StaticCast<rdxCString>();
				}
				if(argParamFlags & rdxOPARGS_RES)
				{
					rdxLargeInt resIndexS;
					if(!DecodeInt(bytecode, bytesRemaining, resIndexS))
						RDX_STHROW(ctx, RDX_ERROR_IL_ARGUMENT_UNDERRUN);
					if(resIndexS < 0)
						RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
					rdxLargeUInt resIndex = static_cast<rdxLargeUInt>(resIndexS);

					if(m->resArgs.IsNull() || resIndex >= m->resArgs->NumElements())
						RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
					decOp.res = m->resArgs->Element(resIndex).ToWeakHdl();
				}

				decOps->Element(i) = decOp;
			}

			if(bytesRemaining)
				RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROW(ctx);
		}
		RDX_ENDTRY
	}
};

void rdxCompactJournals(rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) m, rdxWeakArrayHdl(rdxSStackJournal) journals, rdxLargeUInt numJournals, rdxIObjectManager *objm, rdxLargeUInt &outCompactSize, rdxLargeUInt &outNumDropped)
{
	RDX_TRY(ctx)
	{
		// Determine what is traceable
		for(rdxLargeUInt j=0;j<numJournals;j++)
			journals->Element(j).isTraceable = objm->TypeCanBeTraced(journals->Element(j).vType.ToWeakRTRef());

		// Compact the journals
		rdxLargeUInt compactSize = 0;
		rdxLargeUInt numDroppedJournals = 0;
		for(rdxLargeUInt j=0;j<numJournals;j++)
		{
#ifndef RDX_JOURNAL_ALL_VALUES
			if(journals->Element(j).isTraceable)
#endif
			{
				rdxLargeUInt jBytes;
				journals->Element(j).Compress(NULL, &jBytes);
				compactSize += jBytes;
			}
#ifndef RDX_JOURNAL_ALL_VALUES
			else
				numDroppedJournals++;
#endif
		}

		rdxWeakOffsetHdl(rdxUInt8) outp;
		if(compactSize)
		{
			rdxWeakArrayHdl(rdxUInt8) h;
			RDX_PROTECT_ASSIGN(ctx, h, rdxCInternalObjectFactory::Create1DArray<rdxUInt8>(ctx, objm, compactSize).ToWeakHdl() );
			m->m_native.compactedJournals = h;
			outp = h->OffsetElementRTRef(0).ToHdl();
		}

		for(rdxLargeUInt j=0;j<numJournals;j++)
		{
#ifndef RDX_JOURNAL_ALL_VALUES
			if(journals->Element(j).isTraceable)
#endif
			{
				rdxLargeUInt nCompressedBytes;
				journals->Element(j).Compress(&outp, &nCompressedBytes);
				outp += nCompressedBytes;
			}
		}

		outCompactSize = compactSize;
		outNumDropped = numDroppedJournals;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxJournalNativeMethodParameters(rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) method, rdxIObjectManager *objm)
{
	RDX_TRY(ctx)
	{
		rdxLargeUInt numMethodParameters = 0;
		if(method->parameters.IsNotNull())
			numMethodParameters = method->parameters->NumElements();

		rdxArrayCRef(rdxSStackJournal) journals;
		RDX_PROTECT_ASSIGN(ctx, journals, rdxCInternalObjectFactory::Create1DArray<rdxSStackJournal>(ctx, objm, numMethodParameters) );
		rdxLargeInt poffs = 0;
		for(rdxLargeUInt i=0;i<numMethodParameters;i++)
		{
			rdxWeakOffsetHdl(rdxSMethodParameter) param = method->parameters->OffsetElementRTRef(i).ToHdl();

			// This should be caught earlier, but be safe...
			if(param->type.IsNull())
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
			bool isPointer = rdxCILPassCompiler::TypeIsRefStruct(objm, param->type.ToWeakHdl());
			// NotNull parameters must be constant
			if(param->isNotNull != rdxFalseValue && param->isConstant == rdxFalseValue)
				RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

			{
				rdxSStackJournal sj;
				sj.isParameter = true;
				sj.isFromLocal = false;
				sj.isVarying = (param->type == objm->GetBuiltIns()->st_Varying);
				sj.startInstruction = 0;
				sj.endInstruction = 0;
				sj.isPointer = isPointer;
				sj.pointerSourceType = rdxPST_Parameter;
				sj.vType = param->type.ToWeakHdl();
				sj.name = rdxWeakHdl(rdxCString)::Null();
				sj.isConstant = (param->isConstant != rdxFalseValue);
				sj.isNotNull = (param->isNotNull != rdxFalseValue);
				sj.notNullInstruction = 0;

				rdxLargeUInt psize = 0;
				rdxLargeUInt palign = 0;
				if(sj.isPointer)
					psize = sizeof(rdxWeakOffsetRTRef(rdxCObject));
				else if(sj.isVarying)
					psize = sizeof(rdxSVarying);
				else
					RDX_PROTECT(ctx, objm->TypeValueSize(ctx, param->type.ToWeakHdl(), psize, palign) );

				poffs += rdxPaddedSize(psize, rdxALIGN_RuntimeStackValue);
				sj.offset = -poffs;
				sj.pointerSource = sj.offset;

				if(i == method->thisParameterOffset - 1)
					method->m_native.thisParameterInvokeOffset = sj.offset;

				journals->Element(i) = sj;
			}
		}

		rdxLargeUInt compactSize;
		rdxLargeUInt numDroppedJournals;
		rdxWeakArrayHdl(rdxSStackJournal) journalsHdl;
		journalsHdl = journals.ToWeakHdl();
		RDX_PROTECT(ctx, rdxCompactJournals(ctx, method, journalsHdl, numMethodParameters, objm, compactSize, numDroppedJournals));
		method->m_native.numJournals = numMethodParameters - numDroppedJournals;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxCompileMethod(rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) m, rdxIObjectManager *objm)
{
	RDX_TRY(ctx)
	{
		rdxSSerializationTag *serTag = m->ObjectInfo()->SerializationTag();
		rdxCILPassCompiler ilp;

		if(m->bytecode.IsNull())
		{
			if(m->m_native.isNativeCall)
				RDX_PROTECT(ctx, rdxJournalNativeMethodParameters(ctx, m, objm));
			return;
		}

		const rdxUInt8 *debugGUID = m->ObjectInfo()->SerializationTag()->gstSymbol.m_bytes;
		if(debugGUID[0] == 0x21 && debugGUID[1] == 0x50)
		{
			int bp = 0;
		}

		rdxLargeUInt numInstructions = m->numInstructions;
		rdxArrayCRef(rdxUInt8) decompressedBytecodeArray = rdxArrayCRef(rdxUInt8)::Null();	// Needed to hold intVar references

		rdxSPushList<rdxSVStackValue> opstack;
		rdxSPushList<rdxSVLocal> localstack;
		rdxSPushList<rdxSILInstruction> instructions;
		rdxSPushList<rdxSMarkupInstruction> markupInstructions;
		rdxSPushList<rdxUInt8> instructionResumeFlags;
		rdxSPushList<rdxUILOpCompactArg> compactArgs;
		rdxSPushList<rdxUILOpLargeArg> largeArgs;
		rdxSPushList<rdxSVStackJournal> stackjournal;
		rdxSPushList<rdxSMILStackAction> stackactions;
		rdxSPushList<rdxSStackJournal> journal;
		rdxSPushList<rdxSExceptionHandlerJournal> exHandlers;
		rdxSPushList<rdxSExceptionRecoveryLocation> exRLs;

		rdxArrayCRef(rdxSInstructionMeta) metalist;
		RDX_PROTECT_ASSIGN(ctx, metalist, rdxCInternalObjectFactory::Create1DArray<rdxSInstructionMeta>(ctx, objm, numInstructions) );
		rdxArrayCRef(rdxSILDecodedOp) decOps;
		RDX_PROTECT_ASSIGN(ctx, decOps, rdxCInternalObjectFactory::Create1DArray<rdxSILDecodedOp>(ctx, objm, numInstructions) );

		RDX_PROTECT(ctx, rdxCILPassCompiler::DecodeOps(ctx, m, objm, decompressedBytecodeArray, decOps.ToWeakHdl()));

		ilp.metalist = metalist.ToWeakHdl();
		ilp.method = m;
		ilp.objm = objm;
		ilp.opstack = &opstack;
		ilp.localstack = &localstack;
		ilp.instructions = &instructions;
		ilp.markupInstructions = &markupInstructions;
		ilp.instructionResumeFlags = &instructionResumeFlags;
		ilp.compactArgs = &compactArgs;
		ilp.largeArgs = &largeArgs;
		ilp.stackjournal = &stackjournal;
		ilp.stackactions = &stackactions;
		ilp.exHandlers = &exHandlers;
		ilp.exRLs = &exRLs;
		ilp.journal = &journal;
		ilp.decOps = decOps.ToWeakHdl();
		ilp.lowestOffset = 0;
		ilp.slotMaximum = 0;
		ilp.sequentialID = 0;
		ilp.st_Exception = objm->LookupSymbolSimple(ctx, rdxSObjectGUID::FromObjectName("Core", "Exception")).StaticCast<rdxCStructuredType>();

		ilp.pass = rdxPASS_CreateStacks;
		RDX_PROTECT(ctx, ilp.CompilePass(ctx) );
		RDX_PROTECT_ASSIGN(ctx, opstack.list, rdxCInternalObjectFactory::Create1DArray<rdxSVStackValue>(ctx, objm, opstack.maximum) );
		RDX_PROTECT_ASSIGN(ctx, localstack.list, rdxCInternalObjectFactory::Create1DArray<rdxSVLocal>(ctx, objm, localstack.maximum) );
		RDX_PROTECT_ASSIGN(ctx, stackjournal.list, rdxCInternalObjectFactory::Create1DArray<rdxSVStackJournal>(ctx, objm, stackjournal.maximum) );

		ilp.pass = rdxPASS_GenerateCode;
		instructions.count = 0;
		markupInstructions.count = 0;
		instructionResumeFlags.count = 0;
		compactArgs.count = 0;
		largeArgs.count = 0;
		journal.count = 0;
		stackjournal.count = 0;
		stackactions.count = 0;
		exHandlers.count = 0;
		exRLs.count = 0;
		RDX_PROTECT(ctx, ilp.CompilePass(ctx) );

		// Round up the size of the stack to the opstack boundary
		ilp.lowestOffset &= ~(rdxALIGN_RuntimeStackValue - 1);;

		RDX_PROTECT_ASSIGN(ctx, instructions.list, rdxCInternalObjectFactory::Create1DArray<rdxSILInstruction>(ctx, objm, instructions.maximum) );
		RDX_PROTECT_ASSIGN(ctx, markupInstructions.list, rdxCInternalObjectFactory::Create1DArray<rdxSMarkupInstruction>(ctx, objm, markupInstructions.maximum) );
		RDX_PROTECT_ASSIGN(ctx, instructionResumeFlags.list, rdxCInternalObjectFactory::Create1DArray<rdxUInt8>(ctx, objm, instructionResumeFlags.maximum) );
		RDX_PROTECT_ASSIGN(ctx, compactArgs.list, rdxCInternalObjectFactory::Create1DArray<rdxUILOpCompactArg>(ctx, objm, compactArgs.maximum) );
		RDX_PROTECT_ASSIGN(ctx, largeArgs.list, rdxCInternalObjectFactory::Create1DArray<rdxUILOpLargeArg>(ctx, objm, largeArgs.maximum) );
		RDX_PROTECT_ASSIGN(ctx, journal.list, rdxCInternalObjectFactory::Create1DArray<rdxSStackJournal>(ctx, objm, journal.maximum) );
		RDX_PROTECT_ASSIGN(ctx, exHandlers.list, rdxCInternalObjectFactory::Create1DArray<rdxSExceptionHandlerJournal>(ctx, objm, exHandlers.maximum) );
		RDX_PROTECT_ASSIGN(ctx, exRLs.list, rdxCInternalObjectFactory::Create1DArray<rdxSExceptionRecoveryLocation>(ctx, objm, exRLs.maximum) );
		RDX_PROTECT_ASSIGN(ctx, stackactions.list, rdxCInternalObjectFactory::Create1DArray<rdxSMILStackAction>(ctx, objm, stackactions.maximum) );
		instructions.count = 0;
		markupInstructions.count = 0;
		instructionResumeFlags.count = 0;
		compactArgs.count = 0;
		largeArgs.count = 0;
		journal.count = 0;
		stackjournal.count = 0;
		stackactions.count = 0;
		exHandlers.count = 0;
		exRLs.count = 0;
		RDX_PROTECT(ctx, ilp.CompilePass(ctx) );

		// Jump availability is now known, run a stitch pass
		instructions.count = 0;
		markupInstructions.count = 0;
		instructionResumeFlags.count = 0;
		compactArgs.count = 0;
		largeArgs.count = 0;
		journal.count = 0;
		stackjournal.count = 0;
		stackactions.count = 0;
		exHandlers.count = 0;
		exRLs.count = 0;
		ilp.pass = rdxPASS_StitchJumps;
		RDX_PROTECT(ctx, ilp.CompilePass(ctx) );

		{
			m->m_native.ilinstructions = instructions.list.ToWeakHdl();
			m->m_native.ilResumeFlags = instructionResumeFlags.list.ToWeakHdl();
			m->m_native.numILInstructions = instructions.count;
			m->m_native.largeArgs = largeArgs.list.ToWeakHdl();
			m->m_native.compactArgs = compactArgs.list.ToWeakHdl();
			m->m_native.milStackActions = stackactions.list.ToWeakHdl();
			m->m_native.markupInstructions = markupInstructions.list.ToWeakHdl();

			rdxLargeInt lowestOffsetExtraPadded = ilp.lowestOffset;
			lowestOffsetExtraPadded &= ~(RDX_MAX_ALIGNMENT - 1);
			m->m_native.frameCapacity = static_cast<rdxLargeUInt>(-lowestOffsetExtraPadded);

			m->m_native.exHandlers = exHandlers.list.ToWeakHdl();
		}

		{
			rdxArrayCRef(rdxLargeUInt) translation1;

			RDX_PROTECT_ASSIGN(ctx, translation1, rdxCInternalObjectFactory::Create1DArray<rdxLargeUInt>(ctx, objm, numInstructions).ToWeakHdl() );
			
			m->m_native.translation1 = translation1;
			for(rdxLargeUInt i=0;i<numInstructions;i++)
			{
				translation1->Element(i) = metalist->Element(i).translatedInstrNum;
			}
		}

		// Move everything out of translation space
		{
			for(rdxLargeUInt i=0;i<journal.count;i++)
			{
				rdxWeakOffsetHdl(rdxSInstructionMeta) endMeta = metalist->OffsetElementRTRef(journal.list->Element(i).endInstruction).ToHdl();
				rdxWeakOffsetHdl(rdxSInstructionMeta) startMeta = metalist->OffsetElementRTRef(journal.list->Element(i).startInstruction).ToHdl();

				journal.list->Element(i).startInstruction = startMeta->translatedInstrNum + startMeta->numValidationInstrs;
				journal.list->Element(i).endInstruction = endMeta->translatedInstrNum + endMeta->numValidationInstrs;
			}
		}

		if(journal.maximum)
		{
			rdxLargeUInt compactSize;
			rdxLargeUInt numDroppedJournals;
			rdxCompactJournals(ctx, m, journal.list.ToWeakHdl(), journal.maximum, objm, compactSize, numDroppedJournals);
			m->m_native.numJournals = journal.maximum - numDroppedJournals;
		}

		{
			for(rdxLargeUInt i=0;i<exHandlers.count;i++)
			{
				rdxWeakOffsetHdl(rdxSExceptionHandlerJournal) exj = exHandlers.list->OffsetElementRTRef(i).ToHdl();
				rdxWeakOffsetHdl(rdxSInstructionMeta) endMeta = metalist->OffsetElementRTRef(exj->endInstruction).ToHdl();

				bool foundMatch = false;
				for(rdxLargeUInt j=0;j<exRLs.count;j++)
				{
					const rdxSExceptionRecoveryLocation &exrl = exRLs.list->Element(j);
					if(exrl.instrNum == exj->handlerInstruction - 1)
					{
						exj->catchStackOffset = exrl.bpOffset;
						foundMatch = true;
						break;
					}
				}

				if(!foundMatch)
					RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);

				exj->startInstruction = metalist->Element(exj->startInstruction).translatedInstrNum;
				exj->endInstruction = endMeta->translatedInstrNum + endMeta->numValidationInstrs;
				exj->handlerInstruction = ilp.FindJumpTarget(metalist->Element(exj->handlerInstruction).translatedInstrNum);
			}
		}

		// Create call points and debug info, flag resumption points
		{
			rdxLargeUInt numCallPoints = 0;
			rdxLargeUInt numDebugInfo = 0;

			for(rdxLargeUInt i=0;i<instructions.count;i++)
			{
				bool shouldMarkNext = false;
				switch(instructions.list->Element(i).opcode)
				{
				case rdxILOP_tick:
					shouldMarkNext = true;
					break;
				case rdxILOP_call:
				case rdxILOP_callvirtual:
				case rdxILOP_callinterface:
				case rdxILOP_calldelegatebp:
				case rdxILOP_calldelegateprv:
					shouldMarkNext = true;
					numCallPoints++;
					break;
				case rdxILOP_debuginfo:
					numDebugInfo++;
					break;
				default:
					break;
				}

				if(shouldMarkNext)
					instructionResumeFlags.list->Element((i + 1) / 8) |= (1 << ((i + 1) % 8));
			}

			if(numCallPoints)
			{
				rdxArrayCRef(rdxSILCallPoint) callPoints;
				RDX_PROTECT_ASSIGN(ctx, callPoints, rdxCInternalObjectFactory::Create1DArray<rdxSILCallPoint>(ctx, objm, numCallPoints));
				m->m_native.callPoints = callPoints.ToWeakHdl();
			}
			if(numDebugInfo)
			{
				rdxArrayCRef(rdxSILDebugInfo) debugInfo;
				RDX_PROTECT_ASSIGN(ctx, debugInfo, rdxCInternalObjectFactory::Create1DArray<rdxSILDebugInfo>(ctx, objm, numDebugInfo));
				m->m_native.debugInfo = debugInfo;
			}

			{
				rdxWeakArrayHdl(rdxSILDebugInfo) outDebugInfos = m->m_native.debugInfo.ToWeakHdl();
				rdxWeakArrayHdl(rdxSILCallPoint) outCallPoints = m->m_native.callPoints.ToWeakHdl();
				rdxWeakOffsetHdl(rdxSILDebugInfo) outDebugInfo = outDebugInfos->OffsetElementRTRef(0).ToHdl();
				rdxWeakOffsetHdl(rdxSILCallPoint) outCallPoint = outCallPoints->OffsetElementRTRef(0).ToHdl();

				for(rdxLargeUInt i=0;i<instructions.count;i++)
				{
					rdxWeakOffsetHdl(rdxSILInstruction) instr = instructions.list->OffsetElementRTRef(i).ToHdl();
					switch(instr->opcode)
					{
					case rdxILOP_call:
					case rdxILOP_calldelegatebp:
					case rdxILOP_calldelegateprv:
					case rdxILOP_callvirtual:
					case rdxILOP_callinterface:
						{
							const rdxUILOpCompactArg *argCA = compactArgs.list->ArrayData() + instr->argOffs;
							outCallPoint->ilOpcode = instr->opcode;
							outCallPoint->instrNum = i;
							outCallPoint->paramBaseOffset = argCA[0].li;
							outCallPoint->prvOffset = argCA[1].li;

							switch(instr->opcode)
							{
							case rdxILOP_call:
								outCallPoint->args.call.func = argCA[2].p;
								break;
							case rdxILOP_calldelegatebp:
							case rdxILOP_calldelegateprv:
								outCallPoint->args.callDelegate.offset = argCA[2].li;
								outCallPoint->args.callDelegate.bpRelative = (instr->opcode == rdxILOP_calldelegatebp);
								break;
							case rdxILOP_callvirtual:
								outCallPoint->args.callVirtual.bpOffset = argCA[2].li;
								outCallPoint->args.callVirtual.vftOffset = argCA[3].lui;
								break;
							case rdxILOP_callinterface:
								outCallPoint->args.callInterface.bpOffset = argCA[2].li;
								outCallPoint->args.callInterface.vftOffset = argCA[3].li;
								outCallPoint->args.callInterface.soughtInterface = argCA[8].p;
								break;
							};

							++outCallPoint;
						}
						break;
					case rdxILOP_debuginfo:
						{
							const rdxUILOpCompactArg *argCA = compactArgs.list->ArrayData() + instr->argOffs;
							outDebugInfo->firstInstruction = i;
							outDebugInfo->filename = rdxWeakHdl(rdxCObject)(rdxObjRef_CSignal_GCInfo, rdxRefConverter<rdxGCInfo>::Convert(argCA[0].p)).StaticCast<rdxCString>();
							outDebugInfo->line = argCA[1].lui;
							outDebugInfo++;
						}
						break;
					default:
						break;
					}
				}
			}
		}

		//RDX_VERBOSE( RDX::Intrinsics::DisassembleMethod(m) );

#ifdef RDX_DEBUG_NONVERBOSE_DISASSEMBLE
		FILE *f;
		fopen_s(&f, "disassembly.rdxil", "a");
		RDX::Intrinsics::DisassembleMethod(f, m);
		fclose(f);
#endif
		if(objm->GetCodeProvider())
		{
			RDX_PROTECT(ctx, objm->GetCodeProvider()->CreateExecutable(objm, ctx, m) );

			// TODO MUSTFIX
			m->m_native.nativeCall = NULL;//objm->GetCodeProvider()->GetNativeCallback();
			m->m_native.ipToCurrentInstruction = objm->GetCodeProvider()->GetIPToCurrentInstructionCallback();
			m->m_native.instrNumToIP = objm->GetCodeProvider()->GetInstrNumToIPCallback();
			m->m_native.resumeThread = objm->GetCodeProvider()->GetResumeThreadCallback(objm);
		}
		m->m_native.isNativeCall = false;

		//ilp.pass = PASS_GenerateCode;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}
