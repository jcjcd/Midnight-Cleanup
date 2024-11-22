#pragma once

namespace core
{
	template <typename T>
	const char* GetName()
	{
		return typeid(T).name();
	}

	template <typename T>
	void SaveSnapshot(entt::snapshot* snapshot, cereal::JSONOutputArchive* archive)
	{
		archive->setNextName(typeid(T).name());
		snapshot->get<T>(*archive);
	}

	template <typename T>
	void LoadSnapshot(entt::snapshot_loader* loader, cereal::JSONInputArchive* archive)
	{
		loader->get<T>(*archive);
	}

	template <typename T>
	void SavePrefabSnapshot(entt::snapshot* snapshot, cereal::JSONOutputArchive* archive, std::vector<entt::entity>::iterator first, std::vector<entt::entity>::iterator last)
	{
		archive->setNextName(typeid(T).name());
		snapshot->get<T>(*archive, first, last);
	}

	template <typename T>
	void LoadPrefabSnapshot(entt::continuous_loader* loader, cereal::JSONInputArchive* archive)
	{
		loader->get<T>(*archive);
	}

	template <typename T>
	void Assign(entt::registry* registry, const entt::entity* entity, entt::meta_any* component)
	{
		registry->emplace_or_replace<T>(*entity, component->cast<T>());
	}

	template <typename T>
	void AddComponent(entt::registry* registry, entt::entity* entity)
	{
		registry->emplace<T>(*entity);
	}

	template <typename T>
	void RemoveComponent(entt::registry* registry, entt::entity* entity)
	{
		registry->remove<T>(*entity);
	}

	template <typename T>
	bool HasComponent(entt::registry* registry, entt::entity* entity)
	{
		return registry->any_of<T>(*entity);
	}

	template<typename T>
	T& GetComponent(entt::registry* registry, entt::entity* entity)
	{
		return registry->get<T>(*entity);
	}

	template<typename T>
	entt::meta_handle GetHandle(entt::registry* registry, entt::entity* entity)
	{
		return entt::meta_handle(&registry->get<T>(*entity));
	}

	template<typename T>
	void SetComponent(entt::registry* registry, entt::entity* entity, entt::meta_any* component)
	{
		registry->replace<T>(*entity, component->cast<T>());
	}

	template<typename T>
	void ResetComponent(entt::registry* registry, entt::entity* entity)
	{
		registry->replace<T>(*entity, T{});
	}
}