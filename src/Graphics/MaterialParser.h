#pragma once

#include <floral/stdaliases.h>
#include <floral/gpds/vec.h>
#include <floral/gpds/mat.h>
#include <floral/containers/fast_array.h>
#include <floral/cmds/path.h>

namespace mat_parser
{
// ----------------------------------------------------------------------------

enum class UBParamType
{
	Invalid = 0,
	Int,
	Float,
	Vec2,
	Vec3,
	Vec4,
	Mat3,
	Mat4,
	Count
};

struct UBParam
{
	UBParamType									dataType;
	const_cstr									identifier;
	voidptr										data;
};

struct UBDescription
{
	const_cstr									identifier;
	bool										isPlaceholder;
	size										membersCount;
	UBParam*									members;
};

enum class TextureDimension
{
	Invalid = 0,
	Texture2D,
	TextureCube,
	Count
};

enum class TextureFilter
{
	Invalid = 0,
	Nearest,
	Linear,
	NearestMipmapNearest,
	LinearMipmapNearest,
	NearestMipmapLinear,
	LinearMipmapLinear,
	Count
};

enum class TextureWrap
{
	Invalid = 0,
	ClampToEdge,
	MirroredRepeat,
	Repeat,
	Count
};

struct TextureDescription
{
	const_cstr									identifier;
	bool										isPlaceholder;
	TextureDimension							dimension;
	TextureFilter								minFilter;
	TextureFilter								magFilter;
	TextureWrap									wrapS;
	TextureWrap									wrapT;
	TextureWrap									wrapR;
	const_cstr									texturePath;
};

struct MaterialDescription
{
	const_cstr									vertexShaderPath;
	const_cstr									fragmentShaderPath;
	size										featuresCount;
	const_cstr*									featureList;

	size										buffersCount;
	UBDescription*								bufferDescriptions;

	size										texturesCount;
	TextureDescription*							textureDescriptions;
};

// ----------------------------------------------------------------------------

template <class TMemoryArena>
const MaterialDescription						ParseMaterial(const floral::path i_path, TMemoryArena* i_memoryArena);

template <class TMemoryArena>
const MaterialDescription						ParseMaterial(const_cstr i_descBuffer, TMemoryArena* i_memoryArena);

namespace internal
{
// ----------------------------------------------------------------------------

enum class TokenType
{
	Invalid = 0,

	ValueInt,
	ValueFloat,
	ValueStringLiteral,

	Shader, EndShader,
	VertexShaderPath,
	FragmentShaderPath,
	EnableFeature,

	Params, EndParams,
	UniformBuffer, EndUniformBuffer, UniformBufferHolder,
	Int, Float, Vec2, Vec3, Vec4, Mat3, Mat4,
	Tex, EndTex, TexHolder,
	TexDim, TexMinFilter, TexMagFilter, TexWrapS, TexWrapT, TexWrapR, TexPath,
	EndOfTokenStream,
	Count,
};

struct Token
{
	TokenType									type;
	s32											line;
	s32											position;

	union
	{
		const_cstr								strValue;
		s32										intValue;
		f32										floatValue;
		//TextureDimension						texDimValue;
		//TextureFilter							texFilterValue;
		//TextureWrap							texWrapValue;
	};

	Token()
		: type(TokenType::Invalid)
		, line(-1)
		, position(-1)
	{ }
};

class Tokenizer
{
	typedef voidptr								(*AllocFunc)(const size i_size);
public:
	Tokenizer(const_cstr i_strBuffer, AllocFunc i_allocFunc);
	~Tokenizer();

	Token										GetNextToken();

private:
	void										_SkipBlanks();
	const bool									_IsInt(const char* i_str, const size i_strLen);
	const bool									_IsFloat(const char* i_str, const size i_strLen);
	const size									_GetRawString(const_cstr* o_outputStr);
	const_cstr									_DuplicateStringRange(const_cstr i_inputStr, const size i_strLen);

private:
	const_cstr									m_StrBuffer;
	const_cstr									m_CurrReadPos;
	s32											m_CurrLine, m_CurrPos;
	AllocFunc									m_AllocFunc;
};

template <class TAllocator>
using TokenArray = floral::fast_dynamic_array<Token, TAllocator>;

template <class TMemoryArena>
const_cstr MaterialLexer(const_cstr i_descBuffer, TokenArray<TMemoryArena>* o_tokenArray, TMemoryArena* i_memoryArena);

template <class TMemoryArena>
const_cstr MaterialParser(const TokenArray<TMemoryArena>& i_tokenArray, MaterialDescription* o_material, TMemoryArena* i_memoryArena);

template <class TMemoryArena>
const_cstr _ParseShader(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, MaterialDescription* o_material, TMemoryArena* i_memoryArena);

template <class TMemoryArena>
const_cstr _ParseParams(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, MaterialDescription* o_material, TMemoryArena* i_memoryArena);

template <class TMemoryArena>
const_cstr _ParseUB(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, UBDescription* o_ubDescription, TMemoryArena* i_memoryArena);

template <class TMemoryArena>
const_cstr _ParseTexture(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, TextureDescription* o_texDescription, TMemoryArena* i_memoryArena);

template <class TMemoryArena>
void _ParseIntValues(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, s32* o_data, const size i_count);

template <class TMemoryArena>
void _ParseFloatValues(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, f32* o_data, const size i_count);

// ----------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
}

#include "MaterialParser.inl"
