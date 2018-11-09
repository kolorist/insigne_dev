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

}
