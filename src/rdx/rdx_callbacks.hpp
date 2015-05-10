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
#ifndef __RDX_CALLBACKS_HPP__
#define __RDX_CALLBACKS_HPP__

#include "rdx_platform.hpp"
#include "rdx_coretypes.hpp"
#include "rdx_reftypealiases.hpp"

struct rdxSOperationContext;
struct rdxIObjectManager;
class rdxCObjectManager;
class rdxCMethod;
class rdxCRuntimeThread;
class rdxCStackView;
union rdxURuntimeStackValue;

typedef int (RDX_DECL_API *rdxNativeCallback) (rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCMethod) m, rdxWeakHdl(rdxCRuntimeThread) thread, void *prv);


// SUPER IMPORTANT:
// These two functions are NOT reverses of each other!  IPToCurrentInstructionCallback returns the instruction being executed when
// the IP is set to a specified value, which is the instruction BEFORE the IP.  InstrNumToIPCallback converts an instruction number
// to the IP neccessary to run that instruction next.

// InstrNumToIP(IPToCurrentInstruction(ip) + 1) == ip
typedef rdxLargeUInt (RDX_DECL_API *rdxIPToCurrentInstructionCallback) (rdxWeakHdl(rdxCMethod) method, const void *ip);
typedef const void *(RDX_DECL_API *rdxInstrNumToIPCallback) (rdxSOperationContext *ctx, rdxWeakHdl(rdxCMethod) method, rdxLargeUInt instrNum, bool *resumeAllowed);

typedef int (RDX_DECL_API *rdxResumeThreadCallback) (rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakHdl(rdxCRuntimeThread) thread);

#endif
