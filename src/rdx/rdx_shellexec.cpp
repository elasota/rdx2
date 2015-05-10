/*
 * Copyright (C) 2011-2013 Eric Lasota
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "rdx_pragmas.hpp"

#include <stdlib.h>
#include <stdio.h>

#include "rdx.h"
#include "rdx_io.hpp"
#include "rdx_objectmanagement.hpp"
#include "rdx_constants.hpp"
#include "rdx_interpret.hpp"
#include "rdx_marshal.hpp"
#include "rdx_longflow.hpp"
#include "rdx_zonepolicy.hpp"
#include "rdx_plugin.hpp"
#include "rdx_objectloader.hpp"
#include "rdx_appservices.hpp"


static unsigned int numPlugins = 0;
static unsigned int numPCCM = 0;
static const rdxINativeTypeHost **pluginsList;
static const rdxCPrecompiledCodeModule **pccmList;

int main(int argc, const char **argv)
{
	Allocator alloc;
	alloc.reallocFunc = AppServices::Realloc;
	bool exportBin = false;
	const char *xilPath = NULL;

	int returnValue = 0;

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
				retrFunc = static_cast<const void *(*)()>(AppServices::GetProc(lib, "RDXPCCM"));
				if(retrFunc)
				{
					plugin->pccm = static_cast<const rdxCPrecompiledCodeModule *>(retrFunc());
					numPCCM++;
				}
				else
					plugin->pccm = NULL;
				if(firstPlugin)
					lastPlugin->nextPlugin = plugin;
				else
					firstPlugin = plugin;
				lastPlugin = plugin;
			}
			else
			{
				fprintf(stderr, "Couldn't open plugin %s\n", argv[2]);
				return -1;
			}
			argc -= 2;
			argv += 2;
		}
		else
			break;
	}

	// Flatten
	{
		pluginsList = new const rdxINativeTypeHost*[numPlugins+3];
		pccmList = new const rdxCPrecompiledCodeModule*[numPCCM];
		int pccmIdx = 0;
		int pluginIdx = 0;
		pluginsList[pluginIdx++] = RDX_GetNumericTypesPlugin();
		for(AppPlugin *plug=firstPlugin;plug;plug=plug->nextPlugin)
		{
			if(plug->plugin)
				pluginsList[pluginIdx++] = plug->plugin;
			if(plug->pccm)
				pccmList[pccmIdx++] = plug->pccm;
		}

		pluginsList[pluginIdx++] = &myTypeHost;
		pluginsList[pluginIdx++] = NULL;
	}
	
	CombinedNativeTypeHost pluginsTypeHost(pluginsList);

	RDX_Initialize();
	rdxICodeProvider *myCodeProvider = RDX_CreateInterpreterCodeProvider(&alloc, pccmList, static_cast<rdxLargeInt>(numPCCM));

	rdxIObjectManager *om = RDX_CreateObjectManager(&alloc, &pluginsTypeHost, myCodeProvider);


	{
		CRef<const String> str = NULL;
		CRef<void> obj = NULL;
		//MyPackageHost host;
		CRef<Method> testMethod;

		rdxSOperationContext myContext(om);

		{
			RDX_TRY(&myContext)
			{
				zonePolicyHost = new Security::ZonePolicyPackageHost(&myContext, om, AppServices::GetFileSystem(), true);

				{
					RuntimeObjectLoaderHost zoneLoaderHost(zonePolicyHost, DOMAIN_Runtime, AppServices::GetFileSystem());
					zoneLoaderHost.SetPackage(RDX_STATIC_STRING("Apps.zonepolicy"), true);

					CRef<Security::Zone*> zones;
					
					RDX_PROTECT_ASSIGN(&myContext, zones, om->LoadObject(&myContext, &zoneLoaderHost).Cast<Security::Zone*>());
					RDX_PROTECT(&myContext, zonePolicyHost->LoadZones(&myContext, zones));
				}


				{
					CRef<const String> className;
					CRef<const String> mainExtension;
					RDX_PROTECT_ASSIGN(&myContext, className, om->CreateStringASCII(&myContext, argv[1]) );
					RDX_PROTECT_ASSIGN(&myContext, mainExtension, om->CreateStringASCII(&myContext, "/methods/main(#Core.string[C])") );
					RDX_PROTECT_ASSIGN(&myContext, str, om->CreateStringConcatenated(&myContext, className, mainExtension) );
				}
				RDX_PROTECT_ASSIGN(&myContext, testMethod, om->LookupSymbol(&myContext, str, zonePolicyHost).Cast<Method>() );

				/*
				{
					CRef<const String> str = om->CreateStringASCII(&myContext, "Apps.RegressionTest.RegressionTest/methods/TestDirectInvokeGC(Core.string)", -1, true);
					if(str)
					{
						CRef<Method> gcmeth;
						RDX_PROTECT_ASSIGN(&myContext, gcmeth, om->LookupSymbolSimple(&myContext, str).Cast<Method>() );

						CRef<RuntimeThread> myThread;
						RDX_PROTECT_ASSIGN(&myContext, myThread, om->CreateThread(&myContext, 102400) );
						
						Value<const String *> *mParams;
						RDX_PROTECT(&myContext, myThread->Precall(&myContext, RDX_MARSHAL_NORETURN(mParams)));
						mParams->value = om->CreateStringASCII(&myContext, "Test direct GC");

						int returnedStatus;
						RDX_PROTECT_ASSIGN(&myContext, returnedStatus, gcmeth->Invoke(&myContext, myThread, 1000) );
					}
				}
				*/

				if(!testMethod)
				{
					fprintf(stderr, "Failed to find main method in provided class\n");
					return -1;
				}

				CRef<ArrayOfType> stringArrayType;
				RDX_PROTECT_ASSIGN(&myContext, stringArrayType, om->CreateArrayType(&myContext, om->GetBuiltIns()->st_String, 1, true) );

				CRef<const String*> myArgs;
				RDX_PROTECT_ASSIGN(&myContext, myArgs, om->Create1DArray<const String*>(&myContext, argc - 2, stringArrayType) );

				for(rdxLargeInt i=2;i<argc;i++)
				{
					RDX_PROTECT_ASSIGN(&myContext, myArgs[i-2], om->CreateStringASCII(&myContext, argv[i]) );
				}

				ReturnAndParameterFrame<Value<Int>, CallFrame1<const String **> > *mParams;
				
				CRef<RuntimeThread> myThread;
				RDX_PROTECT_ASSIGN(&myContext, myThread, om->CreateThread(&myContext, 102400) );

				RDX_PROTECT(&myContext, myThread->Precall(&myContext, RDX_MARSHAL_FRAME(mParams)));
				mParams->parameters.value1.GetValue() = myArgs;

				int returnedStatus;
				RDX_PROTECT_ASSIGN(&myContext, returnedStatus, testMethod->Invoke(&myContext, myThread, 10000000) );

				while(returnedStatus == RuntimeState::Exception)
				{
					bool recovered = true;
					RDX_PROTECT_ASSIGN(&myContext, recovered, myThread->Recover(&myContext));
					if(!recovered)
						break;
					RDX_PROTECT_ASSIGN(&myContext, returnedStatus, myThread->Resume(&myContext, 1000000) );
				}

				if(returnedStatus == RuntimeState::Exception)
				{
					wprintf(L"Exception.  Type: %s\n", GCInfo::From(GCInfo::From(myThread->ex)->containerType)->gstSymbol->AsChars());
					RuntimeTrace trace;
					bool traced = myThread->Trace(trace);
					while(traced)
					{
						const String *filename;
						rdxLargeInt line;
						RDX_RuntimeTrace_GetFileNameAndLineNumber(&trace, &filename, &line);
						wprintf(L"Method: %s Line %i File %s\n", GCInfo::From(trace.method)->gstSymbol->AsChars(), line, filename->AsChars());
						traced = (RDX_RuntimeTrace_TraceParent(&trace, &trace) != 0);
					}
				}

				returnValue = mParams->returnValues.GetValue();
			}
			RDX_CATCH(&myContext)
			{
				wprintf(L"An internal error occurred: %i\n", errorCode);
			}
			RDX_ENDTRY
		}
	}
	om->Shutdown();
	myCodeProvider->Shutdown();

	if(zonePolicyHost)
		delete zonePolicyHost;

	return returnValue;
}
