/*
 * Copyright (C) 2011-2013 Eric Lasota
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "rdx_utility.hpp"
#include "rdx_programmability.hpp"
#include "rdx_basictypes.hpp"

namespace RDX
{
	namespace Utility
	{
		using namespace Programmability;

		HashValue HashBytes(HashValue hv, const void *bytes, size_t sz)
		{
			const unsigned char *pbytes = reinterpret_cast<const unsigned char*>(bytes);

			while(sz)
			{
				// Binary with prime bit run lengths
				hv = hv ^ ((hv<<5)+(hv>>2)+0x9c1fc007+pbytes[0]);
				sz--;
				pbytes++;
			}

			return hv;
		}

	}
}
