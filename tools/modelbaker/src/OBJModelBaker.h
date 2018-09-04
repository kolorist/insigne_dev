#pragma once

#include "commons.h"

#include <fstream>

using namespace floral;

namespace baker
{

class OBJModelBaker {
	private:
		static const_cstr						k_WFComment;
		static const_cstr						k_WFMaterialLibrary;
		static const_cstr						k_WFGroup;
		static const_cstr						k_WFObject;
		static const_cstr						k_WFUseMaterial;
		static const_cstr						k_WFVertex;
		static const_cstr						k_WFVertexTexCoord;
		static const_cstr						k_WFVertexNormal;
		static const_cstr						k_WFFace;
		static const_cstr						k_MWFVerticesCount;
		static const_cstr						k_MWFTexCoordsCount;
		static const_cstr						k_MWFNormalsCount;

	public:
		enum {
			WF_OBJ_FACE_TYPE			= 1,
			WF_OBJ_VERTEX_HAS_TEX_COORD = 1 << 1,
			WF_OBJ_VERTEX_HAS_NORMAL	= 1 << 2
		};

		enum {
			WF_OBJ_FACE_TRIANGLE		= 0,
			WF_OBJ_FACE_QUAD			= 1
		};

	public:
		OBJModelBaker(const_cstr filePath, const_cstr outDir);
		~OBJModelBaker();

		void									BakeToFile();

	private:
		// "#"
		const bool								ParseComment(const_cstr line, const u32 index, cstr outBuffer);
		// "mtllib"
		const bool								ParseMaterialLibrary(const_cstr line, const u32 index, cstr matName);
		// "usemtl"
		const bool								ParseMaterialUsage(const_cstr line, const u32 index, cstr matUses);
		// "g"
		const bool								ParseGroup(const_cstr line, const u32 index, cstr groupName);
		// "v"
		const bool								ParseVertex(const_cstr line, const u32 index, f32& x, f32& y, f32& z);
		// "vt"
		const bool								ParseVertexTexCoord(const_cstr line, const u32 index, f32& u, f32& v);
		// "vn"
		const bool								ParseVertexNormal(const_cstr line, const u32 index, f32& x, f32& y, f32& z);
		// "f"
		const u32								ParseFace(cstr line, const u32 index,
													DynamicArray<Vertex>* compiledVertices, DynamicArray<u32>* compiledIndices,
													DynamicArray<vec3f>* vertices, DynamicArray<vec3f>* normals, 
													DynamicArray<vec2f>* texCoords);
		// "uvc"
		const bool								ParseVerticesCount(const_cstr line, const u32 index, u32& uvc);
		// "unc"
		const bool								ParseNormalsCount(const_cstr line, const u32 index, u32& unc);
		// "utc"
		const bool								ParseTexCoordsCount(const_cstr line, const u32 index, u32& utc);

	private:
		void									DoBakeMesh(DynamicArray<Vertex>* compiledVertices, DynamicArray<u32>* compiledIndices);

	private:
		std::ofstream							m_CbObjFile;
		const_cstr								m_FilePath;
		const_cstr								m_OutDir;
		c8										m_TargetFilePath[1024];
		u32										m_CurrSegStartId;
		u32										m_NextSegStartId;
		c8										m_MtlLibPath[256];
		c8										m_CurrentMtl[256];
		DynamicArray<u32>*						m_MeshOffsetsToData;

	private:
		FreelistArena*							m_BakingArena;
};

}
