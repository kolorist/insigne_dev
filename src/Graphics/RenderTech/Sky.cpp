#include "Sky.h"

#include <clover/Logger.h>

#include <floral/io/filesystem.h>
#include <floral/containers/fast_array.h>

#include <insigne/ut_render.h>

#include "Graphics/stb_image_write.h"
#include "precomputed_sky.h"

#include "InsigneImGui.h"

namespace stone
{
namespace tech
{
//-------------------------------------------------------------------

Sky::Sky()
	: m_TexDataArenaRegion { "stone/dynamic/sky", SIZE_MB(128), &m_TexDataArena }
{
}

Sky::~Sky()
{
}

ICameraMotion* Sky::GetCameraMotion()
{
	return nullptr;
}

const_cstr Sky::GetName() const
{
	return k_name;
}

void Sky::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	floral::relative_path wdir = floral::build_relative_path("tests/tech/sky");
	floral::push_directory(m_FileSystem, wdir);

	m_DataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(20));
	m_TaskDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(256));
	g_MemoryManager.initialize_allocator(m_TexDataArenaRegion);

	BakedDataInfos bakedDataInfos;
	SkyFixedConfigs skyFixedConfigs;
	stone::initialize_atmosphere(&m_Atmosphere, &bakedDataInfos, &skyFixedConfigs);
	{
		floral::relative_path oFilePath = floral::build_relative_path("sky.meta");
		floral::file_info oFile = floral::open_file_write(m_FileSystem, oFilePath);
		floral::output_file_stream oStream;
		floral::map_output_file(oFile, &oStream);
		oStream.write(bakedDataInfos);
		oStream.write(skyFixedConfigs);
		floral::close_file(oFile);
	}

	m_TexDataArena.free_all();
	f32* transmittanceTexture =
		LoadCacheTex2D("transmittance.dat", bakedDataInfos.transmittanceTextureWidth, bakedDataInfos.transmittanceTextureHeight, 3);
	if (transmittanceTexture == nullptr)
	{
		transmittanceTexture =
			AllocateTexture2D(bakedDataInfos.transmittanceTextureWidth, bakedDataInfos.transmittanceTextureHeight, 3);
		stone::generate_transmittance_texture(m_Atmosphere, transmittanceTexture);
		WriteCacheTex2D("transmittance.dat", transmittanceTexture,
				bakedDataInfos.transmittanceTextureWidth, bakedDataInfos.transmittanceTextureHeight, 3);
	}
	stbi_write_hdr("transmittanceTexture.hdr",
			bakedDataInfos.transmittanceTextureWidth, bakedDataInfos.transmittanceTextureHeight, 3, transmittanceTexture);

	// we won't cache irradianceTexture (yet), because we won't write onto it in this section
	f32* irradianceTexture = AllocateTexture2D(bakedDataInfos.irrandianceTextureWidth, bakedDataInfos.irrandianceTextureHeight, 3);
	f32* deltaIrradianceTexture = LoadCacheTex2D("delta_irradiance.dat",
			bakedDataInfos.irrandianceTextureWidth, bakedDataInfos.irrandianceTextureHeight, 3);
	if (deltaIrradianceTexture == nullptr)
	{
		deltaIrradianceTexture =
			AllocateTexture2D(bakedDataInfos.irrandianceTextureWidth, bakedDataInfos.irrandianceTextureHeight, 3);
		stone::generate_direct_irradiance_texture(m_Atmosphere, transmittanceTexture, deltaIrradianceTexture);
		WriteCacheTex2D("delta_irradiance.dat", deltaIrradianceTexture,
				bakedDataInfos.irrandianceTextureWidth, bakedDataInfos.irrandianceTextureHeight, 3);
	}
	stbi_write_hdr("deltaIrradianceTexture.hdr",
			bakedDataInfos.irrandianceTextureWidth, bakedDataInfos.irrandianceTextureHeight, 3, deltaIrradianceTexture);

	f32** deltaRayleighScatteringTexture = nullptr;
	f32** deltaMieScatteringTexture = nullptr;
	f32** scatteringTexture = nullptr;
	{
		bool needCompute = false;
		s32 width = bakedDataInfos.scatteringTextureWidth;
		s32 height = bakedDataInfos.scatteringTextureHeight;
		s32 depth = bakedDataInfos.scatteringTextureDepth;

		deltaRayleighScatteringTexture = LoadCacheTex3D("delta_rayleigh_scattering.dat", width, height, depth, 3);
		if (deltaRayleighScatteringTexture == nullptr)
		{
			needCompute = true;
			deltaRayleighScatteringTexture = AllocateTexture3D(width, height, depth, 3);
		}
		deltaMieScatteringTexture = LoadCacheTex3D("delta_mie_scattering.dat", width, height, depth, 3);
		if (deltaMieScatteringTexture == nullptr)
		{
			needCompute = true;
			deltaMieScatteringTexture = AllocateTexture3D(width, height, depth, 3);
		}
		scatteringTexture = LoadCacheTex3D("scattering.dat", width, height, depth, 4);
		if (scatteringTexture == nullptr)
		{
			scatteringTexture = AllocateTexture3D(width, height, depth, 4);
		}

		if (needCompute)
		{
			m_TaskDataArena->free_all();
			std::atomic<u32> counter(depth);
			SingleScatteringTaskData* taskData = m_TaskDataArena->allocate_array<SingleScatteringTaskData>(depth);
			for (int i = 0; i < depth; i++)
			{
				taskData[i].atmosphere = &m_Atmosphere;
				taskData[i].deltaRayleighScatteringTexture = deltaRayleighScatteringTexture;
				taskData[i].deltaMieScatteringTexture = deltaMieScatteringTexture;
				taskData[i].scatteringTexture = scatteringTexture;
				taskData[i].transmittanceTexture = transmittanceTexture;

				taskData[i].currentDepth = i;
			}
			for (int i = 0; i < depth; i++)
			{
				refrain2::Task newTask;
				newTask.pm_Instruction = &Sky::ComputeSingleScattering;
				newTask.pm_Data = &taskData[i];
				newTask.pm_Counter = &counter;
				refrain2::g_TaskManager->PushTask(newTask);
			}

			refrain2::BusyWaitForCounter(counter, 0);

			WriteCacheTex3D("delta_rayleigh_scattering.dat", deltaRayleighScatteringTexture, width, height, depth, 3);
			WriteCacheTex3D("delta_mie_scattering.dat", deltaMieScatteringTexture, width, height, depth, 3);
			WriteCacheTex3D("scattering.dat", scatteringTexture, width, height, depth, 4);
		}

		_DebugWriteHDR3D("deltaRayleighScatteringTexture.hdr", width, height, depth, 3, deltaRayleighScatteringTexture);
		_DebugWriteHDR3D("deltaMieScatteringTexture.hdr", width, height, depth, 3, deltaMieScatteringTexture);
		_DebugWriteHDR3D("scatteringTexture.hdr", width, height, depth, 4, scatteringTexture);
	}

	f32** deltaScatteringDensityTexture = AllocateTexture3D(
			bakedDataInfos.scatteringTextureWidth, bakedDataInfos.scatteringTextureHeight, bakedDataInfos.scatteringTextureDepth, 3);
	f32** deltaMultipleScatteringTexture = deltaRayleighScatteringTexture;
	{
		for (s32 scatteringOrder = 2; scatteringOrder <= 4; scatteringOrder++)
		{
			// Compute the scattering density, and store it in
			// deltaScatteringDensityTexture
			{
				s32 width = bakedDataInfos.scatteringTextureWidth;
				s32 height = bakedDataInfos.scatteringTextureHeight;
				s32 depth = bakedDataInfos.scatteringTextureDepth;

				bool needUpdateCache = false;
				m_TaskDataArena->free_all();
				u32 numTasks = 0;
				ScatteringDensityTaskData* taskData = m_TaskDataArena->allocate_array<ScatteringDensityTaskData>(depth);

				for (s32 w = 0; w < depth; w++)
				{
					c8 cacheName[256];
					sprintf(cacheName, "ms_order_%d_delta_scattering_density.dat%02d", scatteringOrder, w);
					ssize cacheSize = width * height * 3 * sizeof(f32);
					voidptr cacheData = LoadCache(cacheName, cacheSize, (voidptr)deltaScatteringDensityTexture[w]);

					if (cacheData == nullptr)
					{
						CLOVER_DEBUG("Cache miss: %s", cacheName);
						needUpdateCache = true;

						taskData[numTasks].atmosphere = &m_Atmosphere;
						taskData[numTasks].deltaScatteringDensityTexture = deltaScatteringDensityTexture;
						taskData[numTasks].deltaMultipleScatteringTexture = deltaMultipleScatteringTexture;
						taskData[numTasks].deltaRayleighScatteringTexture = deltaRayleighScatteringTexture;
						taskData[numTasks].deltaMieScatteringTexture = deltaMieScatteringTexture;
						taskData[numTasks].transmittanceTexture = transmittanceTexture;
						taskData[numTasks].deltaIrradianceTexture = deltaIrradianceTexture;
						taskData[numTasks].currentDepth = w;
						taskData[numTasks].scatteringOrder = scatteringOrder;
						numTasks++;
					}
					else
					{
						CLOVER_DEBUG("Cache hit: %s", cacheName);
					}
				}

				std::atomic<u32> counter(numTasks);
				for (s32 i = 0; i < numTasks; i++)
				{
					refrain2::Task newTask;
					newTask.pm_Instruction = &Sky::ComputeScatteringDensity;
					newTask.pm_Data = &taskData[i];
					newTask.pm_Counter = &counter;
					refrain2::g_TaskManager->PushTask(newTask);
				}

				refrain2::BusyWaitForCounter(counter, 0);

				if (needUpdateCache)
				{
					c8 cacheName[256];
					sprintf(cacheName, "ms_order_%d_delta_scattering_density.dat", scatteringOrder);
					WriteCacheTex3D(cacheName, deltaScatteringDensityTexture, width, height, depth, 3);
				}

				c8 name[128];
				sprintf(name, "ms_order_%d_deltaScatteringDensityTexture.hdr", scatteringOrder);
				_DebugWriteHDR3D(name, width, height, depth, 3, deltaScatteringDensityTexture);
			}

			// Compute the indirect irradiance, store it in deltaIrradianceTexture and
			// accumulate it in irradianceTexture
			{
				s32 width = bakedDataInfos.irrandianceTextureWidth;
				s32 height = bakedDataInfos.irrandianceTextureHeight;
				stone::generate_indirect_irradiance_texture(m_Atmosphere, deltaRayleighScatteringTexture, deltaMieScatteringTexture,
						deltaMultipleScatteringTexture, deltaIrradianceTexture, irradianceTexture, scatteringOrder - 1);

				c8 name[128];
				sprintf(name, "ms_order_%d_deltaIrradianceTexture.hdr", scatteringOrder);
				stbi_write_hdr(name, width, height, 3, deltaIrradianceTexture);
				sprintf(name, "ms_order_%d_irradianceTexture.hdr", scatteringOrder);
				stbi_write_hdr(name, width, height, 3, irradianceTexture);
			}

			// Compute multiple scattering
			// store in deltaMultipleScatteringTexture and acculumate to scatteringTexture
			{
				s32 width = bakedDataInfos.scatteringTextureWidth;
				s32 height = bakedDataInfos.scatteringTextureHeight;
				s32 depth = bakedDataInfos.scatteringTextureDepth;
				m_TaskDataArena->free_all();
				MultipleScatteringTaskData* taskData = m_TaskDataArena->allocate_array<MultipleScatteringTaskData>(depth);

				for (s32 w = 0; w < depth; w++)
				{
					taskData[w].atmosphere = &m_Atmosphere;
					taskData[w].transmittanceTexture = transmittanceTexture;
					taskData[w].deltaScatteringDensityTexture = deltaScatteringDensityTexture;
					taskData[w].scatteringTexture = scatteringTexture;
					taskData[w].deltaMultipleScatteringTexture = deltaMultipleScatteringTexture;
					taskData[w].currentDepth = w;
				}

				std::atomic<u32> counter(depth);
				for (s32 i = 0; i < depth; i++)
				{
					refrain2::Task newTask;
					newTask.pm_Instruction = &Sky::ComputeMultipleScattering;
					newTask.pm_Data = &taskData[i];
					newTask.pm_Counter = &counter;
					refrain2::g_TaskManager->PushTask(newTask);
				}

				refrain2::BusyWaitForCounter(counter, 0);

				c8 name[128];
				sprintf(name, "ms_order_%d_deltaRayleighScatteringTexture.hdr", scatteringOrder);
				_DebugWriteHDR3D(name, width, height, depth, 3, deltaMultipleScatteringTexture);
				sprintf(name, "ms_order_%d_scatteringTexture.hdr", scatteringOrder);
				_DebugWriteHDR3D(name, width, height, depth, 4, scatteringTexture);
			}
		}

		// now we have to write down
		// transmittanceTexture - 2d
		WriteRawTextureHDR2D("transmittance_texture.rtex2d",
				bakedDataInfos.transmittanceTextureWidth, bakedDataInfos.transmittanceTextureHeight, 3, transmittanceTexture);
		// scatteringTexture - 3d
		WriteRawTextureHDR3D("scattering_texture.rtex3d",
				bakedDataInfos.scatteringTextureWidth, bakedDataInfos.scatteringTextureHeight, bakedDataInfos.scatteringTextureDepth, 4, scatteringTexture);
		// irradianceTexture - 2d
		WriteRawTextureHDR2D("irradiance_texture.rtex2d",
				bakedDataInfos.irrandianceTextureWidth, bakedDataInfos.irrandianceTextureHeight, 3, irradianceTexture);
	}
}

void Sky::_OnUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("Controller##Sky");
	ImGui::End();
}

void Sky::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void Sky::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);

	g_MemoryManager.destroy_allocator(m_TexDataArenaRegion);
	g_StreammingAllocator.free(m_TaskDataArena);
	g_StreammingAllocator.free(m_DataArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------

f32** Sky::AllocateTexture3D(const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel)
{
	ssize sliceSizeBytes = i_w * i_h * i_channel * sizeof(f32);
	f32** tex = (f32**)m_TexDataArena.allocate(i_d * sizeof(f32*));
	for (s32 i = 0; i < i_d; i++)
	{
		tex[i] = (f32*)m_TexDataArena.allocate(sliceSizeBytes);
		memset(tex[i], 0, sliceSizeBytes);
	}
	return tex;
}

f32* Sky::AllocateTexture2D(const s32 i_w, const s32 i_h, const s32 i_channel)
{
	ssize texSizeBytes = i_w * i_h * i_channel * sizeof(f32);
	f32* tex = (f32*)m_TexDataArena.allocate(texSizeBytes);
	memset(tex, 0, texSizeBytes);

	return tex;
}

void Sky::WriteCacheTex2D(const_cstr i_cacheFileName, f32* i_data, const s32 i_w, const s32 i_h, const s32 i_channel)
{
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	WriteCache(i_cacheFileName, i_data, sizeBytes);
}

void Sky::WriteCacheTex3D(const_cstr i_cacheFileName, f32** i_data, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel)
{
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	for (s32 i = 0; i < i_d; i++)
	{
		c8 cacheFileName[256];
		sprintf(cacheFileName, "%s%02d", i_cacheFileName, i);
		WriteCache(cacheFileName, i_data[i], sizeBytes);
	}
}

void Sky::WriteCache(const_cstr i_cacheFileName, voidptr i_data, const ssize i_size)
{
	c8 cacheFileName[256];
	sprintf(cacheFileName, "cache/%s", i_cacheFileName);

	floral::relative_path oFilePath = floral::build_relative_path(cacheFileName);
	floral::file_info oFile = floral::open_file_write(m_FileSystem, oFilePath);
	floral::output_file_stream oStream;
	floral::map_output_file(oFile, &oStream);
	oStream.write_bytes(i_data, i_size);
	floral::close_file(oFile);
	CLOVER_VERBOSE("Cache written: %s", cacheFileName);
}

f32* Sky::LoadCacheTex2D(const_cstr i_cacheFileName, const s32 i_w, const s32 i_h, const s32 i_channel)
{
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	return (f32*)LoadCache(i_cacheFileName, sizeBytes);
}

f32** Sky::LoadCacheTex3D(const_cstr i_cacheFileName, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel)
{
	c8 cacheFileName[256];
	sprintf(cacheFileName, "cache/%s00", i_cacheFileName);

	// try loading the first slice
	floral::relative_path iFilePath = floral::build_relative_path(cacheFileName);
	floral::file_info iFile = floral::open_file_read(m_FileSystem, iFilePath);
	if (iFile.file_size == 0)
	{
		return nullptr;
	}

	f32** data = (f32**)m_TexDataArena.allocate(i_d * sizeof(f32*));
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	for (s32 i = 0; i < i_d; i++)
	{
		sprintf(cacheFileName, "%s%02d", i_cacheFileName, i);
		data[i] = (f32*)LoadCache(cacheFileName, sizeBytes);
		FLORAL_ASSERT(data[i] != nullptr);
	}

	return data;
}

voidptr Sky::LoadCache(const_cstr i_cacheFileName, const ssize i_size, const voidptr i_buffer /* = nullptr */)
{
	c8 cacheFileName[256];
	sprintf(cacheFileName, "cache/%s", i_cacheFileName);
	floral::relative_path iFilePath = floral::build_relative_path(cacheFileName);
	floral::file_info iFile = floral::open_file_read(m_FileSystem, iFilePath);
	if (iFile.file_size == 0)
	{
		return nullptr;
	}
	floral::file_stream iStream;
	FLORAL_ASSERT(iFile.file_size == i_size);
	if (i_buffer == nullptr)
	{
		iStream.buffer = (p8)m_TexDataArena.allocate(iFile.file_size);
	}
	else
	{
		iStream.buffer = (p8)i_buffer;
	}
	floral::read_all_file(iFile, iStream);
	floral::close_file(iFile);
	return (voidptr)iStream.buffer;
}

void Sky::_DebugWriteHDR3D(const_cstr i_fileName, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel, f32** i_data)
{
	m_DataArena->free_all();
	const ssize sliceSizeBytes = i_w * i_h * i_channel * sizeof(f32);

	p8 data = (p8)m_DataArena->allocate(i_d * sliceSizeBytes);
	p8 outData = data;
	for (s32 i = 0; i < i_d; i++)
	{
		memcpy(outData, i_data[i], sliceSizeBytes);
		outData += sliceSizeBytes;
	}

	stbi_write_hdr(i_fileName, i_w, i_h * i_d, i_channel, (f32*)data);
}

void Sky::WriteRawTextureHDR2D(const_cstr i_texFileName, const ssize i_w, const ssize i_h, const s32 i_channel, f32* i_data)
{
	floral::relative_path oFilePath = floral::build_relative_path(i_texFileName);
	floral::file_info oFile = floral::open_file_write(m_FileSystem, oFilePath);
	floral::output_file_stream oStream;
	floral::map_output_file(oFile, &oStream);
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	oStream.write_bytes((voidptr)i_data, sizeBytes);
	floral::close_file(oFile);
}

void Sky::WriteRawTextureHDR3D(const_cstr i_texFileName, const ssize i_w, const ssize i_h, const s32 i_d, const s32 i_channel, f32** i_data)
{
	floral::relative_path oFilePath = floral::build_relative_path(i_texFileName);
	floral::file_info oFile = floral::open_file_write(m_FileSystem, oFilePath);
	floral::output_file_stream oStream;
	floral::map_output_file(oFile, &oStream);
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	for (s32 i = 0; i < i_d; i++)
	{
		oStream.write_bytes((voidptr)i_data[i], sizeBytes);
	}
	floral::close_file(oFile);
}

//-------------------------------------------------------------------

refrain2::Task Sky::ComputeSingleScattering(voidptr i_data)
{
	SingleScatteringTaskData* input = (SingleScatteringTaskData*)i_data;
	Atmosphere* atmosphere = input->atmosphere;
	f32* transmittanceTexture = input->transmittanceTexture;
	f32** deltaRayleighScatteringTexture = input->deltaRayleighScatteringTexture;
	f32** deltaMieScatteringTexture = input->deltaMieScatteringTexture;
	f32** scatteringTexture = input->scatteringTexture;
	s32 depth = input->currentDepth;

	generate_single_scattering_texture(*atmosphere, transmittanceTexture,
			deltaRayleighScatteringTexture, deltaMieScatteringTexture, scatteringTexture, depth);

	return refrain2::Task();
}

refrain2::Task Sky::ComputeScatteringDensity(voidptr i_data)
{
	ScatteringDensityTaskData* input = (ScatteringDensityTaskData*)i_data;
	Atmosphere* atmosphere = input->atmosphere;
	f32* transmittanceTexture = input->transmittanceTexture;
	f32** deltaRayleighScatteringTexture = input->deltaRayleighScatteringTexture;
	f32** deltaMieScatteringTexture = input->deltaMieScatteringTexture;
	f32** deltaMultipleScatteringTexture = input->deltaMultipleScatteringTexture;
	f32* deltaIrradianceTexture = input->deltaIrradianceTexture;
	f32** deltaScatteringDensityTexture = input->deltaScatteringDensityTexture;
	s32 depth = input->currentDepth;
	s32 scatteringOrder = input->scatteringOrder;

	stone::generate_scattering_density_texture(*atmosphere, transmittanceTexture,
			deltaRayleighScatteringTexture, deltaMieScatteringTexture, deltaMultipleScatteringTexture,
			deltaIrradianceTexture, deltaScatteringDensityTexture, scatteringOrder, depth);
	return refrain2::Task();
}

refrain2::Task Sky::ComputeMultipleScattering(voidptr i_data)
{
	MultipleScatteringTaskData* input = (MultipleScatteringTaskData*)i_data;
	Atmosphere* atmosphere = input->atmosphere;
	f32* transmittanceTexture = input->transmittanceTexture;

	f32** scatteringTexture = input->scatteringTexture;
	f32** deltaMultipleScatteringTexture = input->deltaMultipleScatteringTexture;
	f32** deltaScatteringDensityTexture = input->deltaScatteringDensityTexture;
	s32 depth = input->currentDepth;

	stone::generate_multiple_scattering_texture(*atmosphere, transmittanceTexture, deltaScatteringDensityTexture,
			deltaMultipleScatteringTexture, scatteringTexture, depth);

	return refrain2::Task();
}

}
}
