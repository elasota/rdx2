#ifndef __RDX_PCCM_API_HPP__
#define __RDX_PCCM_API_HPP__

#pragma warning(disable:4102)

#include "rdx_coretypes.hpp"
#include "rdx_runtimestackvalue.hpp"
#include "rdx_runtime.hpp"

struct rdxSRuntimeEnum;
struct rdxPCCMCallEnvironment;
class rdxCMethod;
class rdxCRuntimeThread;
struct rdxSOperationContext;

struct rdxPCCMCallEnvironment
{
	void *GetBP() const;
	void *GetPRV() const;
	rdxCObject *GetRes(rdxLargeUInt resIndex) const;
	const rdxCMethod *GetMethodRes(rdxLargeUInt resIndex) const;
	rdxWeakRTRef(rdxCObject) ImmediateResObj(rdxLargeUInt resIndex);
	rdxLargeUInt GetResumeInstruction() const;

	virtual void EnterMethod(const rdxCMethod *method, void *prvBase, void *stackBase, rdxLargeUInt callingInstr) = 0;
	virtual void SwitchScanObjRef(const rdxCObject *objRef, rdxLargeUInt casesResIndex, rdxLargeUInt numCases, rdxLargeUInt callingInstr) = 0;
	virtual bool CheckInherits(const rdxCObject *obj, rdxLargeUInt typeResIndex) const = 0;
	virtual bool CheckEnum(const rdxSRuntimeEnum &value, rdxLargeUInt typeResIndex) const = 0;
	virtual bool CheckEnum(RDX_ENUM_TYPE value, rdxLargeUInt typeResIndex) const = 0;
	virtual void SetStandardException(rdxECommonExceptions ex, rdxLargeUInt instr) = 0;
	virtual void SetException(rdxCException *ex, rdxLargeUInt instr) = 0;
	virtual const rdxCMethod *GetInterfaceCallMethod(void *stackLoc, rdxLargeUInt vstIndex) = 0;
	virtual rdxCObject *GetStackRoot() const = 0;
	virtual bool CreateInstanceFromRes(rdxCObject **objRef, rdxLargeUInt typeResIndex, const rdxLargeUInt *dimensions, rdxLargeUInt numDimensions, rdxLargeUInt instrNum) = 0;
	virtual const rdxCMethod *GetVirtualMethod(const rdxCObject *vreg, rdxLargeUInt methodResIndex) const = 0;
	virtual bool CheckImplements(const rdxCObject *vreg, rdxLargeUInt interfaceResIndex) const = 0;
	virtual void ExitFunction() = 0;
	virtual bool CheckConversion(const rdxCObject *objRef, const rdxCStructuredType *st) const = 0;
	virtual void SelectCase(rdxLargeUInt caseNo, rdxLargeUInt callingInstr) = 0;
	virtual rdxWeakIRef(rdxSObjectInterfaceImplementation) RCast_OToI(rdxCObject *src, rdxLargeUInt targetType, rdxLargeUInt instrNum, bool &succeeded) = 0;
	virtual rdxWeakIRef(rdxSObjectInterfaceImplementation) RCast_IToI(rdxSObjectInterfaceImplementation *src, rdxLargeUInt targetType, rdxLargeUInt instrNum, bool &succeeded) = 0;
	virtual rdxWeakRTRef(rdxCObject) RCast_IToO(rdxSObjectInterfaceImplementation *src, rdxLargeUInt targetType, rdxLargeUInt instrNum, bool &succeeded) = 0;

	template<class T>
	void SwitchScan(const T &obj, rdxLargeUInt casesResIndex, rdxLargeUInt numCases, rdxLargeUInt callingInstr)
	{
		rdxCArray<T> *casesArray = static_cast<rdxCArray<T>*>(GetRes(casesResIndex));
		rdxLargeUInt caseNo = 0;
		for( ;caseNo<numCases;caseNo++)
		{
			if(casesArray->Element(caseNo) == obj)
				break;
		}
		SelectCase(caseNo, callingInstr);
	}

protected:
	void *m_bp;
	void *m_prv;
	rdxLargeUInt m_resumeInstr;
	const rdxCMethod *m_method;
	rdxCRuntimeThread *m_thread;
	rdxSOperationContext *m_ctx;
	rdxERuntimeState m_returnStatus;
};

struct rdxPCCMUtils
{
	template<class TArray, class TElement>
	static rdxECommonExceptions IterateArray(const TArray *pArray, rdxLargeUInt *pIndex, TElement *pElement, bool &shouldExit);

	static RDX_FORCEINLINE bool JCCOp_jccp(rdxPCCMCallEnvironment &pccmEnvironment, const rdxWeakRTRef(rdxCObject) &objRef, const rdxWeakRTRef(rdxCStructuredType) &st)
	{
		return pccmEnvironment.CheckConversion(objRef.Data(), st.Data());
	}

	static RDX_FORCEINLINE bool JCCOp_jccf(rdxPCCMCallEnvironment &pccmEnvironment, const rdxWeakRTRef(rdxCObject) &objRef, const rdxWeakRTRef(rdxCStructuredType) &st)
	{
		return !pccmEnvironment.CheckConversion(objRef.Data(), st.Data());
	}

	static RDX_FORCEINLINE void SetImmediateFinal(rdxInt8 &dest, rdxInt8 src) { dest = src; }
	static RDX_FORCEINLINE void SetImmediateFinal(rdxInt16 &dest, rdxInt16 src) { dest = src; }
	static RDX_FORCEINLINE void SetImmediateFinal(rdxInt32 &dest, rdxInt32 src) { dest = src; }
	static RDX_FORCEINLINE void SetImmediateFinal(rdxInt64 &dest, rdxInt64 src) { dest = src; }
	static RDX_FORCEINLINE void SetImmediateFinal(rdxUInt8 &dest, rdxInt8 src) { dest = static_cast<rdxUInt8>(src); }
	static RDX_FORCEINLINE void SetImmediateFinal(rdxUInt16 &dest, rdxInt16 src) { dest = static_cast<rdxUInt16>(src); }
	static RDX_FORCEINLINE void SetImmediateFinal(rdxUInt32 &dest, rdxInt32 src) { dest = static_cast<rdxUInt32>(src); }
	static RDX_FORCEINLINE void SetImmediateFinal(rdxUInt64 &dest, rdxInt64 src) { dest = static_cast<rdxUInt64>(src); }
	static RDX_FORCEINLINE void SetImmediateFinal(rdxFloat32 &dest, rdxInt32 src) { union { rdxFloat32 f; rdxInt32 i; } u; u.i = src; dest = u.f; }
	static RDX_FORCEINLINE void SetImmediateFinal(rdxFloat64 &dest, rdxInt64 src) { union { rdxFloat64 f; rdxInt64 i; } u; u.i = src; dest = u.f; }
	static void SetImmediateFinal(rdxSRuntimeEnum &dest, rdxHugeInt src);

	static inline rdxECommonExceptions CheckedOp_iumod(::rdxLargeUInt &dest, ::rdxLargeUInt ls, ::rdxLargeUInt rs)
	{
		if(rs == 0)
			return rdxX_DivideByZeroException;
		dest = ls % rs;
		return rdxX_NoException;
	}

	static inline void MarkRTP(rdxWeakTypelessOffsetRTRef &offsetRTRef)
	{
#ifdef RDX_USE_INCREMENTAL_GC
		rdxGCInfo *markedGCI = rdxRefConverter<rdxGCInfo>::Convert(offsetRTRef.GetObjRef()->Modify());
		if(!(markedGCI->objectFlags & rdxGCInfo::GCOF_GCMarkOnAssign))
			markedGCI->ownerObjectManager->ExternalGCMarkObject(markedGCI);
#endif
	}

	template<class TOriginalClass, class TOriginalPropertyType, class TNewClass, class TNewPropertyType>
	static RDX_FORCEINLINE void ChangePtrProperty(rdxWeakTypelessOffsetRTRef &ref, TOriginalPropertyType TOriginalClass::* prop1, TNewPropertyType TNewClass::* prop2)
	{
		const rdxUInt8 *originalOffs = reinterpret_cast<const rdxUInt8*>(&(reinterpret_cast<const TOriginalClass*>(RDX_CNULL)->*prop1));
		const rdxUInt8 *newOffs = reinterpret_cast<const rdxUInt8*>(&(reinterpret_cast<const TNewClass*>(RDX_CNULL)->*prop2));
		ref.TranslateOffset(newOffs - originalOffs);
	}
};

namespace _RDX_CPPX
{
	struct PCCMGlue
	{
		template<rdxUInt64 domainGUID, rdxUInt64 methodGUID>
		static void Call(rdxPCCMCallEnvironment &pccmEnvironment);
	};
}

struct rdxSPCCMObjectIndex
{
	rdxUInt64 codedObjectGUID;
	rdxPCCMCallback callback;
};

struct rdxSPCCMDomainIndex
{
	rdxUInt64 codedDomainGUID;
	rdxLargeUInt objCount;
	const rdxSPCCMObjectIndex *objects;
};

struct rdxSRuntimeEnum
{
	friend struct ::_RDX_CPPX::PCCMGlue;
	friend struct ::rdxPCCMUtils;
protected:
	RDX_ENUM_TYPE m_value;

public:
	RDX_FORCEINLINE rdxSRuntimeEnum() {}
	RDX_FORCEINLINE rdxSRuntimeEnum(const rdxSRuntimeEnum &rs) : m_value(rs.m_value) { }
	RDX_FORCEINLINE rdxSRuntimeEnum &operator =(const rdxSRuntimeEnum &rs) { m_value = rs.m_value; return *this; }
	RDX_FORCEINLINE bool operator == (const rdxSRuntimeEnum &rs) { return m_value == rs.m_value; }
	RDX_FORCEINLINE bool operator != (const rdxSRuntimeEnum &rs) { return m_value != rs.m_value; }
	RDX_FORCEINLINE bool operator < (const rdxSRuntimeEnum &rs) { return m_value < rs.m_value; }
	RDX_FORCEINLINE bool operator > (const rdxSRuntimeEnum &rs) { return m_value > rs.m_value; }
	RDX_FORCEINLINE bool operator <= (const rdxSRuntimeEnum &rs) { return m_value <= rs.m_value; }
	RDX_FORCEINLINE bool operator >= (const rdxSRuntimeEnum &rs) { return m_value >= rs.m_value; }

	RDX_FORCEINLINE RDX_ENUM_TYPE Value() const { return m_value; }

protected:
	RDX_FORCEINLINE rdxSRuntimeEnum(RDX_ENUM_TYPE rs) : m_value(rs) { }
};

#define RDX_PCCM_VSL_CHAINOP(newVReg, baseVReg, valueType)	\
	newVReg = ((static_cast<int>(baseVReg) - static_cast<int>(sizeof(valueType))) & (-static_cast<int>(rdxALIGN_RuntimeStackValue)))

#define RDX_PCCM_VSL_CHAINLOCAL(newVReg, baseVReg, valueType)	\
	newVReg = ((static_cast<int>(baseVReg) - static_cast<int>(sizeof(valueType))) & (-static_cast<int>(rdxAlignOf(valueType))))

#define RDX_PCCM_PRVSTACK(vreg)	\
	(static_cast<void*>(prvBytes + (vreg)))

#define RDX_PCCM_BPSTACK(vreg)	\
	(static_cast<void*>(bpBytes + (vreg)))

template<rdxLargeUInt TSize>
struct rdxPCCMSignedIntResolver
{
};

template<>
struct rdxPCCMSignedIntResolver<1>
{
	typedef rdxInt8 IntType;
};

template<>
struct rdxPCCMSignedIntResolver<2>
{
	typedef rdxInt16 IntType;
};

template<>
struct rdxPCCMSignedIntResolver<4>
{
	typedef rdxInt32 IntType;
};

template<>
struct rdxPCCMSignedIntResolver<8>
{
	typedef rdxInt64 IntType;
};


template<class TDest, class TSrc>
RDX_FORCEINLINE void rdxPCCMSetImmediateTranslate(TDest &vreg, TSrc srcValue)
{
	// Sign extend and then pass
	typedef rdxPCCMSignedIntResolver<sizeof(TSrc)>::IntType SrcInt;
	typedef rdxPCCMSignedIntResolver<sizeof(TDest)>::IntType DestInt;
	DestInt di = static_cast<DestInt>(static_cast<SrcInt>(srcValue));
	rdxPCCMUtils::SetImmediateFinal(vreg, di);
}

template<class T>
RDX_FORCEINLINE void rdxPCCMSetImmediate(T &vreg, rdxUInt8 b0)
{
	rdxPCCMSetImmediateTranslate(vreg, b0);
}

template<class T>
RDX_FORCEINLINE void rdxPCCMSetImmediate(T &vreg, rdxUInt8 b0, rdxUInt8 b1)
{
	rdxUInt16 v = static_cast<rdxUInt16>(b0);
	v |= static_cast<rdxUInt16>(b1) << 8;
	rdxPCCMSetImmediateTranslate(vreg, v);
}

template<class T>
RDX_FORCEINLINE void rdxPCCMSetImmediate(T &vreg, rdxUInt8 b0, rdxUInt8 b1, rdxUInt8 b2, rdxUInt8 b3)
{
	rdxUInt32 v = static_cast<rdxUInt32>(b0);
	v |= static_cast<rdxUInt32>(b1) << 8;
	v |= static_cast<rdxUInt32>(b2) << 16;
	v |= static_cast<rdxUInt32>(b3) << 24;
	rdxPCCMSetImmediateTranslate(vreg, v);
}

template<class T>
RDX_FORCEINLINE void rdxPCCMSetImmediate(T &vreg, rdxUInt8 b0, rdxUInt8 b1, rdxUInt8 b2, rdxUInt8 b3, rdxUInt8 b4, rdxUInt8 b5, rdxUInt8 b6, rdxUInt8 b7)
{
	rdxUInt64 v = static_cast<rdxUInt64>(b0);
	v |= static_cast<rdxUInt64>(b1) << 8;
	v |= static_cast<rdxUInt64>(b2) << 16;
	v |= static_cast<rdxUInt64>(b3) << 24;
	v |= static_cast<rdxUInt64>(b4) << 32;
	v |= static_cast<rdxUInt64>(b5) << 40;
	v |= static_cast<rdxUInt64>(b6) << 48;
	v |= static_cast<rdxUInt64>(b7) << 56;
	rdxPCCMSetImmediateTranslate(vreg, v);
}

RDX_FORCEINLINE void rdxPCCMUtils::SetImmediateFinal(rdxSRuntimeEnum &dest, rdxHugeInt src)
{
	dest.m_value = static_cast<RDX_ENUM_TYPE>(src);
}

template<class TArray, class TElement>
inline rdxECommonExceptions rdxPCCMUtils::IterateArray(const TArray *pArray, rdxLargeUInt *pIndex, TElement *pElement, bool &shouldExit)
{
	if(pArray->IsNull())
		return rdxX_NullReferenceException;
	rdxLargeUInt numElements = (*pArray)->NumElements();
	rdxLargeUInt nextIndex = (*pIndex) + 1;	// TODO MUSTFIX: Adjust this for being unsigned
	if(nextIndex >= numElements)
	{
		shouldExit = true;
		return rdxX_NoException;
	}

	/*
	rdxLargeInt numDimensions;

	if(enableSubs)
	{
		numDimensions = argCA[7].li;
		if(numDimensions != arrayRef.ObjectInfo()->numDimensions)
			return objm->GetBuiltIns()->providerDictionary.ToHandle()[rdxX_IndexOutOfBoundsException].ToHandle().ToRef().DirectCast<rdxCException>();
	}
	*/

	// Nothing can fail now, commit changes
	// Save next index
	*pIndex = nextIndex;	// TODO MUSTFIX: Adjust this for being unsigned
	// Copy value
	*pElement = (*pArray)->Element(nextIndex);

#if 0
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
#endif
	shouldExit = false;

	return rdxX_NoException;
}

////////////////////////////////////////////////////////////
RDX_FORCEINLINE void *rdxPCCMCallEnvironment::GetBP() const
{
	return m_bp;
}

RDX_FORCEINLINE void *rdxPCCMCallEnvironment::GetPRV() const
{
	return m_prv;
}

RDX_FORCEINLINE rdxCObject *rdxPCCMCallEnvironment::GetRes(rdxLargeUInt resIndex) const
{
	return m_method->resArgs->Element(resIndex).Modify();
}

RDX_FORCEINLINE const rdxCMethod *rdxPCCMCallEnvironment::GetMethodRes(rdxLargeUInt resIndex) const
{
	return static_cast<rdxCMethod*>(m_method->resArgs->Element(resIndex).Modify());
}

RDX_FORCEINLINE rdxWeakRTRef(rdxCObject) rdxPCCMCallEnvironment::ImmediateResObj(rdxLargeUInt resIndex)
{
	return m_method->resArgs->Element(resIndex).ToWeakRTRef();
}

RDX_FORCEINLINE rdxLargeUInt rdxPCCMCallEnvironment::GetResumeInstruction() const
{
	return this->m_resumeInstr;
}

/*

	virtual void EnterMethod(const rdxCMethod *method, void *prvBase, void *stackBase, rdxLargeUInt callingInstr) = 0;
	virtual void SwitchScanObjRef(const rdxCObject *objRef, rdxLargeUInt casesResIndex, rdxLargeUInt numCases, rdxLargeUInt callingInstr) = 0;
	virtual bool CheckInherits(const rdxCObject *obj, rdxLargeUInt typeResIndex) const = 0;
	virtual bool CheckEnum(const rdxSRuntimeEnum &value, rdxLargeUInt typeResIndex) const = 0;
	virtual bool CheckEnum(RDX_ENUM_TYPE value, rdxLargeUInt typeResIndex) const = 0;
	virtual void SetStandardException(rdxECommonExceptions ex, rdxLargeUInt instr) = 0;
	virtual void SetException(rdxCException *ex, rdxLargeUInt instr) = 0;
	virtual const rdxCMethod *GetInterfaceCallMethod(void *stackLoc, rdxLargeUInt vstIndex) = 0;
	virtual rdxWeakRTRef(rdxCObject) ImmediateResObj(rdxLargeUInt resIndex) = 0;
	virtual rdxCObject *GetStackRoot() const = 0;
	virtual bool CreateInstanceFromRes(rdxCObject **objRef, rdxLargeUInt typeResIndex, const rdxLargeUInt *dimensions, rdxLargeUInt numDimensions, rdxLargeUInt instrNum) = 0;
	virtual const rdxCMethod *GetVirtualMethod(const rdxCObject *vreg, rdxLargeUInt methodResIndex) const = 0;
	virtual bool CheckImplements(const rdxCObject *vreg, rdxLargeUInt interfaceResIndex) const = 0;
	virtual void ExitFunction() = 0;
	virtual bool CheckConversion(const rdxCObject *objRef, const rdxCStructuredType *st) const = 0;
	virtual void SelectCase(rdxLargeUInt caseNo, rdxLargeUInt callingInstr) = 0;
	virtual rdxWeakIRef(rdxSObjectInterfaceImplementation) RCast_OToI(rdxCObject *src, rdxLargeUInt targetType, rdxLargeUInt instrNum, bool &succeeded) = 0;
	virtual rdxWeakIRef(rdxSObjectInterfaceImplementation) RCast_IToI(rdxSObjectInterfaceImplementation *src, rdxLargeUInt targetType, rdxLargeUInt instrNum, bool &succeeded) = 0;
	virtual rdxWeakRTRef(rdxCObject) RCast_IToO(rdxSObjectInterfaceImplementation *src, rdxLargeUInt targetType, rdxLargeUInt instrNum, bool &succeeded) = 0;
	*/

#endif
