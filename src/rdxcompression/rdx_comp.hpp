#ifndef __RDX_COMPRESSION_HPP__
#define __RDX_COMPRESSION_HPP__

#include "../rdx/rdx_coretypes.hpp"
#include "../rdx/rdx_api.hpp"

RDX_COMPRESSION_DYNLIB_API rdxLargeUInt rdxR5GetWorkBufferSize(rdxLargeUInt inputSize);
RDX_COMPRESSION_DYNLIB_API rdxLargeUInt rdxR5CompressBlock(const void *input, rdxLargeUInt size, void *workBuffer);

#endif
