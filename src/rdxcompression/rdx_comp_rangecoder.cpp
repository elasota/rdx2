#include "rdx_comp_rangecoder.hpp"

void rdxCompress::bssm::CRangeEnc::Init()
{
	m_low = 0;
	m_range = 0xffffffff;
	m_cache = 0;
	m_cacheSize = 1;
}

// Flushes
void rdxCompress::bssm::CRangeEnc::ShiftLow(rdxUInt8 **ppOut)
{
	rdxUInt8 *pOut = *ppOut;
	if(static_cast<rdxUInt32>(m_low >> 32) != 0 || static_cast<rdxUInt32>(m_low) < 0xff000000)
	{
		rdxUInt8 temp = m_cache;
		do
		{
			*pOut++ = static_cast<rdxUInt8>(temp + static_cast<rdxUInt8>(m_low >> 32));
			temp = 0xFF;
		}
		while (--m_cacheSize != 0);
		m_cache = static_cast<rdxUInt8>(static_cast<rdxUInt32>(m_low) >> 24);
	}
	m_cacheSize++;
	m_low = static_cast<rdxUInt32>(m_low & 0xffffffff) << 8;
	*ppOut = pOut;
}

void rdxCompress::bssm::CRangeEnc::Flush(rdxUInt8 **ppOut)
{
	for(rdxLargeUInt i=0;i<5;i++)
		ShiftLow(ppOut);
}

void rdxCompress::bssm::CRangeEnc::Encode(rdxUInt32 start, rdxUInt32 size, rdxUInt32 total, rdxUInt8 **ppOut)
{
	m_low += start * (m_range /= total);
	m_range *= size;
	while(m_range < 0x1000000)
	{
		m_range <<= 8;
		ShiftLow(ppOut);
	}
}

void rdxCompress::bssm::CRangeDec::Normalize(rdxUInt8 const** ppIn)
{
	const rdxUInt8 *pIn = *ppIn;
	if(m_range < 0x1000000)
	{
		do
		{
			m_code = (m_code << 8) | (*pIn++);
		} while(m_range < 0x1000000);
		*ppIn = pIn;
	}
}

void rdxCompress::bssm::CRangeDec::Init(rdxUInt8 const** ppIn)
{
	const rdxUInt8 *pIn = *ppIn;
	m_code = 0;
	m_range = 0xffffffff;
	for(rdxLargeUInt i=0;i<5;i++)
		m_code = (m_code << 8) | (*pIn++);
	*ppIn = pIn;
}
