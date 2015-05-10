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
#include "rdx_pragmas.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <new>
#include <emmintrin.h>

#undef CreateMutex

#include "rdx_threading.hpp"
#include "rdx_longflow.hpp"
#include "rdx_objectmanagement.hpp"
#include "rdx_reftypecode.hpp"

//RDX_DECLARE_EXPLICIT_SUPER(class, rdxWin32Mutex, rdxIMutex);

class rdxWin32Mutex : public rdxIMutex
{
	CRITICAL_SECTION m_cs;
	rdxSAllocator m_alloc;

public:
	rdxWin32Mutex(const rdxSAllocator &alloc)
	{
		m_alloc = alloc;
		InitializeCriticalSectionAndSpinCount(&m_cs, 4000);
	}

	~rdxWin32Mutex()
	{
		DeleteCriticalSection(&m_cs);
	}

	virtual bool TryAcquire(int msec)
	{
		return (TryEnterCriticalSection(&m_cs) != 0);
	}

	virtual void Acquire(int msec)
	{
		EnterCriticalSection(&m_cs);
	}

	virtual void Release()
	{
		LeaveCriticalSection(&m_cs);
	}

	void Destroy()
	{
		rdxSAllocator alloc = m_alloc;
		this->~rdxWin32Mutex();
		alloc.Free(this);
	}
};
//RDX_DECLARE_COMPLEX_NATIVE_CLASS(rdxWin32Mutex, (rdxETIF_LifeCycle));

DWORD WINAPI rdxWin32StartThread(LPVOID lpParameter)
{
	const rdxISystemThreadProc *proc = static_cast<const rdxISystemThreadProc *>(lpParameter);
	proc->Start();
	return 0;
}

class rdxWin32Thread : public rdxISystemThread
{
private:
	HANDLE m_handle;
public:
	rdxWin32Thread(const rdxISystemThreadProc *proc)
	{
		m_handle = CreateThread(NULL, 0, rdxWin32StartThread, const_cast<rdxISystemThreadProc*>(proc), 0, NULL);
	}

	~rdxWin32Thread()
	{
		CloseHandle(m_handle);
	}

	bool WaitFor(int msec)
	{
		return (WaitForSingleObject(m_handle, static_cast<DWORD>(msec < 0 ? INFINITE : msec)) == WAIT_OBJECT_0);
	}

	bool IsActive()
	{
		DWORD exitCode;
		GetExitCodeThread(m_handle, &exitCode);
		return exitCode == STILL_ACTIVE;
	}
};

rdxISystemThread *CreateSystemThread(const rdxISystemThreadProc *proc)
{
	return new rdxWin32Thread(proc);
}

void rdxDestroySystemThread(rdxISystemThread *sysThread)
{
	delete sysThread;
}

void rdxSleepThread(rdxLargeInt msec)
{
	Sleep(static_cast<DWORD>(msec));
}

rdxISystemThread::~rdxISystemThread()
{
}

int rdxCountProcessors()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return static_cast<int>(sysinfo.dwNumberOfProcessors);
}

rdxIMutex *rdxCreateMutex(const rdxSAllocator &alloc)
{
	rdxWin32Mutex *mutex = alloc.CAlloc<rdxWin32Mutex>(1, rdxALLOC_LongTerm);
	if(!mutex)
		return NULL;
	new (mutex) rdxWin32Mutex(alloc);
	return mutex;
}

void rdxDestroyMutex(rdxIMutex *mutex)
{
	static_cast<rdxWin32Mutex*>(mutex)->Destroy();
}

