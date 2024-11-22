#pragma once

#include "Scene.h"
#include "SystemTraits.h"

namespace core
{
	class Scene;

	template <typename T> requires HasSystemTraits<T>
	void LoadSystem(Scene* scene, std::vector<std::string>* systemNames)
	{
		constexpr auto name = SystemTraits<T>::name;

        auto it = std::ranges::find(*systemNames, name);

        if (it != systemNames->end()) 
			scene->RegisterSystem<T>();
	}

	template <typename T> requires HasSystemTraits<T>
	void RegisterSystem(Scene* scene)
	{
		scene->RegisterSystem<T>();
	}

	template <typename T> requires HasSystemTraits<T>
	void RemoveSystem(Scene* scene)
	{
		scene->RemoveSystem<T>();
	}
}
