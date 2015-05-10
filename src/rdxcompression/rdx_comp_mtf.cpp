#include "rdx_comp_mtf.hpp"
#include "rdx_comp_rle.hpp"
#include "rdx_comp_config.hpp"

void rdxCompress::bssm::CMTFStage::Init()
{
	m_last = 0xff;
	for(rdxLargeUInt i=0;i<256;i++)
		m_indexes[i] = static_cast<rdxUInt8>(i);
}

void rdxCompress::bssm::CMTFStage::Encode(rdxUInt8 v, CRLE0EncStage &rle0, CSModelSet &smodel, CRangeEnc &rc, rdxUInt8 **ppOut)
{
	rdxUInt8 codedSym;
	m_last = codedSym = rdxCompress::bssm::CMTFTpl<MTF_VARIANT>::Write(v, m_indexes, m_last);
	rle0.Encode(codedSym, smodel, rc, ppOut);
}
