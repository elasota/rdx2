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

struct rdxConstantTypes
{
	static rdxSObjectGUID coreInt;
	static rdxSObjectGUID coreByte;
	static rdxSObjectGUID coreLargeInt;
	static rdxSObjectGUID coreLargeUInt;
	static rdxSObjectGUID coreFloat;
	static rdxSObjectGUID coreDouble;
	static rdxSObjectGUID coreBool;
	static rdxSObjectGUID coreString;
};

rdxSObjectGUID rdxConstantTypes::coreInt = rdxSObjectGUID::FromObjectName("Core", "int");
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

#define PUSH_INSTR(ili)	do \
	{\
		ili.argOffs = -1;\
		RDX_PROTECT(ctx, instructions->Push(ctx, ili) );\
		instrNumIL++;\
		PUSH_RESUME_FLAG(ili);\
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
	rdxILOP_objinterior,
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

static rdxAliasingType rdxAliasingTypeForFloatSize(rdxLargeUInt size)
{
	switch(size)
	{
	case sizeof(rdxFloat):
		return rdxALIASINGTYPE_Float;
	case sizeof(rdxDouble):
		return rdxALIASINGTYPE_Double;
	};
	return rdxALIASINGTYPE_Block;
}

static rdxAliasingType rdxAliasingTypeForIntSize(rdxLargeUInt size)
{
	switch(size)
	{
	case 1:
		return rdxALIASINGTYPE_Int8;
	case 2:
		return rdxALIASINGTYPE_Int16;
	case 4:
		return rdxALIASINGTYPE_Int32;
	case 8:
		return rdxALIASINGTYPE_Int64;
	};
	return rdxALIASINGTYPE_Block;
}

static rdxAliasingType rdxDetermineAliasingType(rdxIObjectManager *objm, rdxWeakHdl(rdxCType) type, rdxLargeUInt size, rdxLargeUInt align)
{
	if(objm->TypeIsObjectReference(type.ToWeakRTRef()))
		return rdxALIASINGTYPE_Reference;

	if(objm->TypesCompatible(type.ToWeakRTRef(), objm->GetBuiltIns()->st_StructuredType.ToWeakRTRef()))
		return type.StaticCast<rdxCStructuredType>()->m_native.aliasingType;
	return rdxALIASINGTYPE_Block;
}

inline rdxLargeUInt rdxMoveGrayFlagsForSource(const rdxIObjectManager *objm, rdxWeakRTRef(rdxCType) type)
{
	if(type.IsNull())
		return 0;

	// Conditionally mark gray if assigning a reference
	if(objm->TypeIsObjectReference(type))
		return RDX_ILOP_MOVE_SRC_IS_REF;

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

struct rdxSVLocal
{
	rdxWeakHdl(rdxCType) vType;
	bool isPointer;
	bool isParameter;
	bool isConstant;
	bool isNotNull;
	bool hasCreatingInstruction;

	// CAUTION: The local offset calculation code will pull the top local, so parameterOffset and offset need to be different to avoid putting locals in an unusual spot
	rdxLargeInt offset;
	rdxLargeInt parameterOffset;
	rdxLargeUInt creatingInstruction;	// Used to determine valid jump targets
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

class rdxCILPassCompiler
{
public:
	rdxWeakHdl(rdxCMethod) method;
	rdxIObjectManager *objm;
	rdxSPushList<rdxSVStackValue> *opstack;
	rdxSPushList<rdxSVLocal> *localstack;
	rdxSPushList<rdxSILInstruction> *instructions;
	rdxSPushList<rdxUInt8> *instructionResumeFlags;
	rdxSPushList<rdxUILOpCompactArg> *compactArgs;
	rdxSPushList<rdxUILOpLargeArg> *largeArgs;
	rdxSPushList<rdxSVStackJournal> *stackjournal;
	rdxSPushList<rdxSStackJournal> *journal;
	rdxSPushList<rdxSExceptionHandlerJournal> *exHandlers;
	rdxWeakArrayHdl(rdxSILDecodedOp) decOps;
	rdxWeakArrayHdl(rdxSInstructionMeta) metalist;
	rdxCRef(rdxCStructuredType) st_Exception;
	rdxEILCompilePass pass;

	rdxLargeInt localOffsetMaximum;
	rdxLargeInt opstackOffsetMaximum;
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

	rdxLargeUInt OpstackUsed(rdxSOperationContext *ctx, rdxEVSTType vstType, rdxWeakHdl(rdxCType) vType)
	{
		RDX_TRY(ctx)
		{
			rdxLargeUInt sizeUsed = 0;

			switch(vstType)
			{
			case rdxVST_Varying:
				sizeUsed = sizeof(rdxSVarying);
				break;
			case rdxVST_Pointer:
			case rdxVST_ConstPointer:
				sizeUsed = sizeof(rdxSVarying);
				break;
			case rdxVST_Value:
			case rdxVST_ValueShell:
				if(objm->TypeIsObjectReference(vType.ToWeakRTRef()))
					sizeUsed = sizeof(rdxTracedRTRef(rdxCObject));
				else
				{
					if(vType->ObjectInfo()->containerType != objm->GetBuiltIns()->st_StructuredType)
						RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

					sizeUsed = vType.StaticCast<rdxCStructuredType>()->m_native.size;
					if(vType.StaticCast<rdxCStructuredType>()->m_native.alignment > rdxALIGN_RuntimeStackValue)
						RDX_STHROW(ctx, RDX_ERROR_IL_UNALIGNABLE_BYVAL);
				}
				break;
			case rdxVST_LocalRef:
			case rdxVST_Barrier:
				sizeUsed = 0;
				break;
			};

			if(!rdxCheckAddOverflowU(sizeUsed, rdxALIGN_RuntimeStackValue - 1 ))
				RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

			rdxLargeUInt basePadded = sizeUsed + rdxALIGN_RuntimeStackValue - 1;
			sizeUsed = basePadded - (basePadded % rdxALIGN_RuntimeStackValue);
			return sizeUsed;
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
				RDX_PROTECT_ASSIGN(ctx, sizeUsed, OpstackUsed(ctx, vst.vstType, vst.vType) );

				rdxLargeInt opstackOffset = 0;
				if(pass > rdxPASS_CreateStacks)
					opstackOffset = localOffsetMaximum;
				if(opstack->count)
				{
					rdxSVStackValue opTop;
					RDX_PROTECT_ASSIGN(ctx, opTop, opstack->RetrieveTop(ctx, 0));
					opstackOffset = -opTop.offset;
				}

				rdxLargeInt sizeUsedSigned;
				RDX_PROTECT_ASSIGN(ctx, sizeUsedSigned, rdxMakeSigned(ctx, sizeUsed));

				if(!rdxCheckAddOverflowS(sizeUsedSigned, opstackOffset))
					RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

				opstackOffset += sizeUsedSigned;
				if(opstackOffset > opstackOffsetMaximum)
					opstackOffsetMaximum = opstackOffset;

				vst.size = sizeUsed;
				vst.offset = -opstackOffset;
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

			rdxLargeUInt align = 1;
			rdxLargeUInt size = 0;
			if(vl.isPointer)
			{
				size = sizeof(rdxWeakOffsetRTRef(rdxCObject));
				align = rdxAlignOf<rdxWeakOffsetRTRef(rdxCObject)>();
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

				if(localstack->count)
					localOffset = -localstack->RetrieveTop(ctx, 0).offset;

				rdxLargeInt sizeSigned;
				RDX_PROTECT_ASSIGN(ctx, sizeSigned, rdxMakeSigned(ctx, size));
				if(!rdxCheckAddOverflowS(localOffset, sizeSigned))
					RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
				localOffset += sizeSigned;

				
				rdxLargeInt alignSigned;
				RDX_PROTECT_ASSIGN(ctx, alignSigned, rdxMakeSigned(ctx, align));
				if(!rdxCheckAddOverflowS(localOffset, (alignSigned - 1)) ||
					!rdxCheckAddOverflowS(localOffset, (localOffset + (alignSigned - 1)) ) )
					RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

				localOffset = (localOffset + (alignSigned - 1));
				localOffset -= localOffset % alignSigned;

				if(localOffset > localOffsetMaximum)
					localOffsetMaximum = localOffset;
				vl.offset = -localOffset;

				RDX_VERBOSE( wprintf(L"Local %i: %s\n", vl.offset, GCInfo::From(vl.vType)->gstSymbol->_native.characters) );
			}


			rdxSVStackJournal sj;
			sj.startInstruction = instrNum;
			sj.record = false;
			sj.vType = t;
			sj.offset = vl.offset;
			sj.vstType = rdxVST_Value;
			sj.sequentialID = sequentialID++;

			RDX_PROTECT(ctx, stackjournal->Push(ctx, sj) );

			RDX_PROTECT(ctx, localstack->Push(ctx, vl) );
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
	inline static bool DecodeInt(rdxWeakOffsetHdl(rdxUInt8) &bytecode, rdxLargeUInt &bytesRemaining, Ti &out)
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

					{
						rdxSILInstruction ili;
						ili.opcode = rdxILOP_hardenstack;
						PUSH_INSTR(ili);
					}
					break;
				case rdxOP_throw:
					{
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
							opArg0.li = vst.offset;

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
							opArg1.li = vst.offset;

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
						if(opstack->count)
							paramLocation = opstack->RetrieveTop(ctx, 0).offset;

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
						opArg1.li = paramLocation;
						opArg2.li = cmethod->m_native.p2;

						if(decOp->opcode == rdxOP_jumpif)
							ili.opcode = cmethod->m_native.opcode;
						else
							ili.opcode = cmethod->m_native.falseCheckOpcode;

						PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
					}
					break;
				case rdxOP_jumpiftrue:
				case rdxOP_jumpiffalse:
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

							if(bvVst.vType != objm->GetBuiltIns()->st_Bool)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							SAFE_DECODE_LABEL(targetInstruction, sint1);

							rdxSILInstruction ili;
							if(decOp->opcode == rdxOP_jumpiftrue)
								ili.opcode = rdxILOP_jtrue;
							else
								ili.opcode = rdxILOP_jfalse;

							rdxUILOpCompactArg opArg0, opArg1;
							RDX_PROTECT_ASSIGN(ctx, opArg0.lui, StitchJump(ctx, instrNum, targetInstruction) );
							opArg1.li = bvVst.offset;

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
							rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;
							RDX_PROTECT_ASSIGN(ctx, opArg0.lui, StitchJump(ctx, instrNum, targetInstruction) );
							opArg1.li = vst1.offset;
							opArg2.li = vst2.offset;
							opArg3.lui = size;
							opArg4.at = rdxDetermineAliasingType(objm, vst1.vType, size, align);

							PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
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
#ifdef RDX_ENABLE_TIMESLICE_COUNTER
								tickILI.opcode = rdxILOP_tick;
#else
								tickILI.opcode = rdxILOP_hardenstack;
#endif
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

						rdxLargeInt paramLocation = 0;
						if(opstack->count)
							paramLocation = opstack->RetrieveTop(ctx, 0).offset;

						rdxLargeInt thisLocation = 0;
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
									if(!objm->TypeIsObjectReference(params->Element(i).type.ToWeakRTRef()))
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

									paramVST.notNullInstruction = instrNumIL;
									paramVST.isNotNull = true;
									RDX_PROTECT(ctx, opstack->ReinsertTop(ctx, paramStackOffset, paramVST));

									rdxSILInstruction ili;
									ili.opcode = rdxILOP_verifynotnull;
									rdxUILOpCompactArg opArg0, opArg1;
									opArg0.li = paramVST.offset;
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
								// TODO MUSTFIX: Reevaluate this in light of interface call changes
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
						if(decOp->opcode == rdxOP_calldelegate)
						{
							rdxSVStackValue delegateVST;
							RDX_PROTECT_ASSIGN(ctx, delegateVST, PopOpstack(ctx, rdxVST_LocalRef) );
							rdxSVLocal vl = localstack->RetrieveBottom(ctx, delegateVST.index);
							if(vl.isParameter)
							{
								delegateSourceOffset = vl.parameterOffset;
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

						rdxLargeInt rvLocation = -localOffsetMaximum;
						if(opstack->count)
							rvLocation = opstack->RetrieveTop(ctx, 0).offset;

						if(decOp->opcode == rdxOP_call)
						{
							if(calledMethod->m_native.isIntrinsic)
							{
								if(calledMethod->m_native.isBranching)
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								rdxSILInstruction ili;
								rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3;
								ili.opcode = calledMethod->m_native.opcode;
								opArg0.li = paramLocation;
								opArg1.li = rvLocation;
								opArg2.li = calledMethod->m_native.p2;
								opArg3.li = calledMethod->m_native.p3;

								makeResumable = !calledMethod->m_native.neverFails;
								PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
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
								{
									rdxSILInstruction ili;
									ili.opcode = rdxILOP_call;
									rdxUILOpCompactArg opArg0, opArg1, opArg2;
									opArg0.li = (paramLocation & (~static_cast<rdxLargeInt>(RDX_MAX_ALIGNMENT - 1)));
									opArg1.li = rvLocation;
									opArg2.p = calledMethod.GetPOD();
									PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
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
								rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4, opArg5;
								rdxLargeUInt argCount = 4;
								if(st->storageSpecifier == rdxSS_Class)
								{
									ili.opcode = rdxILOP_callvirtual;
								}
								else if(st->storageSpecifier == rdxSS_Interface)
								{
									// Make sure we can deduce this again in source expor
									if(calledMethod->parameters->Element(calledMethod->thisParameterOffset - 1).type != thisType.ToWeakRTRef())
										RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);

									ili.opcode = rdxILOP_callinterface;
									opArg4.p = st.GetPOD();
									opArg5.p = calledMethod.GetPOD();
									argCount = 6;
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

								if(argCount == 6)
									PUSH_INSTR_CA_6(ili, opArg0, opArg1, opArg2, opArg3, opArg4, opArg5);
								else if(argCount == 5)
									PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
								else
									PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
							}
						}
						else if(decOp->opcode == rdxOP_calldelegate)
						{
							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1, opArg2;
							opArg0.li = (paramLocation & (~static_cast<rdxLargeInt>(RDX_MAX_ALIGNMENT - 1)));
							opArg1.li = rvLocation;
							opArg2.li = delegateSourceOffset;
							if(delegateFromParameter)
								ili.opcode = rdxILOP_calldelegateprv;
							else
								ili.opcode = rdxILOP_calldelegatebp;
							PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
						}


						// Clean up the stack and make sure the return values match
						if(pass > rdxPASS_CreateStacks)
						{
							rdxLargeUInt numReturns = 0;
							if(returnTypes.IsNotNull())
								numReturns = returnTypes->NumElements();

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

						rdxLargeInt instanceStackLoc = localstack->RetrieveTop(ctx, 0).offset;

						if(!objm->TypeIsObjectReference(t.ToWeakRTRef()) && t->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType &&
							!(t.StaticCast<rdxCStructuredType>()->m_native.flags & rdxCStructuredType::NativeProperties::STF_ZeroFill))
						{
							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4, opArg5;
							ili.opcode = rdxILOP_move;
							opArg0.p = t.StaticCast<rdxCStructuredType>()->m_native.currentDefaultValue.GetPOD();
							opArg1.li = instanceStackLoc;
							opArg2.lui = size;
							opArg3.at = rdxDetermineAliasingType(objm, t, size, rdxALIGN_RuntimeStackValue);
							opArg4.lui = RDX_ILOP_MOVE_SRC_TYPE_DEFAULT;
							opArg5.p = t.GetPOD();
							PUSH_INSTR_CA_6(ili, opArg0, opArg1, opArg2, opArg3, opArg4, opArg5);
						}
						else
						{
							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1, opArg2;
							ili.opcode = rdxILOP_zero;
							opArg0.li = instanceStackLoc;
							opArg1.lui = size;
							opArg2.at = rdxDetermineAliasingType(objm, t, size, rdxALIGN_RuntimeStackValue);
							PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
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

							rdxLargeUInt size, align;
							RDX_PROTECT(ctx, objm->TypeValueSize(ctx, t, size, align) );

							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;
							ili.opcode = rdxILOP_move;
							opArg0.li = vst.offset;
							opArg1.li = localstack->RetrieveTop(ctx, 0).offset;
							opArg2.lui = size;
							opArg3.li = rdxDetermineAliasingType(objm, t, size, align);
							opArg4.li = RDX_ILOP_MOVE_SRC_TRANSIENT;

							PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
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

						rdxLargeUInt expectedDimensions = static_cast<rdxLargeUInt>(decOp->sint1);;
						

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

						rdxLargeInt dimensionLocation = 0;

						for(rdxLargeUInt i=0;i<numDimensions;i++)
						{
							rdxSVStackValue dimVst;
							RDX_PROTECT_ASSIGN(ctx, dimVst, PopOpstack(ctx, rdxVST_Value) );
							if(pass > rdxPASS_CreateStacks && dimVst.vType != objm->GetBuiltIns()->st_LargeUInt)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							if(i == 0)
								dimensionLocation = dimVst.offset;
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
							if(!objm->TypeIsObjectReference(vst.vType.ToWeakRTRef()) && isStructuredType &&
								(vst.vType.StaticCast<rdxCStructuredType>()->m_native.flags & rdxCStructuredType::NativeProperties::STF_ZeroFill))
							{
								ili.opcode = rdxILOP_zero;
								opArg0.li = vst.offset;
								opArg1.lui = size;
								opArg2.at = rdxDetermineAliasingType(objm, vst.vType, size, align);
								argCount = 3;
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
									ili.opcode = rdxILOP_move;
									opArg0.p = instanceCopy->m_native.currentDefaultValue.GetPOD();
									opArg1.li = vst.offset;
									opArg2.lui = vst.size;
									opArg3.at = rdxDetermineAliasingType(objm, vst.vType, vst.size, rdxALIGN_RuntimeStackValue);
									opArg4.lui = RDX_ILOP_MOVE_SRC_TYPE_DEFAULT;
									opArg5.p = instanceCopy.GetPOD();
									argCount = 6;
								}
								else
								{
									ili.opcode = rdxILOP_newinstance;
									opArg0.li = vst.offset;
									opArg1.p = vst.vType.GetPOD();
									opArg2.li = dimensionLocation;
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

						// If this is a class, create and dispose of a pointer.  This isn't a recorded value, since the sets are all forbidden
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
								ili.opcode = rdxILOP_newinstance;
								opArg0.li = local.offset;
								opArg1.p = iType.GetPOD();
								opArg2.li = 0;
								opArg3.li = 0;
								PUSH_INSTR_CA_4(ili, opArg0, opArg1, opArg2, opArg3);
							}
							else
							{
								// Moves into the object aren't done using pins, which means they'll have aliasing problems with PCCM
								// Stack state needs to be flushed both before and after to allow aliasing
								rdxSILInstruction ili;
								ili.opcode = rdxILOP_hardenstack;
								PUSH_INSTR(ili);
							}

							// It's technically possible for a struct localref that's already been initialized to be clobbered by this
							// We don't care, since it's just as legal to set it up using a property init, and native unrecoverability is not guaranteed on structs

							rdxLargeUInt numTypeProperties = 0;
							if(st->properties.IsNotNull())
								numTypeProperties = st->properties->NumElements();

							rdxLargeUInt lastOffset = 0;

							rdxWeakOffsetHdl(rdxUInt8) argVar = decOp->intVarStart;
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

								rdxLargeUInt propOffset = st->m_native.propertyOffsets->Element(propIndex);
								if(isClass)
								{
									if(i == 0)
									{
										rdxUILOpCompactArg opArg0, opArg1, opArg2;
										rdxSILInstruction ili;
										ili.opcode = rdxILOP_objinterior_notnull_persist;
										opArg0.li = local.offset;
										opArg1.li = pointerVst.offset;
										opArg2.lui = propOffset;
										PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
									}
									else
									{
										rdxUILOpCompactArg opArg0, opArg1;
										rdxSILInstruction ili;
										ili.opcode = rdxILOP_incptr;
										opArg0.li = pointerVst.offset;
										opArg1.lui = propOffset - lastOffset;
										PUSH_INSTR_CA_2(ili, opArg0, opArg1);
									}

									lastOffset = propOffset;

									{
										// Move
										rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;
										rdxSILInstruction ili;
										ili.opcode = rdxILOP_move;
										opArg0.li = vst.offset;
										opArg1.li = pointerVst.offset;
										opArg2.lui = vSize;
										opArg3.at = rdxDetermineAliasingType(objm, prop->type.ToWeakHdl(), vSize, vAlign);
										opArg4.lui = (RDX_ILOP_MOVE_DEST_DEREF | RDX_ILOP_MOVE_DEST_OBJECT | RDX_ILOP_MOVE_SRC_TRANSIENT);
										if(expectPointer)
											opArg4.li |= RDX_ILOP_MOVE_SRC_DEREF;
										PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
									}
								}
								else
								{
									// Move
									rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;
									rdxSILInstruction ili;
									ili.opcode = rdxILOP_move;
									opArg0.li = vst.offset;
									opArg1.lui = local.offset + propOffset;
									opArg2.lui = vSize;
									opArg3.at = rdxDetermineAliasingType(objm, prop->type.ToWeakHdl(), vSize, vAlign);
									opArg4.lui = RDX_ILOP_MOVE_SRC_TRANSIENT;
									if(expectPointer)
										opArg4.li |= RDX_ILOP_MOVE_SRC_DEREF;
									PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
								}
							}

							// Done with properties

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
						rdxUILOpCompactArg opArg0, opArg1, opArg2;
						ili.opcode = rdxILOP_zero;
						opArg0.li = vst.offset;
						opArg1.lui = sizeof(rdxTracedRTRef(rdxCObject));
						opArg2.at = rdxALIASINGTYPE_Reference;

						PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
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

							if(vl.isParameter)
							{
								ili.opcode = rdxILOP_pinp;
								opArg1.li = vl.parameterOffset;
							}
							else
							{
								ili.opcode = rdxILOP_pinl;
								opArg1.li = vl.offset;
							}
							opArg0.li = vst.offset;
							PUSH_INSTR_CA_2(ili, opArg0, opArg1);
						}
					}
					break;
				case rdxOP_tovarying:
					{
						rdxSVStackValue vst;
						RDX_PROTECT_ASSIGN(ctx, vst, PopOpstack(ctx, rdxVST_Indeterminate) );

						rdxEVSTType originalVstType = vst.vstType;

						rdxLargeInt ptrOriginalOffset = vst.offset;
						vst.vstType = rdxVST_Varying;
						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

						if(pass > rdxPASS_CreateStacks)
						{
							rdxSILInstruction ili;
							rdxUILOpCompactArg opArg0, opArg1, opArg2;

							opArg0.li = vst.offset;
							opArg1.p = vst.vType.GetPOD();
							opArg2.li = ptrOriginalOffset;

							rdxEILOpcode staticOp, movingOp;

							if(originalVstType == rdxVST_Pointer || originalVstType == rdxVST_ConstPointer)
							{
								staticOp = rdxILOP_tovarying_static;
								movingOp = rdxILOP_tovarying;
							}
							else
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							if(vst.offset != ptrOriginalOffset)
							{
								ili.opcode = movingOp;
								PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
							}
							else
							{
								ili.opcode = staticOp;
								PUSH_INSTR_CA_2(ili, opArg0, opArg1);
							}
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
						opArg0.li = vst.offset;

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

						opArg1.li = arrayVST.offset;

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
						opArg2.li = opstack->RetrieveTop(ctx, 0).offset;

						opArg3.li = 0;	// Offset

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

							rdxLargeUInt paramOffset = st->m_native.propertyOffsets->Element(propertyIndex);
							rdxWeakHdl(rdxCType) propertyType = st->properties->Element(propertyIndex).type.ToWeakHdl();

							if(propertyType.IsNull())
								RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

							if(VSTIsPointer(vst.vstType))
							{
								// Pointers to object references must be loaded first
								if(objm->TypeIsObjectReference(st.ToWeakRTRef()))
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
									// IL instructions emitted by any of these always have an offset in p3
									rdxUILOpCompactArg offsetArg;
									RDX_PROTECT_ASSIGN(ctx, offsetArg, compactArgs->Pop(ctx) );
									offsetArg.lui += paramOffset;
									RDX_PROTECT(ctx, compactArgs->Push(ctx, offsetArg) );
								}
								else
								{
									rdxSILInstruction ili;
									rdxUILOpCompactArg opArg0, opArg1;
									ili.opcode = rdxILOP_incptr;
									opArg0.li = vst.offset;
									opArg1.lui = paramOffset;
									PUSH_INSTR_CA_2(ili, opArg0, opArg1);
								}
							}
							else if(vst.vstType == rdxVST_Value)
							{
								rdxSILInstruction ili;
								rdxUILOpCompactArg opArg0, opArg1, opArg2;
								ili.opcode = rdxILOP_objinterior;
								opArg0.li = vst.offset;

								if(vst.isNotNull)
									ili.opcode = rdxILOP_objinterior_notnull;

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

								opArg1.li = vst.offset;
								opArg2.lui = paramOffset;

								PUSH_INSTR_CA_3(ili, opArg0, opArg1, opArg2);
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
								rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;
								opArg1.li = destVst.offset;
								opArg2.lui = size;
								opArg3.at = rdxDetermineAliasingType(objm, sourceVst.vType, size, align);
								opArg4.lui = RDX_ILOP_MOVE_DEST_DEREF;

								if(!destVst.isFromLocal)
								{
									opArg4.lui |= RDX_ILOP_MOVE_DEST_OBJECT;
									opArg4.lui |= rdxMoveGrayFlagsForSource(objm, sourceVst.vType.ToWeakRTRef());
								}

								if(sourceVst.vstType == rdxVST_Value)
								{
									opArg0.li = sourceVst.offset;
									opArg4.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
								}
								else if(VSTIsPointer(sourceVst.vstType))
								{
									opArg4.lui |= RDX_ILOP_MOVE_SRC_DEREF;
									opArg4.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
									if(!sourceVst.isFromLocal)
										opArg4.lui |= RDX_ILOP_MOVE_SRC_OBJECT;

									opArg0.li = sourceVst.offset;
								}
								else if(sourceVst.vstType == rdxVST_LocalRef)
								{
									rdxSVLocal vl = localstack->RetrieveBottom(ctx, sourceVst.index);

									if(vl.isPointer)
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

									if(vl.isParameter)
									{
										opArg4.lui |= RDX_ILOP_MOVE_SRC_PARENT_FRAME;
										opArg0.li = vl.parameterOffset;
									}
									else
										opArg0.li = vl.offset;
								}
								else
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								ili.opcode = rdxILOP_move;
								PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
							}
							else if(destVst.vstType == rdxVST_LocalRef)
							{
								rdxSILInstruction ili;
								rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;
								bool destIsParameter;

								{
									rdxSVLocal vl = localstack->RetrieveBottom(ctx, destVst.index);

									if(vl.isPointer)
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

									destIsParameter = vl.isParameter;
									if(destIsParameter)
										opArg1.li = vl.parameterOffset;
									else
										opArg1.li = vl.offset;
								}

								opArg2.lui = size;
								opArg3.li = rdxDetermineAliasingType(objm, sourceVst.vType, size, align);
								opArg4.lui = 0;

								if(destIsParameter)
									opArg4.lui |= RDX_ILOP_MOVE_DEST_PARENT_FRAME;

								if(sourceVst.vstType == rdxVST_Value)
								{
									opArg0.li = sourceVst.offset;
									opArg4.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
								}
								else if(VSTIsPointer(sourceVst.vstType))
								{
									opArg4.lui |= RDX_ILOP_MOVE_SRC_DEREF;
									opArg4.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;

									opArg0.li = sourceVst.offset;
								}
								else if(sourceVst.vstType == rdxVST_LocalRef)
								{
									rdxSVLocal vl = localstack->RetrieveBottom(ctx, sourceVst.index);

									if(vl.isPointer)
										RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

									if(vl.isParameter)
									{
										opArg4.lui |= RDX_ILOP_MOVE_SRC_PARENT_FRAME;
										opArg0.li = vl.parameterOffset;
									}
									else
									{
										opArg0.li = vl.offset;
									}
								}
								else
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

								ili.opcode = rdxILOP_move;
								PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
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
								rdxAliasingType aliasingType = rdxALIASINGTYPE_Block;
								switch(vst.vstType)
								{
								case rdxVST_Value:
									RDX_PROTECT(ctx, objm->TypeValueSize(ctx, vst.vType, size, dummyAlign) );
									aliasingType = rdxDetermineAliasingType(objm, vst.vType, size, rdxALIGN_RuntimeStackValue);
									break;
								case rdxVST_Pointer:
									size = sizeof(rdxWeakOffsetRTRef(rdxCObject));
									aliasingType = rdxALIASINGTYPE_RuntimePointer;
									break;
								case rdxVST_Varying:
									size = sizeof(rdxWeakOffsetRTRef(rdxCObject));
									aliasingType = rdxALIASINGTYPE_Varying;
									break;
								case rdxVST_LocalRef:
									size = 0;
									break;
								case rdxVST_Barrier:
								case rdxVST_ValueShell:
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
									break;
								}

								if(size)
								{
									rdxSILInstruction ili;
									rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;
									ili.opcode = rdxILOP_move;
									opArg0.li = vst.offset;
									opArg1.li = clonedVst.offset;
									opArg2.lui = size;
									opArg3.at = rdxDetermineAliasingType(objm, vst.vType, size, rdxALIGN_RuntimeStackValue);
									opArg4.lui = 0;

									PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
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
									rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;
									ili.opcode = rdxILOP_move;
									opArg0.li = loadedVst.offset;
									opArg1.li = createdVst.offset;
									opArg2.lui = size;
									opArg3.at = rdxDetermineAliasingType(objm, createdVst.vType, size, rdxALIGN_RuntimeStackValue);
									opArg4.lui = 0;
									opArg4.lui |= RDX_ILOP_MOVE_SRC_DEREF;
									opArg4.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
									if(!loadedVst.isFromLocal)
									{
										opArg4.lui |= RDX_ILOP_MOVE_SRC_OBJECT;
										opArg4.lui |= rdxMoveGrayFlagsForSource(objm, loadedVst.vType.ToWeakRTRef());
									}


									PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
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
										rdxAliasingType aliasingType = rdxALIASINGTYPE_Block;
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
											align = rdxAlignOf<rdxWeakOffsetRTRef(rdxCObject)>();
											RDX_PROTECT(ctx, AppendOpstack(ctx, createdVst) );
											aliasingType = rdxALIASINGTYPE_RuntimePointer;
										}
										else
										{
											createdVst.vstType = rdxVST_Value;
											createdVst.isNotNull = vl.isNotNull;
											RDX_PROTECT(ctx, objm->TypeValueSize(ctx, vl.vType, size, align) );
											RDX_PROTECT(ctx, AppendOpstack(ctx, createdVst) );
											aliasingType = rdxDetermineAliasingType(objm, vl.vType, size, align);
										}

										rdxSILInstruction ili;
										rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;
										ili.opcode = rdxILOP_move;
										opArg0.li = vl.parameterOffset;
										opArg1.li = createdVst.offset;
										opArg2.lui = size;
										opArg3.at = aliasingType;
										opArg4.lui = 0;
										opArg4.lui |= RDX_ILOP_MOVE_SRC_PARENT_FRAME;

										PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
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
										rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;
										ili.opcode = rdxILOP_move;
										opArg0.li = vl.offset;
										opArg1.li = createdVst.offset;
										opArg2.lui = size;
										opArg3.at = rdxDetermineAliasingType(objm, vl.vType, size, align);
										opArg4.lui = 0;

										PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
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

						if(pass > rdxPASS_CreateStacks)
						{
							switch(vst.vstType)
							{
							case rdxVST_Value:
							case rdxVST_Pointer:
							case rdxVST_ConstPointer:
							case rdxVST_Varying:
							case rdxVST_LocalRef:
							case rdxVST_ValueShell:
								// Valid thing to pop
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
						RDX_PROTECT(ctx, SyncJournal(ctx) );

						rdxSVStackValue vst;
						RDX_PROTECT_ASSIGN(ctx, vst, PopOpstack(ctx, rdxVST_Value) );

						if(pass > rdxPASS_CreateStacks)
						{
							if(decOp->res.IsNull() || !objm->ObjectCompatible(decOp->res.ToWeakRTRef(), objm->GetBuiltIns()->st_Type.ToWeakRTRef()))
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxWeakHdl(rdxCType) t = decOp->res.StaticCast<rdxCType>();
							if(!objm->TypeIsValid(t.ToWeakRTRef()))
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							if(!objm->TypesCompatible(vst.vType.ToWeakRTRef(), t.ToWeakRTRef()))
							{
								// This will require a polymorphic cast
								if(!objm->TypesCompatiblePolymorphic(vst.vType.ToWeakRTRef(), t.ToWeakRTRef()))
									RDX_STHROW(ctx, RDX_ERROR_INCOMPATIBLE_CONVERSION);

								if(t->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType &&
									t.StaticCast<rdxCStructuredType>()->storageSpecifier == rdxSS_Enum)
								{
									rdxSILInstruction ili;
									rdxUILOpCompactArg opArg0, opArg1;
									ili.opcode = rdxILOP_assertenum;
									opArg0.li = vst.offset;
									opArg1.p = t.GetPOD();

									PUSH_INSTR_CA_2(ili, opArg0, opArg1);
								}
								else if(objm->TypeIsObjectReference(t.ToWeakRTRef()))
								{
									rdxSILInstruction ili;
									rdxUILOpCompactArg opArg0, opArg1;
									ili.opcode = rdxILOP_assertinherits;
									opArg0.li = vst.offset;
									opArg1.p = t.GetPOD();

									PUSH_INSTR_CA_2(ili, opArg0, opArg1);
								}
								else
									RDX_STHROW(ctx, RDX_ERROR_INTERNAL_BAD_TYPE);
							}

							vst.vType = t;
						}

						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );
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

								rdxLargeInt destinationOffset = 0;
								for(rdxLargeUInt i=0;i<numReturnVals;i++)
								{
									rdxSVStackValue valueVst;

									RDX_PROTECT_ASSIGN(ctx, valueVst, PopOpstack(ctx, rdxVST_Indeterminate) );
									rdxWeakHdl(rdxCType) destinationReturnType = returnTypes->Element(numReturnVals-1-i).ToWeakHdl();

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
									rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;

									ili.opcode = rdxILOP_move;
									opArg4.lui = 0;
									opArg4.lui |= RDX_ILOP_MOVE_DEST_PARENT_FRAME;

									if(valueVst.vstType == rdxVST_LocalRef)
									{
										rdxSVLocal vl;
										vl = localstack->RetrieveBottom(ctx, valueVst.index);
										if(vl.isParameter)
										{
											opArg4.lui |= RDX_ILOP_MOVE_SRC_PARENT_FRAME;

											if(vl.isPointer)
											{
												opArg4.lui |= RDX_ILOP_MOVE_SRC_DEREF;
												opArg4.lui |= RDX_ILOP_MOVE_DEST_DEREF;
											}

											opArg0.li = vl.parameterOffset;
											opArg1.li = destinationOffset;
										}
										else
										{
											if(isRefStruct)
												opArg4.lui |= RDX_ILOP_MOVE_DEST_DEREF;

											opArg0.li = vl.offset;
										}
										opArg1.li = destinationOffset;
									}
									else if(VSTIsPointer(valueVst.vstType))
									{
										opArg4.lui |= RDX_ILOP_MOVE_SRC_DEREF;
										opArg4.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
										if(isRefStruct)
											opArg4.lui |= RDX_ILOP_MOVE_DEST_DEREF;
										if(!valueVst.isFromLocal)
											opArg4.lui |= RDX_ILOP_MOVE_SRC_OBJECT;
										opArg0.li = valueVst.offset;
										opArg1.li = destinationOffset;
									}
									else if(valueVst.vstType == rdxVST_Value)
									{
										opArg4.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
										opArg0.li = valueVst.offset;
										opArg1.li = destinationOffset;
									}
									opArg2.lui = size;
									opArg3.li = rdxDetermineAliasingType(objm, valueVst.vType, size, align);

									PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);

									rdxLargeUInt sizeUsed;
									rdxEVSTType vstType = rdxVST_Value;
									if(TypeIsRefStruct(objm, destinationReturnType))
										vstType = rdxVST_Pointer;

									RDX_PROTECT_ASSIGN(ctx, sizeUsed, OpstackUsed(ctx, vstType, destinationReturnType) );

									rdxLargeInt sizeUsedS;
									RDX_PROTECT_ASSIGN(ctx, sizeUsedS, rdxMakeSigned(ctx, sizeUsed));
									if(!rdxCheckAddOverflowS(destinationOffset, sizeUsedS))
										RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
									destinationOffset += sizeUsedS;
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
									rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;
									opArg4.lui = 0;
									ili.opcode = rdxILOP_move;

									if(valueVst.vstType == rdxVST_LocalRef)
									{
										rdxSVLocal vl;
										vl = localstack->RetrieveBottom(ctx, valueVst.index);
										if(vl.isParameter)
										{
											opArg4.lui |= RDX_ILOP_MOVE_SRC_PARENT_FRAME;

											if(vl.isPointer)
											{
												opArg4.lui |= RDX_ILOP_MOVE_SRC_DEREF;
												opArg4.lui |= RDX_ILOP_MOVE_DEST_DEREF;
											}

											opArg0.li = vl.parameterOffset;
											opArg1.li = destinationVst.offset;
										}
										else
										{
											if(isRefStruct)
												opArg4.lui |= RDX_ILOP_MOVE_DEST_DEREF;

											opArg0.li = vl.offset;
											opArg1.li = destinationVst.offset;
										}
									}
									else if(valueVst.vstType == rdxVST_Pointer)
									{
										opArg4.lui |= RDX_ILOP_MOVE_SRC_DEREF;
										opArg4.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
										if(isRefStruct)
											opArg4.lui |= RDX_ILOP_MOVE_DEST_DEREF;
										opArg0.li = valueVst.offset;
										opArg1.li = destinationVst.offset;
									}
									else if(valueVst.vstType == rdxVST_Value)
									{
										opArg4.lui |= RDX_ILOP_MOVE_SRC_TRANSIENT;
										opArg0.li = valueVst.offset;
										opArg1.li = destinationVst.offset;
									}
									opArg2.lui = size;
									opArg3.li = rdxDetermineAliasingType(objm, valueVst.vType, size, align);

									PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
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
									opArg1.ca[1].lui = sizeof(rdxWeakOffsetRTRef(rdxCObject));
								}
								break;
							case rdxSS_Interface:
							case rdxSS_Class:
								{
									vst.vstType = rdxVST_Value;
									vst.vType = resType;

									ili.opcode = rdxILOP_immediate_ptr;
									opArg0.ca[0].p = res.GetPOD();
									opArg1.ca[1].lui = sizeof(rdxTracedRTRef(rdxCObject));
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
							opArg1.ca[1].lui = sizeof(rdxTracedRTRef(rdxCObject));

						}
						else
							RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);

						RDX_PROTECT(ctx, AppendOpstack(ctx, vst) );

						opArg1.ca[0].li = vst.offset;
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
							rdxUILOpLargeArg opArg0, opArg1;
							ili.opcode = rdxILOP_immediate;
							opArg1.ca[0].li = vst.offset;

							//rdxHugeInt hi1 = decOp->hsi1;

							if(vst.vType.IsNull())
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							rdxSSerializationTag *serTag = vst.vType->ObjectInfo()->SerializationTag();
							if(!serTag)
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
							rdxSObjectGUID typeGUID = serTag->gstSymbol;

							if(vst.vType->ObjectInfo()->containerType == objm->GetBuiltIns()->st_StructuredType &&
								vst.vType.StaticCast<rdxCStructuredType>()->storageSpecifier == rdxSS_Enum)
							{
								DECODE_INTEGER(rdxEnumValue, opArg0.ca[0].ev);
								if(!objm->EnumCompatible(opArg0.ca[0].ev, vst.vType.StaticCast<rdxCStructuredType>()->enumerants.ToWeakRTRef()))
									RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);
								opArg1.ca[1].at = rdxAliasingTypeForIntSize(sizeof(rdxEnumValue));
							}
							else if(typeGUID == rdxConstantTypes::coreInt)
							{
								DECODE_INTEGER(rdxInt, opArg0.ca[0].si);
								opArg1.ca[1].at = rdxAliasingTypeForIntSize(sizeof(rdxInt));
							}
							else if(typeGUID == rdxConstantTypes::coreByte)
							{
								DECODE_INTEGER(rdxByte, opArg0.ca[0].b);
								opArg1.ca[1].at = rdxAliasingTypeForIntSize(sizeof(rdxByte));
							}
							else if(typeGUID == rdxConstantTypes::coreLargeInt)
							{
								DECODE_INTEGER(rdxLargeInt, opArg0.ca[0].li);
								opArg1.ca[1].at = rdxAliasingTypeForIntSize(sizeof(rdxLargeInt));
							}
							else if(typeGUID == rdxConstantTypes::coreLargeUInt)
							{
								// Using .li is intentional, since decodes are always signed
								DECODE_INTEGER(rdxLargeInt, opArg0.ca[0].li);
								opArg1.ca[1].at = rdxAliasingTypeForIntSize(sizeof(rdxLargeUInt));
							}
							else if(typeGUID == rdxConstantTypes::coreFloat)
							{
								DECODE_FLOAT(rdxFloat, opArg0.ca[0].f);
								opArg1.ca[1].at = rdxAliasingTypeForFloatSize(sizeof(rdxFloat));
							}
							else if(typeGUID == rdxConstantTypes::coreDouble)
							{
								DECODE_FLOAT(rdxDouble, opArg0.dbl);
								opArg1.ca[1].at = rdxAliasingTypeForFloatSize(sizeof(rdxDouble));
							}
							else if(typeGUID == rdxConstantTypes::coreBool)
							{
								rdxInt bVal;
								DECODE_INTEGER(rdxInt, bVal);
								if(bVal != 0)
									opArg0.ca[0].bo = rdxTrueValue;
								else
									opArg0.ca[0].bo = rdxFalseValue;
								opArg1.ca[1].at = rdxALIASINGTYPE_Bool;
							}
							else
								RDX_STHROW(ctx, RDX_ERROR_IL_INVALID_OPERAND);

							PUSH_INSTR_LA_2(ili, opArg0, opArg1);
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
							opArg1.ca[0].li = vst.offset;

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
							rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4;

							if(VSTIsPointer(valueVSTType))
								ili.opcode = rdxILOP_switch_ptr;
							else
								ili.opcode = rdxILOP_switch;
							opArg0.li = vst.offset;
							opArg1.p = decOp->res.GetPOD();
							opArg2.li = decOp->sint1;
							opArg3.lui = size;
							opArg4.at = rdxDetermineAliasingType(objm, vst.vType, size, align);

							PUSH_INSTR_CA_5(ili, opArg0, opArg1, opArg2, opArg3, opArg4);
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

							rdxUILOpCompactArg opArg0, opArg1, opArg2, opArg3, opArg4, opArg5, opArg6, opArg7;
							opArg0.li = arrayLocal.offset;
							opArg1.li = indexLocal.offset;
							opArg2.li = destLocal.offset;
							RDX_PROTECT_ASSIGN(ctx, opArg3.lui, StitchJump(ctx, instrNum, targetInstruction) );

							RDX_PROTECT(ctx, objm->TypeValueSize(ctx, destVst.vType, opArg4.lui, opArg5.lui) );

							if(decOp->sint2 == 0)
							{
								ili.opcode = rdxILOP_iteratearray;
								PUSH_INSTR_CA_6(ili, opArg0, opArg1, opArg2, opArg3, opArg4, opArg5);
							}
							else
							{
								ili.opcode = rdxILOP_iteratearraysub;
								opArg6.li = subIndexLocal.offset;
								opArg7.lui = static_cast<rdxLargeUInt>(decOp->sint2);
								PUSH_INSTR_CA_8(ili, opArg0, opArg1, opArg2, opArg3, opArg4, opArg5, opArg6, opArg7);
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

	static void DecodeOps(rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) m, rdxIObjectManager *objm, rdxWeakArrayHdl(rdxSILDecodedOp) decOps)
	{
		RDX_TRY(ctx)
		{
			rdxLargeUInt bytesRemaining = m->bytecode->NumElements();
			rdxLargeUInt numInstructions = m->numInstructions;
			rdxWeakOffsetHdl(rdxUInt8) bytecode = m->bytecode->OffsetElementRTRef(0).ToHdl();

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

					decOp.intVarBytes = static_cast<rdxLargeUInt>(bytecode - decOp.intVarStart);
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
			RDX_PROTECT_ASSIGN(ctx, h, objm->Create1DArray<rdxUInt8>(ctx, compactSize).ToWeakHdl() );
			m->m_native.compactedJournals = h;
			outp = h->OffsetElementRTRef(0).ToHdl();
		}

		for(rdxLargeUInt j=0;j<numJournals;j++)
		{
#ifndef RDX_JOURNAL_ALL_VALUES
			if(journals->Element(j).isTraceable)
#endif
				journals->Element(j).Compress(&outp, NULL);
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
		RDX_PROTECT_ASSIGN(ctx, journals, objm->Create1DArray<rdxSStackJournal>(ctx, numMethodParameters) );
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

		rdxLargeUInt numInstructions = m->numInstructions;

		rdxSPushList<rdxSVStackValue> opstack;
		rdxSPushList<rdxSVLocal> localstack;
		rdxSPushList<rdxSILInstruction> instructions;
		rdxSPushList<rdxUInt8> instructionResumeFlags;
		rdxSPushList<rdxUILOpCompactArg> compactArgs;
		rdxSPushList<rdxUILOpLargeArg> largeArgs;
		rdxSPushList<rdxSVStackJournal> stackjournal;
		rdxSPushList<rdxSStackJournal> journal;
		rdxSPushList<rdxSExceptionHandlerJournal> exHandlers;

		rdxArrayCRef(rdxSInstructionMeta) metalist;
		RDX_PROTECT_ASSIGN(ctx, metalist, objm->Create1DArray<rdxSInstructionMeta>(ctx, numInstructions) );
		rdxArrayCRef(rdxSILDecodedOp) decOps;
		RDX_PROTECT_ASSIGN(ctx, decOps, objm->Create1DArray<rdxSILDecodedOp>(ctx, numInstructions) );

		RDX_PROTECT(ctx, rdxCILPassCompiler::DecodeOps(ctx, m, objm, decOps.ToWeakHdl()));

		ilp.metalist = metalist.ToWeakHdl();
		ilp.method = m;
		ilp.objm = objm;
		ilp.opstack = &opstack;
		ilp.localstack = &localstack;
		ilp.instructions = &instructions;
		ilp.instructionResumeFlags = &instructionResumeFlags;
		ilp.compactArgs = &compactArgs;
		ilp.largeArgs = &largeArgs;
		ilp.stackjournal = &stackjournal;
		ilp.exHandlers = &exHandlers;
		ilp.journal = &journal;
		ilp.decOps = decOps.ToWeakHdl();
		ilp.localOffsetMaximum = 0;
		ilp.opstackOffsetMaximum = 0;
		ilp.sequentialID = 0;
		ilp.st_Exception = objm->LookupSymbolSimple(ctx, rdxSObjectGUID::FromObjectName("Core", "Exception")).StaticCast<rdxCStructuredType>();

		ilp.pass = rdxPASS_CreateStacks;
		RDX_PROTECT(ctx, ilp.CompilePass(ctx) );
		RDX_PROTECT_ASSIGN(ctx, opstack.list, objm->Create1DArray<rdxSVStackValue>(ctx, opstack.maximum) );
		RDX_PROTECT_ASSIGN(ctx, localstack.list, objm->Create1DArray<rdxSVLocal>(ctx, localstack.maximum) );
		RDX_PROTECT_ASSIGN(ctx, stackjournal.list, objm->Create1DArray<rdxSVStackJournal>(ctx, stackjournal.maximum) );

		ilp.pass = rdxPASS_GenerateCode;
		instructions.count = 0;
		instructionResumeFlags.count = 0;
		compactArgs.count = 0;
		largeArgs.count = 0;
		journal.count = 0;
		stackjournal.count = 0;
		exHandlers.count = 0;
		RDX_PROTECT(ctx, ilp.CompilePass(ctx) );

		// Round up the size of the localstack to the opstack boundary
		rdxLargeInt lsSize = ilp.localOffsetMaximum;

		if(!rdxCheckAddOverflowS(lsSize, static_cast<rdxLargeInt>(rdxALIGN_RuntimeStackValue - 1)))
			RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

		lsSize += (rdxALIGN_RuntimeStackValue - 1);
		lsSize -= lsSize % rdxALIGN_RuntimeStackValue;
		ilp.localOffsetMaximum = lsSize;

		{
			rdxLargeInt exceptionInsertionOffset;
			rdxLargeUInt opstackUsedU = ilp.OpstackUsed(ctx, rdxVST_Value, rdxWeakHdl(rdxCType)::Null());
			rdxLargeInt opstackUsedS;
			RDX_PROTECT_ASSIGN(ctx, opstackUsedS, rdxMakeSigned(ctx, opstackUsedU));
			RDX_PROTECT_ASSIGN(ctx, exceptionInsertionOffset, -(lsSize + opstackUsedS));
			m->m_native.exceptionInsertionOffset = exceptionInsertionOffset;
		}

		RDX_PROTECT_ASSIGN(ctx, instructions.list, objm->Create1DArray<rdxSILInstruction>(ctx, instructions.maximum) );
		RDX_PROTECT_ASSIGN(ctx, instructionResumeFlags.list, objm->Create1DArray<rdxUInt8>(ctx, instructionResumeFlags.maximum) );
		RDX_PROTECT_ASSIGN(ctx, compactArgs.list, objm->Create1DArray<rdxUILOpCompactArg>(ctx, compactArgs.maximum) );
		RDX_PROTECT_ASSIGN(ctx, largeArgs.list, objm->Create1DArray<rdxUILOpLargeArg>(ctx, largeArgs.maximum) );
		RDX_PROTECT_ASSIGN(ctx, journal.list, objm->Create1DArray<rdxSStackJournal>(ctx, journal.maximum) );
		RDX_PROTECT_ASSIGN(ctx, exHandlers.list, objm->Create1DArray<rdxSExceptionHandlerJournal>(ctx, exHandlers.maximum) );
		instructions.count = 0;
		instructionResumeFlags.count = 0;
		compactArgs.count = 0;
		largeArgs.count = 0;
		journal.count = 0;
		stackjournal.count = 0;
		exHandlers.count = 0;
		RDX_PROTECT(ctx, ilp.CompilePass(ctx) );

		// Jump availability is now known, run a stitch pass
		instructions.count = 0;
		instructionResumeFlags.count = 0;
		compactArgs.count = 0;
		largeArgs.count = 0;
		journal.count = 0;
		stackjournal.count = 0;
		exHandlers.count = 0;
		ilp.pass = rdxPASS_StitchJumps;
		RDX_PROTECT(ctx, ilp.CompilePass(ctx) );

		{
			m->m_native.ilinstructions = instructions.list.ToWeakHdl();
			m->m_native.ilResumeFlags = instructionResumeFlags.list.ToWeakHdl();
			m->m_native.numILInstructions = instructions.count;
			m->m_native.largeArgs = largeArgs.list.ToWeakHdl();
			m->m_native.compactArgs = compactArgs.list.ToWeakHdl();

			rdxLargeInt extraPad = ilp.opstackOffsetMaximum % RDX_MAX_ALIGNMENT;
			if(extraPad)
				extraPad = RDX_MAX_ALIGNMENT - extraPad;
			if(!rdxCheckAddOverflowS(ilp.opstackOffsetMaximum, extraPad))
				RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);

			m->m_native.frameCapacity = static_cast<rdxLargeUInt>(ilp.opstackOffsetMaximum + extraPad);

			m->m_native.exHandlers = exHandlers.list.ToWeakHdl();
		}

		{
			rdxArrayCRef(rdxLargeUInt) translation1;

			RDX_PROTECT_ASSIGN(ctx, translation1, objm->Create1DArray<rdxLargeUInt>(ctx, numInstructions).ToWeakHdl() );
			
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
				RDX_PROTECT_ASSIGN(ctx, callPoints, objm->Create1DArray<rdxSILCallPoint>(ctx, numCallPoints));
				m->m_native.callPoints = callPoints.ToWeakHdl();
			}
			if(numDebugInfo)
			{
				rdxArrayCRef(rdxSILDebugInfo) debugInfo;
				RDX_PROTECT_ASSIGN(ctx, debugInfo, objm->Create1DArray<rdxSILDebugInfo>(ctx, numDebugInfo));
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
								outCallPoint->args.callInterface.soughtInterface = argCA[4].p;
								break;
							};

							++outCallPoint;
						}
						break;
					case rdxILOP_debuginfo:
						{
							const rdxUILOpCompactArg *argCA = compactArgs.list->ArrayData() + instr->argOffs;
							outDebugInfo->firstInstruction = i;
							outDebugInfo->filename = rdxWeakHdl(rdxCObject)(rdxObjRef_CSignal_RefPOD, argCA[0].p).StaticCast<rdxCString>();
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
