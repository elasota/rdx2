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
#include "rdx_objectloader.hpp"
#include "rdx_io.hpp"

rdxCRuntimeObjectLoaderHost::rdxCRuntimeObjectLoaderHost(rdxIPackageHost *parent, rdxSDomainGUID domain, rdxIFileSystem *fs)
{
	m_parentHost = parent;
	m_runtimePackagePath = NULL;
	m_runtimePackageIsText = false;
	m_emulatedDomain = domain;
	m_fs = fs;
}

void rdxCRuntimeObjectLoaderHost::SetPackage(const rdxChar *packagePath, bool isText)
{
	m_runtimePackagePath = packagePath;
	m_runtimePackageIsText = isText;
}

rdxIFileStream *rdxCRuntimeObjectLoaderHost::StreamForDomain(rdxIObjectManager *om, rdxSDomainGUID domain, bool write, bool &isText)
{
	if(write == false && domain == rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime) && m_runtimePackagePath)
	{
		rdxIFileStream *stream = m_fs->Open(m_runtimePackagePath, false);
		if(!stream)
			return NULL;

		isText = m_runtimePackageIsText;
		return stream;
	}

	return m_parentHost->StreamForDomain(om, domain, write, isText);
}

bool rdxCRuntimeObjectLoaderHost::DomainsVisible(rdxSDomainGUID sourceDomain, rdxSDomainGUID destDomain)
{
	if(sourceDomain == rdxSDomainGUID::Builtin(rdxDOMAIN_Runtime))
		sourceDomain = m_emulatedDomain;
	return m_parentHost->DomainsVisible(sourceDomain, destDomain);
}

bool rdxCRuntimeObjectLoaderHost::DomainCanContainMethods(rdxSDomainGUID domain)
{
	return m_parentHost->DomainCanContainMethods(domain);
}
