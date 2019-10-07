namespace stone
{
namespace tech
{

template <class T>
void SHProbeBaker::StartSHBaking(const T& i_shPos, voidptr o_shOutput, const floral::simple_callback<void, const floral::mat4x4f&>& i_renderCb)
{
	m_SHPositions.reserve(i_shPos.get_size(), m_ResourceArena);
	for (size i = 0; i < i_shPos.get_size(); i++)
	{
		m_SHPositions.push_back(i_shPos[i]);
	}

	m_SHOutputBuffer = (SHData*)o_shOutput;
	
	m_RenderCB = i_renderCb;
	m_CurrentProbeCaptured = false;
	m_IsSHBakingFinished = false;
	m_CurrentProbeIdx = 0;
}

}
}
