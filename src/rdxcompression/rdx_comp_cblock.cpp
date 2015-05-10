#include <string.h>
#include "rdx_comp_sm.hpp"
#include "rdx_comp_blocksort.hpp"
#include "rdx_comp_mtf.hpp"
#include "rdx_comp_config.hpp"
#include "rdx_comp_rangecoder.hpp"
#include "rdx_comp_rle.hpp"
#include "../rdx/rdx_api.hpp"

namespace rdxCompress
{
	namespace bssm
	{
		const void *R5BlockSort(const void *input, void *tempspace, rdxUInt32 count, rdxUInt32 *pindex)
		{
			// Prep blocksort
			rdxUInt32 *arr1 = static_cast<rdxUInt32*>(tempspace);
			rdxUInt32 *arr2 = arr1 + count;

			memcpy(arr2, input, count);

			// Sort
			rdxUInt32 ftab[65537];
			bssm::BlockSort(ftab, static_cast<rdxInt32>(count), 30, arr1, arr2);

			// Dump BWT output in the arr2 space
			const rdxUInt8 *inc = static_cast<const rdxUInt8 *>(input);
			rdxUInt8 *outc = static_cast<rdxUInt8*>(tempspace) + count*4;

			rdxUInt32 pi = 0;
			for(rdxUInt32 i=0;i<count;i++)
			{
				rdxUInt32 n = arr1[i];
				if(!n)
					n = count-1;
				else
					n--;

				if(!n)
					pi = i;

				outc[i] = inc[n];
			}

			*pindex = pi;

			return outc;
		}

		rdxLargeUInt R5CompressBlock(const void *input, rdxUInt32 count, void *tempspace)
		{
			rdxLargeUInt headerSize = 6;
			rdxUInt32 pindex;

			if(count <= headerSize)
				return 0;

			const void *bwtoutput = R5BlockSort(input, tempspace, count, &pindex);

			rdxUInt8 *finaloutput = static_cast<rdxUInt8*>(tempspace);

			rdxUInt8 *bitstream = finaloutput + headerSize;
			rdxLargeUInt bitstreamMax = count - headerSize;

			CRangeEnc rc;
			CSModelSet smodel;
			CRLE0EncStage rle0;
			CMTFStage mtf;

			rc.Init();
			smodel.Init();
			rle0.Init();
			mtf.Init();

			const rdxUInt8 *bitstreamBase = bitstream;
			for(rdxLargeUInt i=0;i<count;i++)
			{
				mtf.Encode(static_cast<const rdxUInt8 *>(bwtoutput)[i], rle0, smodel, rc, &bitstream);
				if(static_cast<rdxLargeUInt>(bitstream - bitstreamBase) >= bitstreamMax)
					return 0;
			}
			rle0.Flush(smodel, rc, &bitstream);
			if(static_cast<rdxLargeUInt>(bitstream - bitstreamBase) >= bitstreamMax)
				return 0;
			rc.Flush(&bitstream);
			if(static_cast<rdxLargeUInt>(bitstream - bitstreamBase) >= bitstreamMax)
				return 0;

			finaloutput[0] = static_cast<rdxUInt8>(pindex >> 16);
			finaloutput[1] = static_cast<rdxUInt8>(pindex >> 8);
			finaloutput[2] = static_cast<rdxUInt8>(pindex);
			finaloutput[3] = static_cast<rdxUInt8>(rle0.NumValuesEmitted() >> 16);
			finaloutput[4] = static_cast<rdxUInt8>(rle0.NumValuesEmitted() >> 8);
			finaloutput[5] = static_cast<rdxUInt8>(rle0.NumValuesEmitted() );

			return static_cast<rdxLargeUInt>(bitstream - bitstreamBase) + headerSize;
		}
	}
}

RDX_COMPRESSION_DYNLIB_API rdxLargeUInt rdxR5GetWorkBufferSize(rdxLargeUInt inputSize)
{
	return inputSize * 8 + 160;
}

RDX_COMPRESSION_DYNLIB_API rdxLargeUInt rdxR5CompressBlock(const void *input, rdxLargeUInt size, void *workBuffer)
{
	return rdxCompress::bssm::R5CompressBlock(input, size, workBuffer);
}
