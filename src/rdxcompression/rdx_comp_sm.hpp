#ifndef __RDX_COMPRESS_SM_HPP__
#define __RDX_COMPRESS_SM_HPP__

#include "../rdx/rdx_coretypes.hpp"

// Structured model
namespace rdxCompress
{
	namespace bssm
	{
		class CRangeEnc;

		class CSModelSet
		{
		private:
			static rdxUInt32 SM_PROB_POOL_OFFSETS[];
			static rdxUInt32 SM_PROB_POOL_COUNTS[];
			static rdxUInt32 SM_INCTABLE[];

			static const rdxLargeUInt SM_VAL_RUN_0		= 0;
			static const rdxLargeUInt SM_VAL_RUN_1		= 1;
			static const rdxLargeUInt SM_VAL_1			= 2;
			static const rdxLargeUInt SM_VAL_M_2_3		= 3;
			static const rdxLargeUInt SM_VAL_M_4_7		= 4;
			static const rdxLargeUInt SM_VAL_M_8_15		= 5;
			static const rdxLargeUInt SM_VAL_M_16_31	= 6;
			static const rdxLargeUInt SM_VAL_M_32_63	= 7;
			static const rdxLargeUInt SM_VAL_M_64_127	= 8;
			static const rdxLargeUInt SM_VAL_M_128_255	= 9;

			static const rdxLargeUInt SM_M_BASE			= 0;
			static const rdxLargeUInt SM_M_2_3			= 1;
			static const rdxLargeUInt SM_M_4_7			= 2;
			static const rdxLargeUInt SM_M_8_15			= 3;
			static const rdxLargeUInt SM_M_16_31		= 4;
			static const rdxLargeUInt SM_M_32_63		= 5;
			static const rdxLargeUInt SM_M_64_127		= 6;
			static const rdxLargeUInt SM_M_128_255		= 7;

			rdxUInt32 m_ptotals[8];
			rdxUInt16 m_probPool[264];
			void UpdateProbabilities(rdxUInt32 model, rdxUInt32 symbol);

		public:
			void Init();
			void Encode(rdxUInt16 sym, CRangeEnc &rc, rdxUInt8 **ppOut);
		};
	}
}

#endif
