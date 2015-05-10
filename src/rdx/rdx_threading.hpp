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
#ifndef __RDX_THREADING_HPP__
#define __RDX_THREADING_HPP__

#include "rdx_superresolver.hpp"
//#include "rdx_objectmanagement.hpp"

struct rdxSOperationContext;
struct rdxSAllocator;
struct rdxIObjectManager;

struct rdxIMutex
{
	virtual bool TryAcquire(int msec) = 0;
	virtual void Acquire(int msec) = 0;
	virtual void Release() = 0;

	inline void Acquire()
	{
		this->Acquire(0);
	}
};
RDX_DECLARE_EXPLICIT_SUPER(struct, rdxIMutex, void);

struct rdxISystemThreadProc
{
	virtual void Start() const = 0;
};

struct rdxISystemThread
{
	virtual ~rdxISystemThread() = 0;
	virtual bool WaitFor(int msec = -1) = 0;
	virtual bool IsActive() = 0;
};

#include "rdx_basictypes.hpp"

int rdxCountProcessors();
rdxIMutex *rdxCreateMutex(const rdxSAllocator &alloc);
void rdxDestroyMutex(rdxIMutex *mtx);
rdxISystemThread *rdxCreateSystemThread(const rdxISystemThreadProc *proc);
void rdxDestroySystemThread(rdxISystemThread *sysThread);
void rdxSleepThread(rdxLargeInt msec);

void rdxSynchronize();

class rdxCLightMutex
{
	rdxAtomicUInt m_i;

public:
	inline rdxCLightMutex()
	{
		m_i.WriteUnfenced(0);
	}

	inline bool TryAcquire(int count = 1)
	{
		while(count--)
		{
			if(m_i.CompareExchangeFullFence(1, 0) != 0)
			{
				rdxSynchronize();
				return true;
			}
		}
		return false;
	}

	inline void Acquire()
	{
		while(m_i.CompareExchangeFullFence(1, 0) == 0)
		{
		}
		rdxSynchronize();
	}

	inline void Release()
	{
		m_i.WriteAfterPrevious(0);
	}
};

template<class _Tmutex>
class rdxCMutexLock
{
	_Tmutex *m_mtx;

public:
	inline explicit rdxCMutexLock(_Tmutex *mtx)
	{
		m_mtx = mtx;
		m_mtx->Acquire();
	}

	inline ~rdxCMutexLock()
	{
		m_mtx->Release();
	}
};

#endif
