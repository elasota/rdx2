//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#include "rdx_murmur3.hpp"


struct rdxMurmur3HashUtils
{
	static inline rdxUInt32 RotL32(rdxUInt32 x, rdxInt8 r)
	{
		return (x << r) | (x >> (32 - r));
	}

	static inline rdxUInt32 GetBlock32(const rdxUInt8 *p)
	{
		return static_cast<rdxUInt32>(p[0]) | static_cast<rdxUInt32>(p[1] << 8) | static_cast<rdxUInt32>(p[2] << 16) | static_cast<rdxUInt32>(p[3] << 24);
	}

	static inline rdxUInt32 FMix32(rdxUInt32 h)
	{
		h ^= h >> 16;
		h *= 0x85ebca6b;
		h ^= h >> 13;
		h *= 0xc2b2ae35;
		h ^= h >> 16;

		return h;
	}
};

class rdxXAPI_Murmur3Hash_Shim
{
public:
	static void Murmur3Hash128_Init(rdxCMurmur3Hash128 *v);
	static void Murmur3Hash128_FeedBytes(rdxCMurmur3Hash128 *v, const void *bytes, rdxLargeUInt sz);
	static void Murmur3Hash128_Flush(rdxCMurmur3Hash128 *v, rdxUInt64 *outHigh, rdxUInt64 *outLow);
	static void Murmur3Hash32_Init(rdxCMurmur3Hash32 *v);
	static void Murmur3Hash32_FeedBytes(rdxCMurmur3Hash32 *v, const void *bytes, rdxLargeUInt sz);
	static rdxUInt32 Murmur3Hash32_Flush(rdxCMurmur3Hash32 *v);
};

RDX_UTIL_DYNLIB_API void rdxXAPI_Murmur3Hash32_Init(rdxCMurmur3Hash32 *v)
{
	rdxXAPI_Murmur3Hash_Shim::Murmur3Hash32_Init(v);
}

void rdxXAPI_Murmur3Hash_Shim::Murmur3Hash32_Init(rdxCMurmur3Hash32 *v)
{
	v->m_h1 = v->m_len = 0;
	v->m_backlogPos = 0;
}

RDX_UTIL_DYNLIB_API void rdxXAPI_Murmur3Hash32_FeedBytes(rdxCMurmur3Hash32 *v, const void *data, rdxLargeUInt size)
{
	rdxXAPI_Murmur3Hash_Shim::Murmur3Hash32_FeedBytes(v, data, size);
}

void rdxXAPI_Murmur3Hash_Shim::Murmur3Hash32_FeedBytes(rdxCMurmur3Hash32 *v, const void *data, rdxLargeUInt size)
{
	const rdxUInt8 *dataBytes = static_cast<const rdxUInt8*>(data);
	rdxUInt8 *backlog = v->m_backlog;

	rdxUInt32 h1 = v->m_h1;
	rdxUInt8 backlogPos = v->m_backlogPos;
	
	const rdxUInt32 c1 = 0xcc9e2d51;
	const rdxUInt32 c2 = 0x1b873593;

	for(rdxLargeUInt i=0;i<size;i++)
	{
		backlog[backlogPos++] = *dataBytes++;

		if(backlogPos == 4)
		{
			backlogPos = 0;
			rdxUInt32 k1 = rdxMurmur3HashUtils::GetBlock32(backlog);

			k1 *= c1;
			k1 = rdxMurmur3HashUtils::RotL32(k1,15);
			k1 *= c2;
    
			h1 ^= k1;
			h1 = rdxMurmur3HashUtils::RotL32(h1,13); 
			h1 = h1*5+0xe6546b64;
		}
	}

	v->m_backlogPos = backlogPos;
	v->m_h1 = h1;
	v->m_len += static_cast<rdxUInt32>(size);
}

RDX_UTIL_DYNLIB_API rdxUInt32 rdxXAPI_Murmur3Hash32_Flush(rdxCMurmur3Hash32 *v)
{
	return rdxXAPI_Murmur3Hash_Shim::Murmur3Hash32_Flush(v);
}

rdxUInt32 rdxXAPI_Murmur3Hash_Shim::Murmur3Hash32_Flush(rdxCMurmur3Hash32 *v)
{
	rdxUInt32 h1 = v->m_h1;
	rdxUInt32 k1 = 0;
	const rdxUInt32 c1 = 0xcc9e2d51;
	const rdxUInt32 c2 = 0x1b873593;

	switch(v->m_backlogPos & 3)
	{
	case 3: k1 ^= v->m_backlog[2] << 16;
	case 2: k1 ^= v->m_backlog[1] << 8;
	case 1: k1 ^= v->m_backlog[0];
			k1 *= c1; k1 = rdxMurmur3HashUtils::RotL32(k1,15); k1 *= c2; h1 ^= k1;
	};

	h1 ^= (v->m_len + v->m_backlogPos);

	h1 = rdxMurmur3HashUtils::FMix32(h1);

	return h1;
}

RDX_UTIL_DYNLIB_API void rdxXAPI_Murmur3Hash128_Init(rdxCMurmur3Hash128 *v)
{
	rdxXAPI_Murmur3Hash_Shim::Murmur3Hash128_Init(v);
}

void rdxXAPI_Murmur3Hash_Shim::Murmur3Hash128_Init(rdxCMurmur3Hash128 *v)
{
	v->m_h1 = v->m_h2 = v->m_h3 = v->m_h4 = v->m_len = 0;
	v->m_backlogPos = 0;
}

RDX_UTIL_DYNLIB_API void rdxXAPI_Murmur3Hash128_FeedBytes(rdxCMurmur3Hash128 *v, const void *data, rdxLargeUInt size)
{
	rdxXAPI_Murmur3Hash_Shim::Murmur3Hash128_FeedBytes(v, data, size);
}

void rdxXAPI_Murmur3Hash_Shim::Murmur3Hash128_FeedBytes(rdxCMurmur3Hash128 *v, const void *data, rdxLargeUInt size)
{
	const rdxUInt32 c1 = 0x239b961b;
	const rdxUInt32 c2 = 0xab0e9789;
	const rdxUInt32 c3 = 0x38b34ae5;
	const rdxUInt32 c4 = 0xa1e38b93;

	rdxUInt32 h1 = v->m_h1;
	rdxUInt32 h2 = v->m_h2;
	rdxUInt32 h3 = v->m_h3;
	rdxUInt32 h4 = v->m_h4;
	rdxUInt8 *backlog = v->m_backlog;
	rdxUInt8 backlogPos = v->m_backlogPos;

	const rdxUInt8 *dataBytes = static_cast<const rdxUInt8 *>(data);

	for(rdxLargeUInt i=0;i<size;i++)
	{
		backlog[backlogPos++] = *dataBytes++;

		if(backlogPos == 16)
		{
			backlogPos = 0;

			rdxUInt32 k1 = rdxMurmur3HashUtils::GetBlock32(backlog + 0);
			rdxUInt32 k2 = rdxMurmur3HashUtils::GetBlock32(backlog + 4);
			rdxUInt32 k3 = rdxMurmur3HashUtils::GetBlock32(backlog + 8);
			rdxUInt32 k4 = rdxMurmur3HashUtils::GetBlock32(backlog + 12);
			k1 *= c1; k1  = rdxMurmur3HashUtils::RotL32(k1,15); k1 *= c2; h1 ^= k1;
			h1 = rdxMurmur3HashUtils::RotL32(h1,19); h1 += h2; h1 = h1*5+0x561ccd1b;
			k2 *= c2; k2  = rdxMurmur3HashUtils::RotL32(k2,16); k2 *= c3; h2 ^= k2;
			h2 = rdxMurmur3HashUtils::RotL32(h2,17); h2 += h3; h2 = h2*5+0x0bcaa747;
			k3 *= c3; k3  = rdxMurmur3HashUtils::RotL32(k3,17); k3 *= c4; h3 ^= k3;
			h3 = rdxMurmur3HashUtils::RotL32(h3,15); h3 += h4; h3 = h3*5+0x96cd1c35;
			k4 *= c4; k4  = rdxMurmur3HashUtils::RotL32(k4,18); k4 *= c1; h4 ^= k4;
			h4 = rdxMurmur3HashUtils::RotL32(h4,13); h4 += h1; h4 = h4*5+0x32ac3b17;
		}
	}

	v->m_h1 = h1;
	v->m_h2 = h2;
	v->m_h3 = h3;
	v->m_h4 = h4;
	v->m_len += static_cast<rdxUInt32>(size);
	v->m_backlogPos = backlogPos;
}

void rdxXAPI_Murmur3Hash128_Flush(rdxCMurmur3Hash128 *v, rdxUInt64 *outHigh, rdxUInt64 *outLow)
{
	rdxXAPI_Murmur3Hash_Shim::Murmur3Hash128_Flush(v, outHigh, outLow);
}

void rdxXAPI_Murmur3Hash_Shim::Murmur3Hash128_Flush(rdxCMurmur3Hash128 *v, rdxUInt64 *outHigh, rdxUInt64 *outLow)
{
	rdxUInt32 h1 = v->m_h1;
	rdxUInt32 h2 = v->m_h2;
	rdxUInt32 h3 = v->m_h3;
	rdxUInt32 h4 = v->m_h4;
	rdxUInt32 len = v->m_len;

	const rdxUInt32 c1 = 0x239b961b;
	const rdxUInt32 c2 = 0xab0e9789;
	const rdxUInt32 c3 = 0x38b34ae5;
	const rdxUInt32 c4 = 0xa1e38b93;
	
	// Tail
	const rdxUInt8 *backlog = v->m_backlog;

	rdxUInt32 k1 = 0;
	rdxUInt32 k2 = 0;
	rdxUInt32 k3 = 0;
	rdxUInt32 k4 = 0;

	switch(v->m_backlogPos & 15)
	{
	case 15: k4 ^= backlog[14] << 16;
	case 14: k4 ^= backlog[13] << 8;
	case 13: k4 ^= backlog[12] << 0;
			 k4 *= c4; k4  = rdxMurmur3HashUtils::RotL32(k4,18); k4 *= c1; h4 ^= k4;

	case 12: k3 ^= backlog[11] << 24;
	case 11: k3 ^= backlog[10] << 16;
	case 10: k3 ^= backlog[ 9] << 8;
	case  9: k3 ^= backlog[ 8] << 0;
			 k3 *= c3; k3  = rdxMurmur3HashUtils::RotL32(k3,17); k3 *= c4; h3 ^= k3;

	case  8: k2 ^= backlog[ 7] << 24;
	case  7: k2 ^= backlog[ 6] << 16;
	case  6: k2 ^= backlog[ 5] << 8;
	case  5: k2 ^= backlog[ 4] << 0;
			 k2 *= c2; k2  = rdxMurmur3HashUtils::RotL32(k2,16); k2 *= c3; h2 ^= k2;

	case  4: k1 ^= backlog[ 3] << 24;
	case  3: k1 ^= backlog[ 2] << 16;
	case  2: k1 ^= backlog[ 1] << 8;
	case  1: k1 ^= backlog[ 0] << 0;
			 k1 *= c1; k1  = rdxMurmur3HashUtils::RotL32(k1,15); k1 *= c2; h1 ^= k1;
	};

	h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;

	h1 += h2; h1 += h3; h1 += h4;
	h2 += h1; h3 += h1; h4 += h1;

	h1 = rdxMurmur3HashUtils::FMix32(h1);
	h2 = rdxMurmur3HashUtils::FMix32(h2);
	h3 = rdxMurmur3HashUtils::FMix32(h3);
	h4 = rdxMurmur3HashUtils::FMix32(h4);

	h1 += h2; h1 += h3; h1 += h4;
	h2 += h1; h3 += h1; h4 += h1;

	*outHigh = (static_cast<rdxUInt64>(h1) << 32) | h2;
	*outLow = (static_cast<rdxUInt64>(h3) << 32) | h4;
}
