#pragma once

#include <floral/stdaliases.h>
#include <floral/cmds/path.h>
#include <floral/containers/fast_array.h>
#include <floral/io/filesystem.h>

namespace pfx_parser
{
// -------------------------------------------------------------------

enum class ColorFormat
{
	LDR = 0,
	HDRMedium,
	HDRHigh
};

enum class DepthFormat
{
	Off = 0,
	On
};

enum class Attachment
{
	Color = 0,
	Depth
};

enum class FBSizeType
{
	Relative = 0,
	Absolute
};

struct ColorAttachment
{
	ColorFormat									format;
};

struct FBDescription
{
	const_cstr									name;
	FBSizeType									resType;
	f32											width;
	f32											height;
	s32											colorAttachmentsCount;
	ColorAttachment*							colorAttachmentList;
	DepthFormat									depthFormat;
};

struct BindDescription
{
	const_cstr									samplerName;
	const_cstr									inputFBName;
	Attachment									attachment;
	s32											slot;
};

struct PassDescription
{
	const_cstr									name;
	const_cstr									targetFBName;
	const_cstr									materialFileName;

	s32											bindingsCount;
	BindDescription*							bindingList;
};

struct PresetDescription
{
	const_cstr									name;
	s32											passesCount;
	PassDescription*							passList;
};

struct PostEffectsDescription
{
	ColorFormat									mainFbFormat;

	s32											fbsCount;
	FBDescription*								fbList;

	s32											presetsCount;
	PresetDescription*							presetList;
};

// -------------------------------------------------------------------

template <class TMemoryArena>
const PostEffectsDescription					ParsePostFX(const floral::path& i_path, TMemoryArena* i_memoryArena);

template <class TFileSystem, class TMemoryArena>
const PostEffectsDescription					ParsePostFX(TFileSystem* i_fs, const floral::relative_path& i_path, TMemoryArena* i_memoryArena);

template <class TMemoryArena>
const PostEffectsDescription					ParsePostFX(const_cstr i_descBuffer, TMemoryArena* i_memoryArena);

namespace internal
{
// -------------------------------------------------------------------

enum class TokenType
{
	Invalid = 0,

	ValueInt,
	ValueFloat,
	ValueStringLiteral,

	MainFBFormat,

	FBDeclare, EndFBDeclare,
	FSize, ISize, Depth, Color,

	Preset, EndPreset,
	Pass, EndPass,
	Framebuffer, Material, Bind,
	EndOfTokenStream,
	Count
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
const_cstr PostFXLexer(const_cstr i_descBuffer, TokenArray<TMemoryArena>* o_tokenArray, TMemoryArena* i_memoryArena);

template <class TMemoryArena>
const_cstr PostFXParser(const TokenArray<TMemoryArena>& i_tokenArray, PostEffectsDescription* o_pfxDesc, TMemoryArena* i_memoryArena);

template <class TMemoryArena>
const_cstr _ParseFBDeclare(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, FBDescription* o_fbDesc, TMemoryArena* i_memoryArena);

template <class TMemoryArena>
const_cstr _ParsePreset(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, PresetDescription* o_preset, TMemoryArena* i_memoryArena);

template <class TMemoryArena>
const_cstr _ParsePass(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, PassDescription* o_pass, TMemoryArena* i_memoryArena);

// -------------------------------------------------------------------
}

// -------------------------------------------------------------------
}

#include "PostFXParser.inl"
