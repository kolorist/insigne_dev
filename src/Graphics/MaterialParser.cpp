#include "MaterialParser.h"

#include <ctype.h>
#include <string.h>

namespace mat_parser
{
// ----------------------------------------------------------------------------

static const_cstr k_KeywordShader				= "_shader";
static const_cstr k_KeywordVSPath				= "_vs";
static const_cstr k_KeywordFSPath				= "_fs";
static const_cstr k_KeywordEndShader			= "_end_shader";

namespace internal
{
// ----------------------------------------------------------------------------

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

	if (strncmp(tokenStr, k_KeywordShader, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Shader;
	}
	else if (strncmp(tokenStr, k_KeywordVSPath, tokenStrLen) == 0)
	{
		newToken.type = TokenType::VertexShaderPath;
		_SkipBlanks();
		const_cstr rawStr = nullptr;
		const size rawStrLen = _GetRawString(&rawStr);
		newToken.strValue = _DuplicateStringRange(rawStr, rawStrLen);
	}
	else if (strncmp(tokenStr, k_KeywordFSPath, tokenStrLen) == 0)
	{
		newToken.type = TokenType::FragmentShaderPath;
		_SkipBlanks();
		const_cstr rawStr = nullptr;
		const size rawStrLen = _GetRawString(&rawStr);
		newToken.strValue = _DuplicateStringRange(rawStr, rawStrLen);
	}
	else if (strncmp(tokenStr, k_KeywordEndShader, tokenStrLen) == 0)
	{
		newToken.type = TokenType::EndShader;
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

const size Tokenizer::_GetRawString(const_cstr* o_outputStr)
{
	*o_outputStr = m_CurrReadPos;
	size rawStrLen = 0;
	while (isgraph(*m_CurrReadPos))
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

// ----------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
}
