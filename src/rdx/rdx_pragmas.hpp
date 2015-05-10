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
#ifndef __RDX_PRAGMAS_HPP__
#define __RDX_PRAGMAS_HPP__

#ifdef _MSC_VER

#pragma warning(disable:4127)	// Conditional expression is constant
#pragma warning(disable:4324)	// Structure padded due to __declspec(align())
#pragma warning(disable:4100)	// Unreferenced formal parameter
#pragma warning(disable:4820)	// Padding after member
#pragma warning(disable:4514)	// Unreferenced inline function removed
#pragma warning(disable:4710)	// Function not inlined
#pragma warning(disable:4668)	// Token not defined as a preprocessor value (windows.h causes this a lot)
#pragma warning(disable:4820)	// Padding bytes added to structure
#pragma warning(disable:4510)	// Default constructor could not be generated
#pragma warning(disable:4610)	// Can never be instantiated
#pragma warning(disable:4505)	// Unreferenced local function removed
#pragma warning(disable:4481)	// override specifier
#pragma warning(1:4389)			// signed/unsigned mismatch
#pragma warning(1:4365)			// signed/unsigned mismatch

#define RDX_FINAL sealed
#define RDX_OVERRIDE override
#define RDX_FORCEINLINE __forceinline
#define RDX_DLLEXPORT __declspec(dllexport)

#define _CRT_SECURE_NO_WARNINGS	1

#if 1
#  pragma warning(disable:4189)	// Local variable initialized but not referenced
#endif

#define RDX_MAY_ALIAS

#endif	// _MSC_VER

#ifdef __GNUC__
#define RDX_MAY_ALIAS __attribute__((may_alias))
#define RDX_FINAL
#define RDX_OVERRIDE
#define RDX_FORCEINLINE __attribute_((always_inline))
#endif

#endif
