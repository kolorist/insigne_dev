#include "GeometryBuilder.h"

namespace stone {

static bool CompareVertexPosition(const floral::vec3f& i_a, const floral::vec3f& i_b)
{
	return (floral::length(i_a - i_b) <= 0.001f);
}

static bool CompareVertexNormal(const floral::vec3f& i_a, const floral::vec3f& i_b)
{
	floral::vec3f c = floral::cross(i_a, i_b);
	f32 d = floral::dot(i_a, i_b);
	return (floral::length(c) <= 0.001f && d >= 0.0f);
}

static bool CompareVertex(const VertexPN& i_a, const VertexPN& i_b)
{
	return (CompareVertexPosition(i_a.Position, i_b.Position) && CompareVertexNormal(i_a.Normal, i_b.Normal));
}

// ---------------------------------------------
void GenTessellated3DPlane_Tris(const floral::mat4x4f& i_xform, const f32 i_baseSize, const u32 i_gridsCount,
		TemporalVertices* o_vertices, TemporalIndices* o_indices)
{
	const f32 minX = -i_baseSize;
	const f32 maxX = i_baseSize;
	const f32 minZ = -i_baseSize;
	const f32 maxZ = i_baseSize;

	const f32 gridStep = i_baseSize * 2.0f / i_gridsCount;

	static u32 indices[] = { 0, 1, 2, 2, 3, 0 };

	for (u32 i = 0; i < i_gridsCount; i++) {
		for (u32 j = 0; j < i_gridsCount; j++) {
			VertexPN v[4];
			v[0].Position = floral::vec3f(minX + i * gridStep, 0.0f, minZ + j * gridStep);
			v[0].Normal = floral::vec3f(0.0f, 1.0f, 0.0f);
			v[1].Position = floral::vec3f(minX + i * gridStep, 0.0f, minZ + (j + 1) * gridStep);
			v[1].Normal = floral::vec3f(0.0f, 1.0f, 0.0f);
			v[2].Position = floral::vec3f(minX + (i + 1) * gridStep, 0.0f, minZ + (j + 1) * gridStep);
			v[2].Normal = floral::vec3f(0.0f, 1.0f, 0.0f);
			v[3].Position = floral::vec3f(minX + (i + 1) * gridStep, 0.0f, minZ + j * gridStep);
			v[3].Normal = floral::vec3f(0.0f, 1.0f, 0.0f);

			for (u32 k = 0; k < 6; k++) {
				u32 index = o_vertices->find(v[indices[k]], &CompareVertex);
				if (index == o_vertices->get_terminated_index()) {
					o_vertices->push_back(v[indices[k]]);
					o_indices->push_back(o_vertices->get_size() - 1);
				} else {
					o_indices->push_back(index);
				}
			}
		}
	}
}

void GenQuadTesselated3DPlane_Tris(const f32 i_width, const f32 i_height,
		const f32 i_gridSize,
		TemporalVertices* o_vertices, TemporalIndices* o_indices,
		TemporalQuads* o_quadsList, const bool i_vtxDup /* = false */)
{
	const f32 minX = -i_width / 2.0f;
	const f32 maxX = i_width / 2.0f;
	const f32 minZ = -i_height / 2.0f;
	const f32 maxZ = i_height / 2.0f;

	const f32 gridStep = i_gridSize;
	const u32 gridsCountX = (u32)(i_width / gridStep) + 1;
	const u32 gridsCountZ = (u32)(i_height / gridStep) + 1;

	static u32 indices[] = { 0, 1, 2, 2, 3, 0 };

	for (u32 i = 0; i < gridsCountX; i++) {
		for (u32 j = 0; j < gridsCountZ; j++) {
			VertexPN v[4];
			f32 minGridX = minX + i * gridStep;
			f32 maxGridX = (minGridX + gridStep) > maxX ? maxX : (minGridX + gridStep);
			f32 minGridZ = minZ + j * gridStep;
			f32 maxGridZ = (minGridZ + gridStep) > maxZ ? maxZ : (minGridZ + gridStep);

			v[0].Position = floral::vec3f(minGridX, 0.0f, minGridZ);
			v[0].Normal = floral::vec3f(0.0f, 1.0f, 0.0f);
			v[1].Position = floral::vec3f(minGridX, 0.0f, maxGridZ);
			v[1].Normal = floral::vec3f(0.0f, 1.0f, 0.0f);
			v[2].Position = floral::vec3f(maxGridX, 0.0f, maxGridZ);
			v[2].Normal = floral::vec3f(0.0f, 1.0f, 0.0f);
			v[3].Position = floral::vec3f(maxGridX, 0.0f, minGridZ);
			v[3].Normal = floral::vec3f(0.0f, 1.0f, 0.0f);

			o_quadsList->push_back(v[0].Position);
			o_quadsList->push_back(v[1].Position);
			o_quadsList->push_back(v[2].Position);
			o_quadsList->push_back(v[3].Position);

			if (!i_vtxDup) {
				for (u32 k = 0; k < 6; k++) {
					u32 index = o_vertices->find(v[indices[k]], &CompareVertex);
					if (index == o_vertices->get_terminated_index()) {
						o_vertices->push_back(v[indices[k]]);
						o_indices->push_back(o_vertices->get_size() - 1);
					} else {
						o_indices->push_back(index);
					}
				}
			} else {
				u32 lastIdx = o_vertices->get_size();
				o_vertices->push_back(v[0]);
				o_vertices->push_back(v[1]);
				o_vertices->push_back(v[2]);
				o_vertices->push_back(v[3]);
				for (u32 k = 0; k < 6; k++) {
					o_indices->push_back(lastIdx + indices[k]);
				}
			}
		}
	}
}

void GenTessellated3DPlane_TrisStrip(const floral::mat4x4f& i_xform, const f32 i_baseSize, const u32 i_gridsCount,
		TemporalVertices* o_vertices, TemporalIndices* o_indices)
{
	const f32 minX = -i_baseSize;
	const f32 maxX = i_baseSize;
	const f32 minZ = -i_baseSize;
	const f32 maxZ = i_baseSize;

	const f32 gridStep = i_baseSize * 2.0f / i_gridsCount;

	static u32 indices[] = { 0, 1, 3, 2 };

	for (u32 i = 0; i < i_gridsCount; i++) {
		for (u32 j = 0; j < i_gridsCount; j++) {
			VertexPN v[4];
			v[0].Position = floral::vec3f(minX + i * gridStep, 0.0f, minZ + j * gridStep);
			v[0].Normal = floral::vec3f(0.0f, 1.0f, 0.0f);
			v[1].Position = floral::vec3f(minX + i * gridStep, 0.0f, minZ + (j + 1) * gridStep);
			v[1].Normal = floral::vec3f(0.0f, 1.0f, 0.0f);
			v[2].Position = floral::vec3f(minX + (i + 1) * gridStep, 0.0f, minZ + (j + 1) * gridStep);
			v[2].Normal = floral::vec3f(0.0f, 1.0f, 0.0f);
			v[3].Position = floral::vec3f(minX + (i + 1) * gridStep, 0.0f, minZ + j * gridStep);
			v[3].Normal = floral::vec3f(0.0f, 1.0f, 0.0f);

			for (u32 k = 0; k < 4; k++) {
				u32 index = o_vertices->find(v[indices[k]], &CompareVertex);
				if (index == o_vertices->get_terminated_index()) {
					o_vertices->push_back(v[indices[k]]);
					o_indices->push_back(o_vertices->get_size() - 1);
				} else {
					o_indices->push_back(index);
				}
			}
		}
	}
}

// ---------------------------------------------
void GenBox_Tris(const floral::mat4x4f& i_xform,
		TemporalVertices* o_vertices, TemporalIndices* o_indices)
{
}

void GenBox_TrisStrip(const floral::mat4x4f& i_xform,
		TemporalVertices* o_vertices, TemporalIndices* o_indices)
{
	static floral::vec3f positions[] = {
		floral::vec3f(-1.0f, -1.0f, 1.0f),
		floral::vec3f(1.0f, -1.0f, 1.0f),
		floral::vec3f(1.0f, -1.0f, -1.0f),
		floral::vec3f(-1.0f, -1.0f, -1.0f),
		floral::vec3f(-1.0f, 1.0f, 1.0f),
		floral::vec3f(1.0f, 1.0f, 1.0f),
		floral::vec3f(1.0f, 1.0f, -1.0f),
		floral::vec3f(-1.0f, 1.0f, -1.0f)
	};

	static u32 indices[] = {
		5, 1, 6, 2,								// positive x
		7, 3, 4, 0,								// negative x
		7, 4, 6, 5,								// positive y
		2, 1, 3, 0,								// negative y
		4, 0, 1, 5,								// positive z
		6, 2, 7, 3								// negative z
	};

	static floral::vec3f normals[] = {
		floral::vec3f(1.0f, 0.0f, 0.0f),
		floral::vec3f(-1.0f, 0.0f, 0.0f),
		floral::vec3f(0.0f, 1.0f, 0.0f),
		floral::vec3f(0.0f, -1.0f, 0.0f),
		floral::vec3f(0.0f, 0.0f, 1.0f),
		floral::vec3f(0.0f, 0.0f, -1.0f)
	};

	for (u32 i = 2; i < 4; i++) {
		VertexPN v;
		for (u32 j = 0; j < 4; j++) {
			v.Position = positions[indices[i * 4 + j]];
			v.Normal = normals[i];
			o_vertices->push_back(v);
			o_indices->push_back(o_vertices->get_size() - 1);
		}
	}
}

// ---------------------------------------------
static void TesselateIcosphere(
		floral::inplace_array<floral::vec3f, 1024u>& i_fromVtx,
		floral::inplace_array<u32, 4096u>& i_fromIdx,
		floral::inplace_array<floral::vec3f, 1024u>& o_toVtx,
		floral::inplace_array<u32, 4096u>& o_toIdx)
{
	o_toVtx.empty();
	o_toIdx.empty();

	static const u32 tmpIdx[12] = {
		0,3,5,	3,1,4,	3,4,5,	5,4,2
	};

	for (u32 i = 0; i < i_fromIdx.get_size() / 3; i++) {
		floral::vec3f v[6];
		v[0] = i_fromVtx[i_fromIdx[i * 3]];
		v[1] = i_fromVtx[i_fromIdx[i * 3 + 1]];
		v[2] = i_fromVtx[i_fromIdx[i * 3 + 2]];

		v[3] = floral::normalize((v[0] + v[1]) / 2.0f);
		v[4] = floral::normalize((v[1] + v[2]) / 2.0f);
		v[5] = floral::normalize((v[2] + v[0]) / 2.0f);

		for (u32 k = 0; k < 12; k++) {
			//s32 index = SearchForVertices(v[tmpIdx[k]], o_toVtx, 0.005f);
			u32 index = o_toVtx.find(v[tmpIdx[k]], &CompareVertexPosition);
			if (index == o_toVtx.get_terminated_index()) {
				o_toVtx.push_back(v[tmpIdx[k]]);
				o_toIdx.push_back(o_toVtx.get_size() - 1);
			} else {
				o_toIdx.push_back(index);
			}
		}
	}
}

void GenIcosphere_Tris(TemporalVertices* o_vertices, TemporalIndices* o_indices)
{
	static const floral::vec3f positions[12] = {
		floral::vec3f(-0.525731f, 0, 0.850651f), floral::vec3f(0.525731f, 0, 0.850651f),
		floral::vec3f(-0.525731f, 0, -0.850651f), floral::vec3f(0.525731f, 0, -0.850651f),
		floral::vec3f(0, 0.850651f, 0.525731f), floral::vec3f(0, 0.850651f, -0.525731f),
		floral::vec3f(0, -0.850651f, 0.525731f), floral::vec3f(0, -0.850651f, -0.525731f),
		floral::vec3f(0.850651f, 0.525731f, 0), floral::vec3f(-0.850651f, 0.525731f, 0),
		floral::vec3f(0.850651f, -0.525731f, 0), floral::vec3f(-0.850651f, -0.525731f, 0)
	};

	static const u32 indices[60] = {
		0,4,1,	0,9,4,	9,5,4,	4,5,8,	4,8,1,
		8,10,1,	8,3,10,	5,3,8,	5,2,3,	2,7,3,
		7,10,3,	7,6,10,	7,11,6,	11,0,6,	0,1,6,
		6,1,10,	9,0,11,	9,11,2,	9,2,5,	7,2,11
	};

	floral::inplace_array<floral::vec3f, 1024u> svBuff0;
	floral::inplace_array<floral::vec3f, 1024u> svBuff1;
	floral::inplace_array<u32, 4096u> siBuff0;
	floral::inplace_array<u32, 4096u> siBuff1;

	for (u32 i = 0; i < 12; i++) svBuff0.push_back(positions[i]);
	for (u32 i = 0; i < 60; i++) siBuff0.push_back(indices[i]);

	TesselateIcosphere(svBuff0, siBuff0, svBuff1, siBuff1);
	TesselateIcosphere(svBuff1, siBuff1, svBuff0, siBuff0);
	TesselateIcosphere(svBuff0, siBuff0, svBuff1, siBuff1);

	for (u32 i = 0; i < svBuff1.get_size(); i++) {
		VertexPN v;
		v.Position = svBuff1[i];
		v.Normal = floral::vec3f(0.0f);
		o_vertices->push_back(v);
	}

	for (u32 i = 0; i < siBuff1.get_size() / 3; i++) {
		o_indices->push_back(siBuff1[i * 3 + 2]);
		o_indices->push_back(siBuff1[i * 3 + 1]);
		o_indices->push_back(siBuff1[i * 3]);
	}
}


}
