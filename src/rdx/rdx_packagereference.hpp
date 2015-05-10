#ifndef __RDX_PACKAGEREFERENCE_HPP__
#define __RDX_PACKAGEREFERENCE_HPP__

#include "rdx_packagesymbolloc.hpp"
#include "rdx_coretypes.hpp"
#include "rdx_reftypealiases.hpp"

struct rdxSOperationContext;
class rdxCPackage;

struct rdxSPackageReference
{
	rdxEPackageSymbolLoc symbolLoc;
	rdxLargeUInt index;

	void ConvertToReference(rdxSOperationContext *ctx, rdxWeakHdl(rdxCPackage) pkg, bool allowCloaked, rdxWeakHdl(rdxCObject) *outRef);
};

#endif
