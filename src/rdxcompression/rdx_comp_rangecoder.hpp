#ifndef __RDX_COMPRESS_RANGECODER_HPP__
#define __RDX_COMPRESS_RANGECODER_HPP__

// From LZMA SDK
// 2010-04-16 : Igor Pavlov : Public domain

#include "../rdx/rdx_coretypes.hpp"
#include "../rdx/rdx_pragmas.hpp"

namespace rdxCompress
{
	namespace bssm
	{
		class CRangeEnc
		{
			rdxUInt64	m_low;
			rdxUInt32	m_cacheSize;
			rdxUInt32	m_range;
			rdxUInt8	m_cache;

			void ShiftLow(rdxUInt8 **ppOut);

		public:
			void Init();
			void Encode(rdxUInt32 start, rdxUInt32 size, rdxUInt32 total, rdxUInt8 **ppOut);
			void Flush(rdxUInt8 **ppOut);
		};

		class CRangeDec
		{
			rdxUInt32 m_range;
			rdxUInt32 m_code;

			void Normalize(rdxUInt8 const** ppIn);
		public:
			void Init(rdxUInt8 const** ppIn);
			RDX_FORCEINLINE rdxUInt32 GetThreshold(rdxUInt32 total)
			{
				return m_code / (m_range /= total);
			}
			RDX_FORCEINLINE void Decode(rdxUInt8 const** ppIn, rdxUInt32 symStart, rdxUInt32 symSize)
			{
				m_code -= symStart * m_range;
				m_range *= symSize;
				Normalize(ppIn);
			}
		};
	}
}

#endif
