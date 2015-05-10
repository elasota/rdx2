#include "rdx_pccm_env.hpp"
#include "rdx_builtins.hpp"

void rdxPCCMCallEnvironmentInternal::EnterMethod(const rdxCMethod *method, void *prvBase, void *frameBase, rdxLargeUInt callingInstr)
{
	// TODO MUSTFIX: Null checks on method and bytecode?
	rdxSRuntimeStackFrame currentFrame;
	currentFrame.bp = reinterpret_cast<rdxSRuntimeStackFrame*>(m_bp);
	currentFrame.ip = static_cast<const rdxUInt8 *>(RDX_CNULL) + callingInstr + 1;
	currentFrame.method = rdxWeakRTRef(rdxCMethod)(rdxObjRef_CSignal_DataPointer, const_cast<rdxCMethod*>(m_method));
	currentFrame.prv = reinterpret_cast<rdxURuntimeStackValue*>(m_prv);

	rdxNativeCallback cb = method->m_native.nativeCall;

	if(!method->m_native.isNativeCall)
	{
		if(method->bytecode.IsNull())
			rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
			//THROWEXCEPTIONDICT(rdxX_NullReferenceException);

		// Re-enter
		rdxSRuntimeStackFrame newFrame;
		if(!rdxEnterMethodInline(method->ObjectInfo()->ownerObjectManager, rdxWeakRTRef(rdxCMethod)(rdxObjRef_CSignal_DataPointer, const_cast<rdxCMethod*>(method)), &currentFrame, frameBase, m_thread->stackBytes,
			reinterpret_cast<rdxURuntimeStackValue*>(prvBase), &newFrame))
		{
			rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
			//THROWEXCEPTIONDICT(rdxX_StackOverflowException);
		}

		if(newFrame.method->m_native.pccmCallback)
		{
			m_thread->frame = newFrame;
			m_returnStatus = rdxRS_Active;
			return;
		}

		m_bp = newFrame.bp;
		m_prv = newFrame.prv;
		m_resumeInstr = 0;
		m_method = newFrame.method.Data();
	}
	else
	{
		if(cb == RDX_CNULL)
			rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
			//THROWEXCEPTIONDICT(rdxX_NullReferenceException);

		m_thread->frame = currentFrame;
		m_thread->insertionPoint = frameBase;
		m_thread->activeNativeMethod = rdxWeakRTRef(rdxCMethod)(rdxObjRef_CSignal_DataPointer, const_cast<rdxCMethod*>(method));

		rdxWeakHdl(rdxCMethod) methodHdl(rdxObjRef_CSignal_DataPointer, const_cast<rdxCMethod*>(method));
		rdxWeakHdl(rdxCRuntimeThread) threadHdl(rdxObjRef_CSignal_DataPointer, const_cast<rdxCRuntimeThread*>(m_thread));
		int status = cb(m_ctx, method->ObjectInfo()->ownerObjectManager, methodHdl, threadHdl, prvBase);
		m_thread->activeNativeMethod = rdxWeakRTRef(rdxCMethod)::Null();

		if(status <= 0)
		{
			m_thread->frame = currentFrame;
			m_returnStatus = static_cast<rdxERuntimeState>(status);
			return;
		}
	}
}

void rdxPCCMCallEnvironmentInternal::SwitchScanObjRef(const rdxCObject *objRef, rdxLargeUInt casesResIndex, rdxLargeUInt numCases, rdxLargeUInt callingInstr)
{
	rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
}

bool rdxPCCMCallEnvironmentInternal::CheckInherits(const rdxCObject *obj, rdxLargeUInt typeResIndex) const
{
	if(obj == RDX_CNULL)
		return true;

	const rdxCStructuredType *baseClassST = static_cast<const rdxCStructuredType*>(this->GetRes(typeResIndex));
	const rdxGCInfo *objGCI = obj->ObjectInfo();

	rdxWeakRTRef(rdxCType) containerType = objGCI->containerType.ToWeakRTRef();
	rdxWeakRTRef(rdxCType) containerTypeType = containerType->ObjectInfo()->containerType.ToWeakRTRef();
	if(containerType.IsNull())
		return false;
	const rdxSBuiltIns *builtIns = objGCI->ownerObjectManager->GetBuiltIns();

	rdxWeakRTRef(rdxCStructuredType) searchST;
	searchST = rdxWeakRTRef(rdxCStructuredType)::Null();
	if(containerTypeType == builtIns->st_ArrayOfType)
	{
		searchST = builtIns->st_Array.ToWeakRTRef();
	}
	else if(containerTypeType == builtIns->st_StructuredType)
	{
		rdxWeakRTRef(rdxCStructuredType) st = containerType.StaticCast<rdxCStructuredType>();
		if(st->storageSpecifier == rdxSS_Class)
			searchST = st;
	}

	while(searchST.IsNotNull())
	{
		if(searchST.Data() == baseClassST)
			return true;
		searchST = searchST->parentClass.ToWeakRTRef();
	}

	return false;
}

bool rdxPCCMCallEnvironmentInternal::CheckEnum(const rdxSRuntimeEnum &value, rdxLargeUInt typeResIndex) const
{
	rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
	return false;
}

bool rdxPCCMCallEnvironmentInternal::CheckEnum(RDX_ENUM_TYPE value, rdxLargeUInt typeResIndex) const
{
	rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
	return false;
}

void rdxPCCMCallEnvironmentInternal::SetStandardException(rdxECommonExceptions ex, rdxLargeUInt instr)
{
	rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
}

void rdxPCCMCallEnvironmentInternal::SetException(rdxCException *ex, rdxLargeUInt instr)
{
	rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
}

const rdxCMethod *rdxPCCMCallEnvironmentInternal::GetInterfaceCallMethod(void *stackLoc, rdxLargeUInt vftIndex)
{
	rdxTracedIRef(rdxSObjectInterfaceImplementation) *irefPtr = static_cast<rdxTracedIRef(rdxSObjectInterfaceImplementation)*>(stackLoc);
	rdxLargeUInt vftOffset = (*irefPtr)->vftOffset;
	rdxTracedRTRef(rdxCObject) *rtRefPtr = static_cast<rdxTracedRTRef(rdxCObject)*>(stackLoc);

	*rtRefPtr = rdxWeakRTRef(rdxCObject)(rdxObjRef_CSignal_BaseRef, *irefPtr);
	return (*rtRefPtr)->ObjectInfo()->containerType.StaticCast<rdxCStructuredType>()->virtualMethods->Element(vftOffset + vftIndex).Data();
}

rdxCObject *rdxPCCMCallEnvironmentInternal::GetStackRoot() const
{
	// TODO: This might not be the best thing to return as a stack root
	return RDX_CNULL;
}

bool rdxPCCMCallEnvironmentInternal::CreateInstanceFromRes(rdxCObject **objRef, rdxLargeUInt typeResIndex, const rdxLargeUInt *dimensions, rdxLargeUInt numDimensions, rdxLargeUInt instrNum)
{
	// Save IP since the allocation can block
	rdxCRuntimeThread* thread = this->m_thread;
	thread->frame.ip = reinterpret_cast<rdxUInt8*>(RDX_CNULL) + instrNum;
	
	rdxCRef(rdxCObject) o = rdxNewObjectInstance(this->m_ctx, this->m_method->ObjectInfo()->ownerObjectManager, m_method->resArgs->Element(typeResIndex).ToWeakHdl().StaticCast<rdxCType>(), dimensions, sizeof(dimensions[0]), numDimensions);

	if(o.IsNull())
	{
		this->SetStandardException(rdxX_AllocationFailureException, instrNum);
		return false;
	}
	*objRef = o.Modify();
	return true;
}

const rdxCMethod *rdxPCCMCallEnvironmentInternal::GetVirtualMethod(const rdxCObject *vreg, rdxLargeUInt methodResIndex) const
{
	// TODO: Slow
	rdxWeakRTRef(rdxCMethod) resolvedMethod = vreg->ObjectInfo()->VFT()->Element(methodResIndex).ToWeakRTRef();
	return resolvedMethod.Data();
}

bool rdxPCCMCallEnvironmentInternal::CheckImplements(const rdxCObject *vreg, rdxLargeUInt interfaceResIndex) const
{
	rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);

	if(vreg == RDX_CNULL)
		return false;

	const rdxCStructuredType *checkInterfaceST = static_cast<const rdxCStructuredType*>(this->GetRes(interfaceResIndex));
	const rdxGCInfo *objGCI = vreg->ObjectInfo();

	rdxWeakRTRef(rdxCType) containerType = objGCI->containerType.ToWeakRTRef();
	if(containerType.IsNull())
		return false;
	const rdxSBuiltIns *builtIns = objGCI->ownerObjectManager->GetBuiltIns();
	if(containerType->ObjectInfo()->containerType == builtIns->st_StructuredType)
	{
		rdxWeakRTRef(rdxCStructuredType) st = containerType.StaticCast<rdxCStructuredType>();
		const rdxCArray<rdxSInterfaceImplementation> *interfaces = st->interfaces.Data();
		if(interfaces == RDX_CNULL)
			return false;
		rdxLargeUInt nInterfaces = interfaces->NumElements();
		for(rdxLargeUInt i=0;i<nInterfaces;i++)
		{
			const rdxSInterfaceImplementation &ii = interfaces->Element(i);
			if(ii.type.Data() == checkInterfaceST)
				return true;
		}
	}

	return false;
}

void rdxPCCMCallEnvironmentInternal::ExitFunction()
{
	rdxSRuntimeStackFrame *rfp = reinterpret_cast<rdxSRuntimeStackFrame *>(m_bp);
	if(rfp->method.IsNull() || rfp->method->m_native.precompiledCodeModule)
	{
		m_thread->frame.bp = rfp;
		m_returnStatus = rdxRS_AbandonFrame;
		return;
	}

	m_thread->frame = *rfp;
}

bool rdxPCCMCallEnvironmentInternal::CheckConversion(const rdxCObject *objRef, const rdxCStructuredType *st) const
{
	rdxDebugBreak(rdxBREAKCAUSE_Unimplemented);
	return false;
}

void rdxPCCMCallEnvironmentInternal::SelectCase(rdxLargeUInt caseNo, rdxLargeUInt callingInstr)
{
	m_resumeInstr = callingInstr + caseNo + 1;
}

rdxWeakIRef(rdxSObjectInterfaceImplementation) rdxPCCMCallEnvironmentInternal::RCast_OToI(rdxCObject *src, rdxLargeUInt targetType, rdxLargeUInt instrNum, bool &succeeded)
{
	if(src == RDX_CNULL)
	{
		succeeded = true;
		return rdxWeakIRef(rdxSObjectInterfaceImplementation)::Null();
	}
	rdxWeakIRef(rdxSObjectInterfaceImplementation) ifc =
		src->ObjectInfo()->ownerObjectManager->FindInterface(rdxWeakRTRef(rdxCObject)(rdxObjRef_CSignal_DataPointer, src),
		m_method->resArgs->Element(targetType).StaticCast<rdxCStructuredType>());
	if(ifc.IsNull())
	{
		this->SetStandardException(rdxX_IncompatibleConversionException, instrNum);
		succeeded = false;
		return rdxWeakIRef(rdxSObjectInterfaceImplementation)::Null();
	}
	succeeded = true;
	return ifc;
}

rdxWeakIRef(rdxSObjectInterfaceImplementation) rdxPCCMCallEnvironmentInternal::RCast_IToI(rdxSObjectInterfaceImplementation *src, rdxLargeUInt targetType, rdxLargeUInt instrNum, bool &succeeded)
{
	if(src == RDX_CNULL)
	{
		succeeded = true;
		return rdxWeakIRef(rdxSObjectInterfaceImplementation)::Null();
	}
	return RCast_OToI(src->GetImplementingObject(), targetType, instrNum, succeeded);
}

rdxWeakRTRef(rdxCObject) rdxPCCMCallEnvironmentInternal::RCast_IToO(rdxSObjectInterfaceImplementation *src, rdxLargeUInt targetType, rdxLargeUInt instrNum, bool &succeeded)
{
	if(src == RDX_CNULL)
	{
		succeeded = true;
		return rdxWeakRTRef(rdxCObject)::Null();
	}
	rdxCObject *srcObj = src->GetImplementingObject();
	rdxWeakRTRef(rdxCObject) srcRTRef(rdxObjRef_CSignal_DataPointer, srcObj);
	if(srcObj->ObjectInfo()->ownerObjectManager->ObjectCompatible(srcRTRef, m_method->resArgs->Element(targetType).StaticCast<rdxCType>()))
	{
		succeeded = true;
		return srcRTRef;
	}
	else
	{
		this->SetStandardException(rdxX_IncompatibleConversionException, instrNum);
		succeeded = false;
		return rdxWeakRTRef(rdxCObject)::Null();
	}
}
