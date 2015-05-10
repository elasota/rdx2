#include "rdx_pccm_api.hpp"
#include "rdx_plugin_api.hpp"

const rdxSPCCMDomainIndex *RDX_AppCommon_PCCMGlue();
const rdxSPluginExport *RDX_AppCommon_PluginGlue();

//extern "C" RDX_DLLEXPORT const rdxSPCCMDomainIndex *RDX_SharedLib_LoadPCCM()
//{
//	return RDX_AppCommon_PCCMGlue();
//}


extern "C" RDX_DLLEXPORT const rdxSPluginExport *RDX_SharedLib_LoadPlugin()
{
	return RDX_AppCommon_PluginGlue();
}
