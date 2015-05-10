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
#ifndef __RDX_LONGFLOW_HPP__
#define __RDX_LONGFLOW_HPP__

#include "rdx_platform.hpp"

// Exception handling macros that can work with a C fallback when C++ exceptions are disabled
// For safety, code execution must always either rethrow or complete

#if defined(RDX_DISABLE_SANITY_CHECKS)

#define RDX_NTRY(lbl, ctx)					{

#define RDX_NPROTECT(lbl, ctx, invocation)	invocation

#define RDX_NPROTECT_ASSIGN(lbl, ctx, target, invocation)	(target) = (invocation)

#define RDX_NCATCH(lbl, ctx)				} if(false) {

#define RDX_NENDTRY(lbl)					}

#define RDX_RETHROW(ctx)					do {} while(false)

#define RDX_RETHROWV(ctx, v)				do {} while(false)


#define RDX_LTHROW(ctx, errCode)		do {} while(0)

#define RDX_LTHROWV(ctx, errCode, v)	RDX_LTHROW(ctx, errCode)

#define RDX_NSTHROW(lbl, ctx, errCode)	RDX_LTHROW(ctx, errCode)

#elif defined(RDX_USE_CPP_EXCEPTIONS)

class rdxSOperationContext;

class rdxCInternalException
{
	rdxSOperationContext *m_ctx;
	int m_errorCode;
public:
	inline InternalException(rdxSOperationContext *ctx, int errorCode)
	{
		m_ctx = ctx;
		m_errorCode = errorCode;
	}

	inline int ErrorCode() const
	{
		return m_errorCode;
	}

	inline rdxSOperationContext *rdxSOperationContext() const
	{
		return m_ctx;
	}
}

#define RDX_NTRY(lbl, ctx)					try {

#define RDX_NPROTECT(lbl, ctx, invocation)	invocation

#define RDX_NPROTECT_ASSIGN(lbl, ctx, target, invocation)	(target) = (invocation)

#define RDX_NCATCH(lbl, ctx)				} catch(const rdxCInternalException &p__tdp_iex) { int errorCode = p__tdp_iex->ErrorCode();

#define RDX_NENDTRY(lbl)					}

#define RDX_RETHROW(ctx)					throw

#define RDX_RETHROWV(ctx, v)				throw


#define RDX_LTHROW(ctx, errCode)		do\
										{\
											throw rdxCInternalException ((ctx), (errCode));\
										} while(0)

#define RDX_LTHROWV(ctx, errCode, v)	RDX_LTHROW(ctx, errCode)

#define RDX_NSTHROW(lbl, ctx, errCode)	RDX_LTHROW(ctx, errCode)

#else

#define RDX_NTRY(lbl, ctx)					{\
												if((ctx)->errorCode)\
													goto errorHandler_##lbl;\
												(ctx)->errorCode = 0;

#define RDX_NPROTECT(lbl, ctx, invocation)	do {\
												invocation;\
												if((ctx)->errorCode)\
													goto errorHandler_##lbl;\
											} while(0)


#define RDX_NPROTECT_ASSIGN(lbl, ctx, target, invocation)	do {\
																target = invocation;\
																if((ctx)->errorCode)\
																	goto errorHandler_##lbl;\
															} while(0)

#define RDX_NCATCH(lbl, ctx)					goto noError_##lbl;\
												errorHandler_##lbl:\
												{\
													int errorCode = (ctx)->errorCode;\
													(ctx)->errorCode = 0;

#define RDX_NENDTRY(lbl)						}\
												noError_##lbl:;\
											}

#define RDX_RETHROW(ctx)			(ctx)->errorCode = errorCode;\
									rdxDebugBreak(rdxBREAKCAUSE_Exception);\
									return

#define RDX_RETHROWV(ctx, v)		(ctx)->errorCode = errorCode;\
									rdxDebugBreak(rdxBREAKCAUSE_Exception);\
									return v


#define RDX_NSTHROW(lbl, ctx, errCode)	do\
										{\
											(ctx)->errorCode = errCode;\
											rdxDebugBreak(rdxBREAKCAUSE_Exception);\
											goto errorHandler_##lbl;\
										} while(0)


#define RDX_LTHROW(ctx, errCode)	do\
									{\
										(ctx)->errorCode = errCode;\
										rdxDebugBreak(rdxBREAKCAUSE_Exception);\
										return;\
									} while(0)

#define RDX_LTHROWV(ctx, errCode, v)	do\
									{\
										(ctx)->errorCode = errCode;\
										rdxDebugBreak(rdxBREAKCAUSE_Exception);\
										return v;\
									} while(0)


#endif


#define RDX_TRY(ctx)								RDX_NTRY(EH, ctx)
#define RDX_PROTECT(ctx, invocation)				RDX_NPROTECT(EH, ctx, invocation)
#define RDX_PROTECT_ASSIGN(ctx, target, invocation)	RDX_NPROTECT_ASSIGN(EH, ctx, target, invocation)
#define RDX_CATCH(ctx)								RDX_NCATCH(EH, ctx)
#define RDX_ENDTRY									RDX_NENDTRY(EH)
#define RDX_STHROW(ctx, errCode)					RDX_NSTHROW(EH, ctx, errCode)
#define RDX_PROTECT_PASSTHROUGH(invocation)			(invocation)


// C++ style:
//struct RDX_EXCEPTION_CONTEXT		{ int errorCode; }
//#define RDX_TRY(ctx)				try
//#define RDX_CATCH(ctx)			catch(RDX::rdxSOperationContext *c) { int errorCode = c->errorCode;
//#define RDX_ENDTRY(ctx)			}
//#define RDX_THROW(ctx, errCode)	do { (ctx)->errorCode = errCode; throw (ctx); } while(0)


class rdxCException;
struct rdxIObjectManager;

struct rdxIMutex;

enum
{
	rdxCOREMUTEX_OperationCheck,
	rdxCOREMUTEX_OperationContext,			// Blocks access to operation context activity
	rdxCOREMUTEX_SymbolTableAccess,			// Blocks symbol table access and package operations
	rdxCOREMUTEX_ObjectList,				// Blocks object creation
	rdxCOREMUTEX_StringTableInsert,			// Blocks string table insertions
	rdxCOREMUTEX_AOTTableInsert,			// Blocks array table insertions

	rdxCOREMUTEX_GarbageCollector,
		
	rdxCOREMUTEX_PersistentObjectList,		// Won't cause a global block

	rdxCOREMUTEX_Count,
};

struct rdxSOperationContext
{
	rdxCException *ex;
	rdxIObjectManager *objm;
	int errorCode;

	int mutexAccess[rdxCOREMUTEX_Count];

	explicit rdxSOperationContext(rdxIObjectManager *objm);
	virtual ~rdxSOperationContext();
		
	bool TryAcquire(rdxIObjectManager *objm, int mNum, int msec = 0);
	void Acquire(rdxIObjectManager *objm, int mNum, int msec = 0);
	void Release(rdxIObjectManager *objm, int mNum);

	void Deaden();
};

#include "rdx_threading.hpp"
#include "rdx_objectmanagement.hpp"

inline bool rdxSOperationContext::TryAcquire(rdxIObjectManager *objm, int mNum, int msec)
{
	if(mutexAccess[mNum] == 0 &&
		!objm->GetCoreMutex(mNum)->TryAcquire(msec))
			return false;

	mutexAccess[mNum]++;
	return true;
}

inline void rdxSOperationContext::Acquire(rdxIObjectManager *objm, int mNum, int msec)
{
	if(!mutexAccess[mNum])
		objm->GetCoreMutex(mNum)->Acquire(msec);
	mutexAccess[mNum]++;
}

inline void rdxSOperationContext::Release(rdxIObjectManager *objm, int mNum)
{
	mutexAccess[mNum]--;
	if(!mutexAccess[mNum])
		objm->GetCoreMutex(mNum)->Release();
}

inline rdxSOperationContext::rdxSOperationContext(rdxIObjectManager *objm)
{
	errorCode = 0;
	for(int i=0;i<rdxCOREMUTEX_Count;i++)
		mutexAccess[i] = 0;

	this->objm = objm;
	if(objm)
		objm->IncrementThreadCounter();
}

inline rdxSOperationContext::~rdxSOperationContext()
{
	Deaden();
}

inline void rdxSOperationContext::Deaden()
{
	if(objm)
		objm->DecrementThreadCounter();
	objm = NULL;
}

#endif
