#include <stdio.h>

#include "../rdx/rdx_longflow.hpp"
#include "../rdx/rdx_marshal.hpp"
#include "../exported/RDXInterface/RDX/Compiler/LexState.hpp"
#include "../exported/RDXInterface/RDX/Compiler/TokenType.hpp"
#include "../exported/RDXInterface/RDX/Compiler/ErrorCode.hpp"

using namespace RDXInterface::RDX::Compiler;

class TokenLexer
{
	bool m_eofFlag;
	CodeLocation m_codeLoc;
	rdxLargeUInt m_offset;
	rdxLargeUInt m_max;
	const rdxUInt8 *m_str;
	ErrorCode m_errorCode;
	const rdxUInt8 *m_returnStr;
	rdxLargeUInt m_returnStrSize;
	rdxUInt8 *m_returnStrBuf;
	rdxLargeUInt m_returnStrCapacity;
	TokenType m_tokenType;

public:
	TokenLexer(const CodeLocation &codeLoc, rdxLargeUInt offset, rdxWeakArrayRTRef(rdxUInt8) str);
	~TokenLexer();
	
	void NextChar();
	rdxUInt8 Peek1();
	bool Peek2(rdxUInt8 &out);
	bool Check2(rdxUInt8 b1, rdxUInt8 b2);
	void LexSingle();
	bool AppendChar(rdxUInt8 c);

	static bool IsDigit(rdxUInt8 c);
	static bool IsNameStartChar(rdxUInt8 c);
	static bool IsNameChar(rdxUInt8 c);

	bool MatchPunctuation();
	void Export(rdxSOperationContext *ctx, rdxIObjectManager *objm, CodeLocation const & currentLocPassed, rdxBool & r_1, Token & r_2, ErrorCode & r_3, rdxLargeUInt & r_4);
};

TokenLexer::TokenLexer(const CodeLocation &codeLoc, rdxLargeUInt offset, rdxWeakArrayRTRef(rdxUInt8) str)
{
	m_eofFlag = false;
	m_codeLoc = codeLoc;
	m_codeLoc.filename = rdxWeakRTRef(rdxCString)::Null();	// Other threads may invalidate the filename
	m_offset = offset;
	m_max = static_cast<rdxLargeUInt>(str->NumElements());
	m_str = str->ArrayData();
	m_errorCode = ErrorCode(ErrorCode::NoError);
	m_returnStr = NULL;
	m_returnStrBuf = NULL;
	m_returnStrSize = 0;
	m_returnStrCapacity = 0;
	m_tokenType = TokenType(TokenType::Unknown);
}

TokenLexer::~TokenLexer()
{
	if(m_returnStrBuf)
		realloc(m_returnStrBuf, 0);
}

void TokenLexer::NextChar()
{
	bool wasCR = false;
	rdxUInt8 thisC = m_str[m_offset];
	if(thisC == '\n')
		m_codeLoc.line++;
	if(thisC == '\r')
	{
		m_codeLoc.line++;
		wasCR = true;
	}
	m_offset++;
	if(m_offset >= m_max)
		m_eofFlag = 1;
	else
	{
		if(wasCR && !m_eofFlag && m_str[m_offset] == '\n')
		{
			m_offset++;	// Windows CR/LF line break
			if(m_offset >= m_max)
				m_eofFlag = 1;
		}
	}
}

rdxUInt8 TokenLexer::Peek1()
{
	return m_str[m_offset];
}

bool TokenLexer::Peek2(rdxUInt8 &out)
{
	if(m_offset >= m_max || m_offset + 1 >= m_max)
		return false;
	out = m_str[m_offset + 1];
	return true;
}

bool TokenLexer::Check2(rdxUInt8 b1, rdxUInt8 b2)
{
	if(m_offset >= m_max || m_offset + 1 >= m_max)
		return false;
	return (m_str[m_offset] == b1) && (m_str[m_offset + 1] == b2);
}

bool TokenLexer::AppendChar(rdxUInt8 c)
{
	if(m_returnStrSize == m_returnStrCapacity)
	{
		m_returnStrCapacity *= 2;
		if(m_returnStrCapacity == 0)
			m_returnStrCapacity = 16;
		if(m_returnStrCapacity > RDX_LARGEINT_MAX)
			return false;
		void *newBuf = realloc(m_returnStrBuf, m_returnStrCapacity);
		if(!newBuf)
			return false;
		m_returnStrBuf = static_cast<rdxUInt8 *>(newBuf);
		m_returnStr = m_returnStrBuf;
	}
	m_returnStrBuf[m_returnStrSize++] = c;
	return true;
}

bool TokenLexer::IsDigit(rdxUInt8 c)
{
	return (c >= '0' && c <= '9');
}

bool TokenLexer::IsNameStartChar(rdxUInt8 c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool TokenLexer::IsNameChar(rdxUInt8 c)
{
	return IsNameStartChar(c) || IsDigit(c);
}

bool TokenLexer::MatchPunctuation()
{
	const char *matchStr = NULL;
	rdxUInt8 b1, b2;
	bool has2;
	b1 = Peek1();
	has2 = Peek2(b2);
	switch(b1)
	{
	case ':':
		if(has2 && b2 == '<')
		{
			matchStr = ":<";
			break;
		}
		matchStr = ":";
		break;
	case '<':
		if(has2 && b2 == '=')
		{
			matchStr = "<=";
			break;
		}
		matchStr = "<";
		break;
	case '>':
		if(has2 && b2 == '=')
		{
			matchStr = ">=";
			break;
		}
		matchStr = ">";
		break;
	case '=':
		if(has2 && b2 == '=')
		{
			matchStr = "==";
			break;
		}
		matchStr = "=";
		break;
	case '!':
		if(has2 && b2 == '=')
		{
			matchStr = "!=";
			break;
		}
		matchStr = "!";
		break;
	case '+':
		if(has2)
		{
			if(b2 == '=')
			{
				matchStr = "+=";
				break;
			}
			if(b2 == '+')
			{
				matchStr = "++";
				break;
			}
		}
		matchStr = "+";
		break;
	case '-':
		if(has2)
		{
			if(b2 == '=')
			{
				matchStr = "-=";
				break;
			}
			if(b2 == '-')
			{
				matchStr = "--";
				break;
			}
		}
		matchStr = "-";
		break;
	case '*':
		if(has2 && b2 == '=')
		{
			matchStr = "*=";
			break;
		}
		matchStr = "*";
		break;
	case '/':
		if(has2 && b2 == '=')
		{
			matchStr = "/=";
			break;
		}
		matchStr = "/";
		break;
	case '&':
		if(has2 && b2 == '&')
		{
			matchStr = "&&";
			break;
		}
		matchStr = "&";
		break;
	case '|':
		if(has2 && b2 == '|')
		{
			matchStr = "||";
			break;
		}
		matchStr = "|";
		break;
	case ';': matchStr = ";"; break;
	case ',': matchStr = ","; break;
	case '{': matchStr = "{"; break;
	case '}': matchStr = "}"; break;
	case '[': matchStr = "["; break;
	case ']': matchStr = "]"; break;
	case '(': matchStr = "("; break;
	case ')': matchStr = ")"; break;
	case '.': matchStr = "."; break;
	case '?': matchStr = "?"; break;
	}

	if(matchStr)
	{
		m_returnStr = reinterpret_cast<const rdxUInt8 *>(matchStr);
		m_returnStrSize = 0;
		while(*matchStr)
		{
			NextChar();
			m_returnStrSize++;
			matchStr++;
		}
		m_tokenType = TokenType(TokenType::Punctuation);
		return true;
	}

	return false;
}

class RDXCReservedWord
{
	RDXInterface::RDX::Compiler::TokenType m_tokenType;
	rdxLargeUInt m_len;
	const rdxChar *m_text;

public:
	inline RDXCReservedWord()
	{
		m_len = 0;
		m_tokenType = RDXInterface::RDX::Compiler::TokenType::Unknown;
		m_text = NULL;
	}

	inline RDXCReservedWord(const rdxChar *word, RDXInterface::RDX::Compiler::TokenType tokenType)
	{
		m_text = word;
		m_len = 0;
		m_tokenType = tokenType;
		while(*word++)
			m_len++;
	}

	inline bool IsTerminator() const
	{
		return this->m_len == 0;
	}

	inline bool Match(const rdxChar *word, rdxLargeUInt len) const
	{
		return (len == m_len) && (!memcmp(word, m_text, sizeof(rdxChar) * len));
	}

	inline RDXInterface::RDX::Compiler::TokenType TokenType() const
	{
		return m_tokenType;
	}
};

static const RDXCReservedWord RESERVED_WORDS_A[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("abstract"), RDXInterface::RDX::Compiler::TokenType::RW_Abstract),
	RDXCReservedWord(RDX_STATIC_STRING("anonymous"), RDXInterface::RDX::Compiler::TokenType::RW_Anonymous),
	RDXCReservedWord(RDX_STATIC_STRING("as"), RDXInterface::RDX::Compiler::TokenType::RW_As),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_B[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("branching"), RDXInterface::RDX::Compiler::TokenType::RW_Branching),
	RDXCReservedWord(RDX_STATIC_STRING("break"), RDXInterface::RDX::Compiler::TokenType::RW_Break),
	RDXCReservedWord(RDX_STATIC_STRING("byval"), RDXInterface::RDX::Compiler::TokenType::RW_ByVal),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_C[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("case"), RDXInterface::RDX::Compiler::TokenType::RW_Case),
	RDXCReservedWord(RDX_STATIC_STRING("catch"), RDXInterface::RDX::Compiler::TokenType::RW_Catch),
	RDXCReservedWord(RDX_STATIC_STRING("class"), RDXInterface::RDX::Compiler::TokenType::RW_Class),
	RDXCReservedWord(RDX_STATIC_STRING("coerce"), RDXInterface::RDX::Compiler::TokenType::RW_Coerce),
	RDXCReservedWord(RDX_STATIC_STRING("const"), RDXInterface::RDX::Compiler::TokenType::RW_Const),
	RDXCReservedWord(RDX_STATIC_STRING("continue"), RDXInterface::RDX::Compiler::TokenType::RW_Continue),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_D[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("default"), RDXInterface::RDX::Compiler::TokenType::RW_Default),
	RDXCReservedWord(RDX_STATIC_STRING("delegate"), RDXInterface::RDX::Compiler::TokenType::RW_Delegate),
	RDXCReservedWord(RDX_STATIC_STRING("do"), RDXInterface::RDX::Compiler::TokenType::RW_Do),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_E[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("else"), RDXInterface::RDX::Compiler::TokenType::RW_Else),
	RDXCReservedWord(RDX_STATIC_STRING("enum"), RDXInterface::RDX::Compiler::TokenType::RW_Enum),
	RDXCReservedWord(RDX_STATIC_STRING("explicit"), RDXInterface::RDX::Compiler::TokenType::RW_Explicit),
	RDXCReservedWord(RDX_STATIC_STRING("extends"), RDXInterface::RDX::Compiler::TokenType::RW_Extends),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_F[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("false"), RDXInterface::RDX::Compiler::TokenType::RW_False),
	RDXCReservedWord(RDX_STATIC_STRING("final"), RDXInterface::RDX::Compiler::TokenType::RW_Final),
	RDXCReservedWord(RDX_STATIC_STRING("finally"), RDXInterface::RDX::Compiler::TokenType::RW_Finally),
	RDXCReservedWord(RDX_STATIC_STRING("for"), RDXInterface::RDX::Compiler::TokenType::RW_For),
	RDXCReservedWord(RDX_STATIC_STRING("foreach"), RDXInterface::RDX::Compiler::TokenType::RW_ForEach),
	RDXCReservedWord(RDX_STATIC_STRING("function"), RDXInterface::RDX::Compiler::TokenType::RW_Function),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_I[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("if"), RDXInterface::RDX::Compiler::TokenType::RW_If),
	RDXCReservedWord(RDX_STATIC_STRING("implements"), RDXInterface::RDX::Compiler::TokenType::RW_Implements),
	RDXCReservedWord(RDX_STATIC_STRING("in"), RDXInterface::RDX::Compiler::TokenType::RW_In),
	RDXCReservedWord(RDX_STATIC_STRING("intercept"), RDXInterface::RDX::Compiler::TokenType::RW_Intercept),
	RDXCReservedWord(RDX_STATIC_STRING("interface"), RDXInterface::RDX::Compiler::TokenType::RW_Interface),
	RDXCReservedWord(RDX_STATIC_STRING("is"), RDXInterface::RDX::Compiler::TokenType::RW_Is),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_L[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("local"), RDXInterface::RDX::Compiler::TokenType::RW_Local),
	RDXCReservedWord(RDX_STATIC_STRING("localized"), RDXInterface::RDX::Compiler::TokenType::RW_Localized),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_M[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("mustbeconst"), RDXInterface::RDX::Compiler::TokenType::RW_MustBeConst),
	RDXCReservedWord(RDX_STATIC_STRING("mustberef"), RDXInterface::RDX::Compiler::TokenType::RW_MustBeRef),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_N[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("native"), RDXInterface::RDX::Compiler::TokenType::RW_Native),
	RDXCReservedWord(RDX_STATIC_STRING("namespace"), RDXInterface::RDX::Compiler::TokenType::RW_Namespace),
	RDXCReservedWord(RDX_STATIC_STRING("new"), RDXInterface::RDX::Compiler::TokenType::RW_New),
	RDXCReservedWord(RDX_STATIC_STRING("notnull"), RDXInterface::RDX::Compiler::TokenType::RW_NotNull),
	RDXCReservedWord(RDX_STATIC_STRING("null"), RDXInterface::RDX::Compiler::TokenType::RW_Null),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_P[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("private"), RDXInterface::RDX::Compiler::TokenType::RW_Private),
	RDXCReservedWord(RDX_STATIC_STRING("promote"), RDXInterface::RDX::Compiler::TokenType::RW_Promote),
	RDXCReservedWord(RDX_STATIC_STRING("property"), RDXInterface::RDX::Compiler::TokenType::RW_Property),
	RDXCReservedWord(RDX_STATIC_STRING("protected"), RDXInterface::RDX::Compiler::TokenType::RW_Protected),
	RDXCReservedWord(RDX_STATIC_STRING("public"), RDXInterface::RDX::Compiler::TokenType::RW_Public),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_R[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("resource"), RDXInterface::RDX::Compiler::TokenType::RW_Resource),
	RDXCReservedWord(RDX_STATIC_STRING("return"), RDXInterface::RDX::Compiler::TokenType::RW_Return),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_S[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("static"), RDXInterface::RDX::Compiler::TokenType::RW_Static),
	RDXCReservedWord(RDX_STATIC_STRING("struct"), RDXInterface::RDX::Compiler::TokenType::RW_Struct),
	RDXCReservedWord(RDX_STATIC_STRING("switch"), RDXInterface::RDX::Compiler::TokenType::RW_Switch),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_T[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("this"), RDXInterface::RDX::Compiler::TokenType::RW_This),
	RDXCReservedWord(RDX_STATIC_STRING("throw"), RDXInterface::RDX::Compiler::TokenType::RW_Throw),
	RDXCReservedWord(RDX_STATIC_STRING("try"), RDXInterface::RDX::Compiler::TokenType::RW_Try),
	RDXCReservedWord(RDX_STATIC_STRING("true"), RDXInterface::RDX::Compiler::TokenType::RW_True),
	RDXCReservedWord(RDX_STATIC_STRING("tuple"), RDXInterface::RDX::Compiler::TokenType::RW_Tuple),
	RDXCReservedWord(RDX_STATIC_STRING("typedef"), RDXInterface::RDX::Compiler::TokenType::RW_TypeDef),
	RDXCReservedWord(RDX_STATIC_STRING("typedef"), RDXInterface::RDX::Compiler::TokenType::RW_TypeOf),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_U[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("using"), RDXInterface::RDX::Compiler::TokenType::RW_Using),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_V[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("virtual"), RDXInterface::RDX::Compiler::TokenType::RW_Virtual),
	RDXCReservedWord(RDX_STATIC_STRING("void"), RDXInterface::RDX::Compiler::TokenType::RW_Void),
	RDXCReservedWord(),
};

static const RDXCReservedWord RESERVED_WORDS_W[] =
{
	RDXCReservedWord(RDX_STATIC_STRING("while"), RDXInterface::RDX::Compiler::TokenType::RW_While),
	RDXCReservedWord(),
};

static RDXCReservedWord const * const RESERVED_WORDS[] =
{
	RESERVED_WORDS_A,
	RESERVED_WORDS_B,
	RESERVED_WORDS_C,
	RESERVED_WORDS_D,
	RESERVED_WORDS_E,
	RESERVED_WORDS_F,
	NULL,
	NULL,
	RESERVED_WORDS_I,
	NULL,
	NULL,
	RESERVED_WORDS_L,
	RESERVED_WORDS_M,
	RESERVED_WORDS_N,
	NULL,
	RESERVED_WORDS_P,
	NULL,
	RESERVED_WORDS_R,
	RESERVED_WORDS_S,
	RESERVED_WORDS_T,
	RESERVED_WORDS_U,
	RESERVED_WORDS_V,
	RESERVED_WORDS_W,
	NULL,
	NULL,
	NULL,
};

void TokenLexer::Export(rdxSOperationContext *ctx, rdxIObjectManager *objm, CodeLocation const & currentLocPassed, rdxBool & r_1, Token & r_2, ErrorCode & r_3, rdxLargeUInt & r_4)
{
	Token tk;
	RDX_TRY(ctx)
	{
		tk.codeLocation = m_codeLoc;

		if(m_eofFlag)
		{
			tk.tokenType = TokenType::EOF_;
			tk.str = rdxWeakRTRef(rdxCString)::Null();
			r_1 = rdxTrueValue;
			r_2 = tk;
			r_3 = ErrorCode::NoError;
			r_4 = 0;
			return;
		}

		tk.tokenType = m_tokenType;
		RDX_PROTECT_ASSIGN(ctx, tk.str, objm->CreateStringUTF8(ctx, m_returnStr, true, m_returnStrSize));

		if(tk.tokenType == TokenType::Name)
		{
			const rdxChar *chars = tk.str->AsChars()->ArrayData();
			rdxLargeUInt len = tk.str->Length();
			if(chars[0] >= static_cast<rdxChar>('a') && chars[0] <= static_cast<rdxChar>('z'))
			{
				const RDXCReservedWord *rw = RESERVED_WORDS[static_cast<int>(chars[0] - static_cast<rdxChar>('a'))];
				if(rw != NULL)
				{
					while(!rw->IsTerminator())
					{
						if(rw->Match(chars, len))
						{
							tk.tokenType = rw->TokenType();
							break;
						}
						rw++;
					}
				}
			}
		}

		tk.codeLocation.filename = currentLocPassed.filename;
		r_1 = rdxFalseValue;
		r_2 = tk;
		r_3 = ErrorCode::NoError;
		r_4 = m_offset;
		return;
	}
	RDX_CATCH(ctx)
	{
		memset(&tk.codeLocation, 0, sizeof(tk.codeLocation));
		tk.str = rdxWeakRTRef(rdxCString)::Null();
		tk.tokenType = TokenType::EOF_;
		r_1 = rdxTrueValue;
		r_2 = tk;
		r_3 = ErrorCode::MemoryAllocationFailure;
		r_4 = 0;
	}
	RDX_ENDTRY
}

void TokenLexer::LexSingle()
{
	if(m_offset >= m_max)
	{
		m_eofFlag = true;
		return;
	}

	bool moreWS = true;
	while(moreWS)
	{
		// Skip over whitespace
		rdxUInt8 wsByte = ' ';

		while(Peek1() <= wsByte)
		{
			NextChar();
			if(m_eofFlag)
				return;
		}

		moreWS = false;

		if(Check2('/', '/'))
		{
			rdxUInt8 b = Peek1();
			while(b != '\n' && b != '\r')
			{
				NextChar();
				if(m_eofFlag)
					return;
				b = Peek1();
			}
			moreWS = true;
		}

		if(Check2('/', '*'))
		{
			NextChar();
			NextChar();
			while(!Check2('*', '/'))
			{
				NextChar();
				if(m_eofFlag)
				{
					m_errorCode = ErrorCode::UnexpectedEOF;
					return;
				}
			}
			NextChar();
			NextChar();
			moreWS = true;
		}

		if(!moreWS)
			break;
	}

	// Long strings
	if(Peek1() == '\"')
	{
		while(true)
		{
			NextChar();
			if(m_eofFlag)
			{
				m_errorCode = ErrorCode::UnexpectedEOF;
				return;
			}
			rdxUInt8 c = Peek1();
			if(c == '\n' || c == '\r')
			{
				m_errorCode = ErrorCode::NewlineInStringConstant;
				return;
			}
			if(c == '\"')
				break;
			if(c == '\\')
			{
				NextChar();
				if(m_eofFlag)
				{
					m_errorCode = ErrorCode::UnexpectedEOF;
					return;
				}

				rdxUInt8 escapeC = Peek1();
				switch(escapeC)
				{
				case 'n':
					c = '\n';
					break;
				case 'r':
					c = '\r';
					break;
				case 't':
					c = '\t';
					break;
				case '\"':
					c = '\"';
					break;
				case '\\':
					c = '\\';
					break;
				default:
					{
						m_errorCode = ErrorCode::UnknownEscape;
						return;
					}
				}
			}
			if(!AppendChar(c))
			{
				m_errorCode = ErrorCode::MemoryAllocationFailure;
				return;
			}
		}
		NextChar();
		m_tokenType = TokenType::String;
		return;
	}

	if(IsDigit(Peek1()))
	{
		bool printNum = false;
		rdxLargeUInt strStart = m_offset;
		rdxLargeUInt strEnd = m_offset;
		while(true)
		{
			NextChar();
			if(m_eofFlag)
				break;
			rdxUInt8 c = Peek1();
			if(!IsDigit(c) && c != '.' && c != 'f' && c != '^')
				break;
			if(c == '^')
			{
				if(Check2('^', '-'))
				{
					strEnd = strEnd + 1;
					NextChar();	// Skip one
					c = Peek1();
					printNum = true;
				}
			}
			strEnd++;
		}
		m_tokenType = TokenType::Number;
		m_returnStr = m_str + strStart;
		m_returnStrSize = strEnd - strStart + 1;
		return;
	}

	if(IsNameStartChar(Peek1()))
	{
		rdxLargeUInt strStart = m_offset;
		rdxLargeUInt strEnd = m_offset;

		while(true)
		{
			NextChar();
			if(m_eofFlag)
				break;
			rdxUInt8 c = Peek1();
			if(!IsNameChar(c))
				break;
			strEnd++;
		}
		
		m_returnStr = m_str + strStart;
		m_returnStrSize = strEnd - strStart + 1;
		m_tokenType = TokenType::Name;
		return;
	}

	if(MatchPunctuation())
		return;

	m_errorCode = ErrorCode::UnknownSymbol;
}

// Returns _eof, _currentToken, errCode, m_offset
void RDXInterface::RDX::Compiler::LexState::NextTokenNative(rdxSExportedCallEnvironment &callEnv,
	rdxBool & r_1, Token & r_2, ErrorCode & r_3, rdxLargeUInt & r_4, rdxWeakArrayRTRef(rdxByte) bytes, rdxLargeUInt offset, CodeLocation const & currentLoc)
{
	if(bytes.IsNull())
	{
		Token tk;
		memset(&tk, 0, sizeof(tk));
		r_1 = rdxTrueValue;
		r_2 = tk;
		r_3 = ErrorCode(ErrorCode::InternalError);
		r_4 = 0;
	}

	rdxLargeUInt uoffset = offset;
	TokenLexer lx(currentLoc, uoffset, bytes);
	lx.LexSingle();
	lx.Export(callEnv.ctx, callEnv.objm, currentLoc, r_1, r_2, r_3, r_4);
	// Other threads may invalidate the filename, so recover it
	r_2.codeLocation.filename = currentLoc.filename;
}
