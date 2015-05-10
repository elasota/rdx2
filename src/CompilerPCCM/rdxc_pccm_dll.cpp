#include "rdx_pccm_api.hpp"

const rdxSPCCMDomainIndex *RDX_Compiler_PCCMGlue();

extern "C" RDX_DLLEXPORT const rdxSPCCMDomainIndex *RDX_SharedLib_LoadPCCM()
{
	return RDX_Compiler_PCCMGlue();
}
