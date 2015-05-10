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

-- The purpose of the caching mechanism is to reduce active memory requirements for the compiler.
-- In particular, method code block parse trees consume a lot of memory, so the parser stores them
-- in a cache file and they're retrieved when the method is compiled.  All of the data from the
-- parse tree is lost and garbage collected afterwards, leaving only the compiled instruction list.

setglobal("cacheTable", function(v)
	local valueTable = { }
	local valueToIndex = { }
	local scanIndex = 1
	local insertionIndex = 1

	local valueIndex = function(value)
		assert(value ~= nil)
		local idx = valueToIndex[value]
		if idx == nil then
			valueTable[insertionIndex] = value
			valueToIndex[value] = insertionIndex
			idx = insertionIndex
			insertionIndex = insertionIndex + 1
		end

		return idx
	end

	valueIndex(v)

	while valueTable[scanIndex] ~= nil do

		local value = valueTable[scanIndex]

		if type(value) == "table" then
			local encodedTable = { }
			local index = 1
			for k,v in pairs(value) do
				encodedTable[index] = valueIndex(k)
				encodedTable[index+1] = valueIndex(v)
				index = index + 2
			end

			valueTable[scanIndex] = encodedTable
		end
		scanIndex = scanIndex + 1
	end

	return valueTable
end )


setglobal("unpackCachedTable", function(encodedTable)
	local numValues = #encodedTable

	local unpacked = { }

	for i=1,numValues do
		local v = encodedTable[i]

		if type(v) == "table" then
			unpacked[i] = { }
		else
			unpacked[i] = v
		end
	end

	for i=1,numValues do
		local eval = encodedTable[i]

		if type(eval) == "table" then
			local unpackedTable = unpacked[i]

			for idx=1,#eval,2 do
				local k = unpacked[eval[idx]]
				local v = unpacked[eval[idx+1]]
				unpackedTable[k] = v
			end
		end
	end

	return unpacked[1]
end )
