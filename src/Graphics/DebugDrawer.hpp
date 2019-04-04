namespace stone {

template <class TAllocator>
void DebugDrawer::DrawPolygon3D(const floral::fixed_array<floral::vec3f, TAllocator>& i_polySoup, const floral::vec4f& i_color)
{
	u32 idx = m_DebugVertices[m_CurrentBufferIdx].get_size();
	for (u32 i = 0; i < i_polySoup.get_size(); i++) {
		m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { i_polySoup[i], i_color });
	}

	for (u32 i = 0; i < i_polySoup.get_size() / 3; i++) {
		m_DebugIndices[m_CurrentBufferIdx].push_back(idx);
		m_DebugIndices[m_CurrentBufferIdx].push_back(idx + 1);

		m_DebugIndices[m_CurrentBufferIdx].push_back(idx + 1);
		m_DebugIndices[m_CurrentBufferIdx].push_back(idx + 2);

		m_DebugIndices[m_CurrentBufferIdx].push_back(idx + 2);
		m_DebugIndices[m_CurrentBufferIdx].push_back(idx);
		idx += 3;
	}
}

}
