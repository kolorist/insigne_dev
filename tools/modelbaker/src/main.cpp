#include <floral.h>
#include <helich.h>
#include <clover.h>
#include <stdio.h>

#include "Memory/MemorySystem.h"
#include "PBRTSceneDefs.h"
#include "CBObjDefinitions.h"

#include "CBObjLoader.h"
#include "PlyLoader.h"

using namespace baker;

extern int yylex_pbrtv3(const char* i_input, const baker::pbrt::SceneCreationCallbacks& i_callbacks);

static baker::S32Array*							s_TemporalIndices;
static baker::Vec3Array*						s_TemporalPositions;
static baker::Vec3Array*						s_TemporalNormals;
static baker::Vec2Array*						s_TemporalUVs;
static FILE*									s_OutputFile;
static s32										s_MeshesCount;
static c8										s_SceneFolder[1024];

void OnNewMesh(const baker::S32Array& i_indices, const baker::Vec3Array& i_positions, const baker::Vec3Array& i_normals, const baker::Vec2Array& i_uvs)
{
	CLOVER_INFO("New Mesh: %d indices | %d positions | %d normals | %d texcoords",
			i_indices.get_size(),
			i_positions.get_size(),
			i_normals.get_size(),
			i_uvs.get_size());
	g_TemporalAllocator.free_all();
	s_TemporalIndices = baker::g_TemporalAllocator.allocate<baker::S32Array>(i_indices.get_size(), &baker::g_TemporalAllocator);
	s_TemporalPositions = baker::g_TemporalAllocator.allocate<baker::Vec3Array>(i_positions.get_size(), &baker::g_TemporalAllocator);
	s_TemporalNormals = baker::g_TemporalAllocator.allocate<baker::Vec3Array>(i_normals.get_size(), &baker::g_TemporalAllocator);
	s_TemporalUVs = baker::g_TemporalAllocator.allocate<baker::Vec2Array>(i_uvs.get_size(), &baker::g_TemporalAllocator);

	*s_TemporalIndices = i_indices;
	*s_TemporalPositions = i_positions;
	*s_TemporalNormals = i_normals;
	*s_TemporalUVs = i_uvs;

	{
		c8 filename[128];
		sprintf(filename, "mesh_%d.cbobj", s_MeshesCount);
		s_OutputFile = fopen(filename, "wb");
		cb::ModelDataHeader header;
		memset(&header, 0, sizeof(cb::ModelDataHeader));

		header.Version = 1u;
		header.LODsCount = 1;

		fwrite(&header, sizeof(cb::ModelDataHeader), 1, s_OutputFile);

		cb::ModelLODInfo lodInfo;
		lodInfo.LODIndex = 0;
		lodInfo.VertexCount = s_TemporalPositions->get_size();
		lodInfo.IndexCount = s_TemporalIndices->get_size();
		lodInfo.OffsetToIndexData = 0;
		lodInfo.OffsetToVertexData = 0;
		fwrite(&lodInfo, sizeof(cb::ModelLODInfo), 1, s_OutputFile);

		// write meshdata
		header.IndexOffset = ftell(s_OutputFile);
		fwrite(&(*s_TemporalIndices)[0], sizeof(s32), s_TemporalIndices->get_size(), s_OutputFile);
		header.PositionOffset = ftell(s_OutputFile);
		fwrite(&(*s_TemporalPositions)[0], sizeof(floral::vec3f), s_TemporalPositions->get_size(), s_OutputFile);
		header.NormalOffsets[0] = ftell(s_OutputFile);
		fwrite(&(*s_TemporalNormals)[0], sizeof(floral::vec3f), s_TemporalNormals->get_size(), s_OutputFile);
		header.TexCoordOffsets[0] = ftell(s_OutputFile);
		fwrite(&(*s_TemporalUVs)[0], sizeof(floral::vec2f), s_TemporalUVs->get_size(), s_OutputFile);

		fseek(s_OutputFile, 0, SEEK_SET);
		fwrite(&header, sizeof(cb::ModelDataHeader), 1, s_OutputFile);

		fclose(s_OutputFile);
	}

	s_MeshesCount++;
}

void OnNewPlyMesh(const_cstr i_plyFileName)
{
	c8 tmpFileName[1024];
	c8 fullFileName[1024];
	c8 cbobjFileName[1024];
	strcpy(tmpFileName, &i_plyFileName[1]);
	size len = strlen(tmpFileName);
	tmpFileName[len - 1] = 0;

	memset(fullFileName, 0, 1024);
	memset(cbobjFileName, 0, 1024);
	sprintf(fullFileName, "%s/%s", s_SceneFolder, tmpFileName);
	sprintf(cbobjFileName, "%s.cbobj", fullFileName);
	CLOVER_INFO("ply file: %s -> %s", fullFileName, cbobjFileName);

	g_TemporalArena.free_all();
	cb::PlyLoader<FreelistArena> loader(&g_TemporalArena);
	loader.LoadFromFile(floral::path(fullFileName));
	loader.ConvertToCBOBJ(cbobjFileName);
}

void OnPushTransform(const floral::mat4x4f& i_xform)
{
	CLOVER_INFO("push transform");
}

void OnPopTransform()
{
	CLOVER_INFO("pop transform");
}

int main(int argc, char** argv)
{
	// we have to call it ourself as we do not have calyx here
	helich::init_memory_system();

	clover::Initialize();
	clover::InitializeVSOutput("vs", clover::LogLevel::Verbose);
	clover::InitializeConsoleOutput("console", clover::LogLevel::Verbose);

	CLOVER_INFO("Model Baker v2");

	if (argc != 2)
		return -1;
	
	strcpy(s_SceneFolder, argv[1]);

	baker::pbrt::SceneCreationCallbacks callbacks;
	callbacks.OnNewMesh.bind<&OnNewMesh>();
	callbacks.OnNewPlyMesh.bind<&OnNewPlyMesh>();
	callbacks.OnPushTransform.bind<&OnPushTransform>();
	callbacks.OnPopTransform.bind<&OnPopTransform>();
	{
		cstr buffer = nullptr;
		{
			c8 scenePath[1024];
			sprintf(scenePath, "%s/scene.pbrt", s_SceneFolder);
			floral::file_info pbrtFile = floral::open_file(scenePath);
			buffer = (cstr)baker::g_PersistanceAllocator.allocate(pbrtFile.file_size + 1);
			floral::read_all_file(pbrtFile, buffer);
			buffer[pbrtFile.file_size] = 0;
			floral::close_file(pbrtFile);
		}

		s_MeshesCount = 0;

		yylex_pbrtv3(buffer, callbacks);
		g_PersistanceAllocator.free(buffer);
	}

	{
		cb::ModelLoader<LinearAllocator> loader(&g_PersistanceAllocator);
		loader.LoadFromFile(floral::path("mesh_5.cbobj"));

		floral::fixed_array<floral::vec3f, LinearAllocator> arr(128, &g_PersistanceAllocator);
		loader.ExtractPositionData(0, arr);
		CLOVER_INFO("test done");
	}

	return 0;
}
