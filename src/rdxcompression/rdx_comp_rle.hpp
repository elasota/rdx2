#define __RDX_COMPRESS_RLE_HPP__
#define __RDX_COMPRESS_RLE_HPP__

#include "../rdx/rdx_coretypes.hpp"

namespace rdxCompress
{
	namespace bssm
	{
		class CRangeEnc;
		class CSModelSet;

		class CRLE0EncStage
		{
			bool		m_inRun;
			rdxUInt32	m_runLength;
			rdxUInt32	m_numValuesEmitted;
		public:
			void Init();
			void Encode(rdxUInt8 p, CSModelSet &smodel, CRangeEnc &rc, rdxUInt8 **ppOut);
			void Flush(CSModelSet &smodel, CRangeEnc &rc, rdxUInt8 **ppOut);
			inline rdxUInt32 NumValuesEmitted() const { return m_numValuesEmitted; }
		};
	}
}