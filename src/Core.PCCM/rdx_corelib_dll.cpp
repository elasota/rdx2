#include "rdx_pccm_api.hpp"

const rdxSPCCMDomainIndex *RDX_Corelib_PCCMGlue();
const rdxSPluginExport *RDX_Corelib_PluginGlue();

extern "C" RDX_DLLEXPORT const rdxSPCCMDomainIndex *RDX_SharedLib_LoadPCCM()
{
	return RDX_Corelib_PCCMGlue();
}

extern "C" RDX_DLLEXPORT const rdxSPluginExport *RDX_SharedLib_LoadPlugin()
{
	return RDX_Corelib_PluginGlue();
}
