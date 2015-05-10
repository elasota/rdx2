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
local function writeIndent(f, level)
	for i=1,level do
		f:write("\t")
	end
end

local function writeObject(f, object, dumpState, indent)
	if type(object) == "string" then
		f:write("\""..object.."\"")
	elseif type(object) == "number" then
		f:write(tostring(object))
	elseif type(object) == "boolean" then
		f:write(object and "true" or "false")
	elseif type(object) == "table" then
		if not dumpState.dumpedObjects[object] then
			lon_dump(f, object, nil, indent, dumpState)
		else
			f:write("<<PREVIOUSLY DUMPED OBJECT>>")
		end
	end
end

setglobal("lon_dump", function(f, object, preferredFields, indent, dumpState)
	if indent == nil then
		indent = 0
	end

	if dumpState == nil then
		dumpState = { preferredFields = preferredFields, dumpedObjects = { } }
	end
	dumpState.dumpedObjects[object] = true

	local keysSet = { }
	local keys = { }
	if dumpState.preferredFields then
		for _,k in ipairs(dumpState.preferredFields) do
			keys[#keys+1] = k
			keysSet[k] = true
		end
	end
	for k,v in pairs(object) do
		if keysSet[k] == nil then
			keys[#keys+1] = k
		end
	end

	f:write("{\n")
	for _,k in ipairs(keys) do
		local v = object[k]

		if v ~= nil then
			writeIndent(f, indent+1)
			f:write("[")
			writeObject(f, k, dumpState, indent+1)
			f:write("] = ")
			writeObject(f, v, dumpState, indent+1)
			f:write(",\n")
		end
	end
	writeIndent(f, indent)
	f:write("}")
end )
