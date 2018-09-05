#include "OBJModelBaker.h"

#include <cstdio>
#include <iostream>

#include "Memory/MemorySystem.h"

namespace baker
{

const_cstr OBJModelBaker::k_WFComment					= "#";
const_cstr OBJModelBaker::k_WFMaterialLibrary			= "mtllib";
const_cstr OBJModelBaker::k_WFGroup						= "g";
const_cstr OBJModelBaker::k_WFObject					= "o";
const_cstr OBJModelBaker::k_WFUseMaterial				= "usemtl";
const_cstr OBJModelBaker::k_WFVertex					= "v";
const_cstr OBJModelBaker::k_WFVertexTexCoord			= "vt";
const_cstr OBJModelBaker::k_WFVertexNormal				= "vn";
const_cstr OBJModelBaker::k_WFFace						= "f";

OBJModelBaker::OBJModelBaker(const_cstr filePath, const_cstr outDir)
	: m_FilePath(filePath)
	, m_OutDir(outDir)
	, m_TargetFilePath("")
	, m_CurrSegStartId(0)
	, m_NextSegStartId(0)
{
	m_BakingArena = g_PersistanceAllocator.allocate_arena<FreelistArena>(SIZE_MB(256));
}

OBJModelBaker::~OBJModelBaker()
{
	g_PersistanceAllocator.free(m_BakingArena);
	m_BakingArena = nullptr;
}

// parser
#define		REMOVE_LINE_ENDING(STR)	\
		u32 __idx = 0; \
		while (STR[__idx] != 0 && STR[__idx] != '\r' && STR[__idx] != '\n') __idx++; \
		STR[__idx] = 0;
const bool OBJModelBaker::ParseVerticesCount(const_cstr line, const u32 index, u32& uvc)
{
	u32 currIndex = index;
	while (line[currIndex] == ' ') currIndex++;
	sscanf_s(&line[currIndex], "%d", &uvc);
	return true;
}

const bool OBJModelBaker::ParseNormalsCount(const_cstr line, const u32 index, u32& unc)
{
	u32 currIndex = index;
	while (line[currIndex] == ' ') currIndex++;
	sscanf_s(&line[currIndex], "%d", &unc);
	return true;
}

const bool OBJModelBaker::ParseTexCoordsCount(const_cstr line, const u32 index, u32& utc)
{
	u32 currIndex = index;
	while (line[currIndex] == ' ') currIndex++;
	sscanf_s(&line[currIndex], "%d", &utc);
	return true;
}

const bool OBJModelBaker::ParseComment(const_cstr line, const u32 index, cstr outBuffer)
{
	u32 currIndex = index;
	while (line[currIndex] == ' ') currIndex++;
	strcpy_s(outBuffer, 256, &line[currIndex]);
	REMOVE_LINE_ENDING(outBuffer);
	return true;
}

const bool OBJModelBaker::ParseMaterialLibrary(const_cstr line, const u32 index, cstr matName)
{
	u32 currIndex = index;
	while (line[currIndex] == ' ') currIndex++;
	strcpy_s(matName, 256, &line[currIndex]);
	REMOVE_LINE_ENDING(matName);
	return true;
}

const bool OBJModelBaker::ParseMaterialUsage(const_cstr line, const u32 index, cstr matName)
{
	u32 currIndex = index;
	while (line[currIndex] == ' ') currIndex++;
	c8 buff[128];
	sprintf_s(buff, "%s", &line[currIndex]);
	strcpy_s(matName, 256, buff);
	REMOVE_LINE_ENDING(matName);
	return true;
}

const bool OBJModelBaker::ParseGroup(const_cstr line, const u32 index, cstr groupName)
{
	u32 currIndex = index;
	while (line[currIndex] == ' ') currIndex++;
	if (line[currIndex] != '\0') {
		strcpy_s(groupName, 256, &line[currIndex]);
	}
	else {
		return false;
	}
	REMOVE_LINE_ENDING(groupName);
	return true;
}

const bool OBJModelBaker::ParseVertex(const_cstr line, const u32 index, f32& x, f32& y, f32& z)
{
	u32 currIndex = index;
	while (line[currIndex] == ' ') currIndex++;
	f32 tmpX = 0.0f, tmpY = 0.0f, tmpZ = 0.0f;
	u32 readValues = sscanf_s(&line[currIndex], "%f %f %f", &tmpX, &tmpY, &tmpZ);
	if (readValues != 3) {
		return false;
	}
	else {
		x = tmpX; y = tmpY; z = tmpZ;
		return true;
	}
}

const bool OBJModelBaker::ParseVertexNormal(const_cstr line, const u32 index, f32& x, f32& y, f32& z)
{
	u32 currIndex = index;
	while (line[currIndex] == ' ') currIndex++;
	u32 readValues = sscanf_s(&line[currIndex], "%f %f %f", &x, &y, &z);
	if (readValues != 3)
		return false;
	else return true;
}

const bool OBJModelBaker::ParseVertexTexCoord(const_cstr line, const u32 index, f32& u, f32& v)
{
	u32 currIndex = index;
	while (line[currIndex] == ' ') currIndex++;
	u32 readValues = sscanf_s(&line[currIndex], "%f %f", &u, &v);
	if (readValues != 2)
		return false;
	else return true;
}

const u32 OBJModelBaker::ParseFace(cstr line, const u32 index, 
		DynamicArray<Vertex>* compiledVertices, 
		DynamicArray<u32>* compiledIndices,
		DynamicArray<vec3f>* vertices,
		DynamicArray<vec3f>* normals,
		DynamicArray<vec2f>* texCoords)
{
	u32 primitiveFormat = 0;
	u32 currIndex = index;
	while (line[currIndex] == ' ') currIndex++;
	// IMPORTANT (19-07-2017): you may wonder what I am doing next.
	// but keep in mind that, if I didn't do that, there will be a crash bug
	// in SwapBuffers(), yes, totally unrelated, I don't know why.
	// Maybe a bug in libc?
	u32 dummy1, dummy2, dummy3;
	u32 fType = sscanf_s(&line[currIndex], "%d/%d/%d", &dummy1, &dummy2, &dummy3);
	// original code (which caused crash):
	// u32 fType = sscanf_s(&line[currIndex], "%d/%d/%d");

	u32 vCount = 0;
	const_cstr lp = line;
	for (vCount = 0; lp[vCount]; lp[vCount] == '/' ? vCount++ : *lp++);
	vCount = vCount / (fType == 2 ? 1 : 2);
	// we have format now
	if (fType == 2)					// v1/t1 v2/t2 ... vn/tn
		SET_BIT(primitiveFormat, WF_OBJ_VERTEX_HAS_TEX_COORD);
	else if (fType == 1)			// v1//n1 v2//n2 ... vn//nn
		SET_BIT(primitiveFormat, WF_OBJ_VERTEX_HAS_NORMAL);
	else {							// v1/t1/n1 v2/t2/n2 ... vn/tn/nn
		SET_BIT(primitiveFormat, WF_OBJ_VERTEX_HAS_TEX_COORD);
		SET_BIT(primitiveFormat, WF_OBJ_VERTEX_HAS_NORMAL);
	}
	if (vCount == 3)
		SET_BIT(primitiveFormat, WF_OBJ_FACE_TYPE);
	else
		CLEAR_BIT(primitiveFormat, WF_OBJ_FACE_TYPE);

	s32 pv = 0, pt = 0, pn = 0;
	switch (fType) {
		case 1: {
					Vertex tmpV;
					c8* pEnd = &line[currIndex - 1];
					for (u32 i = 0; i < vCount; i++) {
						s32 v = 0, t = 0, n = 0;
						v = strtol(pEnd + 1, &pEnd, 10);
						n = strtol(pEnd + 2, &pEnd, 10);
						//sscanf_s(&line[currIndex], "%d/%d/%d", &v, &t, &n);
						v = v < 0 ? vertices->get_size() + v : v - 1;
						n = n < 0 ? normals->get_size() + n : n - 1;
						tmpV.Position = vertices->at(v);
						tmpV.Normal = normals->at(n);
						tmpV.TexCoord = floral::vec2f(0.0f);
						// indexing
						u32 idV = compiledVertices->find(tmpV, m_CurrSegStartId);
						if (idV != compiledVertices->get_terminated_index()) {
							compiledIndices->PushBack(idV);
						}
						else {
							compiledVertices->PushBack(tmpV);
							compiledIndices->PushBack(compiledVertices->get_size() - 1);
						}
					}
					break;
				}
		case 2: {
					Vertex tmpV[4];
					u32 idV[4];
					c8* pEnd = &line[currIndex - 1];
					for (u32 i = 0; i < vCount; i++) {
						s32 v, t;
						v = strtol(pEnd + 1, &pEnd, 10);
						t = strtol(pEnd + 1, &pEnd, 10);
						v = v < 0 ? vertices->get_size() + v : v - 1;
						t = t < 0 ? texCoords->get_size() + t : t - 1;

						tmpV[i].Position = vertices->at(v);
						tmpV[i].TexCoord = texCoords->at(t);
					}
					// calculate normals, ccw
					//vec3f faceNormalTmp = tmpV[2].Position - tmpV[1].Position;
					vec3f faceNormal = vec3f(0.0f, 0.0f, 0.0f); //faceNormalTmp.cross(tmpV[0].Position - tmpV[1].Position);
					for (u32 i = 0; i < vCount; i++) tmpV[i].Normal = faceNormal;

					// push 3 vertices as normal
					for (u32 i = 0; i < 3; i++) {
						idV[i] = compiledVertices->find(tmpV[i], m_CurrSegStartId);
						if (idV[i] != compiledVertices->get_terminated_index()) {
							compiledIndices->PushBack(idV[i]);
						}
						else {
							compiledVertices->PushBack(tmpV[i]);
							idV[i] = compiledVertices->get_size() - 1;
							compiledIndices->PushBack(idV[i]);
						}
					}

					if (vCount == 4) {
						idV[3] = compiledVertices->find(tmpV[3], m_CurrSegStartId);
						if (idV[3] != compiledVertices->get_terminated_index()) {
						}
						else {
							compiledVertices->PushBack(tmpV[3]);
							idV[3] = compiledVertices->get_size() - 1;
						}
						compiledIndices->PushBack(idV[2]);
						compiledIndices->PushBack(idV[3]);
						compiledIndices->PushBack(idV[0]);
					}
					break;
				}
		case 3: {
					Vertex tmpV;
					c8* pEnd = &line[currIndex - 1];
					for (u32 i = 0; i < vCount; i++) {
						s32 v = 0, t = 0, n = 0;
						v = strtol(pEnd + 1, &pEnd, 10);
						t = strtol(pEnd + 1, &pEnd, 10);
						n = strtol(pEnd + 1, &pEnd, 10);
						//sscanf_s(&line[currIndex], "%d/%d/%d", &v, &t, &n);
						v = v < 0 ? vertices->get_size() + v : v - 1;
						t = t < 0 ? texCoords->get_size() + t : t - 1;
						n = n < 0 ? normals->get_size() + n : n - 1;
						tmpV.Position = vertices->at(v);
						tmpV.Normal = normals->at(n);
						tmpV.TexCoord = texCoords->at(t);
						// indexing
						u32 idV = compiledVertices->find(tmpV, m_CurrSegStartId);
						if (idV != compiledVertices->get_terminated_index()) {
							compiledIndices->PushBack(idV);
						}
						else {
							compiledVertices->PushBack(tmpV);
							compiledIndices->PushBack(compiledVertices->get_size() - 1);
						}
					}
					break;
				}
	}

	return primitiveFormat;
}
#undef		REMOVE_LINE_ENDING

#define		CHECK_TOKEN(X, Y)	(strncmp(&X, Y, strlen(Y)) == 0)
void OBJModelBaker::BakeToFile()
{
	path tmpPath(m_FilePath);
	memset(m_TargetFilePath, 0, sizeof(m_TargetFilePath));
	strcat(m_TargetFilePath, m_OutDir);
	strcat(m_TargetFilePath, "/");
	strcat(m_TargetFilePath, tmpPath.pm_FileNameNoExt);
	strcat(m_TargetFilePath, ".cbobj");
	m_CbObjFile.open(m_TargetFilePath, std::ofstream::out | std::ofstream::binary);
	// header: file format
	c8* magicCharacters = "CBFM";
	m_CbObjFile.write(magicCharacters, 4 * sizeof(c8));

	// header: total primitive nodes
	u32 totalPrimNodes = 0;
	u32 totalPrimNodesPos = m_CbObjFile.tellp();
	m_CbObjFile.write((c8*)&totalPrimNodes, sizeof(u32));

	u32 totalMeshNodes = 0;
	u32 totalMeshNodePos = m_CbObjFile.tellp();
	m_CbObjFile.write((c8*)&totalMeshNodes, sizeof(u32));

	// header: root geometry node
	const_cstr fileName = tmpPath.pm_FileNameNoExt;
	u32 lenRootGeoName = strlen(fileName);
	m_CbObjFile.write((c8*)&lenRootGeoName, sizeof(u32));
	m_CbObjFile.write(fileName, lenRootGeoName * sizeof(c8));
	totalPrimNodes++;							// increase total prim nodes
	std::cout << "prim #" << totalPrimNodes << ": (root) " << fileName << std::endl;

	DynamicArray<vec3f>* vertices = m_BakingArena->allocate<DynamicArray<vec3f>>(256, m_BakingArena);
	DynamicArray<vec3f>* normals = m_BakingArena->allocate<DynamicArray<vec3f>>(256, m_BakingArena);
	DynamicArray<vec2f>* texCoords = m_BakingArena->allocate<DynamicArray<vec2f>>(256, m_BakingArena);

	DynamicArray<Vertex>* compiledVertices = m_BakingArena->allocate<DynamicArray<Vertex>>(256, m_BakingArena);
	DynamicArray<u32>* compiledIndices = m_BakingArena->allocate<DynamicArray<u32>>(256, m_BakingArena);
	m_MeshOffsetsToData = m_BakingArena->allocate<DynamicArray<u32>>(256, m_BakingArena);

	FILE* objFile = fopen(m_FilePath, "rb");
	c8 line[256];
	std::string filePath(m_FilePath);
	std::string fileDir = filePath.substr(0, filePath.find_last_of('/') + 1);

	// header: root node's children
	u32 childrenNum = 0;
	u32 eldestChildrenNum = 0;
	u32 groupChildrenNumPos = 0;

	u32 eldestChildrenNumPos = m_CbObjFile.tellp();
	m_CbObjFile.write((c8*)&eldestChildrenNum, sizeof(u32));

	while (!feof(objFile)) {
		memset(line, 0, sizeof(line));
		fgets(line, sizeof(line), objFile);
		u32 index = 0;
		while (line[index] == ' ') index++;
		if (CHECK_TOKEN(line[index], k_WFGroup) || CHECK_TOKEN(line[index], k_WFObject)) {
			c8 groupName[256];
			ParseGroup(line, index + strlen(k_WFGroup), groupName);
			// group name
			u32 groupStrLen = strlen(groupName);
			m_CbObjFile.write((c8*)&groupStrLen, sizeof(u32));
			m_CbObjFile.write(groupName, groupStrLen * sizeof(c8));
			totalPrimNodes++;							// increase total prim nodes
			std::cout << "prim #" << totalPrimNodes << ": (group) " << groupName << std::endl;
			strcpy(m_CurrentGroup, groupName);
			m_CurrentMeshIdx = 0;
			// group geometry node offset to data, always 0xFFFFFFFF
			eldestChildrenNum++;
			u32 offsetToData = 0xFFFFFFFF;
			m_CbObjFile.write((c8*)&offsetToData, sizeof(u32));
			// update previous group geometry node's children count
			if (groupChildrenNumPos != 0) {
				u32 currPos = m_CbObjFile.tellp();
				m_CbObjFile.seekp(groupChildrenNumPos, std::ios_base::beg);
				m_CbObjFile.write((c8*)&childrenNum, sizeof(u32));
				m_CbObjFile.seekp(currPos, std::ios_base::beg);
			}
			// this node's children count: dummy zero
			childrenNum = 0;
			groupChildrenNumPos = m_CbObjFile.tellp();
			m_CbObjFile.write((c8*)&childrenNum, sizeof(u32));
		}
		else if (CHECK_TOKEN(line[index], k_WFUseMaterial)) {
			c8 useMtl[256];
			ParseMaterialUsage(line, index + strlen(k_WFUseMaterial), useMtl);
			// mesh node's offset to data
			u32 offsetToData = 0;
			m_MeshOffsetsToData->PushBack(m_CbObjFile.tellp());
			m_CbObjFile.write((c8*)&offsetToData, sizeof(u32));
			totalPrimNodes++;							// increase total prim nodes
			totalMeshNodes++;
			std::cout << "prim #" << totalPrimNodes << ": (mesh " << m_CurrentMeshIdx << " material) " << useMtl << std::endl;
			m_CurrentMeshIdx++;
			// mesh node's children count, always zero
			u32 meshNodeChildrenCount = 0;
			m_CbObjFile.write((c8*)&meshNodeChildrenCount, sizeof(u32));
			childrenNum++;
		}
	}

	// last group geometry node's info
	u32 currPos = m_CbObjFile.tellp();
	m_CbObjFile.seekp(groupChildrenNumPos, std::ios_base::beg);
	m_CbObjFile.write((c8*)&childrenNum, sizeof(u32));
	m_CbObjFile.seekp(eldestChildrenNumPos, std::ios_base::beg);
	m_CbObjFile.write((c8*)&eldestChildrenNum, sizeof(u32));
	// update header: total prim nodes
	std::cout << "-- total primitive nodes: " << totalPrimNodes << std::endl;
	std::cout << "-- total mesh nodes: " << totalMeshNodes << std::endl;
	m_CbObjFile.seekp(totalPrimNodesPos, std::ios_base::beg);
	m_CbObjFile.write((c8*)&totalPrimNodes, sizeof(u32));
	m_CbObjFile.seekp(totalMeshNodePos, std::ios_base::beg);
	m_CbObjFile.write((c8*)&totalMeshNodes, sizeof(u32));
	// go back to continue writing
	m_CbObjFile.seekp(currPos, std::ios_base::beg);


	fseek(objFile, 0, SEEK_SET);
	u32 meshCount = 0;
	bool faceReading = false;
	while (!feof(objFile)) {
		memset(line, 0, sizeof(line));
		fgets(line, sizeof(line), objFile);
		u32 index = 0;
		while (line[index] == ' ') index++;
		if (CHECK_TOKEN(line[index], k_WFComment)) {
			if (faceReading) {
				faceReading = false;
				DoBakeMesh(compiledVertices, compiledIndices);
			}
			c8 cmtBuff[256];
			ParseComment(line, index + strlen(k_WFComment), cmtBuff);
			//CYMBI_LOG_VERBOSE("OBJ: {0}", cmtBuff);
		}
		else if (CHECK_TOKEN(line[index], k_WFMaterialLibrary)) {
			if (faceReading) {
				faceReading = false;
				DoBakeMesh(compiledVertices, compiledIndices);
			}
			memset(m_MtlLibPath, 0, 256);
			ParseMaterialLibrary(line, index + strlen(k_WFMaterialLibrary), m_MtlLibPath);
			//sprintf_s(m_MtlLibPath, "%s%s", fileDir.c_str(), mtlLibName);
			//CYMBI_LOG_VERBOSE("OBJ: mtllib = {0} @ '{1}'", mtlLibName, m_MtlLibPath);
		}
		else if (CHECK_TOKEN(line[index], k_WFGroup) || CHECK_TOKEN(line[index], k_WFObject)) {
			if (faceReading) {
				faceReading = false;
				DoBakeMesh(compiledVertices, compiledIndices);
			}
			c8 groupName[256];
			ParseGroup(line, index + strlen(k_WFGroup), groupName);
			strcpy(m_CurrentGroup, groupName);
			m_CurrentMeshIdx = 0;
			printf("baking group '%s':\n", m_CurrentGroup);
			m_CurrSegStartId = m_NextSegStartId;
			m_NextSegStartId = compiledVertices->get_size();
			//CYMBI_LOG_VERBOSE("OBJ: indices building range[{0} - {1}]", m_CurrSegStartId, m_NetxSegStartId);
			//CYMBI_LOG_VERBOSE("OBJ: group = {0}", groupName);
			//DoConstructGroup(groupName);
		}
		else if (CHECK_TOKEN(line[index], k_WFUseMaterial)) {
			if (faceReading) {
				faceReading = false;
				DoBakeMesh(compiledVertices, compiledIndices);
			}
			memset(m_CurrentMtl, 0, 256);
			ParseMaterialUsage(line, index + strlen(k_WFUseMaterial), m_CurrentMtl);
			//CYMBI_LOG_VERBOSE("OBJ: usemtl = {0}", m_CurrentMtl);

			u32 currPos = m_CbObjFile.tellp();
			m_CbObjFile.seekp(m_MeshOffsetsToData->at(meshCount), std::ios_base::beg);
			m_CbObjFile.write((c8*)&currPos, sizeof(u32));
			m_CbObjFile.seekp(currPos, std::ios_base::beg);

			u32 lenStr = strlen(m_CurrentMtl);
			m_CbObjFile.write((c8*)&lenStr, sizeof(u32));
			m_CbObjFile.write(m_CurrentMtl, lenStr * sizeof(c8));

			// new mesh
			meshCount++;
			compiledVertices->Empty();
			compiledIndices->Empty();
			faceReading = true;
		}
		else if (CHECK_TOKEN(line[index], k_WFVertexNormal)) {
			if (faceReading) {
				faceReading = false;
				DoBakeMesh(compiledVertices, compiledIndices);
			}
			vec3f normal(0, 0, 0);
			ParseVertexNormal(line, index + strlen(k_WFVertexNormal), normal.x, normal.y, normal.z);
			normals->PushBack(normal);
		}
		else if (CHECK_TOKEN(line[index], k_WFVertexTexCoord)) {
			if (faceReading) {
				faceReading = false;
				DoBakeMesh(compiledVertices, compiledIndices);
			}
			vec2f texCoord(0, 0);
			ParseVertexTexCoord(line, index + strlen(k_WFVertexTexCoord), texCoord.x, texCoord.y);
			// invert v-component of texture coordinate: I don't really know why I have to do this
			// but if I don't, the mapping is done wrong.
			// Perhaps it's related to something of Direct3D and OpenGL mapping with WaveFront Object Format
			//texCoord.y = -texCoord.y;
			texCoords->PushBack(texCoord);
		}
		else if (CHECK_TOKEN(line[index], k_WFVertex)) {
			if (faceReading) {
				faceReading = false;
				DoBakeMesh(compiledVertices, compiledIndices);
			}
			vec3f vertex(0, 0, 0);
			ParseVertex(line, index + strlen(k_WFVertex), vertex.x, vertex.y, vertex.z);
			vertices->PushBack(vertex);
		}
		else if (CHECK_TOKEN(line[index], k_WFFace)) {
			Vertex v;
			ParseFace(line, index + strlen(k_WFFace), compiledVertices, compiledIndices, vertices, normals, texCoords);
		}
	}
	if (faceReading)
		DoBakeMesh(compiledVertices, compiledIndices);

	fclose(objFile);
	m_CbObjFile.close();
	m_BakingArena->free_all();
}

#undef		CHECK_TOKEN

void OBJModelBaker::DoBakeMesh(DynamicArray<Vertex>* compiledVertices, DynamicArray<u32>* compiledIndices)
{
	/*
	// adjust normals for smooth surfaces
	vec3f accNorm(0.0f, 0.0f, 0.0f);
	for (u32 i = 0; i < compiledVertices->get_size(); i++) {

		// find adj triangles
		for (u32 j = 0; j < compiledIndices->get_size(); j+= 3) {
			for (u32 k = 0; k < 3; k++) {
				if (compiledIndices->at(j + k) == i) {
					// accumulate normals
					u32 v0 = compiledIndices->at(j);
					u32 v1 = compiledIndices->at(j + 1);
					u32 v2 = compiledIndices->at(j + 2);
					vec3f tmp = compiledVertices->at(v2).Position - compiledVertices->at(v1).Position;
					vec3f fNorm = tmp.cross(compiledVertices->at(v0).Position - compiledVertices->at(v1).Position);
					accNorm += fNorm;
				}
			}
		}

		// update normal
		(*compiledVertices)[i].Normal = accNorm.normalize();
		accNorm = vec3f(0.0f, 0.0f, 0.0f);
	}
	*/
	printf("baked mesh %d with material '%-40s': (%-6d vertices + %-6d indices)\n", m_CurrentMeshIdx, m_CurrentMtl, compiledVertices->get_size(), compiledIndices->get_size());
	u32 vertexFormat = 0;
	m_CbObjFile.write((c8*)&vertexFormat, sizeof(u32));
	u32 verticesLen = compiledVertices->get_size();
	u32 indicesLen = compiledIndices->get_size();
	m_CbObjFile.write((c8*)&verticesLen, sizeof(u32));
	m_CbObjFile.write((c8*)&(*compiledVertices)[0], verticesLen * sizeof(Vertex));
	m_CbObjFile.write((c8*)&indicesLen, sizeof(u32));
	m_CbObjFile.write((c8*)&(*compiledIndices)[0], indicesLen * sizeof(u32));
	m_CurrentMeshIdx++;
}

}
