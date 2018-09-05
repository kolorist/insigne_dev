#include "TextureManager.h"

#include <insigne/render.h>

namespace stone {
	TextureManager::TextureManager()
	{
		m_MemoryArena = g_PersistanceAllocator.allocate_arena<LinearArena>(SIZE_MB(32));
	}

	TextureManager::~TextureManager()
	{
	}

	void TextureManager::Initialize(const u32 i_maxTexturesCount)
	{
		m_CachedTextures.init(i_maxTexturesCount, &g_PersistanceAllocator);
	}


	insigne::texture_handle_t TextureManager::CreateTexture(const floral::path& i_texPath)
	{
		return CreateTexture(i_texPath.pm_PathStr);
	}

	insigne::texture_handle_t TextureManager::CreateTexture(const_cstr i_texPath)
	{
		floral::crc_string textureKey(i_texPath);
		for (u32 i = 0; i < m_CachedTextures.get_size(); i++) {
			if (m_CachedTextures[i].key == textureKey)
				return m_CachedTextures[i].textureHandle;
		}

		floral::file_info texFile = floral::open_file(i_texPath);
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);

		c8 magicChars[4];
		dataStream.read_bytes(magicChars, 4);

		s32 colorRange = 0;
		s32 colorSpace = 0;
		s32 colorChannel = 0;
		f32 encodeGamma = 0.0f;
		s32 mipsCount = 0;
		dataStream.read<s32>(&colorRange);
		dataStream.read<s32>(&colorSpace);
		dataStream.read<s32>(&colorChannel);
		dataStream.read<f32>(&encodeGamma);
		dataStream.read<s32>(&mipsCount);

		s32 width = 1 << (mipsCount - 1);
		s32 height = width;
		size dataSize = ((1 << (2 * mipsCount)) - 1) / 3 * colorChannel;

		voidptr texData = nullptr;
		insigne::texture_handle_t texHdl = insigne::create_texture2d(width, height,
				insigne::texture_format_e::rgb,
				insigne::filtering_e::linear_mipmap_linear, insigne::filtering_e::linear,
				dataSize, texData, true);
		dataStream.read_bytes((p8)texData, dataSize);

		floral::close_file(texFile);
		m_MemoryArena->free_all();

		m_CachedTextures.push_back(TextureRegister{ floral::crc_string(i_texPath), texHdl });

		return texHdl;
	}

	insigne::texture_handle_t TextureManager::CreateTextureCube(const floral::path& i_texPath)
	{
		return CreateTextureCube(i_texPath.pm_PathStr);
	}

	insigne::texture_handle_t TextureManager::CreateTextureCube(const_cstr i_texPath)
	{
		floral::crc_string textureKey(i_texPath);
		for (u32 i = 0; i < m_CachedTextures.get_size(); i++) {
			if (m_CachedTextures[i].key == textureKey)
				return m_CachedTextures[i].textureHandle;
		}

		floral::file_info texFile = floral::open_file(i_texPath);
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);

		c8 magicChars[4];
		dataStream.read_bytes(magicChars, 4);

		s32 colorRange = 0;
		s32 colorSpace = 0;
		s32 colorChannel = 0;
		f32 encodeGamma = 0.0f;
		s32 mipsCount = 0;
		dataStream.read<s32>(&colorRange);
		dataStream.read<s32>(&colorSpace);
		dataStream.read<s32>(&colorChannel);
		dataStream.read<f32>(&encodeGamma);
		dataStream.read<s32>(&mipsCount);

		// TODO: hardcode!!!
		s32 width = 256;
		s32 height = width;
		size dataSizeOneFace = width * height * colorChannel * sizeof(f32);

		voidptr texData = nullptr;
		insigne::texture_handle_t texHdl = insigne::create_texturecube(width, height,
				insigne::texture_format_e::hdr_rgb,
				insigne::filtering_e::linear, insigne::filtering_e::linear,
				dataSizeOneFace, texData, false);
		dataStream.read_bytes((p8)texData, dataSizeOneFace * 6);

		floral::close_file(texFile);
		m_MemoryArena->free_all();

		m_CachedTextures.push_back(TextureRegister{ floral::crc_string(i_texPath), texHdl });

		return texHdl;
	}

	insigne::texture_handle_t TextureManager::CreateMipmapedProbe(const floral::path& i_texPath)
	{
		return CreateMipmapedProbe(i_texPath.pm_PathStr);
	}

	insigne::texture_handle_t TextureManager::CreateMipmapedProbe(const_cstr i_texPath)
	{
		floral::crc_string textureKey(i_texPath);
		for (u32 i = 0; i < m_CachedTextures.get_size(); i++) {
			if (m_CachedTextures[i].key == textureKey)
				return m_CachedTextures[i].textureHandle;
		}

		floral::file_info texFile = floral::open_file(i_texPath);
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);

		c8 magicChars[4];
		dataStream.read_bytes(magicChars, 4);

		s32 colorRange = 0;
		s32 colorSpace = 0;
		s32 colorChannel = 0;
		f32 encodeGamma = 0.0f;
		s32 mipsCount = 0;
		dataStream.read<s32>(&colorRange);
		dataStream.read<s32>(&colorSpace);
		dataStream.read<s32>(&colorChannel);
		dataStream.read<f32>(&encodeGamma);
		dataStream.read<s32>(&mipsCount);

		s32 width = 1 << (mipsCount - 1);
		s32 height = width;
		size dataSizeOneFace = ((1 << (2 * mipsCount)) - 1) / 3 * colorChannel * sizeof(f32);

		voidptr texData = nullptr;
		insigne::texture_handle_t texHdl = insigne::create_texturecube(width, height,
				insigne::texture_format_e::hdr_rgb,
				insigne::filtering_e::linear_mipmap_linear, insigne::filtering_e::linear,
				dataSizeOneFace, texData, true);
		dataStream.read_bytes((p8)texData, dataSizeOneFace * 6);


		floral::close_file(texFile);
		m_MemoryArena->free_all();

		m_CachedTextures.push_back(TextureRegister{ floral::crc_string(i_texPath), texHdl });

		return texHdl;
	}

	insigne::texture_handle_t TextureManager::CreateTexture(const voidptr i_pixels,
			const s32 i_width, const s32 i_height,
			const insigne::texture_format_e i_texFormat)
	{
		return insigne::upload_texture2d(i_width, i_height,
				i_texFormat,
				insigne::filtering_e::nearest, insigne::filtering_e::nearest,
				i_pixels, false);
	}

	//------------------------------------------
	inline const f32 RadicalInverse(u32 i_bits) {
		i_bits = (i_bits << 16u) | (i_bits >> 16u);
		i_bits = ((i_bits & 0x55555555u) << 1u) | ((i_bits & 0xAAAAAAAAu) >> 1u);
		i_bits = ((i_bits & 0x33333333u) << 2u) | ((i_bits & 0xCCCCCCCCu) >> 2u);
		i_bits = ((i_bits & 0x0F0F0F0Fu) << 4u) | ((i_bits & 0xF0F0F0F0u) >> 4u);
		i_bits = ((i_bits & 0x00FF00FFu) << 8u) | ((i_bits & 0xFF00FF00u) >> 8u);
		return f32(i_bits) * 2.3283064365386963e-10f; // / 0x100000000
	}

	floral::vec3f ImportanceSample_GGX(const floral::vec2f& i_uniVars, const f32 i_roughness,
			const floral::vec3f& i_normal)
	{
		f32 alpha = i_roughness * i_roughness;
		f32 alphaSq = alpha * alpha;

		f32 phi = 2.0f * 3.141592f * i_uniVars.x;
		f32 cosTheta = sqrtf( (1.0f - i_uniVars.y) / (1.0f + (alphaSq -1) * i_uniVars.y) );
		f32 sinTheta = sqrtf( 1.0f - cosTheta * cosTheta );

		floral::vec3f h = floral::vec3f(
				sinTheta * cosf(phi),
				sinTheta * sinf(phi),
				cosTheta);
		return h;
	}

	floral::vec2f GetHammersley(const u32 i_idx, const u32 i_numSamples)
	{
		return floral::vec2f((f32)i_idx / (f32)i_numSamples, RadicalInverse(i_idx));
	}

	inline const f32 G_Smith_GGX_Optim(const f32 NoV, const f32 NoL, const f32 roughness)
	{
		f32 alpha = roughness * roughness;
		f32 alphaSq = alpha * alpha;
		f32 G_V = NoL * sqrtf( NoV * ( NoV - NoV * alphaSq ) + alphaSq );
		f32 G_L = NoV * sqrtf( NoL * ( NoL - NoL * alphaSq ) + alphaSq );
		return 0.5f / ( G_V + G_L );
	}

	// TODO: WTF! How did I do this? Needs reimplementation to clarify things.
	inline floral::vec2f CalcSplitSumLUT(f32 roughness, f32 NoV)
	{
		floral::vec3f v;
		v.x = sqrtf(1.0f - NoV * NoV);
		v.y = 0.0f;
		v.z = NoV;

		f32 a = 0, b = 0;

		const u32 numSamples = 32;
		for (u32 i = 0; i < numSamples; i++) {
			floral::vec2f uniVars = GetHammersley(i, numSamples);
			floral::vec3f h = ImportanceSample_GGX(uniVars, roughness, v);
			floral::vec3f l = 2.0f * v.dot(h) * h - v;

			f32 NoL = l.z > 0.0f? l.z : 0.0f;
			f32 NoH = h.z > 0.0f? h.z : 0.0f;
			f32 tmp = v.dot(h);
			f32 VoH = tmp > 0.0f? tmp : 0.0f;

			if (NoL > 0.0f) {
				f32 G = G_Smith_GGX_Optim(NoV, NoL, roughness);
				f32 G_Vis = NoL * G * 4.0f * VoH / NoH;
				f32 Fc = powf(1 - VoH, 5);
				a += (1-Fc) * G_Vis;
				b += Fc * G_Vis;
			}
		}

		return floral::vec2f(a / numSamples, b / numSamples);
	}

	struct hdrpixrg {
		f32 r, g, b;
	};

	void GenerateSplitSumLUT(hdrpixrg* outputTexData, const u32 width, const u32 height)
	{
		f32 roughness = 0.0f;
		f32 NoV = 0.0f;
		// u <=> cos theta (NoV)
		// v <=> roughness
		for (u32 i = 0; i < height; i++)
			for (u32 j = 0; j < width; j++) {
				f32 roughness = 1.0f - ((f32)i) / (f32)height;
				f32 NoV = ((f32)j) / (f32)width;

				floral::vec2f preCalcVal = CalcSplitSumLUT(roughness, NoV);
				outputTexData[i * width + j].r = preCalcVal.x;
				outputTexData[i * width + j].g = preCalcVal.y;
				outputTexData[i * width + j].b = 0.0f;
			}
	}

	insigne::texture_handle_t TextureManager::CreateLUTTexture(const s32 i_width, const s32 i_height)
	{
		voidptr texData = nullptr;
		insigne::texture_handle_t texHdl = insigne::create_texture2d(i_width, i_height,
				insigne::texture_format_e::hdr_rgb,
				insigne::filtering_e::linear, insigne::filtering_e::linear,
				i_width * i_height * sizeof(hdrpixrg), texData, false);
		GenerateSplitSumLUT((hdrpixrg*)texData, i_width, i_height);

		m_CachedTextures.push_back(TextureRegister{ floral::crc_string("split_sum_lut"), texHdl });

		return texHdl;
	}
}
