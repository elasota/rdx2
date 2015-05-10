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
#ifndef __RDX_BASICTYPES_HPP__
#define __RDX_BASICTYPES_HPP__

//#pragma warning(error:4183)

#include <stdlib.h>

#include "rdx_coretypes.hpp"
#include "rdx_pragmas.hpp"
#include "rdx_superresolver.hpp"
#include "rdx_reftypealiases.hpp"

struct rdxGCInfo;


class rdxCMethod;
class rdxCException;
class rdxCType;

template<class T> struct rdxHdl;

// Need to include this here so that we can declare relationships of basic types
#include "rdx_typerelationships.hpp"

//static const rdxLargeUInt rdxALIGN_RuntimePointer		= RDX_ALIGNOF(rdxWeakOffsetRTRef(void));
//static const rdxLargeUInt rdxALIGN_Varying				= RDX_ALIGNOF(rdxSVarying);

#include "rdx_reftypecode.hpp"

#endif
