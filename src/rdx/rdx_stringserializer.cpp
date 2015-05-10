#include "rdx_objectmanagement_internal.hpp"
#include "rdx_processing.hpp"
#include "rdx_io.hpp"

/////////////////////////////////////////////////////////////////////////////
// StringSerializer
void rdxCObjectManager::StringSerializer::DeserializeTextInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakOffsetHdl(void) instance, rdxITextDeserializer *td, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const
{
	RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
}

void rdxCObjectManager::StringSerializer::DeserializeBinaryInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakOffsetHdl(void) instance, rdxIFileStream *reader, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const
{
	RDX_LTHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
}

void rdxCObjectManager::StringSerializer::SerializeBinaryInstance(rdxIObjectManager *objm, rdxWeakOffsetRTRef(void) obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const
{
}

void rdxCObjectManager::StringSerializer::SerializeTextInstance(rdxIObjectManager *objm, rdxWeakOffsetRTRef(void) obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const
{
}

rdxCObjectManager::StringSerializer			rdxCObjectManager::StringSerializer::instance;
