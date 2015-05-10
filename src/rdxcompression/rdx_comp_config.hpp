#ifndef __RDX_COMPRESS_CONFIG_HPP__
#define __RDX_COMPRESS_CONFIG_HPP__

#include "../rdx/rdx_coretypes.hpp"

namespace rdxCompress
{
	namespace bssm
	{
		// Number of probabilities before normalizing.  Hard max is 4096.
		static rdxLargeUInt PMAX = 1000;	// Hard max is 4096

		// Can be 0, 1, or 2
		static const rdxLargeUInt MTF_VARIANT = 2;
	}
}

#endif
