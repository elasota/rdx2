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
#ifndef __RDX_OBJECTLOADER_HPP__
#define __RDX_OBJECTLOADER_HPP__

#include "rdx.h"
#include "rdx_objectmanagement.hpp"

struct rdxIFileSystem;

class rdxCRuntimeObjectLoaderHost : public rdxIPackageHost
{
	const rdxChar *m_runtimePackagePath;
	bool m_runtimePackageIsText;
	rdxIPackageHost *m_parentHost;
	rdxIFileSystem *m_fs;
	rdxSDomainGUID m_emulatedDomain;

public:
	rdxCRuntimeObjectLoaderHost(rdxIPackageHost *parent, rdxSDomainGUID domain, rdxIFileSystem *fs);
	void SetPackage(const rdxChar *packagePath, bool isText);

	virtual rdxIFileStream *StreamForDomain(rdxIObjectManager *om, rdxSDomainGUID domain, bool write, bool &isText) RDX_OVERRIDE;
	virtual bool DomainsVisible(rdxSDomainGUID sourceDomain, rdxSDomainGUID destDomain) RDX_OVERRIDE;
	virtual bool DomainCanContainMethods(rdxSDomainGUID domain) RDX_OVERRIDE;
};

#endif
