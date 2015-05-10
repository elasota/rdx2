#ifndef __RDX_PCCM_ENV_HPP__
#define __RDX_PCCM_ENV_HPP__

#include "rdx_pccm_api.hpp"

class rdxCMethod;
class rdxCRuntimeThread;

struct rdxPCCMCallEnvironmentInternal : public rdxPCCMCallEnvironment
{
	virtual void EnterMethod(const rdxCMethod *method, void *prvBase, void *stackBase, rdxLargeUInt callingInstr);
	virtual void SwitchScanObjRef(const rdxCObject *objRef, rdxLargeUInt casesResIndex, rdxLargeUInt numCases, rdxLargeUInt callingInstr);
	virtual bool CheckInherits(const rdxCObject *obj, rdxLargeUInt typeResIndex) const;
	virtual bool CheckEnum(const rdxSRuntimeEnum &value, rdxLargeUInt typeResIndex) const;
	virtual bool CheckEnum(RDX_ENUM_TYPE value, rdxLargeUInt typeResIndex) const;
	virtual void SetStandardException(rdxECommonExceptions ex, rdxLargeUInt instr);
	virtual void SetException(rdxCException *ex, rdxLargeUInt instr);
	virtual const rdxCMethod *GetInterfaceCallMethod(void *stackLoc, rdxLargeUInt vftIndex);
	virtual rdxCObject *GetStackRoot() const;
	virtual bool CreateInstanceFromRes(rdxCObject **objRef, rdxLargeUInt typeResIndex, const rdxLargeUInt *dimensions, rdxLargeUInt numDimensions, rdxLargeUInt instrNum);
	virtual const rdxCMethod *GetVirtualMethod(const rdxCObject *vreg, rdxLargeUInt methodResIndex) const;
	virtual bool CheckImplements(const rdxCObject *vreg, rdxLargeUInt interfaceResIndex) const;
	virtual void ExitFunction();
	virtual bool CheckConversion(const rdxCObject *objRef, const rdxCStructuredType *st) const;
	virtual void SelectCase(rdxLargeUInt caseNo, rdxLargeUInt callingInstr);
	virtual rdxWeakIRef(rdxSObjectInterfaceImplementation) RCast_OToI(rdxCObject *src, rdxLargeUInt targetType, rdxLargeUInt instrNum, bool &succeeded);
	virtual rdxWeakIRef(rdxSObjectInterfaceImplementation) RCast_IToI(rdxSObjectInterfaceImplementation *src, rdxLargeUInt targetType, rdxLargeUInt instrNum, bool &succeeded);
	virtual rdxWeakRTRef(rdxCObject) RCast_IToO(rdxSObjectInterfaceImplementation *src, rdxLargeUInt targetType, rdxLargeUInt instrNum, bool &succeeded);

	void Setup(rdxSOperationContext *ctx, rdxCRuntimeThread *thread, const rdxCMethod *method, void *bp, void *prv, rdxLargeUInt resumeInstr);
	rdxERuntimeState GetReturnStatus() const;
};

inline void rdxPCCMCallEnvironmentInternal::Setup(rdxSOperationContext *ctx, rdxCRuntimeThread *thread, const rdxCMethod *method, void *bp, void *prv, rdxLargeUInt resumeInstr)
{
	m_ctx = ctx;
	m_thread = thread;
	m_method = method;
	m_bp = bp;
	m_prv = prv;
	m_resumeInstr = resumeInstr;
	m_returnStatus = rdxRS_Active;
}

inline rdxERuntimeState rdxPCCMCallEnvironmentInternal::GetReturnStatus() const
{
	return m_returnStatus;
}

#endif
