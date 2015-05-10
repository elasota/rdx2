#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "../lua/src/lua.h"
#include "../rdx/rdx_basictypes.hpp"

#define MAX_FILES	4096

enum
{
	ValueType_Table,
	ValueType_String,
	ValueType_Number,
};

struct CacheFileInfo
{
	struct CacheFileDictionary
	{
		char contentsHash[8];
		char terminatorByte;
		bool corrupt;			// Flagged until the file is closed.  If set, a parse error occurred and the file is unusable.
		int numFiles;
		long fileTableOffset;
	} dict;

	FILE *f;
};


// filename, contents hash, force rebuild
int createCacheFile(lua_State *L)
{
	CacheFileInfo *cfi = static_cast<CacheFileInfo*>(lua_newuserdata(L, sizeof(CacheFileInfo)));
	memcpy(cfi->dict.contentsHash, lua_tostring(L, 2), 8);
	cfi->dict.terminatorByte = '\0';

	int forceRebuild = lua_toboolean(L, 3);
	
	cfi->f = fopen(lua_tostring(L, 1), "rb");
	if(cfi->f)
	{
		CacheFileInfo::CacheFileDictionary dict;
		dict.terminatorByte = '\0';
		fread(&dict, sizeof(dict), 1, cfi->f);
		fclose(cfi->f);

		if(lua_toboolean(L, 3))
			dict.corrupt = true;

		if(!dict.corrupt && !memcmp(cfi->dict.contentsHash, dict.contentsHash, 8))
		{
			lua_pushnil(L);
			lua_pushinteger(L, dict.numFiles);
			return 2;
		}
	}

	cfi->f = fopen(lua_tostring(L, 1), "wb");
	cfi->dict.corrupt = true;

	fwrite(&cfi->dict, sizeof(CacheFileInfo::CacheFileDictionary), 1, cfi->f);

	// newuserdata will spill through

	return 1;
}

// cfi, object list
int writeCacheObject(lua_State *L)
{
	CacheFileInfo *cfi = static_cast<CacheFileInfo*>(lua_touserdata(L, 1));

	int numObjects = static_cast<int>(lua_objlen(L, 2));
	long foffs = ftell(cfi->f);

	fwrite(&numObjects, sizeof(int), 1, cfi->f);
	for(int i=0;i<numObjects;i++)
	{
		lua_pushinteger(L, i+1);
		lua_gettable(L, 2);

		int tt = lua_type(L, -1);

		fwrite(&tt, sizeof(int), 1, cfi->f);
		switch(tt)
		{
		case LUA_TBOOLEAN:
			{
				int b = lua_toboolean(L, -1);
				fwrite(&b, sizeof(b), 1, cfi->f);
			}
			break;
		case LUA_TNUMBER:
			{
				lua_Number v = lua_tonumber(L, -1);
				fwrite(&v, sizeof(v), 1, cfi->f);
			}
			break;
		case LUA_TSTRING:
			{
				size_t l;
				const char *str = lua_tolstring(L, -1, &l);
				fwrite(&l, sizeof(l), 1, cfi->f);
				fwrite(str, l, 1, cfi->f);
			}
			break;
		case LUA_TTABLE:
			{
				int numValues = static_cast<int>(lua_objlen(L, -1));
				fwrite(&numValues, sizeof(numValues), 1, cfi->f);
				for(int i=0;i<numValues;i++)
				{
					lua_pushinteger(L, i+1);
					lua_gettable(L, -2);
					int vindex = lua_tointeger(L, -1);
					fwrite(&vindex, sizeof(vindex), 1, cfi->f);
					lua_pop(L, 1);
				}
			}
			break;
		}

		lua_pop(L, 1);
	}

	lua_pushinteger(L, foffs);

	return 1;
}

// filename, file table index
int readCacheObject(lua_State *L)
{
	CacheFileInfo::CacheFileDictionary dict;
	FILE *f = fopen(lua_tostring(L, 1), "rb");

	fread(&dict, sizeof(dict), 1, f);

	size_t objectOffset;
	fseek(f, lua_tointeger(L, 2) * static_cast<long>(sizeof(size_t)) + dict.fileTableOffset, SEEK_SET);
	fread(&objectOffset, sizeof(size_t), 1, f);
	fseek(f, static_cast<long>(objectOffset), SEEK_SET);

	int numObjects;
	fread(&numObjects, sizeof(int), 1, f);

	lua_createtable(L, numObjects, 0);

	for(int i=0;i<numObjects;i++)
	{
		lua_pushinteger(L, i+1);

		int tt;

		fread(&tt, sizeof(int), 1, f);
		switch(tt)
		{
		case LUA_TBOOLEAN:
			{
				int b;
				fread(&b, sizeof(b), 1, f);
				lua_pushboolean(L, b);
			}
			break;
		case LUA_TNUMBER:
			{
				lua_Number v;
				fread(&v, sizeof(v), 1, f);
				lua_pushnumber(L, v);
			}
			break;
		case LUA_TSTRING:
			{
				size_t l;
				fread(&l, sizeof(l), 1, f);

				char *str = static_cast<char*>(malloc(l+1));
				str[l] = '\0';

				fread(str, l, 1, f);

				lua_pushlstring(L, str, l);
				free(str);
			}
			break;
		case LUA_TTABLE:
			{
				int numValues;
				fread(&numValues, sizeof(numValues), 1, f);
				
				lua_createtable(L, numValues, 0);
				for(int i=0;i<numValues;i++)
				{
					lua_pushinteger(L, i+1);
					int vindex;
					fread(&vindex, sizeof(vindex), 1, f);
					lua_pushinteger(L, vindex);
					lua_settable(L, -3);
				}
			}
			break;
		}

		lua_settable(L, -3);
	}

	fclose(f);

	return 1;
}


// filename, offset table
int closeCacheFile(lua_State *L)
{
	CacheFileInfo *cfi = static_cast<CacheFileInfo*>(lua_touserdata(L, 1));
	cfi->dict.corrupt = false;
	cfi->dict.fileTableOffset = ftell(cfi->f);

	int numFiles = static_cast<int>(lua_objlen(L, 2));
	cfi->dict.numFiles = numFiles;

	// Write file offsets
	for(int i=0;i<numFiles;i++)
	{
		lua_pushinteger(L, i+1);
		lua_gettable(L, 2);
		size_t offs = static_cast<size_t>(lua_tointeger(L, -1));
		lua_pop(L, 1);

		fwrite(&offs, sizeof(size_t), 1, cfi->f);
	}

	// Rewrite the header
	fseek(cfi->f, 0, SEEK_SET);
	fwrite(&cfi->dict, sizeof(cfi->dict), 1, cfi->f);
	fclose(cfi->f);

	return 0;
}

