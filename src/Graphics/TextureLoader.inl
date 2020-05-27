#include "stb_image.h"

#include <insigne/ut_textures.h>

#include <floral/io/nativeio.h>

namespace tex_loader
{
// ----------------------------------------------------------------------------

template <class TIOAllocator>
const insigne::texture_handle_t LoadCBTexture(const floral::path& i_path, insigne::texture_desc_t& io_desc, TIOAllocator* i_ioAllocator, const bool i_loadMipmaps /* = false */)
{
	// TODO: for texture 2d: I havent test the path for loading texture with no mipmaps
	floral::file_info inp = floral::open_file(i_path);
	floral::file_stream dataStream;
	dataStream.buffer = (p8)i_ioAllocator->allocate(inp.file_size);
	floral::read_all_file(inp, dataStream);
	floral::close_file(inp);

	TextureHeader header;
	dataStream.read(&header);

	io_desc.width = header.resolution;
	io_desc.height = header.resolution;

	if (header.colorRange == ColorRange::LDR)
	{
		switch (header.compression)
		{
		case Compression::DXT:
		{
			io_desc.compression = insigne::texture_compression_e::dxt;
			FLORAL_ASSERT(header.colorSpace == ColorSpace::Linear);
			FLORAL_ASSERT(header.encodedGamma == 1.0f);
			switch (header.colorChannel)
			{
				case ColorChannel::RGB:
					io_desc.format = insigne::texture_format_e::rgb;
					break;
				case ColorChannel::RGBA:
					io_desc.format = insigne::texture_format_e::rgba;
					break;
				default:
					FLORAL_ASSERT(false);
					break;
			}
			break;
		}

		case Compression::NoCompress:
		{
			io_desc.compression = insigne::texture_compression_e::no_compression;
			if (header.colorSpace == ColorSpace::Linear)
			{
				FLORAL_ASSERT(header.encodedGamma == 1.0f);
				switch (header.colorChannel)
				{
					case ColorChannel::RG:
						io_desc.format = insigne::texture_format_e::rg;
						break;
					case ColorChannel::RGB:
						io_desc.format = insigne::texture_format_e::rgb;
						break;
					case ColorChannel::RGBA:
						io_desc.format = insigne::texture_format_e::rgba;
						break;
					default:
						FLORAL_ASSERT(false);
						break;
				}
			}
			else if (header.colorSpace == ColorSpace::GammaCorrected)
			{
				FLORAL_ASSERT(header.encodedGamma < 1.0f);
				switch (header.colorChannel)
				{
					case ColorChannel::RGB:
						io_desc.format = insigne::texture_format_e::srgb;
						break;
					case ColorChannel::RGBA:
						io_desc.format = insigne::texture_format_e::srgba;
						break;
					default:
						FLORAL_ASSERT(false);
						break;
				}
			}
			else
			{
				FLORAL_ASSERT(false);
			}
			break;
		}

		default:
			FLORAL_ASSERT(false);
			break;
		}

	}
	else // hdr
	{
		FLORAL_ASSERT(header.colorSpace == ColorSpace::Linear);
		FLORAL_ASSERT(header.colorSpace == ColorSpace::Linear);
		FLORAL_ASSERT(header.colorChannel == ColorChannel::RGB);
		switch (header.compression)
		{
		case Compression::DXT:
		{
			io_desc.compression = insigne::texture_compression_e::dxt;
			io_desc.format = insigne::texture_format_e::rgba;
			break;
		}

		case Compression::NoCompress:
		{
			FLORAL_ASSERT(header.encodedGamma == 1.0f);
			io_desc.compression = insigne::texture_compression_e::no_compression;
			switch (header.colorChannel)
			{
				case ColorChannel::RGB:
					io_desc.format = insigne::texture_format_e::hdr_rgb;
					break;
				case ColorChannel::RGBA:
					io_desc.format = insigne::texture_format_e::hdr_rgba;
					break;
				default:
					FLORAL_ASSERT(false);
					break;
			}
			break;
		}

		default:
			FLORAL_ASSERT(false);
			break;
		}
	}

	switch (header.textureType)
	{
	case Type::Texture2D:
		io_desc.dimension = insigne::texture_dimension_e::tex_2d;
		break;
	case Type::CubeMap:
	case Type::PMREM:
		io_desc.dimension = insigne::texture_dimension_e::tex_cube;
		break;
	default:
		FLORAL_ASSERT(false);
		break;
	}

	io_desc.has_mipmap = i_loadMipmaps;

	const size dataSize = insigne::prepare_texture_desc(io_desc);

	dataStream.read_bytes(io_desc.data, dataSize);

	i_ioAllocator->free(dataStream.buffer);

	return insigne::create_texture(io_desc);
}

template <class TFileSystem, class TIOAllocator>
const insigne::texture_handle_t LoadCBTexture(TFileSystem* i_fs, const floral::relative_path& i_path, insigne::texture_desc_t& io_desc, TIOAllocator* i_ioAllocator, const bool i_loadMipmaps /* = false */)
{
	// TODO: for texture 2d: I havent test the path for loading texture with no mipmaps
	floral::file_info inp = floral::open_file_read(i_fs, i_path);
	floral::file_stream dataStream;
	dataStream.buffer = (p8)i_ioAllocator->allocate(inp.file_size);
	floral::read_all_file(inp, dataStream);
	floral::close_file(inp);

	TextureHeader header;
	dataStream.read(&header);

	io_desc.width = header.resolution;
	io_desc.height = header.resolution;

	if (header.colorRange == ColorRange::LDR)
	{
		switch (header.compression)
		{
		case Compression::DXT:
		{
			io_desc.compression = insigne::texture_compression_e::dxt;
			FLORAL_ASSERT(header.colorSpace == ColorSpace::Linear);
			FLORAL_ASSERT(header.encodedGamma == 1.0f);
			switch (header.colorChannel)
			{
				case ColorChannel::RGB:
					io_desc.format = insigne::texture_format_e::rgb;
					break;
				case ColorChannel::RGBA:
					io_desc.format = insigne::texture_format_e::rgba;
					break;
				default:
					FLORAL_ASSERT(false);
					break;
			}
			break;
		}

		case Compression::ETC:
		{
			io_desc.compression = insigne::texture_compression_e::etc;
			switch (header.colorChannel)
			{
				case ColorChannel::RGB:
					io_desc.format = insigne::texture_format_e::rgb;
					break;
				case ColorChannel::RGBA:
					io_desc.format = insigne::texture_format_e::rgba;
					break;
				default:
					FLORAL_ASSERT(false);
					break;
			}
			break;
		}

		case Compression::NoCompress:
		{
			io_desc.compression = insigne::texture_compression_e::no_compression;
			if (header.colorSpace == ColorSpace::Linear)
			{
				FLORAL_ASSERT(header.encodedGamma == 1.0f);
				switch (header.colorChannel)
				{
					case ColorChannel::RG:
						io_desc.format = insigne::texture_format_e::rg;
						break;
					case ColorChannel::RGB:
						io_desc.format = insigne::texture_format_e::rgb;
						break;
					case ColorChannel::RGBA:
						io_desc.format = insigne::texture_format_e::rgba;
						break;
					default:
						FLORAL_ASSERT(false);
						break;
				}
			}
			else if (header.colorSpace == ColorSpace::GammaCorrected)
			{
				FLORAL_ASSERT(header.encodedGamma < 1.0f);
				switch (header.colorChannel)
				{
					case ColorChannel::RGB:
						io_desc.format = insigne::texture_format_e::srgb;
						break;
					case ColorChannel::RGBA:
						io_desc.format = insigne::texture_format_e::srgba;
						break;
					default:
						FLORAL_ASSERT(false);
						break;
				}
			}
			else
			{
				FLORAL_ASSERT(false);
			}
			break;
		}

		default:
			FLORAL_ASSERT(false);
			break;
		}

	}
	else // hdr
	{
		FLORAL_ASSERT(header.colorSpace == ColorSpace::Linear);
		FLORAL_ASSERT(header.colorSpace == ColorSpace::Linear);
		FLORAL_ASSERT(header.colorChannel == ColorChannel::RGB);
		switch (header.compression)
		{
		case Compression::DXT:
		{
			io_desc.compression = insigne::texture_compression_e::dxt;
			io_desc.format = insigne::texture_format_e::rgba;
			break;
		}

		case Compression::ETC:
		{
			io_desc.compression = insigne::texture_compression_e::etc;
			switch (header.colorChannel)
			{
				case ColorChannel::RGB:
					io_desc.format = insigne::texture_format_e::rgb;
					break;
				case ColorChannel::RGBA:
					io_desc.format = insigne::texture_format_e::rgba;
					break;
				default:
					FLORAL_ASSERT(false);
					break;
			}
			break;
		}

		case Compression::NoCompress:
		{
			FLORAL_ASSERT(header.encodedGamma == 1.0f);
			io_desc.compression = insigne::texture_compression_e::no_compression;
			switch (header.colorChannel)
			{
				case ColorChannel::RGB:
					io_desc.format = insigne::texture_format_e::hdr_rgb;
					break;
				case ColorChannel::RGBA:
					io_desc.format = insigne::texture_format_e::hdr_rgba;
					break;
				default:
					FLORAL_ASSERT(false);
					break;
			}
			break;
		}

		default:
			FLORAL_ASSERT(false);
			break;
		}
	}

	switch (header.textureType)
	{
	case Type::Texture2D:
		io_desc.dimension = insigne::texture_dimension_e::tex_2d;
		break;
	case Type::CubeMap:
	case Type::PMREM:
		io_desc.dimension = insigne::texture_dimension_e::tex_cube;
		break;
	default:
		FLORAL_ASSERT(false);
		break;
	}

	io_desc.has_mipmap = i_loadMipmaps;

	const size dataSize = insigne::prepare_texture_desc(io_desc);

	dataStream.read_bytes(io_desc.data, dataSize);

	i_ioAllocator->free(dataStream.buffer);

	return insigne::create_texture(io_desc);
}

// ----------------------------------------------------------------------------
}
