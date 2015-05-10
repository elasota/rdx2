#include "rdx_comp_rle.hpp"
#include "rdx_comp_sm.hpp"

void rdxCompress::bssm::CRLE0EncStage::Init()
{
	m_inRun = false;
	m_runLength = 0;
	m_numValuesEmitted = 0;
}

void rdxCompress::bssm::CRLE0EncStage::Encode(rdxUInt8 p, CSModelSet &smodel, CRangeEnc &rc, rdxUInt8 **ppOut)
{
	if(p != 0)
	{
		Flush(smodel, rc, ppOut);
		m_numValuesEmitted++;
		smodel.Encode(static_cast<rdxUInt16>(p + 1), rc, ppOut);
	}
	else
	{
		m_inRun = true;
		m_runLength++;
	}
}

void rdxCompress::bssm::CRLE0EncStage::Flush(CSModelSet &smodel, CRangeEnc &rc, rdxUInt8 **ppOut)
{
	if(m_inRun)
	{
		m_runLength++;
		rdxLargeUInt bits = 0;
		for(rdxLargeUInt i=1;i<30;i++)
			if(m_runLength & (1 << i)) bits = i;
		while(bits)
		{
			bits--;
			rdxUInt16 outputSym;
			if(m_runLength & (1 << bits))
				outputSym = 1;
			else
				outputSym = 0;
			m_numValuesEmitted++;
			smodel.Encode(outputSym, rc, ppOut);
		}
		m_runLength = 0;
		m_inRun = false;
	}
}
