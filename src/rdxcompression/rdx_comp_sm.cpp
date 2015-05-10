#include "rdx_comp_sm.hpp"
#include "rdx_comp_config.hpp"
#include "rdx_comp_rangecoder.hpp"

rdxUInt32 rdxCompress::bssm::CSModelSet::SM_PROB_POOL_OFFSETS[]	=	{0, 10, 12, 16, 24, 40, 72, 136};
rdxUInt32 rdxCompress::bssm::CSModelSet::SM_PROB_POOL_COUNTS[]	=	{10, 2,  4,  8, 16, 32, 64, 128};
rdxUInt32 rdxCompress::bssm::CSModelSet::SM_INCTABLE[]			=	{12, 4, 3, 3, 3, 3, 2, 1};

void rdxCompress::bssm::CSModelSet::UpdateProbabilities(rdxUInt32 model, rdxUInt32 symbol)
{
	const rdxUInt32 *counts = SM_PROB_POOL_COUNTS;
	const rdxUInt32 *offsets = SM_PROB_POOL_OFFSETS;
	const rdxUInt32 *increments = SM_INCTABLE;
	rdxUInt16 *probs;

	probs = m_probPool + offsets[model];

	m_ptotals[model] += increments[model];
	probs[symbol] += increments[model];

	if(m_ptotals[model] >= PMAX)
	{
		// Normalize
		rdxUInt32 t = 0;
		rdxLargeUInt count = counts[model];
		for(rdxLargeUInt i=0;i<count;i++)
		{
			probs[i] = static_cast<rdxUInt16>((probs[i] + 1) >> 1);
			t += probs[i];
		}
		m_ptotals[model] = t;
	}
}

void rdxCompress::bssm::CSModelSet::Init()
{
	const rdxUInt32 *counts = SM_PROB_POOL_COUNTS;
	const rdxUInt32 *increments = SM_INCTABLE;

	rdxLargeUInt n = 0;
	for(rdxLargeUInt i=0;i<8;i++)
	{
		rdxUInt32 c = counts[i];
		for(rdxUInt32 j=0;j<c;j++)
			m_probPool[n++] = increments[i];
		m_ptotals[i] = increments[i] * c;
	}
}

void rdxCompress::bssm::CSModelSet::Encode(rdxUInt16 sym, CRangeEnc &rc, rdxUInt8 **ppOut)
{
	rdxUInt32 firstSym;
	rdxUInt32 secondSym;

	const rdxUInt32 *offsets = SM_PROB_POOL_OFFSETS;

	rdxLargeUInt secondModel = 0;

	if(sym == 0) firstSym = SM_VAL_RUN_0;
	else if(sym == 1) firstSym = SM_VAL_RUN_1;
	else if(sym == 2) firstSym = SM_VAL_1;
	else
	{
		sym--;
		if(sym & 0x80) { firstSym = SM_VAL_M_128_255; secondModel = SM_M_128_255; secondSym = static_cast<rdxUInt32>(sym) - 128; }
		else if(sym & 0x40) { firstSym = SM_VAL_M_64_127; secondModel = SM_M_64_127; secondSym = static_cast<rdxUInt32>(sym) - 64; }
		else if(sym & 0x20) { firstSym = SM_VAL_M_32_63; secondModel = SM_M_32_63; secondSym = static_cast<rdxUInt32>(sym) - 32; }
		else if(sym & 0x10) { firstSym = SM_VAL_M_16_31; secondModel = SM_M_16_31; secondSym = static_cast<rdxUInt32>(sym) - 16; }
		else if(sym & 0x08) { firstSym = SM_VAL_M_8_15; secondModel = SM_M_8_15; secondSym = static_cast<rdxUInt32>(sym) - 8; }
		else if(sym & 0x04) { firstSym = SM_VAL_M_4_7; secondModel = SM_M_4_7; secondSym = static_cast<rdxUInt32>(sym) - 4; }
		else /*if(sym & 0x02)*/ { firstSym = SM_VAL_M_2_3; secondModel = SM_M_2_3; secondSym = static_cast<rdxUInt32>(sym) - 2; }
	}

	{
		rdxUInt32 start = 0;
		for(rdxUInt32 i=0;i<firstSym;i++)
			start += m_probPool[i];
		rc.Encode(start, m_probPool[firstSym], m_ptotals[0], ppOut);
	}

	UpdateProbabilities(0, firstSym);

	if(secondModel)
	{
		const rdxUInt16 *subProbs = m_probPool + offsets[secondModel];

		rdxUInt32 start = 0;
		for(rdxUInt32 i=0;i<secondSym;i++)
			start = subProbs[i];
		rc.Encode(start, subProbs[secondSym], m_ptotals[secondModel], ppOut);
		UpdateProbabilities(secondModel, secondSym);
	}
}
