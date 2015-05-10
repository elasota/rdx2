#include "rdx_pccm_api.hpp"
#include "rdx_plugin_api.hpp"

const rdxSPluginExport *RDX_Compiler_PluginGlue();

extern "C" RDX_DLLEXPORT const rdxSPluginExport *RDX_SharedLib_LoadPlugin()
{
	return RDX_Compiler_PluginGlue();
}
