#include "PostFXParser.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

namespace pfx_parser
{
// ------------------------------------------------------------------=

static const_cstr k_KeywordFBDeclare			= "_fb";
static const_cstr k_KeywordEndFBDeclare			= "_end_fb";

static const_cstr k_KeywordFSize				= "fsize";
static const_cstr k_KeywordISize				= "isize";
static const_cstr k_KeywordDepth				= "depth";
static const_cstr k_KeywordColor				= "color";

static const_cstr k_KeywordPreset				= "_preset";
static const_cstr k_KeywordEndPreset			= "_end_preset";
static const_cstr k_KeywordPass					= "_pass";
static const_cstr k_KeywordEndPass				= "_end_pass";
static const_cstr k_KeywordFramebuffer			= "fb";
static const_cstr k_KeywordMaterial				= "material";
static const_cstr k_KeywordBind					= "bind";

namespace internal
{
// -------------------------------------------------------------------

Tokenizer::Tokenizer(const_cstr i_strBuffer, AllocFunc i_allocFunc)
	: m_StrBuffer(i_strBuffer)
	, m_CurrReadPos(m_StrBuffer)
	, m_CurrLine(0)
	, m_CurrPos(0)
	, m_AllocFunc(i_allocFunc)
{
}

Tokenizer::~Tokenizer()
{
}

Token Tokenizer::GetNextToken()
{
	Token newToken;

	_SkipBlanks();
	newToken.line = m_CurrLine;
	newToken.position = m_CurrPos;
	const_cstr tokenStr = nullptr;
	const size tokenStrLen = _GetRawString(&tokenStr);

	if (tokenStrLen == 0)
	{
		newToken.type = TokenType::EndOfTokenStream;
		return newToken;
	}

	if (strncmp(tokenStr, k_KeywordFBDeclare, tokenStrLen) == 0)
	{
		newToken.type = TokenType::FBDeclare;
	}
	else if (strncmp(tokenStr, k_KeywordEndFBDeclare, tokenStrLen) == 0)
	{
		newToken.type = TokenType::EndFBDeclare;
	}

	else if (strncmp(tokenStr, k_KeywordFSize, tokenStrLen) == 0)
	{
		newToken.type = TokenType::FSize;
	}
	else if (strncmp(tokenStr, k_KeywordISize, tokenStrLen) == 0)
	{
		newToken.type = TokenType::ISize;
	}
	else if (strncmp(tokenStr, k_KeywordDepth, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Depth;
	}
	else if (strncmp(tokenStr, k_KeywordColor, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Color;
	}

	else if (strncmp(tokenStr, k_KeywordPreset, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Preset;
	}
	else if (strncmp(tokenStr, k_KeywordEndPreset, tokenStrLen) == 0)
	{
		newToken.type = TokenType::EndPreset;
	}
	else if (strncmp(tokenStr, k_KeywordPass, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Pass;
	}
	else if (strncmp(tokenStr, k_KeywordEndPass, tokenStrLen) == 0)
	{
		newToken.type = TokenType::EndPass;
	}
	else if (strncmp(tokenStr, k_KeywordFramebuffer, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Framebuffer;
	}
	else if (strncmp(tokenStr, k_KeywordMaterial, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Material;
	}
	else if (strncmp(tokenStr, k_KeywordBind, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Bind;
	}

	else if (_IsInt(tokenStr, tokenStrLen))
	{
		newToken.type = TokenType::ValueInt;
		newToken.intValue = atoi(tokenStr);
	}
	else if (_IsFloat(tokenStr, tokenStrLen))
	{
		newToken.type = TokenType::ValueFloat;
		newToken.floatValue = atof(tokenStr);
	}
	else
	{
		newToken.type = TokenType::ValueStringLiteral;
		newToken.strValue = _DuplicateStringRange(tokenStr, tokenStrLen);
	}

	return newToken;
}

void Tokenizer::_SkipBlanks()
{
	c8 currChar = *m_CurrReadPos;
	// isblank() does not count '\n' as blank, why?
	while (currChar == '\n' || currChar == '\t' || currChar == ' ' || currChar == '\r')
	{
		if (currChar == '\n')
		{
			m_CurrLine++;
			m_CurrPos = 0;
		}
		else if (currChar == '\t')
		{
			m_CurrPos += 4;
		}
		else if (currChar == ' ')
		{
			m_CurrPos++;
		}

		m_CurrReadPos++;
		currChar = *m_CurrReadPos;
	}
}

const bool Tokenizer::_IsInt(const char* i_str, const size i_strLen)
{
	for (size i = 0; i < i_strLen; i++)
	{
		if (!isdigit(i_str[i]))
		{
			return false;
		}
	}
	return true;
}

const bool Tokenizer::_IsFloat(const char* i_str, const size i_strLen)
{
	bool hasDot = false;
	for (size i = 0; i < i_strLen; i++)
	{
		if (!isdigit(i_str[i]) && i_str[i] != '.')
		{
			return false;
		}

		if (i_str[i] == '.')
		{
			if (!hasDot)
			{
				hasDot = true;
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

const size Tokenizer::_GetRawString(const_cstr* o_outputStr)
{
	*o_outputStr = m_CurrReadPos;
	size rawStrLen = 0;
	while (*m_CurrReadPos != 0 && isgraph(*m_CurrReadPos))
	{
		rawStrLen++;
		m_CurrReadPos++;
	}
	return rawStrLen;
}

const_cstr Tokenizer::_DuplicateStringRange(const_cstr i_inputStr, const size i_strLen)
{
	cstr str = (cstr)m_AllocFunc(i_strLen + 1);
	memcpy(str, i_inputStr, i_strLen);
	str[i_strLen] = 0;
	return str;
}

// -------------------------------------------------------------------
}

// -------------------------------------------------------------------
};
