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
setglobal("set", function(tbl)
	if type(tbl) == "table" then
		local setTbl = { }
		for _,str in ipairs(tbl) do
			setTbl[str] = true
		end
		return setTbl
	end
	if type(tbl) == "string" then
		local setTbl = { }
		for i=1,#tbl do
			setTbl[string.sub(tbl,i,i)] = true
		end
		return setTbl
	end
end )


local reserved = set { "namespace", "default", "branching", "native",
	"struct", "class", "interface", "public", "private", "protected",
	"static", "using", "function", "property", "intercept", "new", "extends",
	"implements", "promote", "coerce", "while", "for", "foreach", "in",
	"return", "break", "continue", "if", "else", "local",
	"as", "virtual", "typedef", "byval", "void", "this", "enum",
	"generatehash", "typeof", "explicit", "true", "false", "try",
	"catch", "throw", "abstract", "final", "null", "resource", "delegate",
	"localized", "switch", "case", "do", "allowmutable", "notnull", "const",
	"mustbeconst", "tuple", "mustberef", "finally", "anonymous", "is",
}

RDXC.LexerState = function(str, filename)
	local offset = 1
	local max = #str
	local lineNumber = 1
	local eof = false
	local backlog = nil

	if max == 0 then
		eof = true
	end

	local LexToken = function(tt, tstr)
		assert(filename)
		return {
			type = tt,
			string = tstr,
			line = lineNumber,
			filename = filename
		}
	end

	local LexSingle = function()
		if eof then
			return nil
		end
		
		-- In state: eof, lineNumber, offset, max, str
		-- Out state: eof, lineNumber, offset, error message, token type, token text
		local errorMsg, tt, tstr
		eof, lineNumber, offset, errorMsg, tt, tstr = RDXC.Native.lexSingleToken(eof, lineNumber, offset, max, str);
		
		if errorMsg ~= nil then
			perror(lineNumber, filename, errorMsg)
		end

		if tt == "Name" and reserved[tstr] then
			tt = tstr
		end
		
		if tt == nil then
			return nil
		end
		
		return LexToken(tt, tstr)
	end

	local currentToken

	return {
		GetToken = function()
			if backlog then
				return backlog.token
			end
			return currentToken
		end,

		NextToken = function()
			if backlog then
				backlog = backlog.next
			else
				currentToken = LexSingle()
				if currentToken == nil then
					currentToken = LexToken("EOF", "<<End of File>>")
				end
			end
		end,

		ReinsertToken = function(tk)
			backlog = { token = tk, next = backlog }
		end,
	}
end

