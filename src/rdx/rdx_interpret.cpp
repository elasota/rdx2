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
#include "rdx_programmability.hpp"
#include "rdx_objectmanagement.hpp"
#include "rdx_ilcomp.hpp"
#include "rdx_longflow.hpp"
#include "rdx_intrinsics.hpp"
#include "rdx_blockcopy.hpp"
#include "rdx_objectmanagement_internal.hpp"
#include "rdx_aliasing.hpp"

#include "rdx_interpret_internal.hpp"

#if 0

#define EXPORT_FRAME(loc)	\
	do\
	{\
		(loc)->bp = reinterpret_cast<rdxSRuntimeStackFrame*>(bp);\
		(loc)->ip = nextInstr;\
		(loc)->method = m.ToRef();\
		(loc)->prv = reinterpret_cast<rdxURuntimeStackValue*>(prv);\
	} while(0)

#define BINARY_ARITHMETIC_OP(opName, type, op)	\
	case opName:\
	{\
		USE_ARG_CA;\
		const type *rs = reinterpret_cast<const type*>(bp + argCA[0].li);\
		const type *ls = reinterpret_cast<const type*>(bp + argCA[0].li + rdxPaddedSize(sizeof(type), sizeof(rdxURuntimeStackValue)));\
		type *dest = reinterpret_cast<type*>(bp + argCA[1].li);\
		*dest = ((*ls) op (*rs));\
	}\
	break

#define BINARY_ARITHMETIC_OP_ZERO_CHECK(opName, type, op)	\
	case opName:\
	{\
		USE_ARG_CA;\
		const type *rs = reinterpret_cast<const type*>(bp + argCA[0].li);\
		const type *ls = reinterpret_cast<const type*>(bp + argCA[0].li + rdxPaddedSize(sizeof(type), sizeof(rdxURuntimeStackValue)));\
		type *dest = reinterpret_cast<type*>(bp + argCA[1].li);\
		if((*rs) == static_cast<type>(0))\
			THROWEXCEPTIONDICT(rdxX_DivideByZeroException);\
		*dest = ((*ls) op (*rs));\
	}\
	break

#define UNARY_ARITHMETIC_OP(opName, type, op)	\
	case opName:\
	{\
		USE_ARG_CA;\
		const type *v = reinterpret_cast<const type*>(bp + argCA[0].li);\
		type *dest = reinterpret_cast<type*>(bp + argCA[1].li);\
		*dest = (op (*v));\
	}\
	break


#define CONVERT_OP(opName, srcType, destType)	\
	case opName:\
	{\
		USE_ARG_CA;\
		rdxConvertOp<srcType, destType>(bp, argCA);\
	}\
	break

#define NUMERIC_COMPARE_OP(opName, type, op)	\
	case opName:\
	{\
		USE_ARG_CA;\
		const type *rs = reinterpret_cast<const type*>(bp + argCA[1].li);\
		const type *ls = reinterpret_cast<const type*>(bp + argCA[1].li + rdxPaddedSize(sizeof(type), sizeof(rdxURuntimeStackValue)));\
		if((*ls) op (*rs))\
			nextInstr = instructions + argCA[0].li;\
	}\
	break

#define ITERATE_ARRAY_OP_BASE(opName, elementType, copierType, enableSubs)	\
	case opName:\
		{\
			USE_ARG_CA;\
			rdxRef<rdxCException> ex = rdxIterateArrayOpBase<elementType, copierType, enableSubs>(objm, bp, argCA, instructions, &nextInstr);\
			if(ex.IsNotNull())\
			{\
				THROWEXCEPTION(ex);\
			}\
		}\
		break

		
#define ITERATE_ARRAY_OP(opName, elementType, enableSubs)	\
	ITERATE_ARRAY_OP_BASE(opName, elementType, rdxSUnitCopier, enableSubs)

#define MOVE_OP_GROUP_2(base, copier, type, index)	\
	MOVE_OP(base, copier, type, index+0);\
	MOVE_OP(base, copier, type, index+1)

#define MOVE_OP_GROUP_4(base, copier, type, index)	\
	MOVE_OP_GROUP_2(base, copier, type, index+0);\
	MOVE_OP_GROUP_2(base, copier, type, index+2)

#define MOVE_OP_GROUP_8(base, copier, type, index)	\
	MOVE_OP_GROUP_4(base, copier, type, index+0);\
	MOVE_OP_GROUP_4(base, copier, type, index+4)

#define MOVE_OP_GROUP_16(base, copier, type, index)	\
	MOVE_OP_GROUP_8(base, copier, type, index+0);\
	MOVE_OP_GROUP_8(base, copier, type, index+8)

#define MOVE_OP_GROUP_32(base, copier, type, index)	\
	MOVE_OP_GROUP_16(base, copier, type, index+0);\
	MOVE_OP_GROUP_16(base, copier, type, index+16)

#define MOVE_OP_GROUP(base, copier, type)	\
	MOVE_OP_GROUP_32(base, copier, type, 0);\
	MOVE_OP_GROUP_4(base, copier, type, 32)

// Keep the product of these in sync with RDX_NUM_OPTIMIZED_MOVES in rdx_ilopcodes.hpp
#define MOVE_SRC_MODE_REGULAR	0
#define MOVE_SRC_MODE_PARENT	1
#define MOVE_SRC_MODE_ABSOLUTE	2
#define MOVE_SRC_MODE_COUNT		3

#define MOVE_SRC_DEREF_REGULAR	0
#define MOVE_SRC_DEREF_DEREF	1
#define MOVE_SRC_DEREF_COUNT	2

#define MOVE_DEST_MODE_REGULAR	0
#define MOVE_DEST_MODE_PARENT	1
#define MOVE_DEST_MODE_COUNT	2

#define MOVE_DEST_DEREF_REGULAR				0
#define MOVE_DEST_DEREF_DEREF				1
#define MOVE_DEST_DEREF_DEREF_SRC_HAS_REFS	2
#define MOVE_DEST_DEREF_DEREF_SRC_IS_REF	3
#define MOVE_DEST_DEREF_COUNT				4

#define MOVE_OP(base, copier, type, index)	\
	case (base + index):\
	{\
		USE_ARG_CA;\
		rdxFastMoveOp<type, index, copier>::Operate(objm, thread, bp, prv, argCA);\
	}\
	break


#define BRANCH_OP(opcode, moveType, pointer, ifEqual)	\
	case opcode:\
	{\
		USE_ARG_CA;\
		rdxEqJumpOp<moveType, pointer, ifEqual>(bp, instructions, nextInstr, argCA);\
	}\
	break

#define BRANCH_OP_CLUSTER_BYSIZE(opcode, pointer, ifEqual)	\
	BRANCH_OP(opcode##_i1, rdxUInt8, pointer, ifEqual);	\
		BRANCH_OP(opcode##_i2, rdxUInt16, pointer, ifEqual);	\
		BRANCH_OP(opcode##_i4, rdxUInt32, pointer, ifEqual);	\
		BRANCH_OP(opcode##_i8, rdxUInt64, pointer, ifEqual);	\
		BRANCH_OP(opcode##_flt, rdxFloat, pointer, ifEqual);	\
		BRANCH_OP(opcode##_dbl, rdxDouble, pointer, ifEqual);	\
		BRANCH_OP(opcode##_bool, rdxBool, pointer, ifEqual);	\
		BRANCH_OP(opcode##_ref, rdxBaseRef, pointer, ifEqual)

#define BRANCH_OP_CLUSTER(opcode, ifEqual)	\
	BRANCH_OP_CLUSTER_BYSIZE(opcode##_f, false, ifEqual);	\
	BRANCH_OP_CLUSTER_BYSIZE(opcode##_p, true, ifEqual)

struct rdxSSingleArgIntrinsic
{
	rdxEILOpcode originalOp;
	rdxEILOpcode newOp;
	rdxLargeInt p2check;
};

struct rdxSDualArgIntrinsic
{
	rdxEILOpcode originalOp;
	rdxEILOpcode newOp;
	rdxLargeInt p2check;
	rdxLargeInt p3check;
};

static const rdxSSingleArgIntrinsic RDX_SINGLE_ARG_INTRINSICS[] =
{
	{ rdxILOP_iadd,	rdxILOP_iadd4,		4 },
	{ rdxILOP_isub,	rdxILOP_isub4,		4 },
	{ rdxILOP_imul,	rdxILOP_imul4,		4 },
	{ rdxILOP_idiv,	rdxILOP_idiv4,		4 },
	{ rdxILOP_imod,	rdxILOP_imod4,		4 },
	{ rdxILOP_ineg,	rdxILOP_ineg4,		4 },
			
	{ rdxILOP_ilt,	rdxILOP_ilt4,		4 },
	{ rdxILOP_igt,	rdxILOP_igt4,		4 },
	{ rdxILOP_ile,	rdxILOP_ile4,		4 },
	{ rdxILOP_ige,	rdxILOP_ige4,		4 },
	{ rdxILOP_ieq,	rdxILOP_ieq4,		4 },
	{ rdxILOP_ine,	rdxILOP_ine4,		4 },

	{ rdxILOP_iadd,	rdxILOP_iadd8,		8 },
	{ rdxILOP_isub,	rdxILOP_isub8,		8 },
	{ rdxILOP_imul,	rdxILOP_imul8,		8 },
	{ rdxILOP_idiv,	rdxILOP_idiv8,		8 },
	{ rdxILOP_imod,	rdxILOP_imod8,		8 },
	{ rdxILOP_ineg,	rdxILOP_ineg8,		8 },

	{ rdxILOP_ilt,	rdxILOP_ilt8,		8 },
	{ rdxILOP_igt,	rdxILOP_igt8,		8 },
	{ rdxILOP_ile,	rdxILOP_ile8,		8 },
	{ rdxILOP_ige,	rdxILOP_ige8,		8 },
	{ rdxILOP_ieq,	rdxILOP_ieq8,		8 },
	{ rdxILOP_ine,	rdxILOP_ine8,		8 },

	{ rdxILOP_fadd,	rdxILOP_fadd4,		4 },
	{ rdxILOP_fsub,	rdxILOP_fsub4,		4 },
	{ rdxILOP_fmul,	rdxILOP_fmul4,		4 },
	{ rdxILOP_fdiv,	rdxILOP_fdiv4,		4 },
	{ rdxILOP_fneg,	rdxILOP_fneg4,		4 },
			
	{ rdxILOP_flt4,	rdxILOP_flt4,		4 },
	{ rdxILOP_fgt4,	rdxILOP_fgt4,		4 },
	{ rdxILOP_fle4,	rdxILOP_fle4,		4 },
	{ rdxILOP_fge4,	rdxILOP_fge4,		4 },
	{ rdxILOP_feq4,	rdxILOP_feq4,		4 },
	{ rdxILOP_fne4,	rdxILOP_fne4,		4 },
};

static const rdxSDualArgIntrinsic RDX_DUAL_ARG_INTRINSICS[] =
{
	{ rdxILOP_isx,		rdxILOP_isx18,		1,	8 },
	{ rdxILOP_isx,		rdxILOP_isx28,		2,	8 },
	{ rdxILOP_isx,		rdxILOP_isx48,		4,	8 },
	{ rdxILOP_isx,		rdxILOP_isx14,		1,	4 },
	{ rdxILOP_isx,		rdxILOP_isx24,		2,	4 },
	{ rdxILOP_isx,		rdxILOP_isx12,		1,	2 },
			
	{ rdxILOP_isx,		rdxILOP_ict84,		8,	4 },
	{ rdxILOP_isx,		rdxILOP_ict82,		8,	2 },
	{ rdxILOP_isx,		rdxILOP_ict81,		8,	1 },
	{ rdxILOP_isx,		rdxILOP_ict42,		4,	2 },
	{ rdxILOP_isx,		rdxILOP_ict41,		4,	1 },
	{ rdxILOP_isx,		rdxILOP_ict21,		2,	1 },
			
	{ rdxILOP_isx,		rdxILOP_nconv8,		8,	8 },
	{ rdxILOP_isx,		rdxILOP_nconv4,		4,	4 },
	{ rdxILOP_isx,		rdxILOP_nconv2,		2,	2 },
	{ rdxILOP_isx,		rdxILOP_nconv1,		1,	1 },

	{ rdxILOP_itof,		rdxILOP_itof44,		4,	4 },
	{ rdxILOP_itof,		rdxILOP_itof48,		4,	8 },
	{ rdxILOP_itof,		rdxILOP_itof84,		8,	4 },
	{ rdxILOP_itof,		rdxILOP_itof88,		8,	8 },
			
	{ rdxILOP_ftoi,		rdxILOP_ftoi44,		4,	4 },
	{ rdxILOP_ftoi,		rdxILOP_ftoi48,		4,	8 },
	{ rdxILOP_ftoi,		rdxILOP_ftoi84,		8,	4 },
	{ rdxILOP_ftoi,		rdxILOP_ftoi88,		8,	8 },
};

template<class arrayValueType>
inline rdxLargeInt rdxSwitchOpCase(const void *valSrc, rdxRef<void> arr, rdxLargeInt numCases)
{
	arrayValueType value = *static_cast<const arrayValueType *>(valSrc);
	const arrayValueType *arrData = arr.StaticCast<arrayValueType>().Data();
	rdxLargeInt selectedCase;
	for(selectedCase=0;selectedCase<numCases;selectedCase++)
	{
		arrayValueType caseValue = arrData[selectedCase];
		if(caseValue == value)
			break;	// Matched
	}
	return selectedCase;
}

template<class srcType, class destType>
inline void rdxConvertOp(rdxUInt8 *bp, const rdxUILOpCompactArg *argCA)
{
	const srcType *src = reinterpret_cast<const srcType *>(bp + argCA[0].li);
	destType *dest = reinterpret_cast<destType *>(bp + argCA[1].li);
	*dest = static_cast<destType>(*src);
}

struct rdxSUnitCopier
{
	template<class T>
	struct SizeCheck
	{
		enum
		{
			FixedSize = sizeof(T),
		};
	};

	template<class elementType, int sizeArgNum>
	inline static void Copy(elementType *destPtr, const elementType *arrayRef, rdxLargeInt index, const rdxUILOpCompactArg *argCA)
	{
		*destPtr = arrayRef[index];
	}
};


struct rdxSBlockCopier
{
	template<class T>
	struct SizeCheck
	{
		enum
		{
			FixedSize = 0,
		};
	};

	template<class elementType, int sizeArgNum>
	inline static void Copy(elementType *destPtr, const elementType *arrayRef, rdxLargeInt index, const rdxUILOpCompactArg *argCA)
	{
		rdxBlockCopy(destPtr, reinterpret_cast<const rdxUInt8 *>(arrayRef) + argCA[sizeArgNum].li * index, static_cast<size_t>(argCA[sizeArgNum].li));
	}
};

// Returns null for no exception, or may return an exception
template<class elementType, class copierType, bool enableSubs>
inline rdxRef<rdxCException> rdxIterateArrayOpBase(rdxCObjectManager *objm, rdxUInt8 *bp, const rdxUILOpCompactArg *argCA, const rdxSILInstruction *instructions, const rdxSILInstruction **nextInstr)
{
	rdxRef<elementType> arrayRef = rdxRef<elementType>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[0].li));
	rdxLargeInt *indexPtr = reinterpret_cast<rdxLargeInt *>(bp + argCA[1].li);
	elementType *destPtr = reinterpret_cast<elementType *>(bp + argCA[2].li);
	if(arrayRef.IsNull())
		return objm->GetBuiltIns()->providerDictionary.ToHandle()[rdxX_NullReferenceException].StaticCast<rdxCException>();
	rdxLargeInt numElements = arrayRef.ObjectInfo()->numElements;
	rdxLargeInt nextIndex = (*indexPtr) + 1;
	if(nextIndex < 0 || nextIndex >= numElements)
	{
		*nextInstr = instructions + argCA[3].li;
		return rdxRef<rdxCException>::Null();
	}

	rdxLargeInt numDimensions;

	if(enableSubs)
	{
		numDimensions = argCA[7].li;
		if(numDimensions != arrayRef.ObjectInfo()->numDimensions)
			return objm->GetBuiltIns()->providerDictionary.ToHandle()[rdxX_IndexOutOfBoundsException].ToHandle().ToRef().DirectCast<rdxCException>();
	}

	// Nothing can fail now, commit changes
	// Save next index
	*indexPtr = nextIndex;
	// Extract value
	copierType::Copy<elementType, 4>(destPtr, arrayRef.Data(), nextIndex, argCA);

	if(enableSubs)
	{
		rdxLargeInt *subIndexLocals = reinterpret_cast<rdxLargeInt *>(bp + argCA[6].li);
		const rdxLargeInt *dimensions = arrayRef.ObjectInfo()->dimensions.Data();
		for(rdxLargeInt dimensionIndex=numDimensions-1; ;dimensionIndex--)
		{
			rdxLargeInt *subIndexLocal = subIndexLocals - dimensionIndex;	/* Locals allocate down */
			rdxLargeInt incremented = (*subIndexLocal) + 1;
			if(incremented == dimensions[dimensionIndex] && dimensionIndex != 0)
				*subIndexLocal = 0;	/* Carry */
			else
			{
				*subIndexLocal = incremented;
				break;
			}
		}
	}

	return rdxRef<rdxCException>::Null();
}

template<class valueT, bool pointer, bool ifEqual>
inline void rdxEqJumpOp(rdxUInt8 *bp, const rdxSILInstruction *instructions, const rdxSILInstruction *&nextInstr, const rdxUILOpCompactArg *argCA)
{
	const void *src1 = reinterpret_cast<const void *>(bp + argCA[1].li);
	const void *src2 = reinterpret_cast<const void *>(bp + argCA[2].li);

	if(pointer)
	{
		src1 = rdxSOffsetRef<rdxBaseRef, valueT>::From(*static_cast<const rdxSUntypedOffsetRef *>(src1)).Data();
		src2 = rdxSOffsetRef<rdxBaseRef, valueT>::From(*static_cast<const rdxSUntypedOffsetRef *>(src2)).Data();
	}

	if((*static_cast<const valueT *>(src1) == *static_cast<const valueT *>(src2)) == ifEqual)
		nextInstr = instructions + argCA[0].li;
}

template<class moveT, int index, class copierT>
struct rdxFastMoveOp
{
	enum
	{
		srcMode			= index % MOVE_SRC_MODE_COUNT,
		srcDerefBase	= index / MOVE_SRC_MODE_COUNT,

		srcDeref		= srcDerefBase % MOVE_SRC_DEREF_COUNT,
		destModeBase	= srcDerefBase / MOVE_SRC_DEREF_COUNT,

		destMode		= destModeBase % MOVE_DEST_MODE_COUNT,
		destDerefBase	= destMode / MOVE_DEST_MODE_COUNT,

		destDeref		= destDerefBase % MOVE_DEST_DEREF_COUNT,
	};

	inline static void Operate(rdxCObjectManager *objm, const rdxHandle<rdxCRuntimeThread> &thread, rdxUInt8 *bp, rdxUInt8 *prv, const rdxUILOpCompactArg *argCA)
	{
		if(destDeref == MOVE_DEST_DEREF_DEREF_SRC_IS_REF && copierT::SizeCheck<moveT>::FixedSize != 0 && copierT::SizeCheck<moveT>::FixedSize != sizeof(rdxRef<void>))
			return;

		const void *src;
		void *dest;
		if(srcMode == MOVE_SRC_MODE_PARENT)
			src = prv + argCA[0].li;
		else if(srcMode == MOVE_SRC_MODE_ABSOLUTE)
			src = rdxHandle<void>::From(rdxBaseHandle::From(argCA[0].p)).Data();
		else
			src = bp + argCA[0].li;

		if(srcDeref)
			src = rdxSOffsetRef<rdxBaseRef, void>::From(*reinterpret_cast<const rdxSUntypedOffsetRef *>(src)).Data();

		if(destMode == MOVE_DEST_MODE_PARENT)
			dest = prv + argCA[1].li;
		else
			dest = bp + argCA[1].li;

		if(destDeref == MOVE_DEST_DEREF_DEREF)
		{
			moveT *destView = rdxSOffsetRef<rdxBaseRef, moveT>::From(*reinterpret_cast<const rdxSUntypedOffsetRef *>(dest)).QuietOpen();
			copierT::Copy<moveT, 2>(static_cast<moveT*>(destView), static_cast<const moveT*>(src), 0, argCA);
			return;
		}
		else if(destDeref == MOVE_DEST_DEREF_DEREF_SRC_HAS_REFS)
		{
			rdxSOffsetRef<rdxBaseRef, moveT> destOffsetRef = rdxSOffsetRef<rdxBaseRef, moveT>::From(*reinterpret_cast<const rdxSUntypedOffsetRef *>(dest));
			moveT *destView = destOffsetRef.QuietOpen();
			copierT::Copy<moveT, 2>(static_cast<moveT*>(destView), static_cast<const moveT*>(src), 0, argCA);

			// If the source has references, then mark the destination for a full scan
#ifdef RDX_USE_INCREMENTAL_GC
			objm->GCAddScan(destOffsetRef.objectRef);
#endif
			return;
		}
		else if(destDeref == MOVE_DEST_DEREF_DEREF_SRC_IS_REF)
		{
			rdxSOffsetRef<rdxBaseRef, rdxBaseRef> destOffsetRef = rdxSOffsetRef<rdxBaseRef, rdxBaseRef>::From(*reinterpret_cast<const rdxSUntypedOffsetRef *>(dest));
			rdxBaseRef srcAsRef = *reinterpret_cast<const rdxBaseRef *>(src);

			*destOffsetRef.QuietOpen() = srcAsRef;
			rdxBaseRef *destView = destOffsetRef.QuietOpen();
			
#ifdef RDX_USE_INCREMENTAL_GC
			// If the source is a reference and not in the working set, mark it
			if((rdxRef<void>::From(srcAsRef).GCFlags() & rdxGCInfoBase::GCOF_GCNotGarbage) == 0)
				objm->GCAddScan(srcAsRef);
#endif
			return;
		}
		else
		{
			dest = rdxSOffsetRef<rdxBaseRef, void>::From(*reinterpret_cast<const rdxSUntypedOffsetRef *>(dest)).QuietOpen();
			copierT::Copy<moveT, 2>(static_cast<moveT*>(dest), static_cast<const moveT*>(src), 0, argCA);
		}
	}
};

inline static bool rdxIsBigEndian()
{
	union
	{
		rdxInt16 s;
		rdxUInt8 bytes[2];
	} u;
	u.s = 1;
	return u.bytes[0] == 0;
}

int rdxThrowException(rdxRef<rdxCRuntimeThread> &t, rdxHandle<rdxCException> &e, rdxRef<rdxCMethod> &m, const void *ip, rdxSRuntimeStackFrame *bp, rdxURuntimeStackValue *prv)
{
	rdxCDataView<rdxRef<rdxCRuntimeThread> > tOpened(t);
	tOpened->ex = e;
	tOpened->frame.ip = ip;
	tOpened->frame.bp = bp;
	tOpened->frame.method = m;
	tOpened->frame.prv = prv;
	return rdxRS_Exception;
}

#define THROWEXCEPTION(e) return rdxThrowException(thread.ToRef(), e.ToHandle().StaticCast<rdxCException>(), m.ToRef(), nextInstr, reinterpret_cast<rdxSRuntimeStackFrame*>(bp), reinterpret_cast<rdxURuntimeStackValue*>(prv))
#define THROWEXCEPTIONDICT(dict)	THROWEXCEPTION(objm->GetBuiltIns()->providerDictionary.ToHandle()[dict])



#define REFRESH_METHOD	\
	do {\
		argCAbase = m->m_native.compactArgs.Data();\
		argLAbase = m->m_native.largeArgs.Data();\
	} while(false)

#define USE_ARG_LA	const rdxUILOpLargeArg *argLA = argLAbase + instr->argOffs
#define REFRESH_ARG_LA	argLA = argLAbase + instr->argOffs

#define USE_ARG_CA	const rdxUILOpCompactArg *argCA = argCAbase + instr->argOffs
#define REFRESH_ARG_CA	argCA = argCAbase + instr->argOffs

#define ZERO_OP(opcode, type)	\
	case opcode:\
	{\
		USE_ARG_CA;\
		*reinterpret_cast<type*>(bp + argCA[0].li) = static_cast<type>(0);\
	}\
	break


int rdxResumeThreadInterpreted(rdxSOperationContext *ctx, rdxIObjectManager *objmInterface, rdxHandle<rdxCRuntimeThread> thread)
{
	rdxUInt8 *bp = reinterpret_cast<rdxUInt8*>(thread->frame.bp);
	rdxUInt8 *prv = reinterpret_cast<rdxUInt8*>(thread->frame.prv);
	const rdxSILInstruction *nextInstr = reinterpret_cast<const rdxSILInstruction *>(thread->frame.ip);
	rdxHandle<rdxCMethod> m = thread->frame.method.ToHandle();
	const rdxSILInstruction *instructions = static_cast<const rdxSILInstruction *>(m->m_native.optimizedInstructions);
	rdxUInt8 *stackBase = thread->stackBytes;
	const rdxUILOpCompactArg *argCAbase;
	const rdxUILOpLargeArg *argLAbase;

	rdxCObjectManager *objm = static_cast<rdxCObjectManager*>(objmInterface);

	rdxLargeInt numInstrs = m->m_native.numILInstructions;

	REFRESH_METHOD;

	while(true)
	{
		const rdxSILInstruction *instr = nextInstr++;
		rdxLargeInt instrNum = instr - instructions;

		switch(instr->opcode)
		{
		case rdxILOP_debuginfo:
		case rdxILOP_hardenstack:
			break;

		// <src> <dest> <size> <alignment> <loc flags>    Copy current frame to current frame
		//case rdxILOP_move:
		// ILOP_move is always optimized

		// <dest> <type> <src>             Converts a runtime pointer at the destination to a typed runtime pointer
		case rdxILOP_tovarying:
			{
				USE_ARG_CA;

				rdxSTypedOffsetRef trp;

				trp.rtp = *reinterpret_cast<const rdxSUntypedOffsetRef *>(bp + argCA[2].li);
				trp.type = rdxHandle<rdxCType>::Load(argCA[1].p).ToRef();
				*reinterpret_cast<rdxSTypedOffsetRef *>(bp + argCA[0].li) = trp;
			}
			break;
		// <loc> <type>                    Converts a runtime pointer at the destination to a typed runtime pointer
		case rdxILOP_tovarying_static:
			{
				USE_ARG_CA;

				rdxSTypedOffsetRef *trp = reinterpret_cast<rdxSTypedOffsetRef *>(bp + argCA[0].li);
				trp->type = rdxHandle<rdxCType>::Load(argCA[1].p).ToRef();
			}
			break;
		// <dest> <offset>                 Creates a pointer to the current frame + offset
		case rdxILOP_pinl:
			{
				USE_ARG_CA;

				*reinterpret_cast<rdxSUntypedOffsetRef *>(bp + argCA[0].li) = rdxSUntypedOffsetRef::FromStackPtr(bp + argCA[1].li);
			}
			break;
		// <dest> <offset>                 Creates a pointer to the parent frame + offset
		case rdxILOP_pinp:
			{
				USE_ARG_CA;
				
				*reinterpret_cast<rdxSUntypedOffsetRef *>(bp + argCA[0].li) = rdxSUntypedOffsetRef::FromStackPtr(prv + argCA[1].li);
			}
			break;
		// <loc> <offset>               Increases a pointer's size by offset
		case rdxILOP_incptr:
			{
				USE_ARG_CA;

				reinterpret_cast<rdxSUntypedOffsetRef *>(bp + argCA[0].li)->valueOffset += argCA[1].li;
			}
			break;
		// <src> <dest> <offset>                 NULL check src and create a pointer to the start of src + offset
		case rdxILOP_objinterior:
			{
				USE_ARG_CA;

				rdxRef<void> p = rdxRef<void>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[0].li));
				if(p.IsNull())
					THROWEXCEPTIONDICT(rdxX_NullReferenceException);

				*reinterpret_cast<rdxSUntypedOffsetRef *>(bp + argCA[1].li) = rdxSUntypedOffsetRef::From(p.BaseRef().PODValue(), argCA[2].li);
			}
			break;
		// <src> <dest> <offset>                 create a pointer to the start of src + offset
		case rdxILOP_objinterior_notnull:
		case rdxILOP_objinterior_notnull_persist:
			{
				USE_ARG_CA;
				
				rdxBaseRef::PODType podValue = reinterpret_cast<const rdxBaseRef *>(bp + argCA[0].li)->PODValue();
				*reinterpret_cast<rdxSUntypedOffsetRef *>(bp + argCA[1].li) = rdxSUntypedOffsetRef::From(podValue, argCA[2].li);
			}
			break;
		// <LA0:value> <LA1[0]:dest> <LA1[1]:size>        Copy instruction value to current frame
		case rdxILOP_immediate:
			{
				USE_ARG_LA;

				void *p = bp + argLA[1].ca[0].li;
				rdxBlockCopy(p, &argLA[0], argLA[1].ca[1].lui);
			}
			break;
		case rdxILOP_immediate_i8:
			{
				USE_ARG_LA;

				rdxUInt8 *p = reinterpret_cast<rdxUInt8*>(bp + argLA[1].ca[0].li);
				*p = argLA[0].ca[0].u8;
			}
			break;
		case rdxILOP_immediate_i16:
			{
				USE_ARG_LA;

				rdxUInt16 *p = reinterpret_cast<rdxUInt16*>(bp + argLA[1].ca[0].li);
				*p = argLA[0].ca[0].u16;
			}
			break;
		case rdxILOP_immediate_i32:
			{
				USE_ARG_LA;

				rdxUInt32 *p = reinterpret_cast<rdxUInt32*>(bp + argLA[1].ca[0].li);
				*p = argLA[0].ca[0].u32;
			}
			break;
		case rdxILOP_immediate_i64:
			{
				USE_ARG_LA;

				rdxUInt64 *p = reinterpret_cast<rdxUInt64*>(bp + argLA[1].ca[0].li);
				*p = argLA[0].u64;
			}
			break;
		case rdxILOP_immediate_bool:
			{
				USE_ARG_LA;

				rdxBool *p = reinterpret_cast<rdxBool*>(bp + argLA[1].ca[0].li);
				*p = argLA[0].ca[0].b;
			}
			break;
		case rdxILOP_immediate_flt:
			{
				USE_ARG_LA;

				rdxFloat *p = reinterpret_cast<rdxFloat*>(bp + argLA[1].ca[0].li);
				*p = argLA[0].ca[0].f;
			}
			break;
		case rdxILOP_immediate_dbl:
			{
				USE_ARG_LA;

				rdxDouble *p = reinterpret_cast<rdxDouble*>(bp + argLA[1].ca[0].li);
				*p = argLA[0].dbl;
			}
			break;
		case rdxILOP_immediate_ptr:
			{
				USE_ARG_LA;

				rdxBaseRef *p = reinterpret_cast<rdxBaseRef *>(bp + argLA[1].ca[0].li);
				*p = rdxBaseHandle::From(argLA[0].ca[0].p).ToRef();
			}
			break;
		case rdxILOP_immediate_rtp:
			{
				USE_ARG_LA;

				*reinterpret_cast<rdxSUntypedOffsetRef *>(bp + argLA[1].ca[0].li) = rdxSUntypedOffsetHandle::From(argLA[0].rtp).ToRef();
			}
			break;
		// <indexsrc> <arraysrc> <dest>          NULL check arraysrc from current frame, push internal reference to array head + offset
		case rdxILOP_arrayindex:
			{
				USE_ARG_CA;

				rdxRef<void> o = rdxRef<void>::From(*reinterpret_cast<rdxBaseRef *>(bp + argCA[1].li));

				if(o.IsNull())
					THROWEXCEPTIONDICT(rdxX_NullReferenceException);

				rdxSUntypedOffsetRef interior;
				
				if(!rdxArrayIndex(o.BaseRef(), reinterpret_cast<const rdxURuntimeStackValue*>(bp + argCA[0].li), interior))
					THROWEXCEPTIONDICT(rdxX_IndexOutOfBoundsException);

				interior.valueOffset += argCA[3].li;
				*reinterpret_cast<rdxSUntypedOffsetRef *>(bp + argCA[2].li);
			}
			break;
		// <paramsrc> <rvdest> <objsrc> <(I)vstidx> <interface> Loads the target method, null checks, and transfers control
		case rdxILOP_callinterface:
		// <paramsrc> <rvdest> <objsrc> <(Int)vstidx>     Loads the target method, null checks, and transfers control
		case rdxILOP_callvirtual:
		// <paramsrc> <rvdest> <method>                   Transfers control to the target method
		case rdxILOP_calldelegatebp:
		case rdxILOP_calldelegateprv:
		// <paramsrc> <rvdest> <methodsrc>                Null checks and transfers control to the target method
		case rdxILOP_call:
			{
				USE_ARG_CA;

				rdxRef<rdxCMethod> invokedMethod;
				switch(instr->opcode)
				{
				case rdxILOP_callvirtual:
					{
						rdxRef<void> o = rdxRef<void>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[2].li));

						if(o.IsNull())
							THROWEXCEPTIONDICT(rdxX_NullReferenceException);

						invokedMethod = o.ObjectInfo()->vft[argCA[3].li];
					}
					break;
				case rdxILOP_callinterface:
					{
						rdxRef<void> o = rdxRef<void>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[2].li));

						if(o.IsNull())
							THROWEXCEPTIONDICT(rdxX_NullReferenceException);

						rdxRef<rdxCType> interfaceType = rdxHandle<rdxCType>::From(rdxBaseHandle::From(argCA[4].p)).ToRef();

						rdxLargeInt vftOffset = 0;
						rdxLargeInt numInterfaces = 0;
						rdxRef<rdxSInterfaceImplementation> interfaceImpls = o.ObjectInfo()->containerType.StaticCast<rdxCStructuredType>()->interfaces;
						numInterfaces = interfaceImpls.ObjectInfo()->numElements;
						const rdxSInterfaceImplementation *rawImpls = interfaceImpls.Data();
						for(rdxLargeInt i=0;i<numInterfaces;i++)
						{
							const rdxSInterfaceImplementation *impl = rawImpls + i;
							if(impl->type == interfaceType)
							{
								vftOffset = impl->vftOffset;
								break;
							}
						}

						invokedMethod = o.ObjectInfo()->vft[argCA[3].li + vftOffset];
					}
					break;
				case rdxILOP_call:
					invokedMethod = rdxHandle<rdxCMethod>::From(rdxBaseHandle::From(argCA[2].p)).ToRef();
					break;
				case rdxILOP_calldelegatebp:
					invokedMethod = rdxRef<rdxCMethod>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[2].li));
					break;
				case rdxILOP_calldelegateprv:
					invokedMethod = rdxRef<rdxCMethod>::From(*reinterpret_cast<const rdxBaseRef *>(prv + argCA[2].li));
					break;
				default:
					invokedMethod = rdxRef<rdxCMethod>::Null();	// To shut up compiler warning
					break;
				}

				if(invokedMethod.IsNull())
					THROWEXCEPTIONDICT(rdxX_NullReferenceException);

				rdxSRuntimeStackFrame currentFrame;
				currentFrame.bp = reinterpret_cast<rdxSRuntimeStackFrame*>(bp);
				currentFrame.ip = nextInstr;
				currentFrame.method = m.ToRef();
				currentFrame.prv = reinterpret_cast<rdxURuntimeStackValue*>(prv);

				rdxNativeCallback cb = invokedMethod->m_native.nativeCall;

				if(!invokedMethod->m_native.isNativeCall)
				{
					if(invokedMethod->bytecode.IsNull())
						THROWEXCEPTIONDICT(rdxX_NullReferenceException);

					// Re-enter
					rdxSRuntimeStackFrame newFrame;
					if(!rdxEnterMethodInline(objm, invokedMethod, &currentFrame, bp + argCA[0].li, stackBase,
						reinterpret_cast<rdxURuntimeStackValue*>(bp + argCA[1].li), &newFrame))
					{
						THROWEXCEPTIONDICT(rdxX_StackOverflowException);
					}

					if(newFrame.method->m_native.precompiledCodeModule)
					{
						rdxCDataView<rdxHandle<rdxCRuntimeThread> > openedThread(thread);
						openedThread->frame = newFrame;
						return rdxRS_Active;
					}

					bp = reinterpret_cast<rdxUInt8*>(newFrame.bp);
					prv = reinterpret_cast<rdxUInt8*>(newFrame.prv);
					nextInstr = reinterpret_cast<const rdxSILInstruction *>(newFrame.ip);
					m = newFrame.method.ToHandle();
					instructions = static_cast<const rdxSILInstruction *>(m->m_native.optimizedInstructions);

					REFRESH_METHOD;
				}
				else
				{
					if(cb == NULL)
						THROWEXCEPTIONDICT(rdxX_NullReferenceException);

					{
						rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
						threadView->frame = currentFrame;
						threadView->insertionPoint = bp + argCA[0].li;
						threadView->activeNativeMethod = invokedMethod.ToHandle();
					}

					int status = cb(ctx, objm, invokedMethod.ToHandle(), thread, reinterpret_cast<rdxURuntimeStackValue*>(bp + argCA[1].li));
					{
						rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
						threadView->activeNativeMethod = rdxHandle<rdxCMethod>::Null();
					}

					REFRESH_METHOD;

					if(status <= 0)
					{
						rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
						threadView->frame = currentFrame;
						return status;
					}
				}
			}
			break;
		// <loc> <instr offset>                                 Verify that a current-frame value is not null
		case rdxILOP_verifynotnull:
			{
				USE_ARG_CA;

				rdxRef<void> o = rdxRef<void>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[0].li));
				if(o.IsNull())
					THROWEXCEPTIONDICT(rdxX_NullReferenceException);
			}
			break;
		case rdxILOP_zero:
			{
				USE_ARG_CA;

				memset(bp + argCA[0].li, 0, argCA[1].lui);
			}
			break;
		ZERO_OP(rdxILOP_zero_i8, rdxInt8);
		ZERO_OP(rdxILOP_zero_i16, rdxInt16);
		ZERO_OP(rdxILOP_zero_i32, rdxInt32);
		ZERO_OP(rdxILOP_zero_i64, rdxInt64);
		ZERO_OP(rdxILOP_zero_flt, rdxFloat);
		ZERO_OP(rdxILOP_zero_dbl, rdxDouble);
		
		case rdxILOP_zero_bool:
			{
				USE_ARG_CA;
				*reinterpret_cast<rdxBool*>(bp + argCA[0].li) = RDX_FALSE_VALUE;
			}
			break;

		// <loc> <type> <dimension src> <num dimensions>  Creates a new instance of the specified type in the current frame
		case rdxILOP_newinstance:
			{
				USE_ARG_CA;

				rdxRef<void> o;

				// Allocations can block, make sure the IP is up to date so the thread can be traced
				{
					rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
					EXPORT_FRAME(&threadView->frame);
				}

				o = rdxNewObjectInstance(ctx, objm, rdxHandle<rdxCType>::From(rdxBaseHandle::From(argCA[1].p)),
					reinterpret_cast<rdxURuntimeStackValue*>(bp + argCA[2].li),
					argCA[3].li);

				REFRESH_METHOD;
				REFRESH_ARG_CA;

				if(o.IsNull())
					THROWEXCEPTIONDICT(rdxX_AllocationFailureException);
				*reinterpret_cast<rdxBaseRef *>(bp + argCA[0].li) = o.BaseRef();
			}
			break;
		case rdxILOP_exit:
			{
				rdxSRuntimeStackFrame *rfp = reinterpret_cast<rdxSRuntimeStackFrame *>(bp);
				if(rfp->method.IsNull() || rfp->method->m_native.precompiledCodeModule)
				{
					rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
					threadView->frame.bp = rfp;
					return rdxRS_AbandonFrame;
				}

				nextInstr = static_cast<const rdxSILInstruction*>(rfp->ip);
				prv = reinterpret_cast<rdxUInt8*>(rfp->prv);
				m = rfp->method.ToHandle();
				instructions = static_cast<const rdxSILInstruction *>(m->m_native.optimizedInstructions);
				bp = reinterpret_cast<rdxUInt8*>(rfp->bp);
			}
			break;
		// <instrnum>                                     Jumps to the specified instruction
		case rdxILOP_jump:
			{
				USE_ARG_CA;

				nextInstr = instructions + argCA[0].li;
			}
			break;
		// <instrnum> <src>                               Jumps to the specified instruction if the target value is true
		case rdxILOP_jtrue:
			{
				USE_ARG_CA;

				if(*reinterpret_cast<rdxBool*>(bp + argCA[1].li) != rdxFalseValue)
					nextInstr = instructions + argCA[0].li;
			}
			break;
		// <instrnum> <src>                               Jumps to the specified instruction if the target value is not true
		case rdxILOP_jfalse:
			{
				USE_ARG_CA;

				if(*reinterpret_cast<rdxBool*>(bp + argCA[1].li) == rdxFalseValue)
					nextInstr = instructions + argCA[0].li;
			}
			break;
		// <instrnum> <src> <type>                        Jumps to the specified instruction if the target value inherits from the specified type
		case rdxILOP_jinherits:
			{
				USE_ARG_CA;

				rdxRef<void> o = rdxRef<void>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[1].li));
				rdxHandle<rdxCType> t = rdxHandle<rdxCType>::From(rdxBaseHandle::From(argCA[2].p));

				if(o.IsNull())
					THROWEXCEPTIONDICT(rdxX_NullReferenceException);

				if(objm->ObjectCompatible(o, t.ToRef()))
					nextInstr = instructions + argCA[0].li;
			}
			break;
		case rdxILOP_jccp:
			{
				USE_ARG_CA;

				rdxRef<rdxCType> rs = rdxRef<rdxCType>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[1].li));
				rdxRef<void> ls = rdxRef<void>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[1].li + rdxPaddedSize(sizeof(rdxBaseRef), RDX_ALIGNOF(rdxURuntimeStackValue))));
				if(ls.IsNull())
					THROWEXCEPTIONDICT(rdxX_NullReferenceException);

				if(objm->ObjectCompatible(ls, rs))
					nextInstr = instructions + argCA[0].li;
			}
			break;
		case rdxILOP_jccf:
			{
				USE_ARG_CA;

				rdxRef<rdxCType> rs = rdxRef<rdxCType>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[1].li));
				rdxRef<void> ls = rdxRef<void>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[1].li + rdxPaddedSize(sizeof(rdxBaseRef), RDX_ALIGNOF(rdxURuntimeStackValue))));
				if(ls.IsNull())
					THROWEXCEPTIONDICT(rdxX_NullReferenceException);

				if(!objm->ObjectCompatible(ls, rs))
					nextInstr = instructions + argCA[0].li;
			}
			break;
		case rdxILOP_tick:
			{
				if(rdxNonAtomicDecrement(&thread.QuietOpen()->timeout) == 0)
				{
					rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
					threadView->frame.bp = reinterpret_cast<rdxSRuntimeStackFrame*>(bp);
					threadView->frame.prv = reinterpret_cast<rdxURuntimeStackValue*>(prv);
					threadView->frame.ip = nextInstr;
					threadView->frame.method = m.ToRef();
					rdxSynchronize();
					return rdxRS_TimedOut;
				}
			}
			break;

		// <src> <type>                                   Throws IncompatibleConversionException if value in current frame can not be found in a specified enum table.
		case rdxILOP_assertenum:
			{
				USE_ARG_CA;

				rdxEnumValue ev = *reinterpret_cast<rdxEnumValue *>(bp + argCA[0].li);
				rdxRef<rdxSEnumerant> enums = rdxHandle<rdxCStructuredType>::From(rdxBaseHandle::From(argCA[1].p))->enumerants;

				if(!objm->EnumCompatible(ev, enums))
					THROWEXCEPTIONDICT(rdxX_IncompatibleConversionException);
			}
			break;

		// <src> <type>                                   Throws IncompatibleConversionException if value in current frame can not be cast to type, value may be null
		case rdxILOP_assertinherits:
			{
				USE_ARG_CA;

				rdxRef<void> o = rdxRef<void>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[0].li));
				rdxRef<rdxCType> t = rdxHandle<rdxCType>::From(rdxBaseHandle::From(argCA[1].p)).ToRef();

				if(!objm->ObjectCompatible(o, t))
					THROWEXCEPTIONDICT(rdxX_IncompatibleConversionException);
			}
			break;
		case rdxILOP_jeq_f:
			{
				USE_ARG_CA;

				const void *src1 = bp + argCA[1].li;
				const void *src2 = bp + argCA[2].li;
				if(!memcmp(src1, src2, argCA[3].lui))
					nextInstr = instructions + argCA[0].li;
			}
			break;
		// <instrnum> <src1> <src2> <size> <align>        Jumps to the specified instruction if the two specified pointers are equal
		case rdxILOP_jeq_p:
			{
				USE_ARG_CA;

				
				rdxSOffsetRef<rdxBaseRef, void> ptr1 = rdxSOffsetRef<rdxBaseRef, void>::From(*reinterpret_cast<const rdxSUntypedOffsetRef *>(bp + argCA[1].li));
				rdxSOffsetRef<rdxBaseRef, void> ptr2 = rdxSOffsetRef<rdxBaseRef, void>::From(*reinterpret_cast<const rdxSUntypedOffsetRef *>(bp + argCA[2].li));
				if(!memcmp(ptr1.Data(), ptr1.Data(), argCA[3].lui))
					nextInstr = instructions + argCA[0].li;
			}
			break;
		// <instrnum> <src1> <src2> <size> <align>        Jumps to the specified instruction if the two specified values are not equal
		case rdxILOP_jne_f:
			{
				USE_ARG_CA;

				const void *src1 = bp + argCA[1].li;
				const void *src2 = bp + argCA[2].li;
				if(memcmp(src1, src2, argCA[3].lui))
					nextInstr = instructions + argCA[0].li;
			}
			break;
		// <instrnum> <src1> <src2> <size> <align>        Jumps to the specified instruction if the two specified pointers are not equal
		case rdxILOP_jne_p:
			{
				USE_ARG_CA;
				
				rdxSOffsetRef<rdxBaseRef, void> ptr1 = rdxSOffsetRef<rdxBaseRef, void>::From(*reinterpret_cast<const rdxSUntypedOffsetRef *>(bp + argCA[1].li));
				rdxSOffsetRef<rdxBaseRef, void> ptr2 = rdxSOffsetRef<rdxBaseRef, void>::From(*reinterpret_cast<const rdxSUntypedOffsetRef *>(bp + argCA[2].li));
				if(memcmp(ptr1.Data(), ptr1.Data(), argCA[3].lui))
					nextInstr = instructions + argCA[0].li;
			}
			break;
		// <src>                                          Throws the exception at src.  If src is null, throws UnspecifiedException
		case rdxILOP_throw:
			{
				USE_ARG_CA;

				rdxRef<rdxCException> e = rdxRef<rdxCException>::From(*reinterpret_cast<const rdxBaseRef *>(bp + argCA[0].li));
				if(e.IsNull())
				{
					THROWEXCEPTIONDICT(rdxX_UnspecifiedException);
				}
				THROWEXCEPTION(e);
			}
			break;
		case rdxILOP_xnullref:
			{
				THROWEXCEPTIONDICT(rdxX_NullReferenceException);
			}
			break;
		case rdxILOP_catch:
			{
				THROWEXCEPTIONDICT(rdxX_InvalidOperationException);
			}
			break;
		case rdxILOP_fatal:
			{
				THROWEXCEPTIONDICT(rdxX_InvalidOperationException);
			}
			break;

		// Converts a signed integer of size p2 to a signed integer of size p3
		case rdxILOP_isx:
			{
				USE_ARG_CA;

				if(argCA[3].li <= argCA[2].li)
				{
					if(rdxIsBigEndian())
					{
						rdxBlockCopy(bp + argCA[1].li, bp + argCA[0].li + (argCA[2].lui - argCA[3].lui), argCA[3].lui);
					}
					else
					{
						rdxBlockCopy(bp + argCA[1].li, bp + argCA[0].li, argCA[3].lui);
					}
				}
				else
				{
					if(rdxIsBigEndian())
					{
						union
						{
							rdxUInt8 bytes[2];
							rdxLargeInt extended;
						} u;

						const rdxUInt8 *originalBytes = bp + argCA[0].li;
						rdxLargeInt origSize = argCA[2].li;

						rdxInt8 extendByte = static_cast<rdxInt8>(originalBytes[0]);
						u.extended = extendByte;
						rdxUInt8 extensionByte = u.bytes[0];

						rdxUInt8 *outBytes = static_cast<rdxUInt8 *>(bp + argCA[1].li);
						rdxLargeInt extSize = argCA[3].li - argCA[2].li;
						while(extSize--)
							*outBytes++ = extensionByte;
						while(origSize--)
							*outBytes++ = *originalBytes++;
					}
					else
					{
						// Little endian
						union
						{
							rdxUInt8 bytes[2];
							rdxLargeInt extended;
						} u;

						const rdxUInt8 *originalBytes = bp + argCA[0].li;
						rdxLargeInt origSize = argCA[2].li;

						rdxInt8 extendByte = static_cast<rdxInt8>(originalBytes[origSize-1]);
						u.extended = extendByte;
						rdxUInt8 extensionByte = u.bytes[1];

						rdxUInt8 *outBytes = static_cast<rdxUInt8 *>(bp + argCA[1].li);
						rdxLargeInt extSize = argCA[3].li - argCA[2].li;
						while(origSize--)
							*outBytes++ = *originalBytes++;
						while(extSize--)
							*outBytes++ = extensionByte;
					}
				}
			}
			break;

		// <src> <array res> <count> <size> <alias type>
		case rdxILOP_switch:
		case rdxILOP_switch_ptr:
			{
				USE_ARG_CA;

				const void *valSrc = bp + argCA[0].li;
				if(instr->opcode == rdxILOP_switch_ptr)
					valSrc = rdxSOffsetRef<rdxBaseRef, void>::From(*reinterpret_cast<const rdxSUntypedOffsetRef *>(valSrc)).Data();

				rdxRef<void> arr = rdxHandle<void>::From(rdxBaseHandle::From(argCA[1].p)).ToRef();
				rdxLargeInt numCases = argCA[2].li;
				rdxLargeInt size = argCA[3].li;
				rdxAliasingType aliasingType = argCA[4].at;
				
				rdxLargeInt selectedCase;

				switch(aliasingType)
				{
				case rdxALIASINGTYPE_Int64:
					selectedCase = rdxSwitchOpCase<rdxInt64>(valSrc, arr, numCases);
					break;
				case rdxALIASINGTYPE_Int32:
					selectedCase = rdxSwitchOpCase<rdxInt32>(valSrc, arr, numCases);
					break;
				case rdxALIASINGTYPE_Int16:
					selectedCase = rdxSwitchOpCase<rdxInt16>(valSrc, arr, numCases);
					break;
				case rdxALIASINGTYPE_Int8:
					selectedCase = rdxSwitchOpCase<rdxInt8>(valSrc, arr, numCases);
					break;
				case rdxALIASINGTYPE_Reference:
					selectedCase = rdxSwitchOpCase<rdxBaseRef>(valSrc, arr, numCases);
					break;
				case rdxALIASINGTYPE_Double:
					selectedCase = rdxSwitchOpCase<rdxDouble>(valSrc, arr, numCases);
					break;
				case rdxALIASINGTYPE_Float:
					selectedCase = rdxSwitchOpCase<rdxFloat>(valSrc, arr, numCases);
					break;
				case rdxALIASINGTYPE_Bool:
					selectedCase = rdxSwitchOpCase<rdxBool>(valSrc, arr, numCases);
					break;
				default:
					{
						const rdxUInt8 *arrRaw = arr.StaticCast<rdxUInt8>().Data();
						for(selectedCase=0;selectedCase<numCases;selectedCase++)
						{
							if(!memcmp(arrRaw + size*selectedCase, valSrc, static_cast<size_t>(size)))
								break;	// Matched
						}
					}
					break;
				}

				nextInstr += selectedCase;
			}
			break;

		ITERATE_ARRAY_OP_BASE(rdxILOP_iteratearray, void, rdxSBlockCopier, false);
		ITERATE_ARRAY_OP(rdxILOP_iteratearray4, rdxInt32, false);
		ITERATE_ARRAY_OP(rdxILOP_iteratearray8, rdxInt64, false);
		ITERATE_ARRAY_OP_BASE(rdxILOP_iteratearraysub, void, rdxSBlockCopier, true);
		ITERATE_ARRAY_OP(rdxILOP_iteratearraysub4, rdxInt32, true);
		ITERATE_ARRAY_OP(rdxILOP_iteratearraysub8, rdxInt64, true);

		BINARY_ARITHMETIC_OP(rdxILOP_iadd4, rdxInt32, +);
		BINARY_ARITHMETIC_OP(rdxILOP_isub4, rdxInt32, -);
		BINARY_ARITHMETIC_OP(rdxILOP_imul4, rdxInt32, *);
		BINARY_ARITHMETIC_OP_ZERO_CHECK(rdxILOP_idiv4, rdxInt32, /);
		BINARY_ARITHMETIC_OP_ZERO_CHECK(rdxILOP_imod4, rdxInt32, %);
		UNARY_ARITHMETIC_OP(rdxILOP_ineg4, rdxInt32, -);

		NUMERIC_COMPARE_OP(rdxILOP_ige4, rdxInt32, >=);
		NUMERIC_COMPARE_OP(rdxILOP_igt4, rdxInt32, >);
		NUMERIC_COMPARE_OP(rdxILOP_ile4, rdxInt32, <=);
		NUMERIC_COMPARE_OP(rdxILOP_ilt4, rdxInt32, <);
		NUMERIC_COMPARE_OP(rdxILOP_ine4, rdxInt32, !=);
		NUMERIC_COMPARE_OP(rdxILOP_ieq4, rdxInt32, ==);
				
		BINARY_ARITHMETIC_OP(rdxILOP_iadd8, rdxInt64, +);
		BINARY_ARITHMETIC_OP(rdxILOP_isub8, rdxInt64, -);
		BINARY_ARITHMETIC_OP(rdxILOP_imul8, rdxInt64, *);
		BINARY_ARITHMETIC_OP_ZERO_CHECK(rdxILOP_idiv8, rdxInt64, /);
		BINARY_ARITHMETIC_OP_ZERO_CHECK(rdxILOP_imod8, rdxInt64, %);
		UNARY_ARITHMETIC_OP(rdxILOP_ineg8, rdxInt64, -);

		NUMERIC_COMPARE_OP(rdxILOP_ige8, rdxInt64, >=);
		NUMERIC_COMPARE_OP(rdxILOP_igt8, rdxInt64, >);
		NUMERIC_COMPARE_OP(rdxILOP_ile8, rdxInt64, <=);
		NUMERIC_COMPARE_OP(rdxILOP_ilt8, rdxInt64, <);
		NUMERIC_COMPARE_OP(rdxILOP_ine8, rdxInt64, !=);
		NUMERIC_COMPARE_OP(rdxILOP_ieq8, rdxInt64, ==);

		MOVE_OP_GROUP(rdxILOP_move1, rdxSUnitCopier, rdxInt8);
		MOVE_OP_GROUP(rdxILOP_move2, rdxSUnitCopier, rdxInt16);
		MOVE_OP_GROUP(rdxILOP_move4, rdxSUnitCopier, rdxInt32);
		MOVE_OP_GROUP(rdxILOP_move8, rdxSUnitCopier, rdxInt64);
		MOVE_OP_GROUP(rdxILOP_moveflt, rdxSUnitCopier, rdxFloat);
		MOVE_OP_GROUP(rdxILOP_movedbl, rdxSUnitCopier, rdxDouble);
		MOVE_OP_GROUP(rdxILOP_movebool, rdxSUnitCopier, rdxBool);
		MOVE_OP_GROUP(rdxILOP_movep, rdxSUnitCopier, rdxBaseRef);
		MOVE_OP_GROUP(rdxILOP_movertp, rdxSUnitCopier, rdxSUntypedOffsetRef);
		MOVE_OP_GROUP(rdxILOP_movesz, rdxSBlockCopier, rdxUInt8);

		CONVERT_OP(rdxILOP_isx18, rdxInt8, rdxInt64);
		CONVERT_OP(rdxILOP_isx28, rdxInt16, rdxInt64);
		CONVERT_OP(rdxILOP_isx48, rdxInt32, rdxInt64);
		CONVERT_OP(rdxILOP_isx14, rdxInt8, rdxInt32);
		CONVERT_OP(rdxILOP_isx24, rdxInt16, rdxInt32);
		CONVERT_OP(rdxILOP_isx12, rdxInt8, rdxInt16);

		CONVERT_OP(rdxILOP_ict84, rdxInt64, rdxInt32);
		CONVERT_OP(rdxILOP_ict82, rdxInt64, rdxInt16);
		CONVERT_OP(rdxILOP_ict81, rdxInt64, rdxInt8);
		CONVERT_OP(rdxILOP_ict42, rdxInt32, rdxInt16);
		CONVERT_OP(rdxILOP_ict41, rdxInt32, rdxInt8);
		CONVERT_OP(rdxILOP_ict21, rdxInt16, rdxInt8);

		CONVERT_OP(rdxILOP_nconv8, rdxInt64, rdxInt64);
		CONVERT_OP(rdxILOP_nconv4, rdxInt32, rdxInt32);
		CONVERT_OP(rdxILOP_nconv2, rdxInt16, rdxInt16);
		CONVERT_OP(rdxILOP_nconv1, rdxInt8, rdxInt8);

		CONVERT_OP(rdxILOP_itof44, rdxInt32, rdxFloat32);
		CONVERT_OP(rdxILOP_itof48, rdxInt32, rdxFloat64);
		CONVERT_OP(rdxILOP_itof84, rdxInt64, rdxFloat32);
		CONVERT_OP(rdxILOP_itof88, rdxInt64, rdxFloat64);
			
		CONVERT_OP(rdxILOP_ftoi44, rdxFloat32, rdxInt32);
		CONVERT_OP(rdxILOP_ftoi48, rdxFloat32, rdxInt64);
		CONVERT_OP(rdxILOP_ftoi84, rdxFloat64, rdxInt32);
		CONVERT_OP(rdxILOP_ftoi88, rdxFloat64, rdxInt64);

		BRANCH_OP_CLUSTER(rdxILOP_jeq, true);
		BRANCH_OP_CLUSTER(rdxILOP_jne, false);

		default:
			{
				THROWEXCEPTIONDICT(rdxX_InvalidOperationException);
			}
			break;
		};
	}	// Main loop
}		// Func

int RDX_DECL_API rdxResumeThread(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxHandle<rdxCRuntimeThread> thread)
{
	int status = rdxRS_Active;

	// Cycle through code until the status turns into an exit code
	while(status > 0)
	{
		rdxHandle<rdxCMethod> method = thread->frame.method.ToHandle();

		if(method.IsNull())
			return rdxRS_Exited;

		if(method->m_native.precompiledCodeModule)
			status = rdxCPrecompiledCodeModule::ResumeThread(ctx, objm, thread);
		else
			status = rdxResumeThreadInterpreted(ctx, objm, thread);

		if(status == rdxRS_AbandonFrame)
		{
			// Code signalled to abandon the frame, determine the type of frame being entered
			rdxSRuntimeStackFrame *rsf = thread->frame.bp;
			if(rsf->method.IsNotNull())
			{
				// Standard frame
				rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
				threadView->frame = *rsf;
				status = rdxRS_Active;
			}
			else
			{
				// Root-level call frame
				RuntimeStackFrameRoot *rframe = static_cast<RuntimeStackFrameReentrant *>(rsf);
				thread.QuietOpen()->insertionPoint = rframe->insertionPoint;
				if(rframe->aboveNative)
				{
					// Reentrant frame
					RuntimeStackFrameReentrant *ref = static_cast<RuntimeStackFrameReentrant *>(rframe);
					{
						rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
						threadView->activeNativeMethod = ref->nativeMethod.ToHandle();
						threadView->frame = ref->subNativeFrame;
					}
					return rdxRS_Exited;
				}
				else
				{
					// Absolute root frame
					{
						rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
						threadView->activeNativeMethod = rdxHandle<rdxCMethod>::Null();
						threadView->Reset();
					}
					return rdxRS_Exited;
				}
			}
		}
	}

	return status;
}

int rdxCInterpreterCodeProvider::RunMethod(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxHandle<rdxCMethod> m, rdxHandle<rdxCRuntimeThread> thread, rdxURuntimeStackValue *prv) const
{
	rdxHandle<rdxCMethod> viaMethod = thread->activeNativeMethod;
	int status;

	if(m->m_native.isNativeCall)
	{
		rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
		threadView->activeNativeMethod = m;
		status = m->m_native.nativeCall(ctx, objm, m, thread, prv);
	}
	else
	{
		const void *stackBottom = thread->stackBytes + thread->stackCapacity;
		bool methodEntered;
		{
			rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
			methodEntered = rdxEnterMethodRoot(objm, m.ToRef(), &threadView->precallFrame, threadView->precallFrame.bp, threadView->stackBytes, prv, &threadView->frame, threadView->insertionPoint, (threadView->insertionPoint != stackBottom), viaMethod.ToRef(), &threadView->frame);
		}
		if(!methodEntered)
		{
			rdxRef<rdxCException> stackOverflowException = objm->GetBuiltIns()->providerDictionary.ToHandle()[rdxX_StackOverflowException].StaticCast<rdxCException>();

			{
				rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
				threadView->ex = stackOverflowException.ToHandle();
			}
			return rdxRS_Exception;
		}
		
		{
			rdxCDataView<rdxHandle<rdxCRuntimeThread> > threadView(thread);
			threadView->activeNativeMethod = rdxHandle<rdxCMethod>::Null();
		}
		status = rdxResumeThread(ctx, objm, thread);
	}
	return status;
}

rdxLargeInt RDX_DECL_API rdxIPToCurrentInstruction(rdxHandle<rdxCMethod> method, const void *ip)
{
	if(method->m_native.precompiledCodeModule)
		return rdxCPrecompiledCodeModule::IPToCurrentInstruction(method, ip);

	return static_cast<const rdxSILInstruction*>(ip) - static_cast<const rdxSILInstruction*>(method->m_native.optimizedInstructions) - 1;
}

const void *RDX_DECL_API rdxInstrNumToIP(rdxSOperationContext *ctx, rdxHandle<rdxCMethod> method, rdxLargeInt instrNum, bool *resumeAllowed)
{
	if(method->m_native.precompiledCodeModule)
		return rdxCPrecompiledCodeModule::InstrNumToIP(ctx, method, instrNum, resumeAllowed);

	const rdxSILInstruction *ili = static_cast<const rdxSILInstruction*>(method->m_native.optimizedInstructions) + instrNum;
	if(resumeAllowed)
	{
		bool flag = ((method->m_native.ilResumeFlags[instrNum/8] & (1 << (instrNum & 0x7))) != 0);
		*resumeAllowed = flag;
	}
	return ili;
}

void rdxCInterpreterCodeProvider::CreateExecutable(rdxIObjectManager *objm, rdxSOperationContext *ctx, rdxHandle<rdxCMethod> m) const
{
	RDX_TRY(ctx)
	{
		for(rdxLargeInt i=0;i<m_numModules;i++)
		{
			if(m_modules[i] == NULL)
				continue;

			const void *ip = m_modules[i]->IPForMethod(m.DirectCast<rdxCMethod>());
			if(ip != NULL)
			{
				{
					rdxCDataView<rdxHandle<rdxCMethod> > mView(m);
					mView->m_native.optimizedInstructions = const_cast<void*>(ip);
					mView->m_native.precompiledCodeModule = m_modules[i];
				}

				rdxHandle<rdxCString> str = m.ObjectInfo()->gstSymbol;
				if(str.IsNotNull())
				{
					const rdxSPrecompiledFunctionInfo *funcInfo = m_modules[i]->GetFunctionInfo(str);
					if(funcInfo != NULL)
					{
						// If this is precompiled, we can discard the arguments as well
						rdxCDataView<rdxHandle<rdxCMethod> > mView(m);
						mView->m_native.compactArgs = rdxHandle<rdxUILOpCompactArg>::Null();
						mView->m_native.largeArgs = rdxHandle<rdxUILOpLargeArg>::Null();
					}
				}

				m.QuietOpen()->m_native.ilinstructions = rdxHandle<rdxSILInstruction>::Null();
				return;
			}
		}

		// Convert opcodes to faster ones
		rdxHandle<rdxSILInstruction> originalInstructions = m->m_native.ilinstructions.DirectCast<rdxSILInstruction>();
		rdxHandle<rdxChar> methodName = m.ObjectInfo()->gstSymbol->AsChars();
		rdxLargeInt numInstructions = originalInstructions.ObjectInfo()->numElements;

		rdxSILInstruction *optimizedInstr = objm->GetAllocator()->CAlloc<rdxSILInstruction>(numInstructions, rdxALLOC_ExecutableInstructions);
		if(!optimizedInstr)
		{
			RDX_PROTECT(ctx, objm->CollectGarbage(ctx));
			optimizedInstr = objm->GetAllocator()->CAlloc<rdxSILInstruction>(numInstructions, rdxALLOC_ExecutableInstructions);
			if(!optimizedInstr)
				RDX_STHROW(ctx, RDX_ERROR_ALLOCATION_FAILED);
		}

		m.QuietOpen()->m_native.optimizedInstructions = optimizedInstr;

		const rdxUILOpCompactArg *argCAbase = m->m_native.compactArgs.Data();
		const rdxUILOpLargeArg *argLAbase = m->m_native.largeArgs.Data();
		for(rdxLargeInt i=0;i<numInstructions;i++)
		{
			rdxSILInstruction *instr = optimizedInstr + i;
			*instr = originalInstructions[i];
			const rdxUILOpCompactArg *argCA = argCAbase + instr->argOffs;
			const rdxUILOpLargeArg *argLA = argLAbase + instr->argOffs;

			if(instr->opcode >= rdxILOP_intrinsic)
			{
				bool matched = false;
				rdxLargeInt numSingle = sizeof(RDX_SINGLE_ARG_INTRINSICS) / sizeof(RDX_SINGLE_ARG_INTRINSICS[0]);
				rdxLargeInt numDual = sizeof(RDX_DUAL_ARG_INTRINSICS) / sizeof(RDX_DUAL_ARG_INTRINSICS[0]);

				// Convert to an optimal op
				for(rdxLargeInt intri=0;intri<numSingle;intri++)
				{
					const rdxSSingleArgIntrinsic *intr = RDX_SINGLE_ARG_INTRINSICS + intri;
					if(instr->opcode == intr->originalOp && argCA[2].li == intr->p2check)
					{
						instr->opcode = intr->newOp;
						matched = true;
						break;
					}
				}

				for(rdxLargeInt intri=0;intri<numDual;intri++)
				{
					const rdxSDualArgIntrinsic *intr = RDX_DUAL_ARG_INTRINSICS + intri;
					if(instr->opcode == intr->originalOp &&
						argCA[2].li == intr->p2check &&
						argCA[3].li == intr->p3check)
					{
						instr->opcode = intr->newOp;
						matched = true;
						break;
					}
				}
			}
			else if(instr->opcode == rdxILOP_iteratearray)
			{
				if(argCA[4].li == 8 && (argCA[5].li & 7) == 0)
					instr->opcode = rdxILOP_iteratearray8;
				else if(argCA[4].li == 4 && (argCA[5].li & 3) == 0)
					instr->opcode = rdxILOP_iteratearray4;
			}
			else if(instr->opcode == rdxILOP_iteratearraysub)
			{
				if(argCA[4].li == 8 && (argCA[5].li & 7) == 0)
					instr->opcode = rdxILOP_iteratearraysub8;
				else if(argCA[4].li == 4 && (argCA[5].li & 3) == 0)
					instr->opcode = rdxILOP_iteratearraysub4;
			}
			else if(instr->opcode == rdxILOP_move)
			{
				rdxLargeInt translatedOp = rdxILOP_unused;

				switch(argCA[3].at)
				{
				case rdxALIASINGTYPE_Int8:
					translatedOp = rdxILOP_move1;
					break;
				case rdxALIASINGTYPE_Int16:
					translatedOp = rdxILOP_move2;
					break;
				case rdxALIASINGTYPE_Int32:
					translatedOp = rdxILOP_move4;
					break;
				case rdxALIASINGTYPE_Int64:
					translatedOp = rdxILOP_move8;
					break;
				case rdxALIASINGTYPE_Reference:
					translatedOp = rdxILOP_movep;
					break;
				case rdxALIASINGTYPE_Double:
					translatedOp = rdxILOP_movedbl;
					break;
				case rdxALIASINGTYPE_Float:
					translatedOp = rdxILOP_moveflt;
					break;
				case rdxALIASINGTYPE_Bool:
					translatedOp = rdxILOP_movebool;
					break;
				case rdxALIASINGTYPE_RuntimePointer:
					translatedOp = rdxILOP_movertp;
					break;
				case rdxALIASINGTYPE_Varying:
					translatedOp = rdxILOP_movetrtp;
					break;
				case rdxALIASINGTYPE_Block:
					translatedOp = rdxILOP_movesz;
					break;
				default:
					RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
					break;
				}

				if(translatedOp != rdxILOP_unused)
				{
					// Determine offsetting
					if(argCA[4].li & RDX_ILOP_MOVE_SRC_PARENT_FRAME)
						translatedOp += MOVE_SRC_MODE_PARENT;
					else if(argCA[4].li & RDX_ILOP_MOVE_SRC_TYPE_DEFAULT)
						translatedOp += MOVE_SRC_MODE_ABSOLUTE;

					if(argCA[4].li & RDX_ILOP_MOVE_SRC_DEREF)
						translatedOp += MOVE_SRC_DEREF_DEREF * (MOVE_SRC_MODE_COUNT);

					if(argCA[4].li & RDX_ILOP_MOVE_DEST_PARENT_FRAME)
						translatedOp += MOVE_DEST_MODE_PARENT * (MOVE_SRC_MODE_COUNT * MOVE_SRC_DEREF_COUNT);

					if(argCA[4].li & RDX_ILOP_MOVE_DEST_DEREF)
					{
						if(argCA[4].li & RDX_ILOP_MOVE_SRC_IS_REF)
							translatedOp += MOVE_DEST_DEREF_DEREF_SRC_IS_REF * (MOVE_SRC_MODE_COUNT * MOVE_SRC_DEREF_COUNT * MOVE_DEST_MODE_COUNT);
						else if(argCA[4].li & RDX_ILOP_MOVE_SRC_CONTAINS_REFS)
							translatedOp += MOVE_DEST_DEREF_DEREF_SRC_HAS_REFS * (MOVE_SRC_MODE_COUNT * MOVE_SRC_DEREF_COUNT * MOVE_DEST_MODE_COUNT);
						else
							translatedOp += MOVE_DEST_DEREF_DEREF * (MOVE_SRC_MODE_COUNT * MOVE_SRC_DEREF_COUNT * MOVE_DEST_MODE_COUNT);
					}

					instr->opcode = static_cast<rdxEILOpcode>(translatedOp);
				}
			}
			else if(instr->opcode == rdxILOP_immediate)
			{
				switch(argLA[1].ca[1].at)
				{
				case rdxALIASINGTYPE_Int64: instr->opcode = rdxILOP_immediate_i64; break;
				case rdxALIASINGTYPE_Int32: instr->opcode = rdxILOP_immediate_i32; break;
				case rdxALIASINGTYPE_Int16: instr->opcode = rdxILOP_immediate_i16; break;
				case rdxALIASINGTYPE_Int8: instr->opcode = rdxILOP_immediate_i8; break;
				case rdxALIASINGTYPE_Bool: instr->opcode = rdxILOP_immediate_bool; break;

				//case rdxALIASINGTYPE_Reference: instr->opcode = rdxILOP_immediate_ptr; break;
				case rdxALIASINGTYPE_Double: instr->opcode = rdxILOP_immediate_dbl; break;
				case rdxALIASINGTYPE_Float: instr->opcode = rdxILOP_immediate_flt; break;
				}
			}
			else if(instr->opcode == rdxILOP_zero)
			{
				switch(argCA[1].lui)
				{
					
				case rdxALIASINGTYPE_Int64: instr->opcode = rdxILOP_zero_i64; break;
				case rdxALIASINGTYPE_Int32: instr->opcode = rdxILOP_zero_i32; break;
				case rdxALIASINGTYPE_Int16: instr->opcode = rdxILOP_zero_i16; break;
				case rdxALIASINGTYPE_Int8: instr->opcode = rdxILOP_zero_i8; break;
				case rdxALIASINGTYPE_Bool: instr->opcode = rdxILOP_zero_bool; break;

				//case rdxALIASINGTYPE_Reference: instr->opcode = rdxILOP_zero_ptr; break;
				case rdxALIASINGTYPE_Double: instr->opcode = rdxILOP_zero_dbl; break;
				case rdxALIASINGTYPE_Float: instr->opcode = rdxILOP_zero_flt; break;
				}
			}
			else if(instr->opcode == rdxILOP_jeq_f || instr->opcode == rdxILOP_jeq_p || instr->opcode == rdxILOP_jne_f || instr->opcode == rdxILOP_jne_p)
			{
				int translation = -1;
				switch(argCA[3].at)
				{
				case rdxALIASINGTYPE_Int8: translation = 0; break;
				case rdxALIASINGTYPE_Int16: translation = 1; break;
				case rdxALIASINGTYPE_Int32: translation = 2; break;
				case rdxALIASINGTYPE_Int64: translation = 3; break;
				case rdxALIASINGTYPE_Float: translation = 4; break;
				case rdxALIASINGTYPE_Double: translation = 5; break;
				case rdxALIASINGTYPE_Reference: translation = 6; break;
				case rdxALIASINGTYPE_Bool: translation = 7; break;
				};

				if(translation != -1)
				{
					switch(instr->opcode)
					{
					case rdxILOP_jeq_f: instr->opcode = static_cast<rdxEILOpcode>(rdxILOP_jeq_f_i1 + translation); break;
					case rdxILOP_jeq_p: instr->opcode = static_cast<rdxEILOpcode>(rdxILOP_jeq_p_i1 + translation); break;
					case rdxILOP_jne_f: instr->opcode = static_cast<rdxEILOpcode>(rdxILOP_jne_f_i1 + translation); break;
					case rdxILOP_jne_p: instr->opcode = static_cast<rdxEILOpcode>(rdxILOP_jne_p_i1 + translation); break;
					}
				}
			}
		}
		// Done

		// Throw out the old instruction data
		m.QuietOpen()->m_native.ilinstructions = rdxHandle<rdxSILInstruction>::Null();
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxCInterpreterCodeProvider::ReleaseExecutable(rdxIObjectManager *objm, rdxHandle<rdxCMethod> m) const
{
	if(!m->m_native.precompiledCodeModule)
	{
		objm->GetAllocator()->Free(m.QuietOpen()->m_native.optimizedInstructions);
		m.QuietOpen()->m_native.optimizedInstructions = NULL;
	}
	// Interpreter uses the IL instructions list, which is garbage collected
}

#define INIT_DICT(symbol, dictEntry)	\
	RDX_PROTECT_ASSIGN(ctx, str, objm->CreateStringASCII(ctx, symbol) );\
	{\
		rdxCDataView<rdxHandle<rdxRef<void> > > dictView(dict);\
		RDX_PROTECT_ASSIGN(ctx, dictView[dictEntry], objm->LookupSymbolSimple(ctx, str.ToHandle()).ToHandle().ToRef() );\
	}

void rdxCInterpreterCodeProvider::InitializeSymbolDictionary(rdxSOperationContext *ctx, rdxIObjectManager *objm) const
{
	RDX_TRY(ctx)
	{
		rdxHandle<rdxRef<void> > dict = objm->GetBuiltIns()->providerDictionary.ToHandle();

		if(dict.IsNull())
		{
			rdxCRef<rdxCString> str;
						
			objm->GetBuiltIns()->providerDictionaryTracedSymbolCount = rdxCPD_NumDictionaryTracedObjects;

			RDX_PROTECT_ASSIGN(ctx, objm->GetBuiltIns()->providerDictionary, objm->Create1DArray<rdxRef<void> >(ctx, rdxCPD_NumDictionaryValues) );
			objm->GetBuiltIns()->providerDictionaryTracedSymbolCount = rdxCPD_NumDictionaryTracedObjects;
			dict = objm->GetBuiltIns()->providerDictionary.ToHandle();

			INIT_DICT("Core.RDX.NullReferenceException.instance", rdxX_NullReferenceException);
			INIT_DICT("Core.RDX.IndexOutOfBoundsException.instance", rdxX_IndexOutOfBoundsException);
			INIT_DICT("Core.RDX.AllocationFailureException.instance", rdxX_AllocationFailureException);
			INIT_DICT("Core.RDX.IncompatibleConversionException.instance", rdxX_IncompatibleConversionException);
			INIT_DICT("Core.RDX.InvalidOperationException.instance", rdxX_InvalidOperationException);
			INIT_DICT("Core.RDX.UnspecifiedException.instance", rdxX_UnspecifiedException);
			INIT_DICT("Core.RDX.StackOverflowException.instance", rdxX_StackOverflowException);
			INIT_DICT("Core.RDX.DivideByZeroException.instance", rdxX_DivideByZeroException);
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}


rdxIPToCurrentInstructionCallback rdxCInterpreterCodeProvider::GetIPToCurrentInstructionCallback() const
{
	return rdxIPToCurrentInstruction;
}

rdxInstrNumToIPCallback rdxCInterpreterCodeProvider::GetInstrNumToIPCallback() const
{
	return rdxInstrNumToIP;
}

rdxResumeThreadCallback rdxCInterpreterCodeProvider::GetResumeThreadCallback(rdxIObjectManager *objm) const
{
	return rdxResumeThread;
}

void rdxCInterpreterCodeProvider::Shutdown()
{
	m_alloc.Free(this);
}

#endif
