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
#ifndef __RDX_PLUGIN_HPP__
#define __RDX_PLUGIN_HPP__

#include "rdx_processing.hpp"

class rdxCCombinedNativeTypeHost : public rdxINativeTypeHost
{
private:
	const rdxINativeTypeHost **m_thList;

public:
	explicit rdxCCombinedNativeTypeHost(const rdxINativeTypeHost **thList)
	{
		m_thList = thList;
	}

	rdxIfcTypeInfo TypeInfoForType(rdxWeakRTRef(rdxCStructuredType) st) const RDX_OVERRIDE
	{
		const rdxINativeTypeHost **th = m_thList;
		while(*th)
		{
			rdxIfcTypeInfo typeInfo = (*th)->TypeInfoForType(st);
			th++;
		}
		rdxIfcTypeInfo nullTI;
		nullTI.fetchFunc = NULL;
		return nullTI;
	}
	
	rdxNativeCallback HookMethod(rdxSObjectGUID methodGUID) const RDX_OVERRIDE
	{
		const rdxINativeTypeHost **th = m_thList;
		while(*th)
		{
			rdxNativeCallback cb = (*th)->HookMethod(methodGUID);
			if(cb)
				return cb;
			th++;
		}
		return NULL;
	}
};

const rdxINativeTypeHost *rdxGetNumericTypesPlugin();
const rdxINativeTypeHost *rdxGetHashTablePlugin();

#endif
