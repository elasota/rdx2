#ifndef __RDX_COMPRESS_BLOCKSORT_HPP__
#define __RDX_COMPRESS_BLOCKSORT_HPP__

#include "../rdx/rdx_coretypes.hpp"

namespace rdxCompress
{
	namespace bssm
	{
		static const rdxLargeUInt NUM_RADIX = 2;
		static const rdxLargeUInt NUM_QSORT = 12;
		static const rdxLargeUInt NUM_SHELL = 18;
		static const rdxLargeUInt NUM_OVERSHOOT = NUM_RADIX + NUM_QSORT + NUM_SHELL + 2;

		/*
		   Pre:
			  nblock > 0
			  arr2 exists for [0 .. nblock-1 +N_OVERSHOOT]
			  ((UChar*)arr2)  [0 .. nblock-1] holds block
			  arr1 exists for [0 .. nblock-1]

		   Post:
			  ((UChar*)arr2) [0 .. nblock-1] holds block
			  All other areas of block destroyed
			  ftab [ 0 .. 65536 ] destroyed
			  arr1 [0 .. nblock-1] holds sorted order
		*/
		void BlockSort(rdxUInt32 *ftab, rdxInt32 nblock, rdxInt32 wfact, rdxUInt32 *arr1, rdxUInt32 *arr2);
	}
}

#endif
