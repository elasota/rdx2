--[[
 Copyright (C) 2011-2013 Eric Lasota

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
]]
local rdxcPath = ""
local projectPath = arg[#arg]
local disableCaching = false

asmDumpFile = false
traceErrors = false
instancedTemplateLimit = 100
dumpSymbolsFileName = false
verbose = false
outputToStdOut = false
emitDebugInfo = true
cppConfigPath = false
cppConfig = false
benchmark = false
exportPCCM = false

if #arg[0] > 8 then
	rdxcPath = string.sub(arg[0], 1, #arg[0] - 8)
end

compilerCachePath = rdxcPath.."compilercache"

local function ErrorDisplayUsage()
	io.stderr:write("Usage: lua5.1_gfg_i64 rdxc.lua [options] <project file>\n")
	io.stderr:write("       lua5.1_gfg_i64 rdxc.lua -help\n")
	os.exit(-1)
end

local function ErrorDisplayHelp()
	io.stderr:write("Usage: lua5.1_gfg_i64 rdxc.lua [options] <project file>\n")
	io.stderr:write("Options\n")
	io.stderr:write("   -dumpasm=<path> : Dumps RDX ASM for newly-compiled code\n")
	io.stderr:write("   -listsymbols=<path> : Dumps a list of symbols that were emitted\n")
	io.stderr:write("   -maxtemplates=<n> : Sets maximum number of template types\n")
	io.stderr:write("                       Default "..instancedTemplateLimit.."\n")
	io.stderr:write("   -nocache : Prevents cache files from being loaded, but still makes new ones\n")
	io.stderr:write("   -traceerrors : Causes compiler errors to dump the rdxc stack trace\n")
	io.stderr:write("   -cppexport=<path> : Loads plug-in export settings\n")
	io.stderr:write("   -cachepath=<path> : Sets path to store or load RDX compiler cache files\n")
	io.stderr:write("                       Default "..compilerCachePath.."\n")
	io.stderr:write("   -v : Enables more messages about compiler progress\n")
	io.stderr:write("   -stdout : Outputs to stdout instead of the project file path\n")
	io.stderr:write("   -nodebug : Disables emission of file names and line numbers\n")
	io.stderr:write("   -benchmark : Displays timing info of compile steps\n")
	os.exit(0)
end

if arg[1] == "-help" then
	ErrorDisplayHelp()
end

for ai=1,(#arg-1) do
	local av = arg[ai]

	if string.sub(av, 1, 9) == "-dumpasm=" then
		asmDumpFile = io.open(string.sub(av, 10, #av), "wb")
		if asmDumpFile == nil then
			error("Could not open ASM dump file")
		end
	elseif string.sub(av, 1, 13) == "-listsymbols=" then
		dumpSymbolsFileName = string.sub(av, 14, #av)
	elseif string.sub(av, 1, 14) == "-maxtemplates=" then
		instancedTemplateLimit = tonumber(string.sub(av, 15, #av))
		if instancedTemplateLimit < 0 then
			error("Invalid max templates value")
		end
	elseif av == "-nocache" then
		disableCaching = true
	elseif av == "-traceerrors" then
		traceErrors = true
	elseif string.sub(av, 1, 11) == "-cppexport=" then
		cppConfigPath = string.sub(av, 12, #av)
	elseif string.sub(av, 1, 11) == "-cachepath=" then
		compilerCachePath = string.sub(av, 12, #av)
	elseif av == "-exportpccm" then
		exportPCCM = true
	elseif av == "-v" then
		verbose = true
	elseif av == "-stdout" then
		outputToStdOut = true
	elseif av == "-nodebug" then
		emitDebugInfo = false
	elseif av == "-benchmark" then
		benchmark = true
		benchmarks = { }
	else
		io.stderr:write("Unknown option "..av.."\n")
		ErrorDisplayUsage()
	end
end

if #arg < 1 then
	ErrorDisplayUsage()
end

package.loadlib("rdxclib"..VERSION_TAG..".dll", "RegisterRDXC")()

RDXC.rdxcPath = rdxcPath	-- Needed for cache lookups
RDXC.compilerCachePath = compilerCachePath

function vprint(str)
	if verbose then
		io.stderr:write(str)
		io.stderr:write("\n")
	end
end

function loadcacheablefile(path)
	local f = io.open(path, "rb")
	if f == nil then
		print("Could not open "..path)
		local testScript = loadfile(path)
		if testScript ~= nil then
			print("Somehow loaded it anyway")
		end
		return testScript
	end
	local fileContents = f:read("*a")
	f:close()

	local filenameHash = RDXC.Native.computeguid(path)
	local contentsHash = RDXC.Native.computeguid(fileContents)

	local cacheHashPath = compilerCachePath.."/scripthash_"..filenameHash..".cache"
	local cacheCompilePath = compilerCachePath.."/scriptcompile_"..filenameHash..".cache"

	local hashf = io.open(cacheHashPath, "r")

	local canUseCache = false
	if hashf ~= nil then
		local hashContents = hashf:read("*a")
		hashf:close()

		if hashContents == contentsHash then
			canUseCache = true
		end
	end

	if disableCaching then
		canUseCache = false
	end

	if canUseCache then
		print("Loading cached appcode "..cacheCompilePath)
		local loadedScript = assert(loadfile(cacheCompilePath))
		return loadedScript
	end

	vprint("appcode  "..path)

	print("Loading uncached appcode "..path)
	local script = assert(loadfile(path))
	print(script)

	local contentsf = io.open(cacheCompilePath, "wb")
	contentsf:write(string.dump(script))
	contentsf:close()

	local hashf = io.open(cacheHashPath, "wb")
	hashf:write(contentsHash)
	hashf:close()

	return script
end

function sortedpairs(tbl)
	local nextFunc = function(st, k)
		local i
		if k == nil then
			i = 1
		else
			i = st.k2i[k] + 1
		end
		local nk = st.i2k[i]
		
		if nk == nil then
			return
		end
		return nk, st.tbl[nk]
	end
	
	local sortedtable = { tbl = tbl, k2i = { }, i2k = { } }
	
	local idx = 1
	for k in pairs(tbl) do
		sortedtable.i2k[idx] = k
		idx = idx + 1
	end
	table.sort(sortedtable.i2k)
	
	for i,k in ipairs(sortedtable.i2k) do
		sortedtable.k2i[k] = i
	end

	 return nextFunc, sortedtable
end

function setglobal(k,v)
	rawset(_G, k, v)
end

setmetatable(_G, {
	__newindex = function(t,k,v)
		error("Attempted to create global symbol "..tostring(k).." without using setglobal")
	end,
	__index = function(t,k)
		error("Attempted to access unassigned global symbol "..tostring(k))
	end,
} )

-- BEGIN MAIN APPLICATION
vprint("RDX Compiler (c)2012-2014 Eric Lasota / Gale Force Games")


assert(loadcacheablefile(rdxcPath.."rdx_lex.lua"))()
assert(loadcacheablefile(rdxcPath.."rdx_parse.lua"))()
assert(loadcacheablefile(rdxcPath.."rdx_compile.lua"))()
assert(loadcacheablefile(rdxcPath.."rdx_dumphierarchical.lua"))()
assert(loadcacheablefile(rdxcPath.."rdx_emit.lua"))()
assert(loadcacheablefile(rdxcPath.."rdx_constfolding.lua"))()
assert(loadcacheablefile(rdxcPath.."rdx_packagecfg.lua"))()
assert(loadcacheablefile(rdxcPath.."rdx_cache.lua"))()
assert(loadcacheablefile(rdxcPath.."rdx_cppexport.lua"))()
assert(loadcacheablefile(rdxcPath.."rdx_errors.lua"))()
assert(loadcacheablefile(rdxcPath.."rdx_packageregistry.lua"))()
assert(loadcacheablefile(rdxcPath.."rdx_cppexport_methods.lua"))()

-- Load plugin export settings
if cppConfigPath then
	local cfg = assert(loadfile(cppConfigPath))()

	if cfg == nil then
		io.stderr:write("ERROR: Plugin export config didn't return a value\n")
		os.exit(-1)
	end

	local requiredFields = { "exportPath", "rdxPath", "pluginPath", "pluginName" }
	
	for _,fld in ipairs(requiredFields) do
		if cfg[fld] == nil then
			io.stderr:write("ERROR: Plugin export config is missing the field "..fld.."\n")
			os.exit(-1)
		end
	end

	cppConfig = cfg
end


local parsedFiles = { }

local cacheState = RDXC.CacheState()


local argIndex = 1
local outputName
local includedNamespaces = { }
local inputFiles = { }
local includedProjects = { }

setglobal("importproject", function(currentProj, projName)
	if includedProjects[projName] then
		return
	end
	includedProjects[projName] = true

	local projInfo = assert(loadfile(projName))()
	local currentFileSet = { }
	for _,filename in ipairs(currentProj.files) do
		currentFileSet[filename] = true
	end
	for _,filename in ipairs(projInfo.files) do
		if not currentFileSet[filename] then
			currentFileSet[filename] = true
			currentProj.files[#currentProj.files+1] = filename
		end
	end
end )

local projInfo = assert(loadfile(arg[#arg]))()

inputFiles = projInfo.files
outputName = projInfo.outputFile
includedNamespaces = projInfo.namespaces

local projectCacheName = RDXC.Native.computeguid(arg[#arg])

local fileContentsHashes = { }

for idx,fname in ipairs(inputFiles) do
	local f = io.open(fname, "r")
	if f == nil then
		error("Could not open project file "..fname)
	end
	local fileContents = f:read("*a")
	f:close()

	fileContentsHashes[idx] = RDXC.Native.sha256(fileContents)
end

-- Compare this to the cached version
local outputExists = false
if not outputToStdOut then
	local f = io.open(outputName, "rb")
	if f ~= nil then
		outputExists = true
		f:close()
	end
end

if outputExists then
	local cacheScript = loadfile(compilerCachePath.."/project_"..projectCacheName..".cache")
	if cacheScript then
		local cachedProjInfo = cacheScript()

		if projInfo.outputFile == cachedProjInfo.outputFile
			and #projInfo.namespaces == #cachedProjInfo.namespaces
			and #projInfo.files == #cachedProjInfo.files then

			local matched = true

			for idx,fname in ipairs(projInfo.files) do
				if cachedProjInfo.files[idx].filename ~= fname or
					cachedProjInfo.files[idx].hash ~= fileContentsHashes[idx] then
					matched = false
					break
				end
			end

			for idx,ns in ipairs(projInfo.namespaces) do
				if cachedProjInfo.namespaces[idx] ~= ns then
					matched = false
					break
				end
			end

			if disableCaching then
				matched = false
			end

			if matched then
				os.exit(0)	-- Nothing to do
			end
		end
	end
end

if not outputToStdOut then
	assert(outputName, "No output file specified")
end

local projectBM
if benchmark then
	projectBM = { name = "project", start = RDXC.Native.msecTime() }
	benchmarks[#benchmarks+1] = projectBM
end

vprint("project  "..arg[#arg])

for idx,param in ipairs(inputFiles) do
	local filenameHash = RDXC.Native.computeguid(param)
	local contentsHash = fileContentsHashes[idx]

	local cacheFile, numObjects = RDXC.Native.createCacheFile(compilerCachePath.."/parse_"..filenameHash..".cache", contentsHash, disableCaching)

	if cacheFile then
		vprint("parse    "..param)
		cacheState.library = filenameHash
		cacheState.cacheFile = cacheFile
		cacheState.objectOffsets = { }
		cacheState.numCachedObjects = 0

		local fileParseBM
		if benchmark then
			fileParseBM = { name = "parse "..param, start = RDXC.Native.msecTime() }
			benchmarks[#benchmarks+1] = fileParseBM
		end


		local f = io.open(param, "r")
		local fileContents = f:read("*a")
		f:close()

		local ls = RDXC.LexerState(fileContents, param)
		fileContents = nil

		local parsedFile = RDXC.Parse(ls, cacheState)
		ls = nil

		numObjects = cacheState.numCachedObjects + 1
		local ct = cacheTable(parsedFile)
		parsedFile = nil
		cacheState.objectOffsets[numObjects] = RDXC.Native.writeCacheObject(cacheFile, ct)
		ct = nil

		if benchmark then
			fileParseBM.endTime = RDXC.Native.msecTime()
		end

		RDXC.Native.closeCacheFile(cacheFile, cacheState.objectOffsets)
		cacheFile = nil
	end

	parsedFiles[#parsedFiles+1] = { library = filenameHash, index = numObjects - 1 }
end

local compilerState = RDXC.CompilerState()

local uncacheBM
if benchmark then
	uncacheBM = { name = "read cache", start = RDXC.Native.msecTime() }
	benchmarks[#benchmarks+1] = uncacheBM
end

for _,pfci in ipairs(parsedFiles) do
	local cobj = RDXC.Native.readCacheObject(RDXC.compilerCachePath.."/parse_"..pfci.library..".cache", pfci.index)

	local pf = unpackCachedTable(cobj)

	compilerState:MergeParsedFile(pf)
end

if benchmark then
	uncacheBM.endTime = RDXC.Native.msecTime()
end

local compileBM
if benchmark then
	compileBM = { name = "compile", start = RDXC.Native.msecTime() }
	benchmarks[#benchmarks+1] = compileBM
end

vprint("compile")
compilerState:CompileAll(cacheState, includedNamespaces)

if benchmark then
	compileBM.endTime = RDXC.Native.msecTime()
end

local f

if outputToStdOut then
	f = io.stdout
else
	f = io.open(outputName, "w")
	if f == nil then
		error("Could not open output file "..outputName)
	end
end

vprint("emit")

local emitBM
if benchmark then
	emitBM = { name = "emit", start = RDXC.Native.msecTime() }
	benchmarks[#benchmarks+1] = emitBM
end

RDXC.Export(compilerState, f, includedNamespaces, dumpSymbolsFileName)

if benchmark then
	emitBM.endTime = RDXC.Native.msecTime()
end

if not outputToStdOut then
	f:close()
end

-- Cache the project info

f = io.open(compilerCachePath.."/project_"..projectCacheName..".cache", "w")
f:write("local projectInfo =\n")
f:write("{\n")
f:write("\toutputFile = [["..projInfo.outputFile.."]],\n")
f:write("\tfiles =\n")
f:write("\t{\n")
for idx,fname in ipairs(projInfo.files) do
	f:write("\t\t{ filename = [["..fname.."]], hash = [["..fileContentsHashes[idx].."]] },\n")
end
f:write("\t},\n")
f:write("\tnamespaces =\n")
f:write("\t{\n")
for _,ns in ipairs(projInfo.namespaces) do
	f:write("\t\t[["..ns.."]],\n")
end
f:write("\t},\n")
f:write("}\n")
f:write("return projectInfo\n")
f:close()

vprint("done")

if asmDumpFile then
	asmDumpFile:close()
end

if benchmark then
	projectBM.endTime = RDXC.Native.msecTime()
end

-- Output benchmarks
if benchmark then
	io.stderr:write("Benchmarks:\n")
	for _,bm in ipairs(benchmarks) do
		io.stderr:write(bm.name..": "..(bm.endTime - bm.start).."\n")
	end
end
