#ifndef __RDX_MURMUR2_HPP__
#define __RDX_MURMUR2_HPP__

//-----------------------------------------------------------------------------
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

// Adaption to RDX (c)2013 Eric Lasota/Gale Force Games
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include "rdx_utility.hpp"

inline rdxSIntermediateHash::rdxSIntermediateHash()
{
	this->intermediate = this->next = 0;
	this->byteOffset = 0;
}

inline void rdxSIntermediateHash::FeedBytes(const void *bytes, rdxLargeUInt sz)
{
	if(sz == 0)
		return;
	const rdxUInt8 *cbytes = static_cast<const rdxUInt8 *>(bytes);
	const rdxUInt32 m = 0x5bd1e995;
	const int r = 24;
	rdxUInt32 h = intermediate;
	while(sz)
	{
		rdxUInt8 nextByte = *cbytes++;
		next |= (nextByte << (byteOffset * 8));
		byteOffset++;
		if(byteOffset == 4)
		{
			rdxUInt32 k = next;
			k *= m;
			k ^= k >> r;
			k *= m;

			h *= m;
			h ^= k;
			next = 0;
			byteOffset = 0;
		}

		sz--;
	}
	intermediate = h;
}

inline rdxHashValue rdxSIntermediateHash::Flush()
{
	const rdxUInt32 m = 0x5bd1e995;
	rdxUInt8 zero = 0;
	while(byteOffset != 0)
		FeedBytes(&zero, 1);

	rdxUInt32 h = this->intermediate;
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return static_cast<rdxHashValue>(h);
}

#endif
