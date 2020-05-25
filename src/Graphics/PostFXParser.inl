#include <floral/io/nativeio.h>

namespace pfx_parser
{
// -------------------------------------------------------------------

template <class TMemoryArena>
const PostEffectsDescription ParsePostFX(const floral::path& i_path, TMemoryArena* i_memoryArena)
{
	floral::file_info inp = floral::open_file(i_path);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)i_memoryArena->allocate(inp.file_size + 1);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);

	inpStream.buffer[inp.file_size] = 0;
	return ParsePostFX((const_cstr)inpStream.buffer, i_memoryArena);
}

template <class TFileSystem, class TMemoryArena>
const PostEffectsDescription ParsePostFX(TFileSystem* i_fs, const floral::relative_path& i_path, TMemoryArena* i_memoryArena)
{
	floral::file_info inp = floral::open_file_read(i_fs, i_path);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)i_memoryArena->allocate(inp.file_size + 1);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);

	inpStream.buffer[inp.file_size] = 0;
	return ParsePostFX((const_cstr)inpStream.buffer, i_memoryArena);
}

template <class TMemoryArena>
const PostEffectsDescription ParsePostFX(const_cstr i_descBuffer, TMemoryArena* i_memoryArena)
{
	PostEffectsDescription pfxDesc;

	internal::TokenArray<TMemoryArena> tokenArray(i_memoryArena);

	const_cstr lexResult = PostFXLexer(i_descBuffer, &tokenArray, i_memoryArena);
	FLORAL_ASSERT(lexResult == nullptr);
	const_cstr parseResult = PostFXParser(tokenArray, &pfxDesc, i_memoryArena);
	FLORAL_ASSERT(parseResult == nullptr);

	return pfxDesc;
}

namespace internal
{
// -------------------------------------------------------------------

template <class TMemoryArena>
struct StringAllocatorRegistry
{
	static TMemoryArena* allocator;
	static voidptr allocate(const size i_size)
	{
		return allocator->allocate(i_size);
	}
};

template <class TMemoryArena>
TMemoryArena* StringAllocatorRegistry<TMemoryArena>::allocator = nullptr;

template <class TMemoryArena>
const_cstr PostFXLexer(const_cstr i_descBuffer, TokenArray<TMemoryArena>* o_tokenArray, TMemoryArena* i_memoryArena)
{
	StringAllocatorRegistry<TMemoryArena>::allocator = i_memoryArena;
	Tokenizer tokenizer(i_descBuffer, &StringAllocatorRegistry<TMemoryArena>::allocate);
	Token token;

	do
	{
		token = tokenizer.GetNextToken();
		if (token.type != TokenType::Invalid)
		{
			o_tokenArray->push_back(token);
		}
	} while (token.type != TokenType::Invalid && token.type != TokenType::EndOfTokenStream);

	return nullptr;
}

// -------------------------------------------------------------------

template <class TMemoryArena>
const_cstr PostFXParser(const TokenArray<TMemoryArena>& i_tokenArray, PostEffectsDescription* o_pfxDesc, TMemoryArena* i_memoryArena)
{
	FLORAL_ASSERT(i_tokenArray.get_size() > 0);

	floral::fast_dynamic_array<FBDescription, TMemoryArena> fbDescArray(4, i_memoryArena);
	floral::fast_dynamic_array<PresetDescription, TMemoryArena> presetArray(2, i_memoryArena);

	size tokenIdx = 0;
	bool finished = false;

	o_pfxDesc->mainFbFormat = ColorFormat::HDRMedium;

	do
	{
		const Token& token = i_tokenArray[tokenIdx];

		switch (token.type)
		{
		case TokenType::FBDeclare:
		{
			FBDescription newFBDesc;
			tokenIdx++;
			const Token& expectedName = i_tokenArray[tokenIdx];
			FLORAL_ASSERT(expectedName.type == TokenType::ValueStringLiteral);
			newFBDesc.name = expectedName.strValue;
			const_cstr parseResult = _ParseFBDeclare(i_tokenArray, tokenIdx, &newFBDesc, i_memoryArena);
			if (parseResult != nullptr)
			{
				return parseResult;
			}
			fbDescArray.push_back(newFBDesc);
			break;
		}

		case TokenType::MainFBFormat:
		{
			tokenIdx++;
			const Token& expectFormat = i_tokenArray[tokenIdx];
			FLORAL_ASSERT(expectFormat.type == TokenType::ValueStringLiteral);
			if (strcmp(expectFormat.strValue, "ldr") == 0)
			{
				o_pfxDesc->mainFbFormat = ColorFormat::LDR;
			}
			else if (strcmp(expectFormat.strValue, "hdr_medium") == 0)
			{
				o_pfxDesc->mainFbFormat = ColorFormat::HDRMedium;
			}
			else if (strcmp(expectFormat.strValue, "hdr_high") == 0)
			{
				o_pfxDesc->mainFbFormat = ColorFormat::HDRHigh;
			}
			else
			{
				FLORAL_ASSERT(false);
			}
			tokenIdx++;
			break;
		}

		case TokenType::Preset:
		{
			PresetDescription newPreset;
			tokenIdx++;
			const Token& expectedName = i_tokenArray[tokenIdx];
			FLORAL_ASSERT(expectedName.type == TokenType::ValueStringLiteral);
			newPreset.name = expectedName.strValue;
			const_cstr parseResult = _ParsePreset(i_tokenArray, tokenIdx, &newPreset, i_memoryArena);
			if (parseResult != nullptr)
			{
				return parseResult;
			}
			presetArray.push_back(newPreset);
			break;
		}

		case TokenType::Invalid:
			FLORAL_ASSERT_MSG(false, "Something is wrong");
			return nullptr;

		case TokenType::EndOfTokenStream:
			finished = true;
			break;

		default:
			FLORAL_ASSERT_MSG(false, "Something is SERIOUSLY wrong");
			break;
		}
	}
	while (!finished);

	o_pfxDesc->fbsCount = fbDescArray.get_size();
	if (o_pfxDesc->fbsCount > 0)
	{
		o_pfxDesc->fbList = &fbDescArray[0];
	}
	else
	{
		o_pfxDesc->fbList = nullptr;
	}

	o_pfxDesc->presetsCount = presetArray.get_size();
	o_pfxDesc->presetList = &presetArray[0];

	return nullptr;
}

// -------------------------------------------------------------------

template <class TMemoryArena>
const_cstr _ParseFBDeclare(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, FBDescription* o_fbDesc, TMemoryArena* i_memoryArena)
{
	size i = io_tokenIdx + 1;

	floral::fast_dynamic_array<ColorAttachment, TMemoryArena> colorAttachmentArray(4, i_memoryArena);

	do
	{
		const Token& token = i_tokenArray[i];
		if (token.type == TokenType::EndFBDeclare || token.type == TokenType::Invalid || token.type == TokenType::EndOfTokenStream)
		{
			i++; // consume EndFBDeclare
			break;
		}

		switch (token.type)
		{
		case TokenType::FSize:
		{
			i++;
			const Token& expectedWidth = i_tokenArray[i];
			i++;
			const Token& expectedHeight = i_tokenArray[i];
			FLORAL_ASSERT(expectedWidth.type == TokenType::ValueFloat);
			FLORAL_ASSERT(expectedHeight.type == TokenType::ValueFloat);
			o_fbDesc->resType = FBSizeType::Relative;
			o_fbDesc->width = expectedWidth.floatValue;
			o_fbDesc->height = expectedHeight.floatValue;
			break;
		}

		case TokenType::ISize:
		{
			i++;
			const Token& expectedWidth = i_tokenArray[i];
			i++;
			const Token& expectedHeight = i_tokenArray[i];
			FLORAL_ASSERT(expectedWidth.type == TokenType::ValueInt);
			FLORAL_ASSERT(expectedHeight.type == TokenType::ValueInt);
			o_fbDesc->resType = FBSizeType::Absolute;
			o_fbDesc->width = f32(expectedWidth.intValue);
			o_fbDesc->height = f32(expectedHeight.intValue);
			break;
		}

		case TokenType::Depth:
		{
			i++;
			const Token& expectDepth = i_tokenArray[i];
			FLORAL_ASSERT(expectDepth.type == TokenType::ValueStringLiteral);
			if (strcmp(expectDepth.strValue, "on") == 0)
			{
				o_fbDesc->depthFormat = DepthFormat::On;
			}
			else if (strcmp(expectDepth.strValue, "off") == 0)
			{
				o_fbDesc->depthFormat = DepthFormat::Off;
			}
			else
			{
				FLORAL_ASSERT(false);
			}
			break;
		}

		case TokenType::Color:
		{
			i++;
			const Token& expectIdx = i_tokenArray[i];
			FLORAL_ASSERT(expectIdx.type == TokenType::ValueInt);
			FLORAL_ASSERT(expectIdx.intValue == colorAttachmentArray.get_size());
			i++;
			const Token& expectFormat = i_tokenArray[i];
			FLORAL_ASSERT(expectFormat.type == TokenType::ValueStringLiteral);
			ColorAttachment newColorAttach;
			if (strcmp(expectFormat.strValue, "ldr") == 0)
			{
				newColorAttach.format = ColorFormat::LDR;
			}
			else if (strcmp(expectFormat.strValue, "hdr_medium") == 0)
			{
				newColorAttach.format = ColorFormat::HDRMedium;
			}
			else if (strcmp(expectFormat.strValue, "hdr_high") == 0)
			{
				newColorAttach.format = ColorFormat::HDRHigh;
			}
			else
			{
				FLORAL_ASSERT(false);
			}
			colorAttachmentArray.push_back(newColorAttach);
			break;
		}

		default:
			break;
		}

		i++;
	}
	while (true);

	o_fbDesc->colorAttachmentsCount = colorAttachmentArray.get_size();
	o_fbDesc->colorAttachmentList = &colorAttachmentArray[0];

	io_tokenIdx = i;
	return nullptr;
}

// -------------------------------------------------------------------

template <class TMemoryArena>
const_cstr _ParsePreset(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, PresetDescription* o_preset, TMemoryArena* i_memoryArena)
{
	size i = io_tokenIdx + 1;

	floral::fast_dynamic_array<PassDescription, TMemoryArena> passArray(4, i_memoryArena);

	do
	{
		const Token& token = i_tokenArray[i];
		if (token.type == TokenType::EndPreset || token.type == TokenType::Invalid || token.type == TokenType::EndOfTokenStream)
		{
			i++; // consume EndPreset
			break;
		}

		switch (token.type)
		{
		case TokenType::Pass:
		{
			PassDescription newPass;
			i++;
			const Token& expectedName = i_tokenArray[i];
			FLORAL_ASSERT(expectedName.type == TokenType::ValueStringLiteral);
			newPass.name = expectedName.strValue;
			const_cstr parseResult = _ParsePass(i_tokenArray, i, &newPass, i_memoryArena);
			if (parseResult != nullptr)
			{
				return parseResult;
			}
			passArray.push_back(newPass);
			break;
		}

		default:
			break;
		}
	}
	while (true);

	o_preset->passesCount = passArray.get_size();
	o_preset->passList = &passArray[0];

	io_tokenIdx = i;
	return nullptr;
}

// -------------------------------------------------------------------

template <class TMemoryArena>
const_cstr _ParsePass(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, PassDescription* o_pass, TMemoryArena* i_memoryArena)
{
	size i = io_tokenIdx + 1;

	floral::fast_dynamic_array<BindDescription, TMemoryArena> bindArray(4, i_memoryArena);

	do
	{
		const Token& token = i_tokenArray[i];
		if (token.type == TokenType::EndPass || token.type == TokenType::Invalid || token.type == TokenType::EndOfTokenStream)
		{
			i++; // consume EndPass
			break;
		}

		switch (token.type)
		{
		case TokenType::Framebuffer:
		{
			i++;
			const Token& expectedName = i_tokenArray[i];
			FLORAL_ASSERT(expectedName.type == TokenType::ValueStringLiteral);
			o_pass->targetFBName = expectedName.strValue;
			break;
		}

		case TokenType::Material:
		{
			i++;
			const Token& expectedName = i_tokenArray[i];
			FLORAL_ASSERT(expectedName.type == TokenType::ValueStringLiteral);
			o_pass->materialFileName = expectedName.strValue;
			break;
		}

		case TokenType::Bind:
		{
			BindDescription newBind;
			i++;
			const Token& expectedSampler = i_tokenArray[i];
			FLORAL_ASSERT(expectedSampler.type == TokenType::ValueStringLiteral);
			i++;
			const Token& expectedInputFB = i_tokenArray[i];
			FLORAL_ASSERT(expectedInputFB.type == TokenType::ValueStringLiteral);
			i++;
			const Token& expectedAttachment = i_tokenArray[i];

			newBind.samplerName = expectedSampler.strValue;
			newBind.inputFBName = expectedInputFB.strValue;
			if (expectedAttachment.type == TokenType::Color)
			{
				newBind.attachment = Attachment::Color;
				i++;
				const Token& expectedSlot = i_tokenArray[i];
				FLORAL_ASSERT(expectedSlot.type == TokenType::ValueInt);
				newBind.slot = expectedSlot.intValue;
			}
			else if (expectedAttachment.type == TokenType::Depth)
			{
				newBind.attachment = Attachment::Depth;
				newBind.slot = -1;
			}
			else
			{
				FLORAL_ASSERT(false);
			}

			bindArray.push_back(newBind);
			break;
		}

		default:
			break;
		}

		i++;
	}
	while (true);

	o_pass->bindingsCount = bindArray.get_size();
	o_pass->bindingList = &bindArray[0];

	io_tokenIdx = i;
	return nullptr;
}

// -------------------------------------------------------------------
}

// -------------------------------------------------------------------
}
