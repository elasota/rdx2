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
#ifndef __RDX_IO_HPP__
#define __RDX_IO_HPP__

#include "rdx_basictypes.hpp"
#include "rdx_objectmanagement.hpp"

struct rdxSCharSpan;

struct rdxIFileScanState
{
	virtual void Close() = 0;
	virtual rdxCRef(rdxCString) NextFile(rdxSOperationContext *ctx, rdxIObjectManager *objm) = 0;
};
		
struct rdxIFileSystem
{
	virtual rdxIFileStream *Open(const rdxChar *path, bool write) = 0;
	//virtual rdxIFileScanState *ScanDirectory(const rdxChar *directory, const rdxChar *wildcard, const rdxChar *vext) = 0;
};

struct rdxIFileStream
{
	virtual rdxLargeUInt WriteBytes(const void *src, rdxLargeUInt numBytes) = 0;
	virtual rdxLargeUInt ReadBytes(void *dest, rdxLargeUInt numBytes) = 0;
	virtual void SeekStart(rdxLargeUInt offset) = 0;
	virtual void SeekEnd(rdxLargeInt offset) = 0;
	virtual void SeekCurrent(rdxLargeInt offset) = 0;
	virtual void SeekForward(rdxLargeUInt offset) = 0;
	virtual rdxLargeUInt Tell() = 0;
	virtual void Close() = 0;
	virtual bool HasAborted() const = 0;
	virtual void Abort() = 0;

	inline static bool ShouldByteSwap()
	{
		rdxUInt32 RDX_BYTE_ORDER_LITTLE_ENDIAN = 1;
		rdxUInt32 RDX_BYTE_ORDER_BIG_ENDIAN = 0x01000000;
		
		union
		{
			rdxUInt32 u32;
			rdxUInt8 u8[4];
		} u;

		u.u32 = 0;
		u.u8[0] = 1;

		return (u.u32 != RDX_BINARY_STORAGE_ORDER);
	}

	inline rdxLargeUInt WriteSwappedBytes(const void *src, rdxLargeUInt numBytes)
	{
		rdxUInt8 buffer[16];
		rdxLargeUInt bufferOffset = 0;
		rdxLargeUInt cumulative = 0;

		const rdxUInt8 *bsrc = reinterpret_cast<const rdxUInt8*>(src) + numBytes;

		for(rdxLargeUInt i=0;i<numBytes;i++)
		{
			bsrc--;
			buffer[bufferOffset++] = *bsrc;
			if(bufferOffset == sizeof(buffer))
			{
				cumulative += this->WriteBytes(buffer, sizeof(buffer));
				bufferOffset = 0;
			}
		}
		if(bufferOffset)
			cumulative += this->WriteBytes(buffer, bufferOffset);

		return cumulative;
	}

	inline rdxLargeUInt ReadSwappedBytes(void *dest, rdxLargeUInt numBytes)
	{
		rdxLargeUInt nRead = this->ReadBytes(dest, numBytes);
		if(nRead == numBytes)
		{
			rdxUInt8 *bytes = reinterpret_cast<rdxUInt8*>(dest);
			rdxLargeUInt halfPoint = nRead / 2;
			for(rdxLargeUInt i=0;i<halfPoint;i++)
			{
				rdxUInt8 temp;
				temp = bytes[i];
				bytes[i] = bytes[nRead-1-i];
				bytes[nRead-1-i] = temp;
			}
		}
		return nRead;
	}

	inline rdxLargeUInt ReadSwappableBytes(void *dest, rdxLargeUInt numBytes)
	{
		if(ShouldByteSwap())
			return ReadSwappedBytes(dest, numBytes);
		else
			return ReadBytes(dest, numBytes);
	}

	rdxLargeUInt WriteSwappableBytes(const void *src, rdxLargeUInt numBytes)
	{
		if(ShouldByteSwap())
			return WriteSwappedBytes(src, numBytes);
		else
			return WriteBytes(src, numBytes);
	}

	template<class _Tfrom, class _Tto>
	inline void WriteConverted(_Tfrom val)
	{
		_Tto binVal = static_cast<_Tto>(val);
		if(static_cast<_Tfrom>(binVal) != val)
			this->Abort();
		else
			this->WriteSwappableBytes(&binVal, sizeof(binVal));
	}

	template<class _Tfrom, class _Tto>
	inline bool ReadConverted(_Tto *valOut, bool &overflowed, bool &readFailed)
	{
		_Tfrom fromBin;
		if(this->ReadSwappableBytes(&fromBin, sizeof(_Tfrom)) != sizeof(_Tfrom))
		{
			readFailed = true;
			return false;
			}
		_Tto toVal = static_cast<_Tto>(fromBin);
		if(static_cast<_Tfrom>(toVal) != fromBin)
		{
			overflowed = true;
			return false;
		}
		*valOut = toVal;
		return true;
	}
};

struct rdxITextDeserializer : public rdxIFileStream
{
	static const rdxLargeUInt COMPACT_TOKEN_SIZE = 128;

	struct SCompactToken
	{
		rdxSCharSpan GetCharSpan() const;
		rdxChar *InitCompactChars(rdxLargeUInt numChars);
		void SetStrChars(rdxWeakRTRef(rdxCString) str);

		SCompactToken();
		SCompactToken(const SCompactToken &other);

	private:
		rdxCRef(rdxCString) m_str;
		bool m_isCompact;
		rdxLargeUInt m_compactSize;
		rdxChar m_compactChars[COMPACT_TOKEN_SIZE+1];
	};

	virtual bool CheckToken(const char *str) = 0;
	virtual void ParseToken(rdxSOperationContext *ctx, rdxIObjectManager *objm, bool *pIsString, SCompactToken *outToken = RDX_CNULL) = 0;
	virtual void SkipToken(rdxSOperationContext *ctx) = 0;
};

#include "rdx_charspan.hpp"

inline rdxITextDeserializer::SCompactToken::SCompactToken()
	: m_isCompact(false)
{
}

inline rdxITextDeserializer::SCompactToken::SCompactToken(const SCompactToken &other)
	: m_str(other.m_str)
	, m_isCompact(other.m_isCompact)
	, m_compactSize(other.m_compactSize)
{
	memcpy(m_compactChars, other.m_compactChars, sizeof(m_compactChars));
}

inline rdxSCharSpan rdxITextDeserializer::SCompactToken::GetCharSpan() const
{
	if(m_isCompact)
		return rdxSCharSpan(m_compactChars, m_compactSize);
	return rdxSCharSpan(m_str->AsChars()->ArrayData(), m_str->Length());
}

inline rdxChar *rdxITextDeserializer::SCompactToken::InitCompactChars(rdxLargeUInt numChars)
{
	m_isCompact = true;
	m_str = rdxWeakRTRef(rdxCString)::Null();
	m_compactChars[numChars] = 0;
	m_compactSize = numChars;
	return m_compactChars;
}

inline void rdxITextDeserializer::SCompactToken::SetStrChars(rdxWeakRTRef(rdxCString) str)
{
	m_isCompact = false;
	m_str = str;
}

#endif
