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
#include "rdx_nullcodeprovider.hpp"

#if 0

static int RDX_DECL_API NullCodeProviderEntryPoint(rdxSOperationContext *ctx, ObjectManagement::rdxIObjectManager *objm, const rdxCMethod *m, Programmability::RuntimeThread *thread, Programmability::rdxURuntimeStackValue *prv)
{
	return 0;
}

void NullCodeProvider::CreateExecutable(ObjectManagement::rdxIObjectManager *objm, rdxSOperationContext *ctx, rdxCMethod *m) const
{
}

void NullCodeProvider::ReleaseExecutable(ObjectManagement::rdxIObjectManager *objm, rdxCMethod *m) const
{
}

void NullCodeProvider::InitializeSymbolDictionary(rdxSOperationContext *ctx, ObjectManagement::rdxIObjectManager *objm) const
{
}

//Programmability::NativeCallback NullCodeProvider::GetNativeCallback() const
//{
//	return NullCodeProviderEntryPoint;
//}

Programmability::IPToCurrentInstructionCallback NullCodeProvider::GetIPToCurrentInstructionCallback() const
{
	return NULL;
}

int NullCodeProvider::RunMethod(rdxSOperationContext *ctx, ObjectManagement::rdxIObjectManager *objm, const rdxCMethod *m, Programmability::RuntimeThread *thread, Programmability::rdxURuntimeStackValue *prv) const
{
	return 0;
}

Programmability::InstrNumToIPCallback NullCodeProvider::GetInstrNumToIPCallback() const
{
	return NULL;
}

Programmability::ResumeThreadCallback NullCodeProvider::GetResumeThreadCallback(ObjectManagement::rdxIObjectManager *objm) const
{
	return NULL;
}

void NullCodeProvider::Shutdown()
{
}

#endif
