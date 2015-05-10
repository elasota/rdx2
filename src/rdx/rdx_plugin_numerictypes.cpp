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
//#include "rdx_plugin.hpp"
//#include "rdx_programmability.hpp"
//#include "rdx_processing.hpp"
#include "rdx_constants.hpp"
#include "rdx_runtime.hpp"
#include "rdx_coretypeattribs.hpp"



struct rdxCSerializationDetectorTypeSerializer : public rdxITypeSerializer
{
	void DeserializeTextInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakOffsetHdl(void) instance, rdxITextDeserializer *td, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const RDX_OVERRIDE
	{
		outShouldProcessProperties = false;
		*static_cast<rdxBool*>(instance.Modify()) = rdxTrueValue;
	}

	void DeserializeBinaryInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakOffsetHdl(void) instance, rdxIFileStream *reader, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const RDX_OVERRIDE
	{
		outShouldProcessProperties = false;
		*static_cast<rdxBool*>(instance.Modify()) = rdxTrueValue;
	}

	virtual void SerializeBinaryInstance(rdxIObjectManager *objm, rdxWeakOffsetRTRef(void) obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const RDX_OVERRIDE
	{
	}

	virtual void SerializeTextInstance(rdxIObjectManager *objm, rdxWeakOffsetRTRef(void) obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const RDX_OVERRIDE
	{
	}
};

class rdxCNumericTypesPlugin : public rdxINativeTypeHost
{
private:
	rdxCNumericTypeSerializer<rdxFloat, rdxFloat> m_floatTypeSerializer;
	rdxCNumericTypeSerializer<rdxUShort, rdxUShort> m_ushortTypeSerializer;
	rdxCNumericTypeSerializer<rdxByte, rdxByte> m_byteTypeSerializer;
	rdxCNumericTypeSerializer<rdxHashValue, rdxHashValue> m_hashValueSerializer;
	rdxCSerializationDetectorTypeSerializer m_serializationDetectorSerializer;
public:
	
	const rdxSTypeInfo *TypeInfoForType(rdxWeakRTRef(rdxCStructuredType) st) const RDX_OVERRIDE
	{
		if(!st.ObjectInfo()->SerializationTag())
			return NULL;

		rdxSObjectGUID objectGUID = st.ObjectInfo()->SerializationTag()->gstSymbol;

		static const rdxSObjectGUID floatGUID = rdxSObjectGUID::FromObjectName("Core", "float");
		if(objectGUID == floatGUID)
		{
			st->m_native.user.typeSerializer = &m_floatTypeSerializer;
			st->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_NativeSafeToSerialize;
			return rdxSAutoTypeInfo<rdxFloat>::TypeInfoInterface();
		}

		static const rdxSObjectGUID ushortGUID = rdxSObjectGUID::FromObjectName("Core", "ushort");
		if(objectGUID == ushortGUID)
		{
			st->m_native.user.typeSerializer = &m_ushortTypeSerializer;
			st->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_NativeSafeToSerialize;
			return rdxSAutoTypeInfo<rdxUShort>::TypeInfoInterface();
		}

		static const rdxSObjectGUID byteGUID = rdxSObjectGUID::FromObjectName("Core", "byte");
		if(objectGUID == byteGUID)
		{
			st->m_native.user.typeSerializer = &m_byteTypeSerializer;
			st->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_NativeSafeToSerialize;
			return rdxSAutoTypeInfo<rdxByte>::TypeInfoInterface();
		}

		static const rdxSObjectGUID sdGUID = rdxSObjectGUID::FromObjectName("Core", "serializationdetector");
		if(objectGUID == sdGUID)
		{
			st->m_native.user.typeSerializer = &m_serializationDetectorSerializer;
			return rdxSAutoTypeInfo<rdxByte>::TypeInfoInterface();
		}

		static const rdxSObjectGUID hashcodeGUID = rdxSObjectGUID::FromObjectName("Core", "hashcode");
		if(objectGUID == hashcodeGUID)
		{
			st->m_native.user.typeSerializer = &m_hashValueSerializer;
			st->m_native.user.flags |= rdxCStructuredType::NativeProperties::UserProperties::STUF_NativeSafeToSerialize;
			return rdxSAutoTypeInfo<rdxHashValue>::TypeInfoInterface();
		}

		return NULL;
	}
	
	virtual rdxNativeCallback HookMethod(rdxSObjectGUID methodGUID) const
	{
		return NULL;
	}

};

const rdxINativeTypeHost *rdxPlugins_GetNumericTypes()
{
	static rdxCNumericTypesPlugin plugin;
	return &plugin;
}
