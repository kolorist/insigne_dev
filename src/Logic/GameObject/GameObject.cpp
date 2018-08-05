#include "GameObject.h"

#include <lotus/profiler.h>

namespace stone {

	GameObject::GameObject()
	{
	}

	GameObject::~GameObject()
	{
	}

	void GameObject::Update(Camera* i_camera, f32 i_deltaMs)
	{
		PROFILE_SCOPE(UpdateGameObject);
		for (u32 i = 0; i < m_Components.get_size(); i++)
			m_Components[i]->Update(i_camera, i_deltaMs);
	}

	void GameObject::AddComponent(IComponent* i_comp)
	{
		m_Components.push_back(i_comp);
	}

	IComponent* GameObject::GetComponentByName(const_cstr i_name)
	{
		floral::crc_string nameToFind(i_name);
		for (u32 i = 0; i < m_Components.get_size(); i++)
			if (m_Components[i]->GetName() == nameToFind)
				return m_Components[i];

		return nullptr;
	}

}
