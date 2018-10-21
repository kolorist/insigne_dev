namespace stone {

template <typename TAllocator>
void Gen3DPlane_Tris_PosColor(const floral::vec4f& i_color, const floral::mat4x4f& i_xform,
	floral::fixed_array<DemoVertex, TAllocator>& o_vertices, floral::fixed_array<u32, TAllocator>& o_indices)
{
	static floral::vec3f positions[] = {
		floral::vec3f(1.0f, 0.0f, 1.0f),
		floral::vec3f(1.0f, 0.0f, -1.0f),
		floral::vec3f(-1.0f, 0.0f, -1.0f),
		floral::vec3f(-1.0f, 0.0f, 1.0f)
	};

	static u32 indices[] = { 0, 1, 2, 2, 3, 0 };

	u32 lastIdx = o_vertices.get_size();

	for (u32 i = 0; i < 4; i++) {
		floral::vec4f xformPos = i_xform * floral::vec4f(positions[i].x, positions[i].y, positions[i].z, 1.0f);
		o_vertices.push_back( { floral::vec3f(xformPos.x, xformPos.y, xformPos.z), i_color } );
	}

	for (u32 i = 0; i < 6; i++) {
		o_indices.push_back(lastIdx + indices[i]);
	}
}

template <typename TAllocator>
void Gen3DPlane_Tris_PosNormalColor(const floral::vec4f& i_color, const floral::mat4x4f& i_xform,
	floral::fixed_array<VertexPNC, TAllocator>& o_vertices, floral::fixed_array<u32, TAllocator>& o_indices)
{
	static floral::vec3f positions[] = {
		floral::vec3f(1.0f, 0.0f, 1.0f),
		floral::vec3f(1.0f, 0.0f, -1.0f),
		floral::vec3f(-1.0f, 0.0f, -1.0f),
		floral::vec3f(-1.0f, 0.0f, 1.0f)
	};

	static u32 indices[] = { 0, 1, 2, 2, 3, 0 };

	u32 lastIdx = o_vertices.get_size();
	floral::vec4f xformNormal = i_xform * floral::vec4f(0.0f, 1.0f, 0.0f, 0.0f);
	floral::vec3f finalNormal = floral::normalize(floral::vec3f(xformNormal.x, xformNormal.y, xformNormal.z));

	for (u32 i = 0; i < 4; i++) {
		floral::vec4f xformPos = i_xform * floral::vec4f(positions[i].x, positions[i].y, positions[i].z, 1.0f);
		o_vertices.push_back( { floral::vec3f(xformPos.x, xformPos.y, xformPos.z), finalNormal, i_color } );
	}

	for (u32 i = 0; i < 6; i++) {
		o_indices.push_back(lastIdx + indices[i]);
	}
}

template <typename TAllocator>
void GenBox_Tris_PosColor(const floral::vec4f& i_color, const floral::mat4x4f& i_xform,
	floral::fixed_array<DemoVertex, TAllocator>& o_vertices, floral::fixed_array<u32, TAllocator>& o_indices)
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
		2, 1, 0, 0, 3, 2,
		4, 5, 6, 6, 7, 4,
		3, 0, 4, 4, 7, 3,
		5, 1, 2, 2, 6, 5,
		4, 0, 1, 1, 5, 4,
		6, 2, 3, 3, 7, 6
	};

	u32 lastIdx = o_vertices.get_size();

	for (u32 i = 0; i < 8; i++) {
		floral::vec4f xformPos = i_xform * floral::vec4f(positions[i].x, positions[i].y, positions[i].z, 1.0f);
		o_vertices.push_back( { floral::vec3f(xformPos.x, xformPos.y, xformPos.z), i_color } );
	}

	for (u32 i = 0; i < 36; i++) {
		o_indices.push_back(lastIdx + indices[i]);
	}
}

template <typename TAllocator>
void GenBox_Tris_PosNormalColor(const floral::vec4f& i_color, const floral::mat4x4f& i_xform,
	floral::fixed_array<VertexPNC, TAllocator>& o_vertices, floral::fixed_array<u32, TAllocator>& o_indices)
{
	static floral::vec3f positions[] = {
		// positive Y
		floral::vec3f(1.0f, 1.0f, 1.0f), floral::vec3f(1.0f, 1.0f, -1.0f), floral::vec3f(-1.0f, 1.0f, -1.0f), floral::vec3f(-1.0f, 1.0f, 1.0f),
		// negative Y
		floral::vec3f(-1.0f, -1.0f, 1.0f), floral::vec3f(-1.0f, -1.0f, -1.0f), floral::vec3f(1.0f, -1.0f, -1.0f), floral::vec3f(1.0f, -1.0f, 1.0f),
		// positive Z
		floral::vec3f(-1.0f, -1.0f, 1.0f), floral::vec3f(1.0f, -1.0f, 1.0f), floral::vec3f(1.0f, 1.0f, 1.0f), floral::vec3f(-1.0f, 1.0f, 1.0f),
		// negative Z
		floral::vec3f(1.0f, -1.0f, -1.0f), floral::vec3f(-1.0f, -1.0f, -1.0f), floral::vec3f(-1.0f, 1.0f, -1.0f), floral::vec3f(1.0f, 1.0f, -1.0f),
		// positive X
		floral::vec3f(1.0f, -1.0f, 1.0f), floral::vec3f(1.0f, -1.0f, -1.0f), floral::vec3f(1.0f, 1.0f, -1.0f), floral::vec3f(1.0f, 1.0f, 1.0f),
		// negative X
		floral::vec3f(-1.0f, -1.0f, -1.0f), floral::vec3f(-1.0f, -1.0f, 1.0f), floral::vec3f(-1.0f, 1.0f, 1.0f), floral::vec3f(-1.0f, 1.0f, -1.0f)
	};

	static floral::vec4f normals[] = {
		floral::vec4f(0.0f, 1.0f, 0.0f, 0.0f),		// positive Y
		floral::vec4f(0.0f, -1.0f, 0.0f, 0.0f),		// negative Y
		floral::vec4f(0.0f, 0.0f, 1.0f, 0.0f),		// positive Z
		floral::vec4f(0.0f, 0.0f, -1.0f, 0.0f),		// negative Z
		floral::vec4f(1.0f, 0.0f, 0.0f, 0.0f),		// positive X
		floral::vec4f(-1.0f, 0.0f, 0.0f, 0.0f)		// negative X
	};

	static u32 indices[] = {
		// positive Y
		0, 1, 2, 2, 3, 0,
		// negative Y
		4, 5, 6, 6, 7, 4,
		// positive Z
		8, 9, 10, 10, 11, 8,
		// negative Z
		12, 13, 14, 14, 15, 12,
		// positive X
		16, 17, 18, 18, 19, 16,
		// negative X
		20, 21, 22, 22, 23, 20
	};

	u32 lastIdx = o_vertices.get_size();

	for (u32 i = 0; i < 6; i++) {
		floral::vec4f xformNormal = i_xform * normals[i];
		floral::vec3f faceNormal = floral::normalize(floral::vec3f(xformNormal.x, xformNormal.y, xformNormal.z));
		for (u32 j = 0; j < 4; j++) {
			floral::vec4f xformPos = i_xform * floral::vec4f(positions[4 * i + j].x, positions[4 * i + j].y, positions[4 * i + j].z, 1.0f);
			o_vertices.push_back({
					floral::vec3f(xformPos.x, xformPos.y, xformPos.z),
					faceNormal,
					i_color});
		}
	}

	for (u32 i = 0; i < 36; i++) {
		o_indices.push_back(lastIdx + indices[i]);
	}
}

template <typename TAllocator>
void GenIcosahedron_Tris_PosColor(const floral::vec4f& i_color, const floral::mat4x4f& i_xform,
	floral::fixed_array<DemoVertex, TAllocator>& o_vertices, floral::fixed_array<u32, TAllocator>& o_indices)
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
		0,4,1,0,9,4,9,5,4,4,5,8,4,8,1,
		8,10,1,8,3,10,5,3,8,5,2,3,2,7,3,
		7,10,3,7,6,10,7,11,6,11,0,6,0,1,6,
		6,1,10,9,0,11,9,11,2,9,2,5,7,2,11
	};

	u32 lastIdx = o_vertices.get_size();

	for (u32 i = 0; i < 12; i++) {
		floral::vec4f xformPos = i_xform * floral::vec4f(positions[i].x, positions[i].y, positions[i].z, 1.0f);
		o_vertices.push_back( { floral::vec3f(xformPos.x, xformPos.y, xformPos.z), i_color } );
	}

	for (u32 i = 0; i < 60; i++) {
		o_indices.push_back(lastIdx + indices[i]);
	}
}

}
