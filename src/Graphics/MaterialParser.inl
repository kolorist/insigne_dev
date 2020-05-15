#include <floral/io/nativeio.h>

namespace mat_parser
{
// ----------------------------------------------------------------------------

template <class TMemoryArena>
const MaterialDescription ParseMaterial(const floral::path i_path, TMemoryArena* i_memoryArena)
{
	floral::file_info inp = floral::open_file(i_path);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)i_memoryArena->allocate(inp.file_size + 1);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);

	inpStream.buffer[inp.file_size] = 0;
	return ParseMaterial((const_cstr)inpStream.buffer, i_memoryArena);
}

// ----------------------------------------------------------------------------

template <class TFileSystem, class TMemoryArena>
const MaterialDescription ParseMaterial(TFileSystem* i_fs, const floral::relative_path& i_path, TMemoryArena* i_memoryArena)
{
	floral::file_info inp = floral::open_file_read(i_fs, i_path);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)i_memoryArena->allocate(inp.file_size + 1);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);

	inpStream.buffer[inp.file_size] = 0;
	return ParseMaterial((const_cstr)inpStream.buffer, i_memoryArena);
}

// ----------------------------------------------------------------------------

template <class TMemoryArena>
const MaterialDescription ParseMaterial(const_cstr i_descBuffer, TMemoryArena* i_memoryArena)
{
	MaterialDescription material;

	internal::TokenArray<TMemoryArena> tokenArray(i_memoryArena);

	const_cstr lexResult = MaterialLexer(i_descBuffer, &tokenArray, i_memoryArena);
	FLORAL_ASSERT(lexResult == nullptr);
	const_cstr parseResult = MaterialParser(tokenArray, &material, i_memoryArena);
	FLORAL_ASSERT(parseResult == nullptr);

	return material;
}

namespace internal
{
// ----------------------------------------------------------------------------

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
const_cstr MaterialLexer(const_cstr i_descBuffer, TokenArray<TMemoryArena>* o_tokenArray, TMemoryArena* i_memoryArena)
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

// ----------------------------------------------------------------------------

template <class TMemoryArena>
const_cstr MaterialParser(const TokenArray<TMemoryArena>& i_tokenArray, MaterialDescription* o_material, TMemoryArena* i_memoryArena)
{
	FLORAL_ASSERT(i_tokenArray.get_size() > 0);
	size tokenIdx = 0;
	do
	{
		const Token& token = i_tokenArray[tokenIdx];

		switch (token.type)
		{
		case TokenType::Shader:
		{
			const_cstr parseResult = _ParseShader(i_tokenArray, tokenIdx, o_material, i_memoryArena);
			if (parseResult != nullptr)
			{
				return parseResult;
			}
			break;
		}

		case TokenType::RenderState:
		{
			const_cstr parseResult = _ParseRenderState(i_tokenArray, tokenIdx, &(o_material->renderState), i_memoryArena);
			if (parseResult != nullptr)
			{
				return parseResult;
			}
			break;
		}

		case TokenType::Params:
		{
			const_cstr parseResult = _ParseParams(i_tokenArray, tokenIdx, o_material, i_memoryArena);
			if (parseResult != nullptr)
			{
				return parseResult;
			}
			break;
		}

		case TokenType::Invalid:
			FLORAL_ASSERT_MSG(false, "Something is wrong");
			return nullptr;

		case TokenType::EndOfTokenStream:
			return nullptr;

		default:
			FLORAL_ASSERT_MSG(false, "Something is SERIOUSLY wrong");
			break;
		}
	}
	while (true);

	return nullptr;
}

template <class TMemoryArena>
const_cstr _ParseShader(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, MaterialDescription* o_material, TMemoryArena* i_memoryArena)
{
	// 1- scan & fill as much as possible
	io_tokenIdx++;
	o_material->vertexShaderPath = nullptr;
	o_material->fragmentShaderPath = nullptr;
	o_material->featuresCount = 0;
	o_material->featureList = nullptr;

	size beginTokenIdx = io_tokenIdx;
	size endTokenIdx = beginTokenIdx;
	do
	{
		const Token& token = i_tokenArray[io_tokenIdx];
		if (token.type == TokenType::EndShader || token.type == TokenType::Invalid || token.type == TokenType::EndOfTokenStream)
		{
			endTokenIdx = io_tokenIdx;
			io_tokenIdx++; // consume EndShader
			break;
		}

		switch (token.type)
		{
		case TokenType::VertexShaderPath:
		{
			io_tokenIdx++;
			const Token& expectedToken = i_tokenArray[io_tokenIdx];
			FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
			FLORAL_ASSERT(o_material->vertexShaderPath == nullptr);
			o_material->vertexShaderPath = expectedToken.strValue;
			break;
		}

		case TokenType::FragmentShaderPath:
		{
			io_tokenIdx++;
			const Token& expectedToken = i_tokenArray[io_tokenIdx];
			FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
			FLORAL_ASSERT(o_material->fragmentShaderPath == nullptr);
			o_material->fragmentShaderPath = expectedToken.strValue;
			break;
		}

		case TokenType::EnableFeature:
		{
			o_material->featuresCount++;
			break;
		}

		default:
			break;
		}

		io_tokenIdx++;
	}
	while (true);

	// 2- check and pre-allocate and fill
	FLORAL_ASSERT(o_material->vertexShaderPath != nullptr);
	FLORAL_ASSERT(o_material->fragmentShaderPath != nullptr);
	if (o_material->featuresCount > 0)
	{
		size parsedFeatures = 0;
		o_material->featureList = i_memoryArena->template allocate_array<const_cstr>(o_material->featuresCount);
		for (size i = beginTokenIdx; i < endTokenIdx; i++)
		{
			const Token& token = i_tokenArray[i];
			if (token.type == TokenType::EnableFeature)
			{
				i++;
				const Token& expectedToken = i_tokenArray[i];
				FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
				o_material->featureList[parsedFeatures] = expectedToken.strValue;
				parsedFeatures++;
			}
		}
		FLORAL_ASSERT(parsedFeatures == o_material->featuresCount);
	}

	return nullptr;
}

template <class TMemoryArena>
const_cstr _ParseRenderState(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, RenderState* o_renderState, TMemoryArena* i_memoryArena)
{
	size i = io_tokenIdx + 1;

	do
	{
		const Token& token = i_tokenArray[i];
		if (token.type == TokenType::EndRenderState || token.type == TokenType::Invalid || token.type == TokenType::EndOfTokenStream)
		{
			i++; // consume EndRenderState
			break;
		}

		switch (token.type)
		{
		case TokenType::DepthWrite:
		{
			i++;
			const Token& expectedToken = i_tokenArray[i];
			FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
			o_renderState->depthWrite = StringToToggle(expectedToken.strValue);
			break;
		}

		case TokenType::DepthTest:
		{
			i++;
			const Token& expectedToken = i_tokenArray[i];
			FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
			o_renderState->depthTest = StringToToggle(expectedToken.strValue);
			if (o_renderState->depthTest == Toggle::Enable)
			{
				i++;
				const Token& expectedFunc = i_tokenArray[i];
				FLORAL_ASSERT(expectedFunc.type == TokenType::ValueStringLiteral);
				o_renderState->depthFunc = StringToCompareFunction(expectedFunc.strValue);
			}
			break;
		}

		case TokenType::CullFace:
		{
			i++;
			const Token& expectedToken = i_tokenArray[i];
			FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
			o_renderState->cullFace = StringToToggle(expectedToken.strValue);
			if (o_renderState->cullFace == Toggle::Enable)
			{
				i++;
				const Token& expectedFaceSide = i_tokenArray[i];
				FLORAL_ASSERT(expectedFaceSide.type == TokenType::ValueStringLiteral);
				o_renderState->faceSide = StringToFaceSide(expectedFaceSide.strValue);

				i++;
				const Token& expectedFrontFace = i_tokenArray[i];
				FLORAL_ASSERT(expectedFrontFace.type == TokenType::ValueStringLiteral);
				o_renderState->frontFace = StringToFrontFace(expectedFrontFace.strValue);
			}
			break;
		}

		case TokenType::Blending:
		{
			i++;
			const Token& expectedToken = i_tokenArray[i];
			FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
			o_renderState->blending = StringToToggle(expectedToken.strValue);
			if (o_renderState->blending == Toggle::Enable)
			{
				i++;
				const Token& expectedEquation = i_tokenArray[i];
				FLORAL_ASSERT(expectedEquation.type == TokenType::ValueStringLiteral);
				o_renderState->blendEquation = StringToBlendEquation(expectedEquation.strValue);

				i++;
				const Token& expectedSFactor = i_tokenArray[i];
				FLORAL_ASSERT(expectedSFactor.type == TokenType::ValueStringLiteral);
				o_renderState->blendSourceFactor = StringToBlendFactor(expectedSFactor.strValue);

				i++;
				const Token& expectedDFactor = i_tokenArray[i];
				FLORAL_ASSERT(expectedDFactor.type == TokenType::ValueStringLiteral);
				o_renderState->blendDestinationFactor = StringToBlendFactor(expectedDFactor.strValue);
			}
			break;
		}

		default:
			break;
		}

		i++;
	}
	while (true);
	io_tokenIdx = i;

	return nullptr;
}

template <class TMemoryArena>
const_cstr _ParseParams(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, MaterialDescription* o_material, TMemoryArena* i_memoryArena)
{
	// 1- scan
	io_tokenIdx++;
	o_material->buffersCount = 0;
	o_material->bufferDescriptions = nullptr;
	o_material->texturesCount = 0;
	o_material->textureDescriptions = nullptr;

	size beginTokenIdx = io_tokenIdx;
	size endTokenIdx = beginTokenIdx;
	do
	{
		const Token& token = i_tokenArray[io_tokenIdx];
		if (token.type == TokenType::EndParams || token.type == TokenType::Invalid || token.type == TokenType::EndOfTokenStream)
		{
			endTokenIdx = io_tokenIdx;
			io_tokenIdx++; // consume EndParams
			break;
		}

		switch (token.type)
		{
		case TokenType::UniformBuffer:
		case TokenType::UniformBufferHolder:
		{
			o_material->buffersCount++;
			break;
		}

		case TokenType::Tex:
		case TokenType::TexHolder:
		{
			o_material->texturesCount++;
			break;
		}

		default:
			break;
		}

		io_tokenIdx++;
	}
	while (true);

	// 2- fill
	if (o_material->buffersCount > 0)
	{
		size parsedBuffers = 0;
		o_material->bufferDescriptions = i_memoryArena->template allocate_array<UBDescription>(o_material->buffersCount);
		for (size i = beginTokenIdx; i < endTokenIdx; i++)
		{
			const Token& token = i_tokenArray[i];
			if (token.type == TokenType::UniformBuffer)
			{
				i++;
				const Token& expectedToken = i_tokenArray[i];
				FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
				o_material->bufferDescriptions[parsedBuffers].identifier = expectedToken.strValue;
				_ParseUB(i_tokenArray, i, &(o_material->bufferDescriptions[parsedBuffers]), i_memoryArena);
				parsedBuffers++;
			}
			else if (token.type == TokenType::UniformBufferHolder)
			{
				i++;
				const Token& expectedToken = i_tokenArray[i];
				FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
				o_material->bufferDescriptions[parsedBuffers].identifier = expectedToken.strValue;
				o_material->bufferDescriptions[parsedBuffers].isPlaceholder = true;
				o_material->bufferDescriptions[parsedBuffers].membersCount = 0;
				o_material->bufferDescriptions[parsedBuffers].members = nullptr;
				parsedBuffers++;
			}
		}
		FLORAL_ASSERT(parsedBuffers == o_material->buffersCount);
	}

	if (o_material->texturesCount > 0)
	{
		size parsedTextures = 0;
		o_material->textureDescriptions = i_memoryArena->template allocate_array<TextureDescription>(o_material->texturesCount);
		for (size i = beginTokenIdx; i < endTokenIdx; i++)
		{
			const Token& token = i_tokenArray[i];
			if (token.type == TokenType::Tex)
			{
				i++;
				const Token& expectedToken = i_tokenArray[i];
				FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
				o_material->textureDescriptions[parsedTextures].identifier = expectedToken.strValue;
				_ParseTexture(i_tokenArray, i, &(o_material->textureDescriptions[parsedTextures]), i_memoryArena);
				parsedTextures++;
			}
			else if (token.type == TokenType::TexHolder)
			{
				i++;
				const Token& expectedDim = i_tokenArray[i];
				FLORAL_ASSERT(expectedDim.type == TokenType::ValueStringLiteral);
				if (strcmp(expectedDim.strValue, "tex2d") == 0)
				{
					o_material->textureDescriptions[parsedTextures].dimension = TextureDimension::Texture2D;
				}
				else if (strcmp(expectedDim.strValue, "texcube") == 0)
				{
					o_material->textureDescriptions[parsedTextures].dimension = TextureDimension::TextureCube;
				}
				else
				{
					FLORAL_ASSERT_MSG(false, "Missing dimension");
				}

				i++;
				const Token& expectedId = i_tokenArray[i];
				FLORAL_ASSERT(expectedId.type == TokenType::ValueStringLiteral);

				o_material->textureDescriptions[parsedTextures].identifier = expectedId.strValue;
				o_material->textureDescriptions[parsedTextures].isPlaceholder = true;
				o_material->textureDescriptions[parsedTextures].minFilter = TextureFilter::Invalid;
				o_material->textureDescriptions[parsedTextures].magFilter = TextureFilter::Invalid;
				o_material->textureDescriptions[parsedTextures].wrapS = TextureWrap::Invalid;
				o_material->textureDescriptions[parsedTextures].wrapT = TextureWrap::Invalid;
				o_material->textureDescriptions[parsedTextures].wrapR = TextureWrap::Invalid;
				o_material->textureDescriptions[parsedTextures].texturePath = nullptr;
				parsedTextures++;
			}
		}
		FLORAL_ASSERT(parsedTextures = o_material->texturesCount);
	}

	return nullptr;
}

template <class TMemoryArena>
const_cstr _ParseUB(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, UBDescription* o_ubDescription, TMemoryArena* i_memoryArena)
{
	// 1- scan
	io_tokenIdx++;
	o_ubDescription->isPlaceholder = false;
	o_ubDescription->membersCount = 0;
	o_ubDescription->members = nullptr;

	size beginTokenIdx = io_tokenIdx;
	size endTokenIdx = beginTokenIdx;
	do
	{
		const Token& token = i_tokenArray[io_tokenIdx];
		if (token.type == TokenType::EndUniformBuffer || token.type == TokenType::Invalid || token.type == TokenType::EndOfTokenStream)
		{
			endTokenIdx = io_tokenIdx;
			//io_tokenIdx++; // don't consume EndUniformBuffer
			break;
		}

		switch (token.type)
		{
		case TokenType::Int:
		case TokenType::Float:
		case TokenType::Vec2:
		case TokenType::Vec3:
		case TokenType::Vec4:
		case TokenType::Mat3:
		case TokenType::Mat4:
		{
			o_ubDescription->membersCount++;
			break;
		}

		default:
			break;
		}
		io_tokenIdx++;
	}
	while (true);

	//2- fill
	FLORAL_ASSERT(o_ubDescription->identifier != nullptr);
	FLORAL_ASSERT(o_ubDescription->membersCount > 0);

	size parsedMembers = 0;
	o_ubDescription->members = i_memoryArena->template allocate_array<UBParam>(o_ubDescription->membersCount);
	for (size i = beginTokenIdx; i < endTokenIdx; i++)
	{
		const Token& token = i_tokenArray[i];
		UBParam& param = o_ubDescription->members[parsedMembers];
		param.dataType = UBParamType::Invalid;
		param.identifier = nullptr;
		param.data = nullptr;

		if (token.type == TokenType::Int)
		{
			i++;
			const Token& expectedToken = i_tokenArray[i];
			FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
			param.identifier = expectedToken.strValue;
			param.dataType = UBParamType::Int;
			s32* data = i_memoryArena->template allocate<s32>(0);
			_ParseIntValues(i_tokenArray, i, data, 1);
			param.data = data;
		}
		else
		{
			size expectedFloatsCount = 0;
			switch (token.type)
			{
			case TokenType::Float:
				param.dataType = UBParamType::Float;
				expectedFloatsCount = 1;
				break;
			case TokenType::Vec2:
				param.dataType = UBParamType::Vec2;
				expectedFloatsCount = 2;
				break;
			case TokenType::Vec3:
				param.dataType = UBParamType::Vec3;
				expectedFloatsCount = 3;
				break;
			case TokenType::Vec4:
				param.dataType = UBParamType::Vec4;
				expectedFloatsCount = 4;
				break;
			case TokenType::Mat3:
				param.dataType = UBParamType::Mat3;
				expectedFloatsCount = 9;
				break;
			case TokenType::Mat4:
				param.dataType = UBParamType::Mat4;
				expectedFloatsCount = 16;
				break;
			default:
				FLORAL_ASSERT_MSG(false, "Unknown data type declaration inside UniformBuffer");
				break;
			}

			if (expectedFloatsCount > 0)
			{
				i++;
				const Token& expectedToken = i_tokenArray[i];
				FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
				param.identifier = expectedToken.strValue;
				f32* data = i_memoryArena->template allocate_array<f32>(expectedFloatsCount);
				_ParseFloatValues(i_tokenArray, i, data, expectedFloatsCount);
				param.data = data;
			}
		}

		FLORAL_ASSERT(param.dataType != UBParamType::Invalid);
		FLORAL_ASSERT(param.identifier != nullptr);
		FLORAL_ASSERT(param.data != nullptr);
		parsedMembers++;
	}
	FLORAL_ASSERT(parsedMembers == o_ubDescription->membersCount);

	return nullptr;
}

template <class TMemoryArena>
const_cstr _ParseTexture(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, TextureDescription* o_texDescription, TMemoryArena* i_memoryArena)
{
	// 1- scan
	io_tokenIdx++;
	o_texDescription->isPlaceholder = false;
	o_texDescription->dimension = TextureDimension::Invalid;
	o_texDescription->minFilter = TextureFilter::Invalid;
	o_texDescription->magFilter = TextureFilter::Invalid;
	o_texDescription->wrapS = TextureWrap::Invalid;
	o_texDescription->wrapT = TextureWrap::Invalid;
	o_texDescription->wrapR = TextureWrap::Invalid;
	o_texDescription->texturePath = nullptr;

	do
	{
		const Token& token = i_tokenArray[io_tokenIdx];
		if (token.type == TokenType::EndTex || token.type == TokenType::Invalid || token.type == TokenType::EndOfTokenStream)
		{
			//io_tokenIdx++; // don't consume EndTex
			break;
		}

		switch (token.type)
		{
		case TokenType::TexDim:
		{
			io_tokenIdx++;
			const Token& expectedToken = i_tokenArray[io_tokenIdx];
			FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
			if (strcmp(expectedToken.strValue, "tex2d") == 0)
			{
				o_texDescription->dimension = TextureDimension::Texture2D;
			}
			else if (strcmp(expectedToken.strValue, "texcube") == 0)
			{
				o_texDescription->dimension = TextureDimension::TextureCube;
			}
			else
			{
				FLORAL_ASSERT_MSG(false, "Unknown value for 'dim'");
			}
			break;
		}

		case TokenType::TexMinFilter:
		{
			io_tokenIdx++;
			const Token& expectedToken = i_tokenArray[io_tokenIdx];
			FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
			if (strcmp(expectedToken.strValue, "nearest") == 0)
			{
				o_texDescription->minFilter = TextureFilter::Nearest;
			}
			else if (strcmp(expectedToken.strValue, "linear") == 0)
			{
				o_texDescription->minFilter = TextureFilter::Linear;
			}
			else if (strcmp(expectedToken.strValue, "nearest_mipmap_nearest") == 0)
			{
				o_texDescription->minFilter = TextureFilter::NearestMipmapNearest;
			}
			else if (strcmp(expectedToken.strValue, "linear_mipmap_nearest") == 0)
			{
				o_texDescription->minFilter = TextureFilter::LinearMipmapNearest;
			}
			else if (strcmp(expectedToken.strValue, "nearest_mipmap_linear") == 0)
			{
				o_texDescription->minFilter = TextureFilter::NearestMipmapLinear;
			}
			else if (strcmp(expectedToken.strValue, "linear_mipmap_linear") == 0)
			{
				o_texDescription->minFilter = TextureFilter::LinearMipmapLinear;
			}
			else
			{
				FLORAL_ASSERT_MSG(false, "Unknown value for 'min_filter'");
			}
			break;
		}

		case TokenType::TexMagFilter:
		{
			io_tokenIdx++;
			const Token& expectedToken = i_tokenArray[io_tokenIdx];
			FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
			if (strcmp(expectedToken.strValue, "nearest") == 0)
			{
				o_texDescription->magFilter = TextureFilter::Nearest;
			}
			else if (strcmp(expectedToken.strValue, "linear") == 0)
			{
				o_texDescription->magFilter = TextureFilter::Linear;
			}
			else
			{
				FLORAL_ASSERT_MSG(false, "Unknown value for 'mag_filter'");
			}
			break;
		}

		case TokenType::TexWrapS:
		case TokenType::TexWrapT:
		case TokenType::TexWrapR:
		{
			io_tokenIdx++;
			const Token& expectedToken = i_tokenArray[io_tokenIdx];
			FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
			TextureWrap tw(TextureWrap::Invalid);
			if (strcmp(expectedToken.strValue, "clamp_to_edge") == 0)
			{
				tw = TextureWrap::ClampToEdge;
			}
			else if (strcmp(expectedToken.strValue, "mirrored_repeat") == 0)
			{
				tw = TextureWrap::MirroredRepeat;
			}
			else if (strcmp(expectedToken.strValue, "repeat") == 0)
			{
				tw = TextureWrap::Repeat;
			}
			else
			{
				FLORAL_ASSERT_MSG(false, "Unknown value for 'wrap_*'");
			}

			if (token.type == TokenType::TexWrapS)
			{
				o_texDescription->wrapS = tw;
			}
			else if (token.type == TokenType::TexWrapT)
			{
				o_texDescription->wrapT = tw;
			}
			else if (token.type == TokenType::TexWrapR)
			{
				o_texDescription->wrapR = tw;
			}
			break;
		}

		case TokenType::TexPath:
		{
			io_tokenIdx++;
			const Token& expectedToken = i_tokenArray[io_tokenIdx];
			FLORAL_ASSERT(expectedToken.type == TokenType::ValueStringLiteral);
			o_texDescription->texturePath = expectedToken.strValue;
			break;
		}

		default:
			break;
		}

		io_tokenIdx++;
	}
	while (true);

	FLORAL_ASSERT(o_texDescription->dimension != TextureDimension::Invalid);
	FLORAL_ASSERT(o_texDescription->minFilter != TextureFilter::Invalid);
	FLORAL_ASSERT(o_texDescription->magFilter != TextureFilter::Invalid);
	FLORAL_ASSERT(o_texDescription->wrapS != TextureWrap::Invalid);
	FLORAL_ASSERT(o_texDescription->wrapT != TextureWrap::Invalid);
	if (o_texDescription->dimension == TextureDimension::TextureCube)
	{
		FLORAL_ASSERT(o_texDescription->wrapR != TextureWrap::Invalid);
	}
	FLORAL_ASSERT(o_texDescription->texturePath != nullptr);
	return nullptr;
}

template <class TMemoryArena>
void _ParseIntValues(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, s32* o_data, const size i_count)
{
	for (size i = 0; i < i_count; i++)
	{
		io_tokenIdx++;
		const Token& token = i_tokenArray[io_tokenIdx];
		FLORAL_ASSERT(token.type == TokenType::ValueInt);
		o_data[i] = token.intValue;
	}
}

template <class TMemoryArena>
void _ParseFloatValues(const TokenArray<TMemoryArena>& i_tokenArray, size& io_tokenIdx, f32* o_data, const size i_count)
{
	for (size i = 0; i < i_count; i++)
	{
		io_tokenIdx++;
		const Token& token = i_tokenArray[io_tokenIdx];
		if (token.type == TokenType::ValueFloat)
		{
			o_data[i] = token.floatValue;
		}
		else if (token.type == TokenType::ValueInt)
		{
			o_data[i] = (f32)token.intValue;
		}
	}
}

// ----------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
}
