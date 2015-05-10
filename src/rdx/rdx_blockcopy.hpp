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
#ifndef __RDX_BLOCKCOPY_HPP__
#define __RDX_BLOCKCOPY_HPP__

#include <string.h>
#include "rdx_basictypes.hpp"
#include "rdx_pragmas.hpp"

#pragma intrinsic(memcpy)

struct rdxAddressAliasMemIO
{
	static inline void Put(void *dest, const rdxLargeInt src)
	{
		typedef rdxLargeInt RDX_MAY_ALIAS rdxAliasableLargeInt;
		*static_cast<rdxAliasableLargeInt *>(dest) = src;
	}

	static inline rdxLargeInt Read(const void *src)
	{
		typedef rdxLargeInt RDX_MAY_ALIAS rdxAliasableLargeInt;
		rdxAliasableLargeInt result = *static_cast<const rdxAliasableLargeInt *>(src);
		return static_cast<rdxLargeInt>(result);
	}
};

template<class _T>
struct rdxSimpleMemIO
{
	static inline void Put(void *dest, const _T src)
	{
		*static_cast<_T *>(dest) = src;
	}

	static inline _T Read(const void *src)
	{
		return *static_cast<const _T *>(src);
	}
};

template<class _T, class _Tcopier>
inline void rdxArrayCopy(_T *dest, const _T *src, rdxLargeUInt numUnits)
{
	while(numUnits--)
		_Tcopier::Put(dest++, _Tcopier::Read(src++));
}

template<class _T, class _Tcopier>
inline void rdxArrayCopyDown(_T *dest, const _T *src, rdxLargeUInt numUnits)
{
	dest += numUnits;
	src += numUnits;
	while(numUnits--)
		_Tcopier::Put(--dest, _Tcopier::Read(--src));
}

inline bool rdxPtrAligned(const void *ptr, rdxLargeUInt alignment)
{
	return ((static_cast<const rdxUInt8 *>(ptr) - static_cast<const rdxUInt8 *>(NULL)) % alignment == 0);
}

inline void rdxBlockCopyUp(void *dest, const void *src, rdxLargeUInt sz)
{
	if(sz % sizeof(rdxLargeInt) == 0 && rdxPtrAligned(dest, sizeof(rdxLargeInt)) && rdxPtrAligned(src, sizeof(rdxLargeInt)))
		rdxArrayCopy<rdxLargeInt, rdxAddressAliasMemIO>(static_cast<rdxLargeInt *>(dest), static_cast<const rdxLargeInt *>(src), sz / sizeof(rdxLargeInt));
	else
		rdxArrayCopy<rdxUInt8, rdxSimpleMemIO<rdxUInt8> >(static_cast<rdxUInt8 *>(dest), static_cast<const rdxUInt8 *>(src), sz);
}

inline void rdxBlockCopyDown(void *dest, const void *src, rdxLargeUInt sz)
{
	if(sz % sizeof(rdxLargeInt) == 0 && rdxPtrAligned(dest, sizeof(rdxLargeInt)) && rdxPtrAligned(src, sizeof(rdxLargeInt)))
		rdxArrayCopyDown<rdxLargeInt, rdxAddressAliasMemIO>(static_cast<rdxLargeInt *>(dest), static_cast<const rdxLargeInt *>(src), sz / sizeof(rdxLargeInt));
	else
		rdxArrayCopyDown<rdxUInt8, rdxSimpleMemIO<rdxUInt8> >(static_cast<rdxUInt8 *>(dest), static_cast<const rdxUInt8 *>(src), sz);
}

inline void rdxBlockMove(void *dest, const void *src, rdxLargeUInt sz)
{
	if(dest == src)
		return;
	void *destEnd = static_cast<rdxUInt8 *>(dest) + sz;
	const void *srcEnd = static_cast<const rdxUInt8 *>(src) + sz;

	if(dest < src || src >= destEnd || dest >= srcEnd)
		rdxBlockCopyUp(dest, src, sz);
	else
		rdxBlockCopyDown(dest, src, sz);
}

inline void rdxBlockCopy(void *dest, const void *src, rdxLargeUInt sz)
{
	rdxUInt8 *destBytes = static_cast<rdxUInt8*>(dest);
	const rdxUInt8 *srcBytes = static_cast<const rdxUInt8*>(src);

	rdxLargeUInt destHeadSpace = static_cast<rdxLargeUInt>(destBytes - reinterpret_cast<const rdxUInt8*>(RDX_CNULL)) % sizeof(rdxLargeUInt);
	rdxLargeUInt srcHeadSpace = static_cast<rdxLargeUInt>(srcBytes - reinterpret_cast<const rdxUInt8*>(RDX_CNULL)) % sizeof(rdxLargeUInt);
	if(destHeadSpace != srcHeadSpace || destHeadSpace > sz)
	{
		for(rdxLargeUInt i=0;i<sz;i++)
		{
			destBytes[i] = srcBytes[i];
		}
	}
	else
	{
		rdxLargeUInt headSpace = destHeadSpace;
		if(headSpace)
		{
			for(rdxLargeUInt i=0;i<headSpace;i++)
			{
				destBytes[i] = srcBytes[i];
			}
			destBytes += headSpace;
			srcBytes += headSpace;
			sz -= headSpace;
		}
		rdxLargeUInt numAddrs = sz / sizeof(rdxLargeUInt);

		// Copy addr-size blocks
		typedef rdxLargeUInt RDX_MAY_ALIAS TAliasableAddr;

		TAliasableAddr *destAddrs = reinterpret_cast<TAliasableAddr*>(destBytes);
		const TAliasableAddr *srcAddrs = reinterpret_cast<const TAliasableAddr*>(srcBytes);
		for(rdxLargeUInt i=0;i<numAddrs;i++)
		{
			destAddrs[i] = srcAddrs[i];
		}
		
		sz -= numAddrs * sizeof(rdxLargeUInt);
		if(sz)
		{
			destBytes += numAddrs * sizeof(rdxLargeUInt);
			srcBytes += numAddrs * sizeof(rdxLargeUInt);
			for(rdxLargeUInt i=0;i<sz;i++)
			{
				destBytes[i] = srcBytes[i];
			}
		}
	}
}

#endif
