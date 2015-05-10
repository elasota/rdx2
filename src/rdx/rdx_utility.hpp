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
#ifndef __RDX_UTILITY_HPP__
#define __RDX_UTILITY_HPP__

#include <cstddef>

#include "rdx_coretypes.hpp"
#include "rdx_breakcause.hpp"
#include "rdx_api.hpp"

struct rdxSOperationContext;

void rdxDebugBreak(rdxBreakCause cause);
rdxHashValue rdxHashBytes(const void *bytes, rdxLargeUInt sz);
rdxLargeUInt rdxHashValueIndex(rdxHashValue hv, rdxLargeUInt maximum);
rdxLargeUInt rdxEncodeUTF8Char(rdxChar c, rdxUInt8 *output);
rdxChar rdxDecodeUTF8Char(const void **pData, rdxLargeUInt *pAvailableBytes);
template<class T> T rdxMax(const T &a, const T &b);
rdxLargeUInt rdxFieldOffset(const void *base, const void *field);
bool rdxCheckAddOverflowS(const rdxLargeInt &a, const rdxLargeInt &b);
bool rdxCheckAddOverflowU(const rdxLargeUInt &a, const rdxLargeUInt &b);
bool rdxCheckMulOverflowS(const rdxLargeInt &a, const rdxLargeInt &b);
bool rdxCheckMulOverflowU(const rdxLargeUInt &a, const rdxLargeUInt &b);
rdxLargeUInt rdxPaddedSize(rdxLargeUInt v, rdxLargeUInt alignment, bool &overflowed);
rdxLargeUInt rdxPaddedSize(rdxLargeUInt v, rdxLargeUInt alignment);
rdxLargeUInt rdxStripSign(rdxSOperationContext *ctx, rdxLargeInt v);
rdxLargeInt rdxMakeSigned(rdxSOperationContext *ctx, rdxLargeUInt v);

#include "rdx_pragmas.hpp"

/*
#include "rdx_platform.hpp"
#include "rdx_coretypes.hpp"
#include "rdx_errorcodes.hpp"
*/

inline void rdxDebugBreak(rdxBreakCause cause)
{
	__asm { int 3 }
}

class rdxXAPI_Murmur3Hash_Shim;
class rdxCMurmur3Hash128;
class rdxCMurmur3Hash32;
class rdxXAPI_CRC32_Shim;
class rdxCCRC32Generator;
class rdxXAPI_SHA256_Shim;
class rdxCSHA256Generator;

RDX_UTIL_DYNLIB_API void rdxXAPI_Murmur3Hash128_Init(rdxCMurmur3Hash128 *v);
RDX_UTIL_DYNLIB_API void rdxXAPI_Murmur3Hash128_FeedBytes(rdxCMurmur3Hash128 *v, const void *bytes, rdxLargeUInt sz);
RDX_UTIL_DYNLIB_API void rdxXAPI_Murmur3Hash128_Flush(rdxCMurmur3Hash128 *v, rdxUInt64 *outHigh, rdxUInt64 *outLow);
RDX_UTIL_DYNLIB_API void rdxXAPI_Murmur3Hash32_Init(rdxCMurmur3Hash32 *v);
RDX_UTIL_DYNLIB_API void rdxXAPI_Murmur3Hash32_FeedBytes(rdxCMurmur3Hash32 *v, const void *bytes, rdxLargeUInt sz);
RDX_UTIL_DYNLIB_API rdxUInt32 rdxXAPI_Murmur3Hash32_Flush(rdxCMurmur3Hash32 *v);
RDX_UTIL_DYNLIB_API void rdxXAPI_CRC32_Init(rdxCCRC32Generator *v);
RDX_UTIL_DYNLIB_API void rdxXAPI_CRC32_FeedBytes(rdxCCRC32Generator *v, const void *bytes, rdxLargeUInt sz);
RDX_UTIL_DYNLIB_API rdxUInt32 rdxXAPI_CRC32_Flush(rdxCCRC32Generator *v);
RDX_UTIL_DYNLIB_API void rdxXAPI_SHA256_Init(rdxCSHA256Generator *v);
RDX_UTIL_DYNLIB_API void rdxXAPI_SHA256_FeedBytes(rdxCSHA256Generator *v, const void *bytes, rdxLargeUInt sz);
RDX_UTIL_DYNLIB_API void rdxXAPI_SHA256_Flush(rdxCSHA256Generator *v, rdxUInt8 *out);


class rdxCMurmur3Hash128
{
	friend class rdxXAPI_Murmur3Hash_Shim;

public:
	RDX_FORCEINLINE rdxCMurmur3Hash128() { rdxXAPI_Murmur3Hash128_Init(this); }
	RDX_FORCEINLINE void FeedBytes(const void *bytes, rdxLargeUInt sz) { rdxXAPI_Murmur3Hash128_FeedBytes(this, bytes, sz); }
	RDX_FORCEINLINE void Flush(rdxUInt64 *outHigh, rdxUInt64 *outLow) { rdxXAPI_Murmur3Hash128_Flush(this, outHigh, outLow); }

private:
	rdxUInt32 m_h1;
	rdxUInt32 m_h2;
	rdxUInt32 m_h3;
	rdxUInt32 m_h4;
	rdxUInt32 m_len;
	rdxUInt8 m_backlog[16];
	rdxUInt8 m_backlogPos;
};

class rdxCMurmur3Hash32
{
	friend class rdxXAPI_Murmur3Hash_Shim;

public:
	RDX_FORCEINLINE rdxCMurmur3Hash32() { rdxXAPI_Murmur3Hash32_Init(this); }
	RDX_FORCEINLINE void FeedBytes(const void *bytes, rdxLargeUInt sz) { rdxXAPI_Murmur3Hash32_FeedBytes(this, bytes, sz); }
	RDX_FORCEINLINE rdxUInt32 Flush() { return rdxXAPI_Murmur3Hash32_Flush(this); }

private:
	rdxUInt32 m_h1;
	rdxUInt32 m_len;
	rdxUInt8 m_backlog[4];
	rdxUInt8 m_backlogPos;
};

class rdxCCRC32Generator
{
	friend class rdxXAPI_CRC32_Shim;

public:
	RDX_FORCEINLINE rdxCCRC32Generator() { rdxXAPI_CRC32_Init(this); }
	RDX_FORCEINLINE void FeedBytes(const void *ptr, rdxLargeUInt sz) { rdxXAPI_CRC32_FeedBytes(this, ptr, sz); }
	RDX_FORCEINLINE rdxUInt32 Flush() { return rdxXAPI_CRC32_Flush(this); }

private:
	rdxUInt32 m_crc;
};

class rdxCSHA256Generator
{
	friend class rdxXAPI_SHA256_Shim;

public:
	static const rdxLargeUInt OUTPUT_SIZE_BYTES = 32;

	RDX_FORCEINLINE rdxCSHA256Generator() { rdxXAPI_SHA256_Init(this); }
	RDX_FORCEINLINE void FeedBytes(const void *ptr, rdxLargeUInt sz) { rdxXAPI_SHA256_FeedBytes(this, ptr, sz); }
	RDX_FORCEINLINE void Flush(rdxUInt8 output[32]) { rdxXAPI_SHA256_Flush(this, output); }

private:
	rdxUInt32 m_state[8];
	rdxUInt32 m_seeds[64];
	rdxLargeUInt m_seedOffset;
	rdxLargeUInt m_messageLength;

};

// Generic hash funcs
class rdxCIntermediateHash : public rdxCMurmur3Hash32
{
public:
	inline rdxCIntermediateHash()
		: rdxCMurmur3Hash32()
	{
	}

	inline rdxHashValue Flush()
	{
		return static_cast<rdxHashValue>(rdxCMurmur3Hash32::Flush());
	}
};

inline rdxHashValue rdxHashBytes(const void *bytes, size_t sz)
{
	rdxCIntermediateHash ihash;
	ihash.FeedBytes(bytes, sz);
	return ihash.Flush();
}

inline rdxLargeUInt rdxHashValueIndex(rdxHashValue hv, rdxLargeUInt maximum)
{
	return static_cast<rdxLargeUInt>(static_cast<rdxLargeUInt>(hv) % static_cast<rdxLargeUInt>(maximum));
}

template<class _T>
inline _T rdxSwapValue(_T v)
{
	if(sizeof(_T) == 1)
		return v;

	union
	{
		_T v;
		rdxUInt8 bytes[1];
	} u;

	const rdxUInt8 *inBytes = reinterpret_cast<const rdxUInt8*>(&v);
	for(rdxLargeUInt i=0;i<sizeof(_T);i++)
		u.bytes[sizeof(_T)-1-i] = inBytes[i];
	return u.v;
}

inline rdxLargeUInt rdxEncodeUTF8Char(rdxChar c, rdxUInt8 *output)
{
	rdxUInt32 u32char = (static_cast<rdxUInt32>(c) & (static_cast<rdxUInt32>(0xffffffff) >> (8*(4-sizeof(rdxChar)))));
	u32char &= 0x7fffffff;	// Top bit being set is invalid

	rdxLargeUInt extraBytes = 0;
	rdxUInt8 firstByte;

	if(u32char & 0xfc000000)		// 31
	{
		firstByte = static_cast<rdxUInt8>(0xfc | (u32char >> 30));
		extraBytes = 5;
	}
	else if(u32char & 0xffe00000)	// 26
	{
		firstByte = static_cast<rdxUInt8>(0xf8 | (u32char >> 24));
		extraBytes = 4;
	}
	else if(u32char & 0xffff0000)	// 21
	{
		firstByte = static_cast<rdxUInt8>(0xf0 | (u32char >> 18));
		extraBytes = 3;
	}
	else if(u32char & 0xfffff800)	// 16
	{
		firstByte = static_cast<rdxUInt8>(0xe0 | (u32char >> 12));
		extraBytes = 2;
	}
	else if(u32char & 0xffffff80)	// 11
	{
		firstByte = static_cast<rdxUInt8>(0xc0 | (u32char >> 6));
		extraBytes = 1;
	}
	else
	{
		firstByte = static_cast<rdxUInt8>(u32char);
		extraBytes = 0;
	}

	output[0] = firstByte;

	rdxLargeUInt bitOffset = (extraBytes-1)*6;
	for(rdxLargeUInt i=0;i<extraBytes;i++,bitOffset-=6)
		output[i+1] = ((u32char >> bitOffset) & 0x3f) | 0x80;

	return extraBytes + 1;
}

inline rdxChar rdxDecodeUTF8Char(const void **pData, rdxLargeUInt *pAvailableBytes)
{
#ifdef RDX_UNICODE_DISABLE_UTF8
	(*pAvailableBytes)--;
	const char *chars = *reinterpret_cast<const char *>(*pData);
	*pData = chars + 1;
	return static_cast<rdxChar>(*chars);
#else
	rdxLargeUInt availableBytes = *pAvailableBytes;
	const rdxUInt8 *bytes = reinterpret_cast<const rdxUInt8 *>(*pData);

	rdxUInt8 firstByte = bytes[0];
	availableBytes--;

	rdxLargeUInt extraBytes;
	rdxUInt32 enforceMask = 0;

	if ((firstByte & 0x80) == 0)			// 7
	{
		extraBytes = 0;
		firstByte &= 0x7f;
		enforceMask = 0x7f;
	}
	else if ((firstByte & 0xe0) == 0xc0)	// 11
	{
		extraBytes = 1;
		firstByte &= 0x1f;
		enforceMask = 0x7e0;
	}
	else if ((firstByte & 0xf0) == 0xe0)	// 16
	{
		extraBytes = 2;
		firstByte &= 0xf;
		enforceMask = 0xfc00;
	}
	else if ((firstByte & 0xf8) == 0xf0)	// 21
	{
		extraBytes = 3;
		firstByte &= 0x7;
		enforceMask = 0x1f8000;
	}
	else
		return rdxCHAR_Invalid;

	if (availableBytes < extraBytes)
		return rdxCHAR_Invalid;

	rdxUInt32 finalValue = static_cast<rdxUInt32>(firstByte << (extraBytes*6));
	rdxLargeUInt bitOffset = (extraBytes-1)*6;

	for(rdxLargeUInt i=0;i<extraBytes;i++,bitOffset-=6)
	{
		rdxUInt8 v = bytes[i+1];
		if((v & 0xc0) != 0x80)
			return rdxCHAR_Invalid;
		finalValue |= ((static_cast<rdxUInt32>(v & 0x3f)) << bitOffset);
	}

	if(finalValue > 0x10ffff)
		return rdxCHAR_Invalid;		// Invalid UTF-8
	if(finalValue >= 0xd800 && finalValue <= 0xdfff)
		return rdxCHAR_Invalid;		// UTF-16 surrogates
	if (finalValue >= (1 << (sizeof(rdxChar)*8)))
		return rdxCHAR_Invalid;		// Too big

	*pData = static_cast<const rdxUInt8 *>(*pData) + 1 + extraBytes;
	*pAvailableBytes = (*pAvailableBytes) - 1 - extraBytes;

	return static_cast<rdxChar>(finalValue);
#endif
}

template<class T>
inline T rdxMax(const T &a, const T &b)
{
	return a > b ? a : b;
}

inline rdxLargeUInt rdxFieldOffset(const void *base, const void *field)
{
	return static_cast<rdxLargeUInt>((reinterpret_cast<const rdxUInt8 *>(field)) - (reinterpret_cast<const rdxUInt8 *>(base)));
}

inline bool rdxCheckAddOverflowS(const rdxLargeInt &a, const rdxLargeInt &b)
{
	// Signed
	if(b < 0)
		return (RDX_LARGEINT_MIN - b) <= a;
	return (RDX_LARGEINT_MAX - b) >= a;
}

inline bool rdxCheckAddOverflowU(const rdxLargeUInt &a, const rdxLargeUInt &b)
{
	// Signed
	return (RDX_LARGEUINT_MAX - b) >= a;
}

inline bool rdxCheckMulOverflowS(const rdxLargeInt &a, const rdxLargeInt &b)
{
	if(a == 0 || b == 0)
		return true;
	// Signed
	if(b < 0)
		return (RDX_LARGEINT_MIN / b) <= a;
	return (RDX_LARGEINT_MAX / b) >= a;
}

inline bool rdxCheckMulOverflowU(const rdxLargeUInt &a, const rdxLargeUInt &b)
{
	if(a == 0 || b == 0)
		return true;
	// Signed
	return (RDX_LARGEUINT_MAX / b) >= a;
}

inline rdxLargeUInt rdxPaddedSize(rdxLargeUInt v, rdxLargeUInt alignment, bool &overflowed)
{
	rdxLargeUInt basePad = alignment - 1;
	if(!rdxCheckAddOverflowU(v, basePad))
	{
		overflowed = true;
		return 0;
	}
	v += basePad;
	overflowed = false;
	return v - (v % alignment);
}

inline rdxLargeUInt rdxPaddedSize(rdxLargeUInt v, rdxLargeUInt alignment)
{
	rdxLargeUInt basePad = alignment - 1;
	v += basePad;
	return v - (v % alignment);
}

#ifdef RDX_HASH_MURMUR3
#	include "rdx_murmur3.hpp"
#endif

#include "rdx_longflow.hpp"
#include "rdx_errorcodes.hpp"

inline rdxLargeUInt rdxStripSign(rdxSOperationContext *ctx, rdxLargeInt v)
{
	if(v < 0)
		RDX_LTHROWV(ctx, RDX_ERROR_INTEGER_OVERFLOW, 0);
	return static_cast<rdxLargeUInt>(v);
}

inline rdxLargeInt rdxMakeSigned(rdxSOperationContext *ctx, rdxLargeUInt v)
{
	rdxLargeUInt intMax = (~static_cast<rdxLargeUInt>(0)) / 2;

	if(v > intMax)
		RDX_LTHROWV(ctx, RDX_ERROR_INTEGER_OVERFLOW, 0);
	return static_cast<rdxLargeInt>(v);
}

#endif
