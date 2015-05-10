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
#ifndef __RDX_INTERPRET_HPP__
#define __RDX_INTERPRET_HPP__

#include "rdx_runtime.hpp"
#include "rdx_objectmanagement.hpp"


class rdxCInterpreterCodeProvider : public rdxICodeProvider
{
public:
	rdxCInterpreterCodeProvider(const rdxSAllocator &alloc);

	virtual void CreateExecutable(rdxIObjectManager *objm, rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) m) const RDX_OVERRIDE;
	virtual void ReleaseExecutable(rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m) const RDX_OVERRIDE;
	virtual void InitializeSymbolDictionary(rdxSOperationContext *ctx, rdxIObjectManager *objm) const RDX_OVERRIDE;
	virtual int RunMethod(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, rdxURuntimeStackValue *prv) const RDX_OVERRIDE;

	virtual rdxIPToCurrentInstructionCallback GetIPToCurrentInstructionCallback() const RDX_OVERRIDE;
	virtual rdxInstrNumToIPCallback GetInstrNumToIPCallback() const RDX_OVERRIDE;
	virtual rdxResumeThreadCallback GetResumeThreadCallback(rdxIObjectManager *objm) const RDX_OVERRIDE;
	virtual void Shutdown() RDX_OVERRIDE;

private:
	rdxSAllocator m_alloc;

	void CompileInterpreterCode(rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) m);

};

#if 0

class rdxCInterpreterCodeProvider : public rdxICodeProvider
{
private:
	const rdxCPrecompiledCodeModule **m_modules;
	rdxLargeInt m_numModules;
	rdxSAllocator m_alloc;

public:
	rdxCInterpreterCodeProvider(const rdxSAllocator &alloc, const rdxCPrecompiledCodeModule **modules, rdxLargeInt numModules);

	void CreateExecutable(rdxIObjectManager *objm, rdxSOperationContext *ctx, rdxHandle<rdxCMethod> m) const RDX_OVERRIDE RDX_FINAL;
	void ReleaseExecutable(rdxIObjectManager *objm, rdxHandle<rdxCMethod> m) const RDX_OVERRIDE RDX_FINAL;
	void InitializeSymbolDictionary(rdxSOperationContext *ctx, rdxIObjectManager *objm) const RDX_OVERRIDE RDX_FINAL;
	int RunMethod(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxHandle<rdxCMethod> m, rdxHandle<rdxCRuntimeThread> thread, rdxURuntimeStackValue *prv) const RDX_OVERRIDE RDX_FINAL;

	rdxIPToCurrentInstructionCallback GetIPToCurrentInstructionCallback() const RDX_OVERRIDE RDX_FINAL;
	rdxInstrNumToIPCallback GetInstrNumToIPCallback() const RDX_OVERRIDE RDX_FINAL;
	rdxResumeThreadCallback GetResumeThreadCallback(rdxIObjectManager *objm) const RDX_OVERRIDE RDX_FINAL;
	void Shutdown() RDX_OVERRIDE RDX_FINAL;

	inline rdxLargeInt NumModules() const
	{
		return m_numModules;
	}

	inline const rdxCPrecompiledCodeModule **Modules() const
	{
		return m_modules;
	}
};


inline rdxCInterpreterCodeProvider::rdxCInterpreterCodeProvider(const rdxSAllocator &alloc, const rdxCPrecompiledCodeModule **modules, rdxLargeInt numModules)
{
	m_modules = modules;
	m_numModules = numModules;
	m_alloc = alloc;
}

#endif

#endif
