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
#ifndef __RDX_PROCESSING_HPP__
#define __RDX_PROCESSING_HPP__

#include "rdx_coretypes.hpp"
#include "rdx_pragmas.hpp"
#include "rdx_gcslots.hpp"
#include "rdx_callbacks.hpp"
#include "rdx_lutdecls.hpp"

struct rdxSOperationContext;
	
template<class Tvalue> class rdxCStaticLookupPODKey;
template<class Tkey, class TlookupType> class rdxCStaticLookupTable;
template<class T, bool TCounting, class TBaseRef> struct rdxRef;

class rdxCRuntimeThread;
class rdxCString;
class rdxCType;
class rdxCStructuredType;

struct rdxITextDeserializer;
struct rdxIFileStream;
struct rdxIObjectManager;
struct rdxIPackageHost;
class rdxCPackage;
class rdxShellableRef;

struct rdxIObjectReferenceVisitor
{
	// Object reference visitors must visit any contained reference in an object
	virtual void VisitReference(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef &ref) = 0;
	virtual void VisitReference(rdxIObjectManager *objm, rdxBaseRTRef &ref) = 0;
	virtual void VisitReference(rdxIObjectManager *objm, rdxBaseHdl &ref) = 0;
	virtual void VisitReference(rdxIObjectManager *objm, rdxBaseIfcRef &ref) = 0;
	virtual void VisitReference(rdxIObjectManager *objm, rdxTracedTypelessRTRef &ref) = 0;
	virtual void VisitReference(rdxIObjectManager *objm, rdxTracedTypelessIfcRef &ref) = 0;

	template<class T, bool TCounting, class TBaseRef>
	RDX_FORCEINLINE void VisitReference(rdxIObjectManager *objm, rdxRef<T, TCounting, TBaseRef> &ref)
	{
		this->VisitReference(objm, ref.GetBaseRef());
	}
};

struct rdxISerializer
{
	// Marks a dependency.  If this returns true, then the object should be serialized
	virtual bool TryIncludeObject(rdxWeakRTRef(rdxCObject) obj) = 0;
	virtual void SerializeReference(rdxWeakRTRef(rdxCObject) obj) = 0;
	virtual void SerializeBulk(rdxWeakTypelessOffsetRTRef data, rdxLargeUInt sz) = 0;
	virtual void SerializeData(rdxWeakRTRef(rdxCType) type, rdxWeakTypelessOffsetRTRef data) = 0;
};

struct rdxITypeSerializer
{
	virtual void DeserializeTextInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxITextDeserializer *td, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const = 0;
	virtual void DeserializeBinaryInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxIFileStream *reader, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const = 0;

	virtual void SerializeBinaryInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const = 0;
	virtual void SerializeTextInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const = 0;
};

#endif

