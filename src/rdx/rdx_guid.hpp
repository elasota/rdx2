#ifndef __RDX_GUID_HPP__
#define __RDX_GUID_HPP__

#include "rdx_coretypes.hpp"
#include "rdx_api.hpp"
#include "rdx_builtindomain.hpp"

struct rdxSDomainGUID;

RDX_UTIL_DYNLIB_API void RDX_BuiltinDomainGUID(rdxEBuiltinDomain builtinDomain, rdxSDomainGUID *outGUID);
RDX_UTIL_DYNLIB_API void RDX_ComputeGUID(const char *name, rdxUInt8 *bytes);

#endif
