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

#include <stdlib.h>
#include <stdio.h>
#include <new>

#include "rdx.h"
#include "rdx_io.hpp"
#include "rdx_objectmanagement.hpp"
#include "rdx_constants.hpp"
#include "rdx_interpret.hpp"
#include "rdx_marshal.hpp"
#include "rdx_longflow.hpp"
#include "rdx_zonepolicy.hpp"
#include "rdx_plugin.hpp"
#include "rdx_objectloader.hpp"
#include "rdx_pragmas.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#ifdef RDX_WCHAR_SYSTEM_SERVICES
#define SYS_strcpy	wcscpy
#define SYS_strncat	wcsncat
#define SYS_fopen	_wfopen
#define SYS_strlen	wcslen
#else
#define SYS_strcpy	strcpy
#define SYS_strncat	strncat
#define SYS_fopen	fopen
#define SYS_strlen	strlen
#endif

struct rdxWin32FileStream : public rdxIFileStream
{
	FILE *f;
	bool aborted;

	explicit rdxWin32FileStream(FILE *f);

	virtual rdxLargeUInt Tell();
	virtual rdxLargeUInt WriteBytes(const void *dest, rdxLargeUInt numBytes) RDX_OVERRIDE;
	virtual rdxLargeUInt ReadBytes(void *dest, rdxLargeUInt numBytes) RDX_OVERRIDE;
	virtual void SeekStart(rdxLargeUInt offset) RDX_OVERRIDE;
	virtual void SeekEnd(rdxLargeInt offset) RDX_OVERRIDE;
	virtual void SeekCurrent(rdxLargeInt offset) RDX_OVERRIDE;
	virtual void SeekForward(rdxLargeUInt offset) RDX_OVERRIDE;
	virtual void Close() RDX_OVERRIDE;
	virtual bool HasAborted() const RDX_OVERRIDE;
	virtual void Abort() RDX_OVERRIDE;
};

#if 0

class Win32FileScanState : public rdxIFileScanState
		{
		private:
#ifdef RDX_WCHAR_SYSTEM_SERVICES
			WIN32_FIND_DATAW _findData;
#else
			WIN32_FIND_DATAA _findData;
#endif
			HANDLE _hfind;
			Char filter[Security::MAX_ZONEPOLICY_PATH + 20];

		public:
			Win32FileScanState(const Char *path, const Char *wildcard, const Char *vext);
			CRef<const String> NextFile(rdxSOperationContext *ctx, rdxIObjectManager *objm);
			void Close();
		};


		struct Win32Library
		{
			HMODULE lib;
		};
	}
}


namespace RDX
{
	namespace AppServices
	{
		void *GetProc(void *lib, const char *procName)
		{
			return GetProcAddress(static_cast<Win32Library *>(lib)->lib, procName);
		}

		void *LoadLib(const char *libName)
		{
			char pathBuf[4000];
			if(strlen(libName) + strlen(VERSION_TAG) + 5 >= sizeof(pathBuf))
				return NULL;
			sprintf(pathBuf, "%s%s.dll", libName, VERSION_TAG);

			Win32Library *lib = static_cast<Win32Library *>(malloc(sizeof(Win32Library)));
			if(lib == NULL)
				return NULL;
			HMODULE module = LoadLibrary(pathBuf);
			if(module == NULL)
			{
				free(lib);
				return NULL;
			}
			lib->lib = module;
			return lib;
		}

		void UnloadLib(void *lib)
		{
			FreeLibrary(static_cast<Win32Library *>(lib)->lib);
			free(lib);
		}

	}
}

#endif

struct rdxWin32FileSystem : public rdxIFileSystem
{
	rdxIFileStream *Open(const rdxChar *path, bool write);
//	rdxIFileScanState *ScanDirectory(const rdxChar *directory, const rdxChar *wildcard, const rdxChar *vext);
};

static rdxWin32FileSystem w32fs;

void *rdxWin32Realloc(void *opaque, void *ptr, rdxLargeUInt sz, rdxLargeUInt align)
{
	return _aligned_realloc(ptr, sz, align);
}

RDX_UTIL_DYNLIB_API rdxIFileSystem *rdxGetFileSystem()
{
	return &w32fs;
}

rdxWin32FileStream::rdxWin32FileStream(FILE *f)
{
	this->f = f;
	this->aborted = false;
}

rdxLargeUInt rdxWin32FileStream::Tell()
{
	return static_cast<rdxLargeUInt>(ftell(f));
}

rdxLargeUInt rdxWin32FileStream::WriteBytes(const void *dest, rdxLargeUInt numBytes)
{
	if(this->aborted)
		return 0;
	rdxLargeUInt numWritten = fwrite(dest, 1, numBytes, f);
	if(numWritten != numBytes)
		Abort();
	return numWritten;
}

bool rdxWin32FileStream::HasAborted() const
{
	return this->aborted;
}

void rdxWin32FileStream::Abort()
{
	this->aborted = true;
}

rdxLargeUInt rdxWin32FileStream::ReadBytes(void *dest, rdxLargeUInt numBytes)
{
	return fread(dest, 1, numBytes, f);
}

void rdxWin32FileStream::SeekStart(rdxLargeUInt offset)
{
	fseek(f, static_cast<long>(offset), SEEK_SET);
}

void rdxWin32FileStream::SeekEnd(rdxLargeInt offset)
{
	fseek(f, static_cast<long>(offset), SEEK_END);
}

void rdxWin32FileStream::SeekCurrent(rdxLargeInt offset)
{
	fseek(f, static_cast<long>(offset), SEEK_SET);
}

void rdxWin32FileStream::SeekForward(rdxLargeUInt offset)
{
	fseek(f, static_cast<long>(offset), SEEK_CUR);
}

void rdxWin32FileStream::Close()
{
	if(f)
		fclose(f);
	rdxWin32Realloc(NULL, this, 0, 1);
}

#if 0
RDX::AppServices::Win32FileScanState::Win32FileScanState(const rdxChar *path, const rdxChar *wildcard, const rdxChar *vext)
{
	memcpy(filter, RDX_STATIC_STRING("rdxclasses"), sizeof(Char) * 10);
	filter[10] = static_cast<Char>(RDX_FILESYSTEM_DELIMITER);

	rdxLargeInt len = 11;
	for(rdxLargeInt i=0;path[i];i++,len++)
		filter[len] = path[i];
		
	for(rdxLargeInt i=0;wildcard[i];i++,len++)
		filter[len] = wildcard[i];

	filter[len] = static_cast<Char>(0);

	_hfind = NULL;
}

void RDX::AppServices::Win32FileScanState::Close()
{
	if(_hfind)
		FindClose(_hfind);
	Realloc(NULL, this, 0, 1);
}

CRef<const String> RDX::AppServices::Win32FileScanState::NextFile(rdxSOperationContext *ctx, rdxIObjectManager *objm)
{
	while(true)
	{
		if(!_hfind)
		{
#ifdef RDX_WCHAR_SYSTEM_SERVICES
			_hfind = FindFirstFileW(filter, &_findData);
#else
			_hfind = FindFirstFileA(filter, &_findData);
#endif
			if(_hfind == INVALID_HANDLE_VALUE)
			{
				_hfind = NULL;
				return NULL;
			}
		}
		else
		{
#ifdef RDX_WCHAR_SYSTEM_SERVICES
			if(!FindNextFileW(_hfind, &_findData))
#else
			if(!FindNextFileA(_hfind, &_findData))
#endif
				return NULL;
		}
		if((_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			break;
	}

	return objm->CreateString(ctx, _findData.cFileName);
}
#endif

static wchar_t *CreateUniPath(const rdxChar *basePath)
{
	rdxLargeUInt prefixSize = 4;
	rdxLargeUInt pathLen = SYS_strlen(basePath);

	if(sizeof(rdxChar) == 1)
	{
		// ASCII paths must actually be ASCII
		for(rdxLargeUInt i=0;i<pathLen;i++)
			if(basePath[i] & 0x80)
				return RDX_CNULL;
	}

	if(!rdxCheckAddOverflowU(pathLen, prefixSize + 1))
		return RDX_CNULL;
	wchar_t *exPath = new wchar_t[pathLen + prefixSize + 1];
	exPath[0] = exPath[1] = exPath[3] = '\\';
	exPath[2] = '?';
	exPath[pathLen + prefixSize] = 0;
	for(rdxLargeUInt i=0;i<pathLen;i++)
	{
		rdxChar c = basePath[i];
		if(c == '/')
			c = '\\';
		exPath[i + prefixSize] = static_cast<wchar_t>(c);
	}
	return exPath;
}

rdxIFileStream *rdxWin32FileSystem::Open(const rdxChar *path, bool write)
{
	wchar_t *uniPath = CreateUniPath(path);

	// DO NOT COMMIT - Use uniPath
	FILE *f = SYS_fopen(path, write ? RDX_STATIC_STRING("wb") : RDX_STATIC_STRING("rb"));
	delete[] uniPath;

	if(!f)
		return NULL;
	rdxWin32FileStream *fs = static_cast<rdxWin32FileStream *>(rdxWin32Realloc(NULL, NULL, sizeof(rdxWin32FileStream), rdxAlignOf(rdxWin32FileStream)));
	if(!fs)
	{
		fclose(f);
		return NULL;
	}
	new (fs) rdxWin32FileStream(f);

	return fs;
}

#if 0
rdxIFileScanState *rdxWin32FileSystem::ScanDirectory(const rdxChar *directory, const rdxChar *wildcard, const rdxChar *vext)
{
	rdxWin32FileScanState *fss = static_cast<Win32FileScanState *>(Realloc(NULL, NULL, sizeof(Win32FileScanState), RDX_ALIGNOF(Win32FileScanState)));
	if(!fss)
		return NULL;
	new (fss) Win32FileScanState(directory, wildcard, vext);
	return fss;
}
#endif

