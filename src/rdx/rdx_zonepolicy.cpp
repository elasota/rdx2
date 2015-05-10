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
//#include "rdx_zonepolicy.hpp"
#include "rdx_typeprocessor.hpp"
//#include "rdx_gcinfo.hpp"
//#include "rdx_io.hpp"
//#include "rdx_blockcopy.hpp"

#if 0

rdxZonePolicyPackageHost::~rdxZonePolicyPackageHost()
{
}

rdxZonePolicyPackageHost::rdxZonePolicyPackageHost(rdxSOperationContext *ctx, rdxIObjectManager *objm, IO::IFileSystem *fs, bool forceTextRead)
{
	m_fs = fs;
	m_objm = objm;
	m_numZones = 0;
	m_forceTextRead = forceTextRead;

			RDX_TRY(ctx)
			{
				CRef<const String> str;
				CRef<Type> t;

				RDX_PROTECT_ASSIGN(ctx, str, objm->CreateStringASCII(ctx, "Core.RDX.Zone"));
				RDX_PROTECT_ASSIGN(ctx, t, objm->LookupSymbol(ctx, str, this).Cast<Type>());
				RDX_PROTECT_ASSIGN(ctx, _zoneArrayType, objm->CreateArrayType(ctx, t, 1, true));
				
				RDX_PROTECT_ASSIGN(ctx, str, objm->CreateStringASCII(ctx, "Core.string"));
				RDX_PROTECT_ASSIGN(ctx, t, objm->LookupSymbol(ctx, str, this).Cast<Type>());
				RDX_PROTECT_ASSIGN(ctx, _stringArrayType, objm->CreateArrayType(ctx, t, 1, true));
			}
			RDX_CATCH(ctx)
			{
				RDX_RETHROW(ctx);
			}
			RDX_ENDTRY
		}

		static void InsertZones(rdxSOperationContext *ctx, rdxIObjectManager *objm, rdxLargeInt numZones, Zone **zones, IO::IFileSystem *fsys,
			ZonePolicyPackageHost::ZoneInfo *zoneInfos, rdxLargeInt *packageZones, const String **packageNames, rdxLargeInt &numPackages)
		{
			IO::IFileScanState *scanState = NULL;
			Char expandedPath[MAX_ZONEPOLICY_PATH+1];

			RDX_TRY(ctx)
			{
				CRef<const String> str;

				for(rdxLargeInt zoneNum=0;zoneNum<numZones;zoneNum++)
				{
					if(zoneInfos)
					{
						zoneInfos[zoneNum].firstPackage = numPackages;
						zoneInfos[zoneNum].numPackages = 0;
					}

					Char lastPath[MAX_ZONEPOLICY_PATH+1];
					rdxLargeInt lastPathLength = 0;

					rdxLargeInt zonePathLength = zones[zoneNum]->namespacePrefix->Length();
					BlockCopy(expandedPath, zones[zoneNum]->namespacePrefix->AsChars(), sizeof(Char) * zonePathLength);
					expandedPath[zonePathLength] = static_cast<Char>(0);

					// Remap to a directory
					for(rdxLargeInt j=0;j<zonePathLength;j++)
						if(expandedPath[j] == '.')
							expandedPath[j] = RDX_FILESYSTEM_DELIMITER;

					RDX_PROTECT_ASSIGN(ctx, scanState, fsys->ScanDirectory(expandedPath, RDX_STATIC_STRING("*.rx?"), NULL));

					RDX_PROTECT_ASSIGN(ctx, str, scanState->NextFile(ctx, objm));
					while(str != NULL)
					{
						if(str->Length() > MAX_ZONEPOLICY_PATH || MAX_ZONEPOLICY_PATH - str->Length() < zonePathLength)
							RDX_STHROW(ctx, RDX_ERROR_INTEGER_OVERFLOW);
						
						// Rebuild with the period on it
						BlockCopy(expandedPath, zones[zoneNum]->namespacePrefix->AsChars(), sizeof(Char) * zonePathLength);
						BlockCopy(expandedPath + zonePathLength, str->AsChars(), sizeof(Char) * (str->Length() - 3));	// Keep the final .
						rdxLargeInt expandedPathLength = zonePathLength + str->Length() - 3;
						expandedPath[expandedPathLength] = static_cast<Char>(0);

						// See if this was the same as the previous
						if(expandedPathLength != lastPathLength || memcmp(expandedPath, lastPath, sizeof(Char) * expandedPathLength))
						{
							BlockCopy(lastPath, expandedPath, sizeof(Char) * (expandedPathLength + 1));
							lastPathLength = expandedPathLength;
							
							if(zoneInfos)
								zoneInfos[zoneNum].numPackages++;

							if(packageZones)
								packageZones[numPackages] = zoneNum;

							if(packageNames)
							{
								RDX_PROTECT_ASSIGN(ctx, packageNames[numPackages], objm->CreateString(ctx, expandedPath, expandedPathLength));
							}

							numPackages++;
						}

						RDX_PROTECT_ASSIGN(ctx, str, scanState->NextFile(ctx, objm));
					}
					scanState->Close();
					scanState = NULL;

				}
			}
			RDX_CATCH(ctx)
			{
				if(scanState)
					scanState->Close();
				RDX_RETHROW(ctx);
			}
			RDX_ENDTRY
		}

		void ZonePolicyPackageHost::LoadZones(rdxSOperationContext *ctx, Zone **zones)
		{

			RDX_TRY(ctx)
			{
				rdxLargeInt numZones = GCInfo::From(zones)->numElements;
				rdxLargeInt numPackages = 0;

				RDX_PROTECT(ctx, InsertZones(ctx, _objm, numZones, zones, _fs, NULL, NULL, NULL, numPackages));
				RDX_PROTECT_ASSIGN(ctx, _zoneInfo, _objm->Create1DArray<ZoneInfo>(ctx, numZones));
				RDX_PROTECT_ASSIGN(ctx, _packageZones, _objm->Create1DArray<rdxLargeInt>(ctx, numPackages));
				RDX_PROTECT_ASSIGN(ctx, _packageNames, _objm->Create1DArray<const String*>(ctx, numPackages, this->_stringArrayType));

				numPackages = 0;
				RDX_PROTECT(ctx, InsertZones(ctx, _objm, numZones, zones, _fs, _zoneInfo, _packageZones, _packageNames, numPackages));

				_zones = zones;
				_numZones = numZones;
			}
			RDX_CATCH(ctx)
			{
				RDX_RETHROW(ctx);
			}
			RDX_ENDTRY
		}

		Domain ZonePolicyPackageHost::DomainForSymbolName(rdxIObjectManager *om, const String *str, Domain domain)
		{
			if(str->StartsWith("#"))
				return DOMAIN_Duplicable;

			if(str->StartsWith("Core."))
				return DOMAIN_Core;

			for(rdxLargeInt i=0;i<_numZones;i++)
			{
				if(str->StartsWith(_zones[i]->namespacePrefix))
				{
					rdxLargeInt numPackages = _zoneInfo[i].numPackages;
					rdxLargeInt firstPackage = _zoneInfo[i].firstPackage;
					for(rdxLargeInt j=0;j<numPackages;j++)
					{
						if(str->StartsWith(_packageNames[j + firstPackage]))
							return static_cast<Domain>(static_cast<rdxLargeInt>(DOMAIN_Custom) + firstPackage + j);
					}
					break;
				}
			}

			return DOMAIN_Invalid;
		}

		IO::IFileStream *ZonePolicyPackageHost::StreamForDomain(rdxIObjectManager *om, Domain domain, bool write, bool &isText)
		{
			Char fullPath[MAX_ZONEPOLICY_PATH + 5];

			bool preferTextOut;
			rdxLargeInt pathLength;

			if(domain == DOMAIN_Core)
			{
				const char *basePath = "Core.";
				pathLength = static_cast<rdxLargeInt>(strlen(basePath));
				for(rdxLargeInt i=0;i<pathLength;i++)
					fullPath[i] = static_cast<Char>(basePath[i]);
				preferTextOut = false;
			}
			else if(domain >= DOMAIN_Custom)
			{
				rdxLargeInt packageIndex = static_cast<rdxLargeInt>(domain) - static_cast<rdxLargeInt>(DOMAIN_Custom);

				const String *path = _packageNames[packageIndex];
				pathLength = path->Length();
				BlockCopy(fullPath, path->AsChars(), sizeof(Char) * pathLength);

				preferTextOut = (_zones[_packageZones[packageIndex]]->preferTextOut != FalseValue);
			}
			else
				return NULL;

			fullPath[pathLength+0] = static_cast<Char>('r');
			fullPath[pathLength+1] = static_cast<Char>('x');
			fullPath[pathLength+2] = static_cast<Char>('b');
			fullPath[pathLength+3] = static_cast<Char>('\0');

			for(rdxLargeInt i=0;i<pathLength-1;i++)
			{
				if(fullPath[i] == '.')
					fullPath[i] = static_cast<Char>(RDX_FILESYSTEM_DELIMITER);
			}
			IO::IFileStream *stream;
			
			if( !preferTextOut && (write || !_forceTextRead) )
			{
				stream = _fs->Open(fullPath, write);
				if(stream)
				{
					isText = false;
					return stream;
				}
			}
			
			fullPath[pathLength+2] = static_cast<Char>('t');
			stream = _fs->Open(fullPath, write);
			if(stream)
			{
				isText = true;
				return stream;
			}
			return NULL;
		}

		bool ZonePolicyPackageHost::DomainCanContainMethods(Domain domain)
		{
			if(domain == DOMAIN_Core)
				return true;
			if(domain == DOMAIN_Runtime)
				return false;
			if(domain == DOMAIN_Duplicable)
				return false;	// Domain should be a package domain, this isn't valid
			
			rdxLargeInt packageIndex = static_cast<rdxLargeInt>(domain) - static_cast<rdxLargeInt>(DOMAIN_Custom);
			return (_zones[_packageZones[packageIndex]]->canContainMethods != FalseValue);
		}


		bool ZonePolicyPackageHost::DomainsVisible(Domain sourceDomain, Domain destDomain)
		{
			if(sourceDomain == DOMAIN_Core)
				return destDomain == DOMAIN_Core;
			if(sourceDomain == DOMAIN_Runtime)
				return true;	// Runtime loader should intercept this
			if(sourceDomain == DOMAIN_Duplicable)
				return false;	// Duplicable symbols shouldn't have a package ever

			if(sourceDomain < DOMAIN_Custom)
				return false;

			rdxLargeInt srcIdx = static_cast<rdxLargeInt>(sourceDomain) - static_cast<rdxLargeInt>(DOMAIN_Custom);
			const Zone *srcZone = _zones[_packageZones[srcIdx]];

			if(destDomain == DOMAIN_Core)
				return true;	// Core is always visible
			if(destDomain == DOMAIN_Runtime)
				return false;	// Not a valid import
			if(destDomain == DOMAIN_Duplicable)
				return false;	// Not a valid import

			if(destDomain < DOMAIN_Custom)
				return false;

			rdxLargeInt dstIdx = static_cast<rdxLargeInt>(destDomain) - static_cast<rdxLargeInt>(DOMAIN_Custom);
			const Zone *dstZone = _zones[_packageZones[srcIdx]];

			// See if this is considered a visible zone
			if(srcZone->visibleZones)
			{
				rdxLargeInt numZones = GCInfo::From(srcZone->visibleZones)->numElements;
				for(rdxLargeInt i=0;i<numZones;i++)
					if(srcZone->visibleZones[i] == dstZone)
						return true;
			}
			
			return false;
		}

		void ZonePolicyPackageHost::SetForceTextReadPreference(bool enable)
		{
			_forceTextRead = enable;
		}
	}

}

#endif
