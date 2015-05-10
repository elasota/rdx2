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
#include "rdx_basictypes.hpp"
#include "rdx_constants.hpp"

template<class Tchar, class Tint>
inline int rdxAToLL(const Tchar *str, Tint &out)
{
	const Tchar *baseStr = str;

	bool negative = false;
	if(str[0] == '-')
	{
		negative = true;
		str++;
	}
	Tint rv = 0;
	while(str[0] >= '0' && str[0] <= '9')
	{
		if(negative)
			rv = static_cast<Tint>(rv * 10 - (str[0] - '0'));
		else
			rv = static_cast<Tint>(rv * 10 + (str[0] - '0'));
		str++;
	}
	out = rv;
	return str - baseStr;
}

template<class Tchar, class Tint>
inline int rdxIntToString(Tchar *out, Tint v)
{
	Tchar backstack[50];
	int digits = 0;
	int chars = 0;
	bool negative = false;

	if(v < 0)
	{
		*out++ = '-';
		chars++;
		negative = true;
	}

	do
	{
		if(negative)
			backstack[digits++] = static_cast<Tchar>('0' - (v % static_cast<Tint>(10)));
		else
			backstack[digits++] = static_cast<Tchar>('0' + (v % static_cast<Tint>(10)));
		v = static_cast<Tint>(v / static_cast<Tint>(10));
	} while(v);

	while(digits)
	{
		*out++ = backstack[--digits];
		chars++;
	}

	*out = '\0';

	return chars;
}

template<class Tchar, class Tfrac, class Tx>
inline int rdxDecomposedToString(Tchar *out, Tfrac frac, Tx x)
{
	int nChars = 0;
	nChars += rdxIntToString<Tchar, Tfrac>(out, frac);
	if(x)
	{
		out[nChars++] = '^';
		nChars += rdxIntToString<Tchar, Tx>(out + nChars, x);
	}
	out[nChars] = static_cast<Tchar>('\0');
	return nChars;
}

template<class Tchar, class Tfrac, class Tx>
inline void rdxDecomposeString(const Tchar *str, Tfrac &frac, Tx &x)
{
	str += rdxAToLL<Tchar, Tfrac>(str, frac);

	if(*str == '^')
	{
		str++;
		rdxAToLL<Tchar, Tx>(str, x);
	}
	else
		x = 0;
}

RDX_UTIL_DYNLIB_API void rdxDecomposeFloat(rdxFloat32 f, rdxInt32 &frac, rdxInt32 &x)
{
	union
	{
		rdxFloat32 f;
		rdxInt32 i;
	} u;

	u.f = f;
	if(u.i == 0)
	{
		frac = 0;
		x = 0;
		return;
	}

	rdxInt32 baseFrac = u.i & 0x7FFFFF;
	baseFrac |= 0x800000;
	rdxInt32 baseX = (u.i & 0x7F800000) >> 23;

	while(!(baseFrac & 1))
	{
		baseX++;
		baseFrac >>= 1;
	}

	x = baseX - 150;

	if((u.i & 0x80000000) != 0)
		baseFrac = -baseFrac;
	frac = baseFrac;
}

RDX_UTIL_DYNLIB_API rdxFloat32 rdxRecomposeFloat(rdxInt32 frac, rdxInt32 x)
{
	union
	{
		rdxFloat32 f;
		rdxInt32 i;
	} u;

	if(frac == 0)
		return 0;

	bool sign = false;
	if(frac < 0)
	{
		sign = true;
		frac = -frac;
	}

	x += 150;

	while(!(frac & 0x800000))
	{
		x--;
		frac <<= 1;
	}

	frac &= 0x7FFFFF;
	u.i = frac | (x << 23);
	if(sign)
		u.i |= 0x80000000;

	return u.f;
}

RDX_UTIL_DYNLIB_API void rdxDecomposeDouble(rdxFloat64 f, rdxInt64 &frac, rdxInt32 &x)
{
	union
	{
		rdxFloat64 f;
		rdxInt64 i;
	} u;

	u.f = f;
	if(u.i == 0)
	{
		frac = 0;
		x = 0;
		return;
	}

	rdxInt64 baseFrac = u.i & 0xFFFFFFFFFFFFFLL;
	baseFrac |= 0x10000000000000LL;
	rdxInt32 baseX = static_cast<rdxInt32>( (u.i & 0x7FF0000000000000LL) >> 52LL);

	while(!(baseFrac & 1))
	{
		baseX++;
		baseFrac >>= 1LL;
	}

	x = baseX - 1075;

	if((u.i & 0x8000000000000000LL) != 0)
		baseFrac = -baseFrac;
	frac = baseFrac;
}

RDX_UTIL_DYNLIB_API rdxFloat64 rdxRecomposeDouble(rdxInt64 frac, rdxInt32 x)
{
	union
	{
		rdxFloat64 f;
		rdxInt64 i;
	} u;

	x += 1075;

	bool sign = false;
	if(frac < 0LL)
	{
		sign = true;
		frac = -frac;
	}

	while(!(frac & 0x10000000000000LL))
	{
		x--;
		frac <<= 1LL;
	}

	frac &= 0xFFFFFFFFFFFFFLL;
	u.i = frac | (static_cast<rdxInt64>(x) << 52LL);
	if(sign)
		u.i |= 0x8000000000000000LL;
	
	return u.f;
}

RDX_UTIL_DYNLIB_API void rdxDecodeString_S8(const rdxChar *s, rdxInt8 &i)
{
	rdxAToLL<rdxChar, rdxInt8>(s, i);
}

RDX_UTIL_DYNLIB_API void rdxDecodeString_S16(const rdxChar *s, rdxInt16 &i)
{
	rdxAToLL<rdxChar, rdxInt16>(s, i);
}

RDX_UTIL_DYNLIB_API void rdxDecodeString_S32(const rdxChar *s, rdxInt32 &i)
{
	rdxAToLL<rdxChar, rdxInt32>(s, i);
}

RDX_UTIL_DYNLIB_API void rdxDecodeString_S64(const rdxChar *s, rdxInt64 &i)
{
	rdxAToLL<rdxChar, rdxInt64>(s, i);
}

RDX_UTIL_DYNLIB_API void rdxDecodeString_WC(const rdxChar *s, wchar_t &i)
{
	rdxAToLL<rdxChar, wchar_t>(s, i);
}

RDX_UTIL_DYNLIB_API void rdxDecodeString_U8(const rdxChar *s, rdxUInt8 &i)
{
	rdxAToLL<rdxChar, rdxUInt8>(s, i);
}

RDX_UTIL_DYNLIB_API void rdxDecodeString_U16(const rdxChar *s, rdxUInt16 &i)
{
	rdxAToLL<rdxChar, rdxUInt16>(s, i);
}

RDX_UTIL_DYNLIB_API void rdxDecodeString_U32(const rdxChar *s, rdxUInt32 &i)
{
	rdxAToLL<rdxChar, rdxUInt32>(s, i);
}

RDX_UTIL_DYNLIB_API void rdxDecodeString_U64(const rdxChar *s, rdxUInt64 &i)
{
	rdxAToLL<rdxChar, rdxUInt64>(s, i);
}

RDX_UTIL_DYNLIB_API void rdxDecodeString_F32(const rdxChar *s, rdxFloat32 &f)
{
	rdxInt32 frac,x;
	rdxDecomposeString<rdxChar, rdxInt32, rdxInt32>(s, frac, x);
	f = rdxRecomposeFloat(frac, x);
}

RDX_UTIL_DYNLIB_API void rdxDecodeString_F64(const rdxChar *s, rdxFloat64 &f)
{
	rdxInt64 frac;
	rdxInt32 x;
	rdxDecomposeString<rdxChar, rdxInt64, rdxInt32>(s, frac, x);
	f = rdxRecomposeDouble(frac, x);
}

RDX_UTIL_DYNLIB_API int rdxEncodeString_S64(rdxChar *s, rdxInt64 i)
{
	return rdxIntToString<rdxChar, rdxInt64>(s, i);
}

RDX_UTIL_DYNLIB_API int rdxEncodeString_S32(rdxChar *s, rdxInt32 i)
{
	return rdxIntToString<rdxChar, rdxInt32>(s, i);
}

RDX_UTIL_DYNLIB_API int rdxEncodeString_S16(rdxChar *s, rdxInt16 i)
{
	return rdxIntToString<rdxChar, rdxInt16>(s, i);
}

RDX_UTIL_DYNLIB_API int rdxEncodeString_U64(rdxChar *s, rdxUInt64 i)
{
	return rdxIntToString<rdxChar, rdxUInt64>(s, i);
}

RDX_UTIL_DYNLIB_API int rdxEncodeString_U32(rdxChar *s, rdxUInt32 i)
{
	return rdxIntToString<rdxChar, rdxUInt32>(s, i);
}

RDX_UTIL_DYNLIB_API int rdxEncodeString_U16(rdxChar *s, rdxUInt16 i)
{
	return rdxIntToString<rdxChar, rdxUInt16>(s, i);
}

RDX_UTIL_DYNLIB_API int rdxEncodeString_U8(rdxChar *s, rdxUInt8 i)
{
	return rdxIntToString<rdxChar, rdxUInt8>(s, i);
}

RDX_UTIL_DYNLIB_API int rdxEncodeString_F32(rdxChar *s, rdxFloat32 f)
{
	rdxInt32 frac,x;
	rdxDecomposeFloat(f, frac, x);
	return rdxDecomposedToString<rdxChar, rdxInt32, rdxInt32>(s, frac, x);
}

RDX_UTIL_DYNLIB_API int rdxEncodeString_F64(rdxChar *s, rdxFloat64 f)
{
	rdxInt64 frac;
	rdxInt32 x;
	rdxDecomposeDouble(f, frac, x);
	return rdxDecomposedToString<rdxChar, rdxInt64, rdxInt32>(s, frac, x);
}
