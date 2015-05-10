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
#include "rdx_precompiled.hpp"
#include "rdx_programmability.hpp"
#include "rdx_objectmanagement.hpp"
#include "rdx_lut.hpp"
#include "rdx_runtime.hpp"
#include "rdx_pccm_env.hpp"

int rdxCPrecompiledCodeModule::ResumeThread(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakRTRef(rdxCRuntimeThread) t, rdxPCCMCallback pccmCallback)
{
	rdxLargeUInt instrNum = static_cast<rdxLargeUInt>(static_cast<const rdxUInt8 *>(t->frame.ip) - static_cast<const rdxUInt8 *>(RDX_CNULL));
	rdxPCCMCallEnvironmentInternal callEnvInternal;
	callEnvInternal.Setup(ctx, t.Modify(), t->frame.method.Data(), t->frame.bp, t->frame.prv, instrNum);
	pccmCallback(callEnvInternal);
	return callEnvInternal.GetReturnStatus();
}


#if 0

using namespace RDX;
using namespace RDX::Programmability;
using namespace RDX::ObjectManagement;

namespace RDX
{
	namespace Programmability
	{
		bool rdxCPrecompiledCodeModule::CanRunIP(const Method *method, const void *ip) const
		{
			rdxLargeInt offset = reinterpret_cast<const char *>(ip) - reinterpret_cast<const char *>(_instructionTable);
			return (offset >= 0 && offset < _instructionTableSize);
		}
		
		const PrecompiledFunctionInfo *rdxCPrecompiledCodeModule::GetFunctionInfo(const String *str) const
		{
			StaticLookupStringKey<char, Char> strKey(str->AsChars(), str->Length());
			return this->_functionLookupTable->Lookup(strKey);
		}

		rdxLargeInt rdxCPrecompiledCodeModule::IPToCurrentInstruction(const Method *method, const void *ip)
		{
			return (static_cast<const rdxLargeInt *>(ip) - static_cast<const rdxLargeInt *>(method->_native.nativeInstructions)) - 1;
		}

		const void *rdxCPrecompiledCodeModule::InstrNumToIP(rdxSOperationContext* ctx, const Method *method, rdxLargeInt instrNum, bool *resumeAllowed)
		{
			const rdxLargeInt *ip = static_cast<const rdxLargeInt *>(method->_native.nativeInstructions) + instrNum;
			rdxLargeInt offset = *ip;

			if(offset == 0)
			{
				const GCInfo *methodGCI = GCInfo::From(method);
				RDX_LTHROWV(ctx, RDX_ERROR_INVALID_PATCH, NULL);
			}

			if(resumeAllowed)
				*resumeAllowed = (offset > 0);
			return ip;
		}

		int rdxCPrecompiledCodeModule::RunNativeMethod(rdxSOperationContext *ctx, rdxIObjectManager *objm, const Method *m, RuntimeThread *thread, rdxURuntimeStackValue *prv)
		{
			const void *stackBottom = thread->stackBytes + thread->stackCapacity;
			if(!RuntimeUtilities::EnterMethodInline(objm, m, &thread->frame, thread->frame.bp, thread->stackBytes, prv, &thread->frame))
			{
				thread->ex = static_cast<Exception*>(objm->GetBuiltIns()->providerDictionary[X_StackOverflowException]);
				return RuntimeState::Exception;
			}

			return ResumeThread(ctx, objm, thread);
		}

		
		const void *rdxCPrecompiledCodeModule::IPForMethod(const Method *m) const
		{
			const String *str = GCInfo::From(m)->gstSymbol;
			if(str == NULL)
				return NULL;
			StaticLookupStringKey<char, Char> strKey(str->AsChars(), str->Length());
			const PrecompiledFunctionInfo *funcInfo = _functionLookupTable->Lookup(strKey);
			if(funcInfo != NULL)
				return funcInfo->compiledInstructions;
			return NULL;
		}
	}
}

#endif
