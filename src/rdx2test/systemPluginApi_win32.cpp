#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <direct.h>
#include "systemPluginApi.hpp"

struct rdxTestDLL
{
	HMODULE dll;
	const rdxSPCCMDomainIndex *pccm;
	const rdxSPluginExport *plugin;
};

struct systemPluginApiWin32 : public systemPluginApi
{
private:
	std::vector<rdxTestDLL> dlls;
	void AddLibrary(const char *path);

public:
	virtual void Init();
	virtual void Shutdown();
	virtual rdxLargeUInt NumPlugins() const;
	virtual const rdxSPCCMDomainIndex *GetPCCM(rdxLargeUInt pluginIndex) const;
	virtual const rdxSPluginExport *GetAPI(rdxLargeUInt pluginIndex) const;

	static systemPluginApiWin32 instance;
};

systemPluginApiWin32 systemPluginApiWin32::instance;
systemPluginApi *rdxTestPluginApi = &systemPluginApiWin32::instance;

void systemPluginApiWin32::AddLibrary(const char *path)
{
	char *exPath = new char[strlen(path) + 4 + 1];
	strcpy(exPath, path);
	exPath[strlen(exPath) - 4] = '\0';
	strcat(exPath, "_dbg.dll");

	HMODULE module = LoadLibraryA(exPath);
	if(module)
	{
		typedef const rdxSPCCMDomainIndex *(*pccmFunc_t)();
		typedef const rdxSPluginExport *(*pluginFunc_t)();

		rdxTestDLL dll;
		dll.dll = module;
		dll.plugin = RDX_CNULL;
		dll.pccm = RDX_CNULL;
		pccmFunc_t pccmFunc = reinterpret_cast<pccmFunc_t>(GetProcAddress(module, "RDX_SharedLib_LoadPCCM"));
		pluginFunc_t pluginFunc = reinterpret_cast<pluginFunc_t>(GetProcAddress(module, "RDX_SharedLib_LoadPlugin"));
		if(pccmFunc)
			dll.pccm = pccmFunc();
		if(pluginFunc)
			dll.plugin = pluginFunc();
		dlls.push_back(dll);
	}
	delete[] exPath;
}

void systemPluginApiWin32::Init()
{
	AddLibrary("Apps.Common.dll");
	AddLibrary("Apps.MyApp.dll");
	AddLibrary("Core.dll");
	//AddLibrary("CompilerPCCM.dll");
	AddLibrary("rdxcompilerservices.dll");
}

void systemPluginApiWin32::Shutdown()
{
	for(std::vector<rdxTestDLL>::iterator it = dlls.begin(), itEnd = dlls.end(); it != itEnd; ++it)
	{
		CloseHandle(it->dll);
	}
	dlls.clear();
}

rdxLargeUInt systemPluginApiWin32::NumPlugins() const
{
	return dlls.size();
}

const rdxSPCCMDomainIndex *systemPluginApiWin32::GetPCCM(rdxLargeUInt pluginIndex) const
{
	return dlls[pluginIndex].pccm;
}

const rdxSPluginExport *systemPluginApiWin32::GetAPI(rdxLargeUInt pluginIndex) const
{
	return dlls[pluginIndex].plugin;
}
