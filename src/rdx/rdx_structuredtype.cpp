#include "rdx_objectmanagement.hpp"
#include "rdx_io.hpp"
#include "rdx_package.hpp"

//////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_CLASS(rdxCStructuredType, (rdxETIF_VisitReferences | rdxETIF_CompareDuplicate));

RDX_BEGIN_PROPERTY_LOOKUP_CLASS(rdxCStructuredType)
	RDX_DEFINE_LOOKUP_PROPERTY(parentClass)
	RDX_DEFINE_LOOKUP_PROPERTY(interfaces)
	RDX_DEFINE_LOOKUP_PROPERTY(storageSpecifier)
	RDX_DEFINE_LOOKUP_PROPERTY(virtualMethods)
	RDX_DEFINE_LOOKUP_PROPERTY(properties)
	RDX_DEFINE_LOOKUP_PROPERTY(enumerants)
	RDX_DEFINE_LOOKUP_PROPERTY(isFinal)
	RDX_DEFINE_LOOKUP_PROPERTY(isAbstract)
	RDX_DEFINE_LOOKUP_PROPERTY(isLocalized)
RDX_END_PROPERTY_LOOKUP

//////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxCStructuredType::NativeProperties::ContainedReference, (rdxETIF_VisitReferences));

//////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxSInterfaceImplementation, (rdxETIF_NoFlags));

RDX_BEGIN_PROPERTY_LOOKUP_STRUCT(rdxSInterfaceImplementation)
	RDX_DEFINE_LOOKUP_PROPERTY(type)
	RDX_DEFINE_LOOKUP_PROPERTY(vftOffset)
RDX_END_PROPERTY_LOOKUP

//////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxSProperty, (rdxETIF_NoFlags));

RDX_BEGIN_PROPERTY_LOOKUP_STRUCT(rdxSProperty)
	RDX_DEFINE_LOOKUP_PROPERTY(name)
	RDX_DEFINE_LOOKUP_PROPERTY(type)
	RDX_DEFINE_LOOKUP_PROPERTY(isConstant)
	RDX_DEFINE_LOOKUP_PROPERTY(mustBeConstant)
RDX_END_PROPERTY_LOOKUP

//////////////////////////////////////////////////////////////////////
RDX_IMPLEMENT_COMPLEX_NATIVE_STRUCT(rdxSEnumerant, (rdxETIF_NoFlags));

RDX_BEGIN_PROPERTY_LOOKUP_STRUCT(rdxSEnumerant)
	RDX_DEFINE_LOOKUP_PROPERTY(name)
	RDX_DEFINE_LOOKUP_PROPERTY(value)
RDX_END_PROPERTY_LOOKUP

rdxCStructuredType::rdxCStructuredType(rdxIObjectManager *objm, rdxGCInfo *info)
	: rdxCType(objm, info)
{
}

void rdxCStructuredType::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	if(ObjectInfo()->SerializationTag() && ObjectInfo()->SerializationTag()->gstSymbol == rdxSObjectGUID::FromObjectName("RDX.Compiler", "MemberDeclNode"))
	{
		int bp = 0;
	}

	if(visitNonSerializable)
	{
		visitor->VisitReference(objm, m_native.containedReferences);
		visitor->VisitReference(objm, m_native.propertyOffsets);
		visitor->VisitReference(objm, m_native.interfaceOffsets);
		visitor->VisitReference(objm, m_native.currentDefaultValue);
	}
	visitor->VisitReference(objm, m_native.originalDefaultValue);
}

bool rdxCStructuredType::DuplicateEqual(const rdxCStructuredType &other) const
{
	return this->parentClass == other.parentClass &&
		this->interfaces == other.interfaces &&
		this->storageSpecifier == other.storageSpecifier &&
		this->virtualMethods == other.virtualMethods &&
		this->properties == other.properties &&
		this->enumerants == other.enumerants &&
		this->isFinal == other.isFinal &&
		this->isAbstract == other.isAbstract &&
		this->isLocalized == other.isLocalized &&
		// Duplicates must use the same default
		this->m_native.originalDefaultValue == other.m_native.originalDefaultValue;
}


void rdxCStructuredTypeSerializer::SetDefaultFromPackageRef(rdxSOperationContext *ctx, rdxWeakTypelessOffsetHdl instance, const rdxSPackageReference &pkgRef) const
{
	RDX_TRY(ctx)
	{
		rdxWeakOffsetHdl(rdxCStructuredType) st = instance.StaticCast<rdxCStructuredType>();

		switch(pkgRef.symbolLoc)
		{
		case rdxPSL_Null:
		case rdxPSL_Local:
			st->m_native.defaultValueReference = pkgRef;
			break;

		// Anything else is illegal
		default:
			RDX_STHROW(ctx, RDX_ERROR_INTERNAL_GENERAL);
		}
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxCStructuredTypeSerializer::DeserializeTextInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxITextDeserializer *td, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const
{
	RDX_TRY(ctx)
	{
		rdxITextDeserializer::SCompactToken token;

		bool isString;
		RDX_PROTECT(ctx, td->ParseToken(ctx, objm, &isString, &token));
		if(!token.GetCharSpan().Equal("defaultValue") || isString)
			RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		RDX_PROTECT(ctx, td->ParseToken(ctx, objm, &isString, &token));
		if(!token.GetCharSpan().Equal(":") || isString)
			RDX_STHROW(ctx, RDX_ERROR_PACKAGE_CORRUPTED);
		rdxSPackageReference pkgRef;
		RDX_PROTECT(ctx, rdxDeserializeReference(ctx, objm, pkg, host, td, true, false, NULL, &pkgRef));
		RDX_PROTECT(ctx, SetDefaultFromPackageRef(ctx, instance, pkgRef));

		outShouldProcessProperties = true;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxCStructuredTypeSerializer::DeserializeBinaryInstance(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxWeakTypelessOffsetHdl instance, rdxIFileStream *reader, rdxIPackageHost *host, rdxWeakHdl(rdxCPackage) pkg, bool &outShouldProcessProperties) const
{
	RDX_TRY(ctx)
	{
		rdxSPackageReference pkgRef;
		RDX_PROTECT(ctx, rdxDeserializeReference(ctx, objm, pkg, host, reader, false, false, NULL, &pkgRef));
		RDX_PROTECT(ctx, SetDefaultFromPackageRef(ctx, instance, pkgRef));

		outShouldProcessProperties = true;
	}
	RDX_CATCH(ctx)
	{
		RDX_RETHROW(ctx);
	}
	RDX_ENDTRY
}

void rdxCStructuredTypeSerializer::SerializeBinaryInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const
{
	rdxWriteReference(objm, fs, false, ssidTable, obj.StaticCast<rdxCStructuredType>()->m_native.originalDefaultValue.ToWeakHdl(), true);
	outShouldProcessProperties = true;
}

void rdxCStructuredTypeSerializer::SerializeTextInstance(rdxIObjectManager *objm, rdxWeakTypelessOffsetRTRef obj, rdxIFileStream *fs, const rdxCSSIDTable *ssidTable, bool &outShouldProcessProperties) const
{
	fs->WriteBytes("defaultValue : ", 15);
	rdxWriteReference(objm, fs, true, ssidTable, obj.StaticCast<rdxCStructuredType>()->m_native.originalDefaultValue.ToWeakHdl(), true);
	outShouldProcessProperties = true;
}


void rdxCStructuredType::NativeProperties::ContainedReference::VisitReferences(rdxIObjectManager *objm, rdxIObjectReferenceVisitor *visitor, bool visitNonSerializable)
{
	visitor->VisitReference(objm, this->requiredType);
}

rdxCStructuredTypeSerializer rdxCStructuredTypeSerializer::instance;
