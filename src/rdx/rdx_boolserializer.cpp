#include "rdx_objectmanagement_internal.hpp"
#include "rdx_processing.hpp"
#include "rdx_io.hpp"

////////////////////////////////////////////////////////////////////////////////////////////
// BoolSerializer
void rdxCObjectManager::BoolSerializer::DeserializeTextInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxITextDeserializer *td, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const
{
	RDX_TRY(ctx)
	{
		rdxITextDeserializer::SCompactToken token;
		bool isString;
		RDX_PROTECT(ctx, td->ParseToken(ctx, objm, &isString, &token));
		if(token.GetCharSpan().Equal("true"))
			*instance.StaticCast<rdxBool>().Modify() = rdxTrueValue;
		else
			*instance.StaticCast<rdxBool>().Modify() = rdxFalseValue;
		outShouldProcessProperties = false;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxCObjectManager::BoolSerializer::DeserializeBinaryInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxIFileStream *reader, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const
{
	rdxBool bVal;
	if(reader->ReadSwappableBytes(&bVal, sizeof(rdxBool)) != sizeof(rdxBool))
	{
		RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
	}
	*instance.StaticCast<rdxBool>().Modify() = ((bVal == rdxFalseValue) ? rdxFalseValue : rdxTrueValue);
	outShouldProcessProperties = false;
}
		
void rdxCObjectManager::BoolSerializer::SerializeBinaryInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const
{
	fs->WriteSwappableBytes(obj.Data(), sizeof(rdxBool));
	outShouldProcessProperties = false;
}

void rdxCObjectManager::BoolSerializer::SerializeTextInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const
{
	if(*obj.StaticCast<rdxBool>().Data() != rdxFalseValue)
		fs->WriteBytes(" true", 5);
	else
		fs->WriteBytes(" false", 6);
	outShouldProcessProperties = false;
}


rdxCObjectManager::BoolSerializer			rdxCObjectManager::BoolSerializer::instance;
