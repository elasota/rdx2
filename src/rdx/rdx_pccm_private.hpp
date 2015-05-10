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

#include "rdx.h"

// Common functions that all PCCM use
static int CallMethod(OperationContext *ctx, IObjectManager *objm, const Method *invokedMethod, LargeInt instrNum, LargeInt frameOffs, LargeInt prvOffs, RuntimeThread *thread, const LargeInt *instrTable,
	void *bp, void **providerDictionary, bool &shouldContinue)
{
	thread->frame.ip = instrTable + (instrNum + 1);
	if(invokedMethod->bytecode != NULL)
	{
		RuntimeStackFrame newFrame;
		if(!RDX_RuntimeUtilities_EnterMethodInline(objm, invokedMethod, &thread->frame, static_cast<UInt8*>(bp) + (frameOffs), thread->stackBytes,
			reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8*>(bp) + (prvOffs)), &newFrame))
			THROWEXCEPTION(objm->GetBuiltIns()->providerDictionary[X_StackOverflowException], instrNum);
		thread->frame = newFrame;
		return RuntimeState::Active;
	}
	else
	{
		NativeCallback cb = invokedMethod->_native.nativeCall;
		NULLCHECK(cb, instrNum);
		RuntimeStackFrame currentFrame = thread->frame;
		thread->insertionPoint = reinterpret_cast<UInt8 *>(bp) + frameOffs;
		thread->activeNativeMethod = invokedMethod;
		int status = cb(ctx, objm, invokedMethod, thread, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8*>(bp) + (prvOffs)));
		thread->activeNativeMethod = NULL;
		if(status <= 0)
		{
			thread->frame = currentFrame;
			return status;
		}
		shouldContinue = true;
		return RuntimeState::Active;
	}
}
static int CallMethodNative(OperationContext *ctx, IObjectManager *objm, const Method *invokedMethod, LargeInt instrNum, LargeInt frameOffs, LargeInt prvOffs, RuntimeThread *thread, const LargeInt *instrTable,
	void *bp, bool &shouldContinue)
{
	thread->frame.ip = instrTable + (instrNum + 1);
	thread->insertionPoint = reinterpret_cast<UInt8 *>(bp) + frameOffs;
	thread->activeNativeMethod = invokedMethod;
	NativeCallback cb = invokedMethod->_native.nativeCall;
	RuntimeStackFrame currentFrame = thread->frame;
	int status = cb(ctx, objm, invokedMethod, thread, reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8*>(bp) + (prvOffs)));
	thread->activeNativeMethod = NULL;
	if(status <= 0)
	{
		thread->frame = currentFrame;
		return status;
	}
	shouldContinue = true;
	return RuntimeState::Active;
}
static int CallMethodNotNative(OperationContext *ctx, IObjectManager *objm, const Method *invokedMethod, LargeInt instrNum, LargeInt frameOffs, LargeInt prvOffs, RuntimeThread *thread, const LargeInt *instrTable,
	void *bp, void **providerDictionary)
{
	thread->frame.ip = instrTable + (instrNum + 1);
	thread->insertionPoint = reinterpret_cast<UInt8 *>(bp) + frameOffs;
	thread->activeNativeMethod = invokedMethod;

	RuntimeStackFrame newFrame;
		if(!RDX_RuntimeUtilities_EnterMethodInline(objm, invokedMethod, &thread->frame, static_cast<UInt8*>(bp) + (frameOffs), thread->stackBytes,
			reinterpret_cast<RuntimeStackValue*>(static_cast<UInt8*>(bp) + (prvOffs)), &newFrame))
			THROWEXCEPTION(objm->GetBuiltIns()->providerDictionary[X_StackOverflowException], instrNum);
		thread->frame = newFrame;
	return RuntimeState::Active;
}
