#include "MaterialParser.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

namespace mat_parser
{
// ----------------------------------------------------------------------------

static const_cstr k_KeywordShader				= "_shader";
static const_cstr k_KeywordVSPath				= "vs";
static const_cstr k_KeywordFSPath				= "fs";
static const_cstr k_KeywordEnableFeature		= "enable_feature";
static const_cstr k_KeywordEndShader			= "_end_shader";

static const_cstr k_KeywordParams				= "_params";
static const_cstr k_KeywordUB					= "_p_ub";
static const_cstr k_KeywordInt					= "int";
static const_cstr k_KeywordFloat				= "float";
static const_cstr k_KeywordVec2					= "vec2";
static const_cstr k_KeywordVec3					= "vec3";
static const_cstr k_KeywordVec4					= "vec4";
static const_cstr k_KeywordMat3					= "mat3";
static const_cstr k_KeywordMat4					= "mat4";
static const_cstr k_KeywordEndUB				= "_end_p_ub";
static const_cstr k_KeywordUBHolder				= "_p_ub_holder";
static const_cstr k_KeywordTex					= "_p_tex";
static const_cstr k_KeywordDim					= "dim";
static const_cstr k_KeywordMinFilter			= "min_filter";
static const_cstr k_KeywordMagFilter			= "mag_filter";
static const_cstr k_KeywordWrapS				= "wrap_s";
static const_cstr k_KeywordWrapT				= "wrap_t";
static const_cstr k_KeywordWrapR				= "wrap_r";
static const_cstr k_KeywordPath					= "path";
static const_cstr k_KeywordEndTex				= "_end_p_tex";
static const_cstr k_KeywordTexHolder			= "_p_tex_holder";
static const_cstr k_KeywordEndParams			= "_end_params";

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

	if (tokenStrLen == 0)
	{
		newToken.type = TokenType::EndOfTokenStream;
		return newToken;
	}

	if (strncmp(tokenStr, k_KeywordShader, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Shader;
	}
	else if (strncmp(tokenStr, k_KeywordVSPath, tokenStrLen) == 0)
	{
		newToken.type = TokenType::VertexShaderPath;
	}
	else if (strncmp(tokenStr, k_KeywordFSPath, tokenStrLen) == 0)
	{
		newToken.type = TokenType::FragmentShaderPath;
	}
	else if (strncmp(tokenStr, k_KeywordEnableFeature, tokenStrLen) == 0)
	{
		newToken.type = TokenType::EnableFeature;
	}
	else if (strncmp(tokenStr, k_KeywordEndShader, tokenStrLen) == 0)
	{
		newToken.type = TokenType::EndShader;
	}


	else if (strncmp(tokenStr, k_KeywordParams, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Params;
	}
	else if (strncmp(tokenStr, k_KeywordUB, tokenStrLen) == 0)
	{
		newToken.type = TokenType::UniformBuffer;
	}
	else if (strncmp(tokenStr, k_KeywordInt, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Int;
	}
	else if (strncmp(tokenStr, k_KeywordFloat, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Float;
	}
	else if (strncmp(tokenStr, k_KeywordVec2, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Vec2;
	}
	else if (strncmp(tokenStr, k_KeywordVec3, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Vec3;
	}
	else if (strncmp(tokenStr, k_KeywordVec4, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Vec4;
	}
	else if (strncmp(tokenStr, k_KeywordMat3, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Mat3;
	}
	else if (strncmp(tokenStr, k_KeywordMat4, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Mat4;
	}
	else if (strncmp(tokenStr, k_KeywordEndUB, tokenStrLen) == 0)
	{
		newToken.type = TokenType::EndUniformBuffer;
	}
	else if (strncmp(tokenStr, k_KeywordUBHolder, tokenStrLen) == 0)
	{
		newToken.type = TokenType::UniformBufferHolder;
	}
	else if (strncmp(tokenStr, k_KeywordTex, tokenStrLen) == 0)
	{
		newToken.type = TokenType::Tex;
	}
	else if (strncmp(tokenStr, k_KeywordDim, tokenStrLen) == 0)
	{
		newToken.type = TokenType::TexDim;
	}
	else if (strncmp(tokenStr, k_KeywordMinFilter, tokenStrLen) == 0)
	{
		newToken.type = TokenType::TexMinFilter;
	}
	else if (strncmp(tokenStr, k_KeywordMagFilter, tokenStrLen) == 0)
	{
		newToken.type = TokenType::TexMagFilter;
	}
	else if (strncmp(tokenStr, k_KeywordWrapS, tokenStrLen) == 0)
	{
		newToken.type = TokenType::TexWrapS;
	}
	else if (strncmp(tokenStr, k_KeywordWrapT, tokenStrLen) == 0)
	{
		newToken.type = TokenType::TexWrapT;
	}
	else if (strncmp(tokenStr, k_KeywordWrapR, tokenStrLen) == 0)
	{
		newToken.type = TokenType::TexWrapR;
	}
	else if (strncmp(tokenStr, k_KeywordPath, tokenStrLen) == 0)
	{
		newToken.type = TokenType::TexPath;
	}
	else if (strncmp(tokenStr, k_KeywordEndTex, tokenStrLen) == 0)
	{
		newToken.type = TokenType::EndTex;
	}
	else if (strncmp(tokenStr, k_KeywordTexHolder, tokenStrLen) == 0)
	{
		newToken.type = TokenType::TexHolder;
	}
	else if (strncmp(tokenStr, k_KeywordEndParams, tokenStrLen) == 0)
	{
		newToken.type = TokenType::EndParams;
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

// ----------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
}
