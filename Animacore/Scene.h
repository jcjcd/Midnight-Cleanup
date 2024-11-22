#pragma once
#include <random>

#include "Entity.h"
#include "SystemTraits.h"

class Renderer;

namespace core
{
	class ISystem;
	class Graphics;
	class PhysicsScene;
	class IFixedSystem;
	class IPreUpdateSystem;
	class IUpdateSystem;
	class IPreRenderSystem;
	class IRenderSystem;
	class IPostRenderSystem;
	class PhysicsSystem;
	struct OnChangeScene;
	struct OnDestroyEntity;
	struct OnRemoveComponent;
	class SoundSystem;
	struct OnSetTimeScale;

	class Scene
	{
		struct SystemInfo
		{
			SystemType type;
			size_t systemIndex;
			entt::id_type systemId;
		};

	public:
		static constexpr const char* SCENE_EXTENSION = ".scene";
		static constexpr const char* PREFAB_EXTENSION = ".prefab";
		static constexpr const char* MATERIAL_EXTENSION = ".material";
		static constexpr const char* CONTROLLER_EXTENSION = ".controller";
		static constexpr const char* PHYSIC_MATERIAL_EXTENSION = ".pmaterial";
		static constexpr const char* DEFAULT_SCENE_NAME = "New Scene";
		static constexpr const char* FBX_EXTENSION = ".fbx";
		static constexpr const char* SOUND_EXTENSION = ".wav";

		Scene();

		// 엔티티 생성
		Entity CreateEntity();

		// 엔티티 지연 삭제
		void DestroyEntity(entt::entity topEntity);

		// 엔티티 즉각 삭제 (툴 전용)
		void DestroyEntityImmediately(entt::entity topEntity);

		template <typename T> requires HasSystemTraits<T>
		void RegisterSystem();

		template <typename T> requires HasSystemTraits<T>
		void RemoveSystem();

		void SaveScene(const std::filesystem::path& path, bool clearEmptyEntities = true);
		void LoadScene(std::filesystem::path path);

		void SavePrefab(const std::filesystem::path& path, core::Entity& entity, bool clearEmptyEntities = true);
		entt::entity LoadPrefab(const std::filesystem::path& path);

		void Update(float tick);
		void Render(float tick, Renderer* renderer);

		void ProcessEvent();

		void Start(Renderer* renderer);
		void Finish(Renderer* renderer);

		void Clear();

		const std::string& GetName() { return _name; }
		void SetName(const std::string& name) { _name = name; }
		entt::registry* GetRegistry() { return &_registry; }
		entt::dispatcher* GetDispatcher() { return &_dispatcher; }
		std::shared_ptr<PhysicsScene> GetPhysicsScene() { return _physicsScene; }
		std::mt19937* GetGenerator() { return &_gen; }
		float GetTimeScale() const { return _timeScale; }
		bool IsPlaying() const { return _timeScale > 0.0f; }

#ifdef _EDITOR
		template <typename T> requires HasSystemTraits<T>
		T* GetSystem(SystemType sysType);

		const auto& GetSystemMap() { return _systemMap; }
#endif

	private:
		void destroyEntity(entt::entity topEntity);
		void updateSystemMapIndex(SystemType type, size_t oldIndex, size_t newIndex);

		// system event
		void removeComponent(const OnRemoveComponent& event);

		void removeEmptyEntities(); // for Save Scene
		void removeEmptyEntities(const std::vector<entt::entity>& entities); // for Save Prefab
		void destroyEntities();

		void setTimeScale(const OnSetTimeScale& event);

	private:
		std::string _name = DEFAULT_SCENE_NAME;
		entt::registry _registry;
		entt::dispatcher _dispatcher;
		std::shared_ptr<PhysicsScene> _physicsScene;
		std::mt19937 _gen;

		// 시스템 컨테이너
		std::multimap<std::string, SystemInfo> _systemMap;
		std::vector<std::shared_ptr<ISystem>> _systems;
		std::vector<std::shared_ptr<IPreUpdateSystem>> _preUpdates;
		std::vector<std::shared_ptr<IUpdateSystem>> _updates;
		std::vector<std::shared_ptr<IFixedSystem>> _fixeds;
		std::vector<std::shared_ptr<IPreRenderSystem>> _preRenders;
		std::vector<std::shared_ptr<IRenderSystem>> _renders;
		std::vector<std::shared_ptr<IPostRenderSystem>> _postRenders;
		std::vector<std::shared_ptr<ICollisionHandler>> _collisionHandlers;

		// 모든 이벤트 처리 후 삭제를 위한 대기 큐
		std::queue<entt::entity> _destroyedEntities;
		std::vector<entt::entity> _destroyedEntitiesEvent;

		// fixed update 를 위한 누적 시간
		float _accumulator = 0.f;
		float _timeScale = 1.0f;

		// sound
		SoundSystem* _soundSystem = nullptr;
	};

	template <typename T> requires HasSystemTraits<T>
	void Scene::RegisterSystem()
	{
		// 시스템 메타 데이터
		constexpr auto name = SystemTraits<T>::name;
		constexpr auto systemId = entt::type_hash<T>::value();

		if (_systemMap.contains(name))
			return;

		std::shared_ptr<T> system = std::make_shared<T>(*this);

		if constexpr (std::is_base_of_v<ISystem, T>)
		{
			_systems.push_back(system);
			_systemMap.insert({ name,  { SystemType::None, _systems.size() - 1, systemId } });
		}
		if constexpr (std::is_base_of_v<IPreUpdateSystem, T>)
		{
			_preUpdates.push_back(system);
			_systemMap.insert({ name,  { SystemType::PreUpdate, _preUpdates.size() - 1, systemId } });
		}
		if constexpr (std::is_base_of_v<IUpdateSystem, T>)
		{
			_updates.push_back(system);
			_systemMap.insert({ name,  { SystemType::Update, _updates.size() - 1, systemId } });
		}
		if constexpr (std::is_base_of_v<IFixedSystem, T>)
		{
			_fixeds.push_back(system);
			_systemMap.insert({ name,  { SystemType::FixedUpdate, _fixeds.size() - 1, systemId } });
		}
		if constexpr (std::is_base_of_v<IPreRenderSystem, T>)
		{
			_preRenders.push_back(system);
			_systemMap.insert({ name,  { SystemType::PreRender, _preRenders.size() - 1, systemId } });
		}
		if constexpr (std::is_base_of_v<IRenderSystem, T>)
		{
			_renders.push_back(system);
			_systemMap.insert({ name,  { SystemType::Render, _renders.size() - 1, systemId } });
		}
		if constexpr (std::is_base_of_v<IPostRenderSystem, T>)
		{
			_postRenders.push_back(system);
			_systemMap.insert({ name,  { SystemType::PostRender, _postRenders.size() - 1, systemId } });
		}
		if constexpr (std::is_base_of_v<ICollisionHandler, T>)
		{
			_collisionHandlers.push_back(system);
			_systemMap.insert({ name, { SystemType::CollisionHandler, _collisionHandlers.size() - 1, systemId } });
		}
	}

	template <typename T> requires HasSystemTraits<T>
	void Scene::RemoveSystem()
	{
		constexpr auto name = SystemTraits<T>::name;

		auto range = _systemMap.equal_range(name);

		for (auto it = range.first; it != range.second; ++it)
		{
			auto [sysType, index, systemId] = it->second;

			// 시스템 타입에 따라 적절한 컨테이너에서 시스템 삭제
			switch (sysType)
			{
			case SystemType::None:
				if (index < _systems.size() - 1)
				{
					std::swap(_systems[index], _systems.back());
					updateSystemMapIndex(sysType, _systems.size() - 1, index);
				}
				_systems.pop_back();
				break;
			case SystemType::PreUpdate:
				if (index < _preUpdates.size() - 1)
				{
					std::swap(_preUpdates[index], _preUpdates.back());
					updateSystemMapIndex(sysType, _preUpdates.size() - 1, index);
				}
				_preUpdates.pop_back();
				break;
			case SystemType::Update:
				if (index < _updates.size() - 1)
				{
					std::swap(_updates[index], _updates.back());
					updateSystemMapIndex(sysType, _updates.size() - 1, index);
				}
				_updates.pop_back();
				break;
			case SystemType::FixedUpdate:
				if (index < _fixeds.size() - 1)
				{
					std::swap(_fixeds[index], _fixeds.back());
					updateSystemMapIndex(sysType, _fixeds.size() - 1, index);
				}
				_fixeds.pop_back();
				break;
			case SystemType::PreRender:
				if (index < _preRenders.size() - 1)
				{
					std::swap(_preRenders[index], _preRenders.back());
					updateSystemMapIndex(sysType, _preRenders.size() - 1, index);
				}
				_preRenders.pop_back();
				break;
			case SystemType::Render:
				if (index < _renders.size() - 1)
				{
					std::swap(_renders[index], _renders.back());
					updateSystemMapIndex(sysType, _renders.size() - 1, index);
				}
				_renders.pop_back();
				break;
			case SystemType::PostRender:
				if (index < _postRenders.size() - 1)
				{
					std::swap(_postRenders[index], _postRenders.back());
					updateSystemMapIndex(sysType, _postRenders.size() - 1, index);
				}
				_postRenders.pop_back();
				break;
			case SystemType::CollisionHandler:
				if (index < _collisionHandlers.size() - 1)
				{
					std::swap(_collisionHandlers[index], _collisionHandlers.back());
					updateSystemMapIndex(sysType, _collisionHandlers.size() - 1, index);
				}
				_renders.pop_back();
				break;
			default:
				break;
			}
		}

		if (_systemMap.contains(name))
			_systemMap.erase(name);
	}

#ifdef _EDITOR
	template <typename T> requires HasSystemTraits<T>
	T* Scene::GetSystem(SystemType sysType)
	{
		constexpr auto name = SystemTraits<T>::name;

		auto range = _systemMap.equal_range(name);

		for (auto it = range.first; it != range.second; ++it)
		{
			auto [type, index, id] = it->second;

			if (type == sysType)
			{
				switch (sysType)
				{
				case SystemType::PreUpdate:
					return dynamic_cast<T*>(_preUpdates[index].get());
				case SystemType::Update:
					return dynamic_cast<T*>(_updates[index].get());
				case SystemType::FixedUpdate:
					return dynamic_cast<T*>(_fixeds[index].get());
				case SystemType::PreRender:
					return dynamic_cast<T*>(_preRenders[index].get());
				case SystemType::Render:
					return dynamic_cast<T*>(_renders[index].get());
				case SystemType::PostRender:
					return dynamic_cast<T*>(_postRenders[index].get());
				default:
					return nullptr;
				}
			}
		}

		return nullptr;
	}
#endif
}


