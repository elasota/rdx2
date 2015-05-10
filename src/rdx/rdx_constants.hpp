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
#ifndef __RDX_CONSTANTS_HPP__
#define __RDX_CONSTANTS_HPP__

#include "rdx_basictypes.hpp"
#include "rdx_longflow.hpp"
#include "rdx_io.hpp"

RDX_UTIL_DYNLIB_API void rdxDecodeString_S64(const rdxChar *s, rdxInt64 &i);
RDX_UTIL_DYNLIB_API void rdxDecodeString_S32(const rdxChar *s, rdxInt32 &i);
RDX_UTIL_DYNLIB_API void rdxDecodeString_S16(const rdxChar *s, rdxInt16 &i);
RDX_UTIL_DYNLIB_API void rdxDecodeString_S8(const rdxChar *s, rdxInt8 &i);
RDX_UTIL_DYNLIB_API void rdxDecodeString_U64(const rdxChar *s, rdxUInt64 &i);
RDX_UTIL_DYNLIB_API void rdxDecodeString_U32(const rdxChar *s, rdxUInt32 &i);
RDX_UTIL_DYNLIB_API void rdxDecodeString_U16(const rdxChar *s, rdxUInt16 &i);
RDX_UTIL_DYNLIB_API void rdxDecodeString_U8(const rdxChar *s, rdxUInt8 &i);
RDX_UTIL_DYNLIB_API void rdxDecodeString_F32(const rdxChar *s, rdxFloat32 &f);
RDX_UTIL_DYNLIB_API void rdxDecodeString_F64(const rdxChar *s, rdxFloat64 &f);
RDX_UTIL_DYNLIB_API void rdxDecodeString_WC(const rdxChar *s, wchar_t &f);
RDX_UTIL_DYNLIB_API int rdxEncodeString_S64(rdxChar *s, rdxInt64 i);
RDX_UTIL_DYNLIB_API int rdxEncodeString_S32(rdxChar *s, rdxInt32 i);
RDX_UTIL_DYNLIB_API int rdxEncodeString_S16(rdxChar *s, rdxInt16 i);
RDX_UTIL_DYNLIB_API int rdxEncodeString_U8(rdxChar *s, rdxUInt8 i);
RDX_UTIL_DYNLIB_API int rdxEncodeString_U16(rdxChar *s, rdxUInt16 i);
RDX_UTIL_DYNLIB_API int rdxEncodeString_U32(rdxChar *s, rdxUInt32 i);
RDX_UTIL_DYNLIB_API int rdxEncodeString_U64(rdxChar *s, rdxUInt64 i);
RDX_UTIL_DYNLIB_API int rdxEncodeString_F32(rdxChar *s, rdxFloat32 f);
RDX_UTIL_DYNLIB_API int rdxEncodeString_F64(rdxChar *s, rdxFloat64 f);

inline void rdxDecodeString(const rdxChar *s, rdxInt64 &i) { rdxDecodeString_S64(s, i); }
inline void rdxDecodeString(const rdxChar *s, rdxInt32 &i) { rdxDecodeString_S32(s, i); }
inline void rdxDecodeString(const rdxChar *s, rdxInt16 &i) { rdxDecodeString_S16(s, i); }
inline void rdxDecodeString(const rdxChar *s, rdxInt8 &i) { rdxDecodeString_S8(s, i); }
inline void rdxDecodeString(const rdxChar *s, rdxUInt64 &i) { rdxDecodeString_U64(s, i); }
inline void rdxDecodeString(const rdxChar *s, rdxUInt32 &i) { rdxDecodeString_U32(s, i); }
inline void rdxDecodeString(const rdxChar *s, rdxUInt16 &i) { rdxDecodeString_U16(s, i); }
inline void rdxDecodeString(const rdxChar *s, rdxUInt8 &i) { rdxDecodeString_U8(s, i); }
inline void rdxDecodeString(const rdxChar *s, rdxFloat32 &f) { rdxDecodeString_F32(s, f); }
inline void rdxDecodeString(const rdxChar *s, rdxFloat64 &f) { rdxDecodeString_F64(s, f); }
#ifdef RDX_WCHAR_T_IS_NATIVE
	inline void rdxDecodeString(const rdxChar *s, wchar_t &wc) { rdxDecodeString_WC(s, wc); }
#endif
inline int rdxEncodeString(rdxChar *s, rdxInt64 i) { return rdxEncodeString_S64(s, i); }
inline int rdxEncodeString(rdxChar *s, rdxInt32 i) { return rdxEncodeString_S32(s, i); }
inline int rdxEncodeString(rdxChar *s, rdxInt16 i) { return rdxEncodeString_S16(s, i); }
inline int rdxEncodeString(rdxChar *s, rdxUInt8 i) { return rdxEncodeString_U8(s, i); }
inline int rdxEncodeString(rdxChar *s, rdxUInt16 i) { return rdxEncodeString_U16(s, i); }
inline int rdxEncodeString(rdxChar *s, rdxUInt32 i) { return rdxEncodeString_U32(s, i); }
inline int rdxEncodeString(rdxChar *s, rdxUInt64 i) { return rdxEncodeString_U64(s, i); }
inline int rdxEncodeString(rdxChar *s, rdxFloat32 f) { return rdxEncodeString_F32(s, f); }
inline int rdxEncodeString(rdxChar *s, rdxFloat64 f) { return rdxEncodeString_F64(s, f); }

RDX_UTIL_DYNLIB_API void rdxDecomposeDouble(rdxFloat64 f, rdxInt64 &frac, rdxInt32 &x);
RDX_UTIL_DYNLIB_API void rdxDecomposeFloat(rdxFloat32 f, rdxInt32 &frac, rdxInt32 &x);
RDX_UTIL_DYNLIB_API rdxFloat32 rdxRecomposeFloat(rdxInt32 frac, rdxInt32 x);
RDX_UTIL_DYNLIB_API rdxFloat64 rdxRecomposeDouble(rdxInt64 frac, rdxInt32 x);
inline void rdxRecomposeVariant(rdxInt64 frac, rdxInt32 x, rdxFloat32 &out) { out = rdxRecomposeFloat(static_cast<rdxInt32>(frac), x); }
inline void rdxRecomposeVariant(rdxInt64 frac, rdxInt32 x, rdxFloat64 &out) { out = rdxRecomposeDouble(frac, x); }

template<class Tinternal, class Tstored>
class rdxCNumericTypeSerializer : public rdxITypeSerializer
{
	void DeserializeTextInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxITextDeserializer *td,
		rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const RDX_OVERRIDE
	{
		outShouldProcessProperties = false;

		RDX_TRY(ctx)
		{
			Tinternal v;
			rdxITextDeserializer::SCompactToken token;
			bool isString;
			RDX_PROTECT(ctx, td->ParseToken(ctx, objm, &isString, &token));
			const rdxChar *charData = token.GetCharSpan().Chars();
			rdxDecodeString(charData, v);
			*(instance.StaticCast<Tinternal>().Modify()) = v;
		}
		RDX_CATCH(ctx)
		{
			RDX_RETHROW(ctx);
		}
		RDX_ENDTRY
	}

	void DeserializeBinaryInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl obj, rdxIFileStream *reader, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const RDX_OVERRIDE
	{
		outShouldProcessProperties = false;

		bool overflowed = false;
		bool readFailed = false;
		if(!reader->ReadConverted<Tstored, Tinternal>(obj.StaticCast<Tinternal>().Modify(), overflowed, readFailed))
		{
			if(overflowed)
				RDX_LTHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
			if(readFailed)
				RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		}
	}

	void SerializeBinaryInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const RDX_OVERRIDE
	{
		outShouldProcessProperties = false;
		fs->WriteConverted<Tinternal, Tstored>(*obj.StaticCast<Tinternal>().Data());
	}

	void SerializeTextInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const RDX_OVERRIDE
	{
		outShouldProcessProperties = false;

		rdxChar chars[RDX_MAX_ENCODED_NUMBER_SIZE];
		rdxUInt8 encoded[RDX_MAX_ENCODED_NUMBER_SIZE];

		rdxEncodeString(chars, *obj.StaticCast<Tinternal>().Data());
		rdxLargeUInt len;
		for(len=0;chars[len];len++)
			encoded[len] = static_cast<rdxUInt8>(chars[len]);
		fs->WriteBytes(" ", 1);
		fs->WriteBytes(encoded, len);
	}

	static rdxCNumericTypeSerializer instance;
};

#endif
