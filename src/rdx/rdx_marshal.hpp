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
#ifndef __RDX_MARSHAL_HPP__
#define __RDX_MARSHAL_HPP__

#include "rdx_coretypes.hpp"
#include "rdx_reftypedefs.hpp"
#include "rdx_reftypealiases.hpp"
#include "rdx_runtimestackvalue.hpp"

struct rdxSOperationContext;
struct rdxIObjectManager;
union rdxURuntimeStackValue;
class rdxCRuntimeThread;
class rdxCStackView;

struct rdxSExportedCallEnvironment
{
	rdxSOperationContext *ctx;
	rdxIObjectManager *objm;
	rdxCRuntimeThread *thread;
	int status;

	int Throw(rdxWeakHdl(rdxCException) ex);
	int Throw(const char *exDomain, const char *exSymbol);
};

enum rdxEStackViewType
{
	rdxSTACKVIEWTYPE_Parameter,
	rdxSTACKVIEWTYPE_ReturnValue,
};

class rdxSStackProxyBase
{
public:
	rdxSStackProxyBase *next;
	void *valueAddr;
};

template<class T>
class rdxCStackValueProxy : public rdxSStackProxyBase
{
public:
	rdxCStackValueProxy(rdxCStackView &stackView, rdxEStackViewType svt);

	const T &Value() const;
	T &Modify() const;
};

template<class T>
class rdxCStackObjectRefProxy : public rdxCStackValueProxy<typename rdxTracedRTRef(T)>
{
public:
	inline rdxCStackObjectRefProxy(rdxCStackView &stackView, rdxEStackViewType svt)
		: rdxCStackValueProxy<typename rdxTracedRTRef(T)>(stackView, svt)
	{
	}
};

template<class T>
class rdxCStackStructRefProxy : public rdxSStackProxyBase
{
public:
	rdxCStackStructRefProxy(rdxCStackView &stackView, rdxEStackViewType svt);

	typename rdxWeakOffsetRTRef(T) ToOffsetRTRef() const;
	const T &Value() const;
	T &Modify() const;
};

class rdxCStackView
{
public:
	virtual void PushProxy(rdxSStackProxyBase &proxy, rdxLargeUInt size, rdxEStackViewType stackViewType) = 0;
};

class rdxCStackViewCallback : public rdxCStackView
{
	void *m_parametersBase;
	void *m_returnValuesBase;

	rdxSStackProxyBase *m_lastReturnValue;

public:
	explicit rdxCStackViewCallback(void *prv);
	void PushProxy(rdxSStackProxyBase &proxy, rdxLargeUInt size, rdxEStackViewType stackViewType) RDX_OVERRIDE;
};

class rdxCStackViewPrecall : public rdxCStackView
{
	void *m_parametersBase;
	void *m_returnValuesBase;
	rdxLargeUInt m_paramSize;
	rdxLargeUInt m_rvSize;

	rdxSStackProxyBase *m_lastReturnValue;
	rdxSStackProxyBase *m_lastParameter;

public:
	explicit rdxCStackViewPrecall(void *prv);
	void PushProxy(rdxSStackProxyBase &proxy, rdxLargeUInt size, rdxEStackViewType stackViewType) RDX_OVERRIDE;
	void *GetPRV() const;
	rdxLargeUInt ParameterSize() const;
	rdxLargeUInt ReturnValueSize() const;
};

inline int rdxSExportedCallEnvironment::Throw(const char *exDomain, const char *exSymbol)
{
	rdxCRef(rdxCException) exObj;
	exObj = this->objm->LookupSymbolSimple(this->ctx, rdxSObjectGUID::FromObjectName(exDomain, exSymbol)).StaticCast<rdxCException>();
	return this->Throw(exObj.ToWeakHdl());
}

#include "rdx_runtime.hpp"

inline int rdxSExportedCallEnvironment::Throw(rdxWeakHdl(rdxCException) ex)
{
	this->thread->ex = ex.ToWeakRTRef();
	this->status = rdxRS_Exception;
	return rdxRS_Exception;
}

inline rdxCStackViewCallback::rdxCStackViewCallback(void *prv)
{
	m_parametersBase = m_returnValuesBase = prv;
	m_lastReturnValue = RDX_CNULL;
}

inline void rdxCStackViewCallback::PushProxy(rdxSStackProxyBase &proxy, rdxLargeUInt size, rdxEStackViewType stackViewType)
{
	rdxLargeUInt paddedSize = rdxPaddedSize(size, rdxALIGN_RuntimeStackValue);

	if(stackViewType == rdxSTACKVIEWTYPE_Parameter)
	{
		// Parameters start below the PRV
		m_parametersBase = static_cast<rdxUInt8*>(m_parametersBase) - paddedSize;
		proxy.valueAddr = m_parametersBase;
		proxy.next = RDX_CNULL;
	}
	else if(stackViewType == rdxSTACKVIEWTYPE_ReturnValue)
	{
		for(rdxSStackProxyBase *oldRV=m_lastReturnValue; oldRV != RDX_CNULL; oldRV=oldRV->next)
			oldRV->valueAddr = static_cast<rdxUInt8*>(oldRV->valueAddr) + paddedSize;
		proxy.valueAddr = m_returnValuesBase;
		proxy.next = m_lastReturnValue;
		m_lastReturnValue = &proxy;
	}
}

inline rdxCStackViewPrecall::rdxCStackViewPrecall(void *prv)
{
	m_parametersBase = m_returnValuesBase = prv;
	m_paramSize = m_rvSize = 0;
	m_lastReturnValue = RDX_CNULL;
	m_lastParameter = RDX_CNULL;
}

inline void *rdxCStackViewPrecall::GetPRV() const
{
	if(m_lastReturnValue != RDX_CNULL)
	{
		for(rdxSStackProxyBase *rv=m_lastReturnValue; ;rv=rv->next)
			if(rv->next == RDX_CNULL)
				return rv->valueAddr;
	}
	return m_returnValuesBase;
}

inline rdxLargeUInt rdxCStackViewPrecall::ParameterSize() const
{
	return m_paramSize;
}

inline rdxLargeUInt rdxCStackViewPrecall::ReturnValueSize() const
{
	return m_rvSize;
}

inline void rdxCStackViewPrecall::PushProxy(rdxSStackProxyBase &proxy, rdxLargeUInt size, rdxEStackViewType stackViewType)
{
	rdxLargeUInt paddedSize = rdxPaddedSize(size, rdxALIGN_RuntimeStackValue);

	if(stackViewType == rdxSTACKVIEWTYPE_Parameter)
	{
		// Parameters start below the PRV
		proxy.valueAddr = m_parametersBase;
		proxy.next = m_lastParameter;
		m_lastParameter = &proxy;
		m_parametersBase = static_cast<rdxUInt8*>(m_parametersBase) - paddedSize;
		m_paramSize += paddedSize;
	}
	else if(stackViewType == rdxSTACKVIEWTYPE_ReturnValue)
	{
		proxy.valueAddr = m_returnValuesBase;
		proxy.next = m_lastReturnValue;
		m_lastReturnValue = &proxy;
		for(rdxSStackProxyBase *oldRV=m_lastReturnValue; oldRV != RDX_CNULL; oldRV=oldRV->next)
			oldRV->valueAddr = static_cast<rdxUInt8*>(oldRV->valueAddr) - paddedSize;
		for(rdxSStackProxyBase *oldParam=m_lastParameter; oldParam != RDX_CNULL; oldParam=oldParam->next)
			oldParam->valueAddr = static_cast<rdxUInt8*>(oldParam->valueAddr) - paddedSize;
		m_parametersBase = static_cast<rdxUInt8*>(m_parametersBase) - paddedSize;
		m_rvSize += paddedSize;
	}
}


template<class T>
inline rdxCStackValueProxy<T>::rdxCStackValueProxy(rdxCStackView &stackView, rdxEStackViewType svt)
{
	stackView.PushProxy(*this, sizeof(T), svt);
}

template<class T>
inline const T &rdxCStackValueProxy<T>::Value() const
{
	return *static_cast<const T*>(this->valueAddr);
}

template<class T>
inline T &rdxCStackValueProxy<T>::Modify() const
{
	return *static_cast<T*>(this->valueAddr);
}

template<class T>
inline rdxCStackStructRefProxy<T>::rdxCStackStructRefProxy(rdxCStackView &stackView, rdxEStackViewType svt)
{
	stackView.PushProxy(*this, sizeof(rdxRuntimeOffsetRTRef), svt);
}

template<class T>
inline typename rdxWeakOffsetRTRef(T) rdxCStackStructRefProxy<T>::ToOffsetRTRef() const
{
	const rdxRuntimeOffsetRTRef *runtimeRef = static_cast<const rdxRuntimeOffsetRTRef *>(this->valueAddr);
	return rdxWeakOffsetRTRef(T)(runtimeRef->rtref.ToWeakRTRef(), runtimeRef->offset);
}

template<class T>
inline const T &rdxCStackStructRefProxy<T>::Value() const
{
	const rdxRuntimeOffsetRTRef *runtimeRef = static_cast<const rdxRuntimeOffsetRTRef *>(this->valueAddr);
	return *rdxWeakOffsetRTRef(T)(runtimeRef->rtref.ToWeakRTRef(), runtimeRef->offset).Data();
}

template<class T>
inline T &rdxCStackStructRefProxy<T>::Modify() const
{
	const rdxRuntimeOffsetRTRef *runtimeRef = static_cast<const rdxRuntimeOffsetRTRef *>(this->valueAddr);
	return *rdxWeakOffsetRTRef(T)(runtimeRef->rtref.ToWeakRTRef(), runtimeRef->offset).Modify();
}

#endif
