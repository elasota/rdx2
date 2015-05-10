#ifndef __RDXTEST_SYSTEMPLUGINAPI_HPP__
#define __RDXTEST_SYSTEMPLUGINAPI_HPP__

#include "../rdx/rdx_coretypes.hpp"

struct rdxSPCCMDomainIndex;
struct rdxSPluginExport;

struct systemPluginApi
{
	virtual void Init() = 0;
	virtual void Shutdown() = 0;
	virtual rdxLargeUInt NumPlugins() const = 0;
	virtual const rdxSPCCMDomainIndex *GetPCCM(rdxLargeUInt pluginIndex) const = 0;
	virtual const rdxSPluginExport *GetAPI(rdxLargeUInt pluginIndex) const = 0;
};

extern systemPluginApi *rdxTestPluginApi;


#endif
