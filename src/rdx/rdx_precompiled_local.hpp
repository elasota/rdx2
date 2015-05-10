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
typedef int (*precompiledFunction_t)(OperationContext *ctx, IObjectManager *objm, RuntimeThread *thread, const LargeInt *instrTable, void **providerDictionary, int timeoutCounter);

static bool CanRunIP(const Method *method, const void *ip)
{
	return (ip >= globalInstructionTable || ip < (globalInstructionTable + (sizeof(globalInstructionTable) / sizeof(LargeInt))));
}

static LargeInt IPToCurrentInstruction(const Method *method, const void *ip)
{
	return (*static_cast<const LargeInt *>(ip)) - 1;
}

static const void *InstrNumToIP(OperationContext* ctx, const Method *method, LargeInt instrNum, bool *resumeAllowed)
{
	const LargeInt *ip = static_cast<const LargeInt *>(ip);
	LargeInt offset = *ip;

	if(offset == 0)
		RDX_LTHROWV(ctx, Errors::RDX_ERROR_INVALID_PATCH, NULL);

	if(resumeAllowed)
		*resumeAllowed = (offset > 0);
	return ip;
}

static int RDX_DECL_API ResumeThread(OperationContext *ctx, IObjectManager *objm, RuntimeThread *t)
{
	const LargeInt *ip = static_cast<const LargeInt *>(ip);
	LargeInt offset = *ip;
	const LargeInt *funcBaseIP = ip + ((offset < 0) ? offset : -offset);

	precompiledFunction_t funcBase = reinterpret_cast<precompiledFunction_t>(reinterpret_cast<UInt8*>(NULL) + *funcBaseIP);
	int result = funcBase(ctx, objm, t, funcBaseIP + 1, objm->GetBuiltIns()->providerDictionary, t->timeout);
	if(result > 0)
		t->timeout = result;
	return result;
}

static int RunNativeMethod(OperationContext *ctx, IObjectManager *objm, const Method *m, RuntimeThread *thread,
	RuntimeStackValue *prv)
{
	if(!RuntimeUtilities::EnterMethod(objm, m, &thread->frame, thread->frame.bp, thread->stackBytes, prv, &thread->frame))
	{
		thread->ex = static_cast<Exception*>(objm->GetBuiltIns()->providerDictionary[X_StackOverflowException]);
		return RuntimeState::Exception;
	}

	GCInfo *threadInfo = GCInfo::From(thread);

	return ResumeThread(ctx, objm, thread);
}
