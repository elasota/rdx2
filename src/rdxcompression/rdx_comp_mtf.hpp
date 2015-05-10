#ifndef __RDX_COMPRESS_MTF_HPP__
#define __RDX_COMPRESS_MTF_HPP__

#include "../rdx/rdx_coretypes.hpp"

namespace rdxCompress
{
	namespace bssm
	{
		template<int TVariation>
		class CMTFTpl
		{
		};

		template<>
		class CMTFTpl<0>
		{
		public:
			static rdxUInt8 Write(rdxUInt8 original, rdxUInt8 *indexes, rdxUInt8 last)
			{
				rdxUInt8 result;

				result = indexes[original];

				for(rdxLargeUInt i=0;i<256;i++)
					if(indexes[i] < result)
						indexes[i]++;

				indexes[original] = 0;

				return result;
			}

			static rdxUInt8 Read(rdxUInt8 original, rdxUInt8 *indexes, rdxUInt8 last)
			{
				rdxUInt8 result;

				result = indexes[original];

				for(rdxLargeUInt i=0;i<original;i++)
					indexes[original-i] = indexes[original-i-1];
				indexes[0] = result;

				return result;
			}
		};

		template<int TVariant>
		class CMTFTpl_Var1or2
		{
		public:
			static rdxUInt8 Write(rdxUInt8 original, rdxUInt8 *indexes, rdxUInt8 last)
			{
				rdxUInt8 result;

				result = indexes[original];

				if(result == 1)
				{
					if(TVariant == 1 || last != 0)
					{
						for(rdxLargeUInt i=0;i<256;i++)
						{
							if(!indexes[i])
							{
								indexes[i] = 1;
								break;
							}
						}
						indexes[original] = 0;
					}
				}
				else if(result)
				{
					for(rdxLargeUInt i=0;i<256;i++)
					{
						if(indexes[i] && indexes[i] < result)
							indexes[i]++;
					}
					indexes[original] = 1;
				}

				return result;
			}

			static rdxUInt8 Read(rdxUInt8 original, rdxUInt8 *indexes, rdxUInt8 last)
			{
				rdxUInt8 result;

				result = indexes[original];

				if(original == 1)
				{
					if(TVariant == 1 || last != 0)
					{
						rdxUInt8 temp = indexes[0];
						indexes[0] = indexes[1];
						indexes[1] = temp;
					}
				}
				else if(original)
				{
					for(i=original;i>=1;i--)
						indexes[i] = indexes[i-1];
					indexes[1] = result;
				}

				return result;
			}
		};

		template<>
		class CMTFTpl<1> : public CMTFTpl_Var1or2<1>
		{
		};

		template<>
		class CMTFTpl<2> : public CMTFTpl_Var1or2<2>
		{
		};

		class CRLE0EncStage;
		class CSModelSet;
		class CRangeEnc;

		class CMTFStage
		{
			rdxUInt8 m_indexes[256];
			rdxUInt8 m_last;
		public:
			void Init();
			void Encode(rdxUInt8 v, CRLE0EncStage &rle0, CSModelSet &smodel, CRangeEnc &rc, rdxUInt8 **ppOut);
		};
	}
}

#endif
