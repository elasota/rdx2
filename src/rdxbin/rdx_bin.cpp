#include "../rdx/rdx_pragmas.hpp"

#include <stdlib.h>
#include <stdio.h>

#include "../rdx/rdx.h"
#include "../rdx/rdx_io.hpp"
#include "../rdx/rdx_objectmanagement.hpp"
#include "../rdx/rdx_constants.hpp"
#include "../rdx/rdx_interpret.hpp"
#include "../rdx/rdx_marshal.hpp"
#include "../rdx/rdx_longflow.hpp"
#include "../rdx/rdx_zonepolicy.hpp"
#include "../rdx/rdx_plugin.hpp"
#include "../rdx/rdx_objectloader.hpp"
#include "../rdx/rdx_appservices.hpp"
#include "../rdx/rdx_ilcomp.hpp"
#include "../rdx/rdx_nullcodeprovider.hpp"

using namespace RDX;
using namespace RDX::ObjectManagement;
using namespace RDX::Programmability;
using namespace RDX::Utility;
using namespace RDX::Marshaling;



enum
{
	DOMAIN_C2Test = DOMAIN_Custom,
};

int RDX_DECL_API DummyCallback(rdxSOperationContext *ctx, rdxIObjectManager *objm, const Method *m, RuntimeThread *thread,
	rdxURuntimeStackValue *prv)
{
	return RuntimeState::Active;
}


struct CMyTypeHost : public rdxINativeTypeHost
{
	void SetTypeProperties(rdxIObjectManager *objm, StructuredType *st) const
	{
		const String *str = GCInfo::From(st)->gstSymbol;
		if(!str)
			return;
	}

	NativeCallback HookMethod(const String *str) const
	{
		return DummyCallback;
	}
};
CMyTypeHost myTypeHost;

struct AppPlugin
{
	void *lib;
	const rdxINativeTypeHost *plugin;
	AppPlugin *nextPlugin;
};

AppPlugin *lastPlugin;
AppPlugin *firstPlugin;

static int numPlugins = 0;
static const rdxINativeTypeHost **pluginsList;

NullCodeProvider myCodeProvider;

int main(int argc, const char **argv)
{
	Allocator alloc;
	alloc.reallocFunc = AppServices::Realloc;
	const char *xilPath = NULL;
	bool exportRXB = false;

	int returnValue = 0;

	RDX_Initialize();

	// Scan for execution arguments
	while(true)
	{
		if(argc == 1)
			break;
		const char *nextArg = argv[1];
		if(!strcmp(nextArg, "-plugin"))
		{
			if(argc == 2)
			{
				fprintf(stderr, "No argument specified for -plugin");
				return -1;
			}
			void *lib = AppServices::LoadLib(argv[2]);
			if(lib)
			{
				AppPlugin *plugin = new AppPlugin();
				plugin->lib = lib;
				plugin->nextPlugin = NULL;
				const void *(*retrFunc)() = static_cast<const void *(*)()>(AppServices::GetProc(lib, "RDXPlugin"));
				if(retrFunc)
				{
					plugin->plugin = static_cast<const rdxINativeTypeHost *>(retrFunc());
					numPlugins++;
				}
				else
					plugin->plugin = NULL;
				if(firstPlugin)
					lastPlugin->nextPlugin = plugin;
				else
					firstPlugin = plugin;
				lastPlugin = plugin;
			}
			argc -= 2;
			argv += 2;
		}
		else if(!strcmp(argv[1], "-xil"))
		{
			if(argc == 2)
			{
				fprintf(stderr, "No argument specified for -xil");
				return -1;
			}
			xilPath = argv[2];
			argc -= 2;
			argv += 2;
		}
		else if(!strcmp(argv[1], "-package"))
		{
			exportRXB = true;
			argc--;
			argv++;
		}
		else
			break;
	}

	// Flatten
	{
		pluginsList = new const rdxINativeTypeHost*[numPlugins+3];
		int pccmIdx = 0;
		int pluginIdx = 0;
		pluginsList[pluginIdx++] = RDX_GetNumericTypesPlugin();
		for(AppPlugin *plug=firstPlugin;plug;plug=plug->nextPlugin)
		{
			if(plug->plugin)
				pluginsList[pluginIdx++] = plug->plugin;
		}
		pluginsList[pluginIdx++] = &myTypeHost;
		pluginsList[pluginIdx++] = NULL;
	}
	
	CombinedNativeTypeHost pluginsTypeHost(pluginsList);


	rdxIObjectManager *om = RDX_CreateObjectManager(&alloc, &pluginsTypeHost, &myCodeProvider);


	{
		CRef<const String> str = NULL;
		CRef<void> obj = NULL;
		//MyPackageHost host;

		rdxSOperationContext myContext(om);

		{
			RDX_TRY(&myContext)
			{
				Security::ZonePolicyPackageHost zonePolicyHost(&myContext, om, AppServices::GetFileSystem(), true);

				{
					RuntimeObjectLoaderHost zoneLoaderHost(&zonePolicyHost, DOMAIN_Runtime, AppServices::GetFileSystem());
					zoneLoaderHost.SetPackage(RDX_STATIC_STRING("Apps.zonepolicy"), true);

					CRef<Security::Zone*> zones;
					
					RDX_PROTECT_ASSIGN(&myContext, zones, om->LoadObject(&myContext, &zoneLoaderHost).Cast<Security::Zone*>());
					RDX_PROTECT(&myContext, zonePolicyHost.LoadZones(&myContext, zones));
				}
				
				RDX_PROTECT_ASSIGN(&myContext, str, om->CreateStringASCII(&myContext, argv[1]) );
				RDX_PROTECT_ASSIGN(&myContext, obj, om->LookupSymbol(&myContext, str, &zonePolicyHost) );
				if(!obj)
				{
					fprintf(stderr, "Failed to find specified object");
					return -1;
				}
				
				if(exportRXB)
					om->SavePackage(&myContext, GCInfo::From(obj)->domain, &zonePolicyHost);

				if(xilPath)
				{
					FILE *f = fopen(xilPath, "wb");
					if(!f)
						fprintf(stderr, "Could not open %s for write\n", xilPath);
					RDX_ExportSource(om, f, GCInfo::From(obj)->domain);
					fclose(f);
				}
			}
			RDX_CATCH(&myContext)
			{
				fprintf(stderr, "Excepted: %i\n", errorCode);
			}
			RDX_ENDTRY
		}
	}
	om->Shutdown();

	return returnValue;
}
