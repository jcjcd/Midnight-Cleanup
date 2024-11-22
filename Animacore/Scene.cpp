#include "pch.h"
#include "Scene.h"

#include "MetaCtxs.h"
#include "PhysicsScene.h"
#include "CoreSerialize.h" // Relationship 때문에 사용
#include "CoreComponents.h"

// Systems
#include "InputSystem.h"
#include "SoundSystem.h"
#include "RenderSystems.h"
#include "PhysicsSystem.h"
#include "AnimatorSystem.h"
#include "TransformSystem.h"
#include "PreRenderSystem.h"
#include "PostRenderSystem.h"

#include <fstream>


core::Scene::Scene()
	: _gen(std::random_device{}())
{
	_physicsScene = std::make_shared<PhysicsScene>(*this);

	_dispatcher.sink<OnRemoveComponent>().connect<&Scene::removeComponent>(this);
	_dispatcher.sink<OnSetTimeScale>().connect<&Scene::setTimeScale>(this);

	// 기본 시스템 추가 (제거 가능)
	RegisterSystem<InputSystem>();
	RegisterSystem<RenderSystem>();
	RegisterSystem<PhysicsSystem>();
	RegisterSystem<AnimatorSystem>();
	RegisterSystem<TransformSystem>();
	RegisterSystem<PreRenderSystem>();
	RegisterSystem<PostRenderSystem>();

	// 싱글톤 컴포넌트 추가
	_registry.ctx().emplace<Configuration>();

	_soundSystem = new SoundSystem(*this);
}

core::Entity core::Scene::CreateEntity()
{
	Entity entity = { _registry.create(), _registry };

	// 기본 컴포넌트 추가 (제거 가능)
	auto& name = entity.Emplace<Name>().name;
	entity.Emplace<LocalTransform>();
	entity.Emplace<WorldTransform>();

	// 엔티티 기본 이름
	name += std::format(" ({})", static_cast<uint32_t>(entity.GetHandle()));

	// 생성 이벤트 전송
	_dispatcher.enqueue<OnCreateEntity>(entity, *this);

	return entity;
}

void core::Scene::DestroyEntity(entt::entity topEntity)
{
	_destroyedEntities.push(topEntity);
}

void core::Scene::DestroyEntityImmediately(entt::entity topEntity)
{
	destroyEntity(topEntity);
}

void core::Scene::destroyEntity(entt::entity topEntity)
{
	// 유효성 확인
	if (!_registry.valid(topEntity))
		return;

	// 코어 엔티티 생성
	Entity top = { topEntity, _registry };

	// 삭제될 하위 엔티티
	std::vector<entt::entity> descendants;
	descendants.push_back(topEntity);

	// 하위 엔티티 추가
	for (const auto e : _registry.view<entt::entity>())
	{
		Entity coreE = { e, _registry };

		if (top.IsAncestorOf(coreE))
			descendants.push_back(coreE);
	}

	// 상위 엔티티의 자식에서 제거
	if (const auto relationship = top.TryGet<Relationship>())
	{
		if (relationship->parent != entt::null)
		{
			if (auto* pRelationship = _registry.try_get<Relationship>(relationship->parent))
			{
				auto& parentChildren = pRelationship->children;
				parentChildren.erase(std::ranges::find(parentChildren, topEntity));
			}
		}
	}

	// 삭제
	for (const auto e : descendants)
	{
		if (_registry.valid(e))
		{
			_registry.destroy(e);
			_destroyedEntitiesEvent.push_back(e);
		}
	}
}

void core::Scene::SaveScene(const std::filesystem::path& path, bool clearEmptyEntities)
{
	if (clearEmptyEntities)
		removeEmptyEntities();

	std::stringstream ss;

	// 씬 스냅샷 생성
	{
		entt::snapshot snapshot(_registry);
		cereal::JSONOutputArchive archive(ss);

		archive.setNextName("entities");
		snapshot.get<entt::entity>(archive);

		// 컴포넌트 저장
		for (auto&& [id, type] : entt::resolve(global::componentMetaCtx))
		{
			if (auto save = type.func("SaveSnapshot"_hs))
			{
				save.invoke({}, &snapshot, &archive);
			}
		}

		// 시스템 이름 저장
		std::vector<std::string> systemNames;
		for (const auto& name : _systemMap | std::views::keys)
		{
			auto it = std::ranges::find(systemNames, name);

			if (it == systemNames.end())
				systemNames.push_back(name);
		}
		archive(cereal::make_nvp("systems", systemNames));

		// 씬 설정(Configuration) 저장
		archive(cereal::make_nvp("configuration", _registry.ctx().get<Configuration>()));
	}

	auto str = ss.str();

	auto savePath = !path.has_extension() ? path.string() + SCENE_EXTENSION : path.string();

	std::ofstream outFile(savePath, std::ios::out | std::ios::trunc);

	// 씬 스냅샷 파일 저장
	if (outFile)
	{
		outFile << ss.str();
		outFile.close();
	}
	else
	{
		LOG_ERROR(*this, "Cannot open file : {}", path.filename().string());
	}
}

void core::Scene::LoadScene(std::filesystem::path path)
{
	Clear();

	std::ifstream file(path);

	if (!file.is_open() or path.extension() != SCENE_EXTENSION)
	{
		LOG_ERROR(*this, "Cannot open file : {}", path.filename().string());
		return;
	}

	_name = path.filename().replace_extension().string();

	std::stringstream ss;
	ss << file.rdbuf();
	file.close();

	// 씬 스냅샷 로드
	{
		cereal::JSONInputArchive archive(ss);
		entt::snapshot_loader loader(_registry);

		// 불필요 엔티티 제거
		loader.orphans();

		loader.get<entt::entity>(archive);

		for (auto&& [id, type] : entt::resolve(global::componentMetaCtx))
		{
			if (auto getName = type.func("GetName"_hs))
			{
				const char* name = nullptr;
				name = getName.invoke({}).cast<const char*>();

				// Archive에 저장된 컴포넌트 이름과 일치하는지 확인
				if (!name or !archive.getNodeName() or strcmp(archive.getNodeName(), name))
					continue;
			}

			// 컴포넌트 로드 시도
			if (auto loadStorage = type.func("LoadSnapshot"_hs))
			{
				try
				{
					loadStorage.invoke({}, &loader, &archive);  // 정상적으로 로드
				}
				catch (const std::exception& e)
				{
					LOG_WARN(*this, "Failed to load component: {}", e.what());
				}
			}
		}

		// 로드할 시스템 목록
		std::vector<std::string> systemNames;

		try
		{
			archive(cereal::make_nvp("systems", systemNames));
		}
		catch (const std::exception& e)
		{
			LOG_ERROR(*this, "Invalid Scene file : Failed to load systems", e.what());
			return;
		}

		for (auto&& [id, type] : entt::resolve(global::systemMetaCtx))
		{
			if (auto loadSystem = type.func("LoadSystem"_hs))
			{
				try
				{
					loadSystem.invoke({}, this, &systemNames);
				}
				catch (const std::exception& e)
				{
					LOG_WARN(*this, "Failed to load system: {}", e.what());
				}
			}
		}

		// 씬 설정(Configuration) 저장
		//archive(cereal::make_nvp("configuration", _registry.ctx().get<Configuration>()));
	}
}

void core::Scene::SavePrefab(const std::filesystem::path& path, core::Entity& entity, bool clearEmptyEntities)
{
	// 계층구조 저장용 벡터
	std::vector<entt::entity> descendents;
	descendents.push_back(entity);

	// 하위 엔티티 저장
	auto entities = _registry.view<entt::entity>();
	for (auto& element : entities)
	{
		auto tempEntity = Entity{ element, _registry };

		if (entity.IsAncestorOf(tempEntity))
			descendents.push_back(tempEntity);
	}

	if (clearEmptyEntities)
		removeEmptyEntities(descendents);

	// 프리팹 스냅샷 생성
	std::stringstream ss;
	{
		entt::snapshot snapshot(_registry);
		cereal::JSONOutputArchive archive(ss);

		archive(descendents.size());
		archive(descendents.size());
		for (auto& ent : descendents)
			archive(ent);

		for (auto&& [id, type] : entt::resolve(global::componentMetaCtx))
		{
			if (auto save = type.func("SavePrefabSnapshot"_hs))
				save.invoke({}, &snapshot, &archive, descendents.begin(), descendents.end());
		}
	}

	std::ofstream outFile;
	if (path.extension() == PREFAB_EXTENSION)
		outFile.open(path.generic_string(), std::ios::out | std::ios::trunc);
	else
		outFile.open(path.generic_string() + PREFAB_EXTENSION, std::ios::out | std::ios::trunc);

	// 프리팹 파일 저장
	if (outFile)
	{
		outFile << ss.str();
		outFile.close();
		LOG_INFO(*this, "Prefab Saved : {}", path.filename().string());
	}
	else
	{
		LOG_ERROR(*this, "Cannot save file : {}", path.filename().string());
	}
}

entt::entity core::Scene::LoadPrefab(const std::filesystem::path& path)
{
	std::ifstream file(path);

	if (!file.is_open() or path.extension() != PREFAB_EXTENSION)
	{
		LOG_ERROR(*this, "Cannot Open File : {}", path.filename().string());
		return entt::null;
	}

	std::stringstream ss;
	ss << file.rdbuf();
	file.close();

	// 프리팹 스냅샷 로드
	cereal::JSONInputArchive archive(ss);
	entt::continuous_loader loader(_registry);

	std::vector<entt::entity> entities;

	// 최상위 엔티티
	entt::entity top = entt::null;
	entt::entity cur = entt::null;

	auto callback = [&archive, &loader, &top, &cur, &entities]<typename T>(T & value) {
		archive(value);

		if constexpr (std::is_same_v<T, entt::entity>)
		{
			if (top == entt::null)
				top = value;

			if (std::ranges::find(entities, value) == entities.end())
				entities.push_back(value);
		}

		if constexpr (std::is_same_v<T, Relationship>)
		{
			value.parent = loader.map(value.parent);

			auto originChildren = value.children;
			value.children.clear();

			for (auto child : originChildren)
				value.children.push_back(loader.map(child));
		}
	};

	loader.get<entt::entity>(callback);
	top = loader.map(top);

	for (auto& entity : entities)
		entity = loader.map(entity);

	for (auto&& [id, type] : entt::resolve(global::componentMetaCtx))
	{
		if (auto getName = type.func("GetName"_hs))
		{
			const char* name = nullptr;
			name = getName.invoke({}).cast<const char*>();
			if (!name or !archive.getNodeName() or strcmp(archive.getNodeName(), name))
				continue;
		}

		try
		{
			if (id == entt::type_hash<Relationship>())
				loader.get<Relationship>(callback);
			else if (auto load = type.func("LoadPrefabSnapshot"_hs))
				load.invoke({}, &loader, &archive);  // 컴포넌트 로드 시도
		}
		catch (const std::exception& e)
		{
			LOG_WARN(*this, "Failed to load component: {}", e.what());  // 컴포넌트 로드 실패 시 경고 출력
		}
	}

	removeEmptyEntities();

	_dispatcher.enqueue<OnCreateEntity>(entities, *this);

	return top;
}


void core::Scene::Update(float tick)
{
	_accumulator += tick;

	for (auto& preUpdate : _preUpdates)
	{
		preUpdate->PreUpdate(*this, tick);
	}

	for (auto& update : _updates)
	{
		(*update)(*this, tick);
	}
	(*_soundSystem)(*this, tick);

	// 고정 업데이트는 누적된 시간이 고정 시간 간격을 넘어설 때마다 실행
	while (_accumulator >= IFixedSystem::FIXED_TIME_STEP)
	{
		for (auto& fixed : _fixeds)
		{
			(*fixed)(*this);
		}
		_accumulator -= IFixedSystem::FIXED_TIME_STEP;
	}

}



void core::Scene::Render(float tick, Renderer* renderer)
{
	// 렌더
	for (auto& preRender : _preRenders)
	{
		preRender->PreRender(*this, *renderer, tick);
	}

	for (auto& render : _renders)
	{
		(*render)(*this, *renderer, tick);
	}

	for (auto& postRender : _postRenders)
	{
		postRender->PostRender(*this, *renderer, tick);
	}
}

void core::Scene::ProcessEvent()
{
	// 이벤트 처리
	_dispatcher.update();

	// 엔티티 삭제
	destroyEntities();
}

void core::Scene::Start(Renderer* renderer)
{
	_timeScale = 1.0f;
	_dispatcher.trigger<OnPreStartSystem>({ *this, renderer });
	_dispatcher.trigger<OnStartSystem>({ *this, renderer });
	_dispatcher.trigger<OnPostStartSystem>({ *this, renderer });
}

void core::Scene::Finish(Renderer* renderer)
{
	_dispatcher.trigger<OnFinishSystem>({ *this, renderer });
}

void core::Scene::Clear()
{
	_name = "New Scene";
	_accumulator = 0.f;

	_registry.clear();
	_physicsScene->Clear();
	_dispatcher.clear();

	_systemMap.clear();
	_systems.clear();
	_preUpdates.clear();
	_updates.clear();
	_fixeds.clear();
	_preRenders.clear();
	_renders.clear();
	_postRenders.clear();
	_collisionHandlers.clear();

	while (!_destroyedEntities.empty())
		_destroyedEntities.pop();

	// 기본 시스템 추가 (제거 가능)
	RegisterSystem<InputSystem>();
	RegisterSystem<RenderSystem>();
	RegisterSystem<PhysicsSystem>();
	RegisterSystem<AnimatorSystem>();
	RegisterSystem<TransformSystem>();
}

void core::Scene::updateSystemMapIndex(SystemType type, size_t oldIndex, size_t newIndex)
{
	// 시스템 맵에서 스왑된 시스템을 찾아 인덱스를 업데이트
	for (auto& info : _systemMap | std::views::values)
	{
		if (info.type == type && info.systemIndex == oldIndex)
		{
			info.systemIndex = newIndex;
			break;
		}
	}
}
void core::Scene::removeComponent(const OnRemoveComponent& event)
{
	// 멀티 스레드 환경에서는 즉각 삭제 불가
	auto func = entt::resolve(global::componentMetaCtx, event.componentType).func("RemoveComponent"_hs);

	if (func)
		func.invoke({}, &_registry, &event.entity);
	else
		LOG_ERROR(*this, "{} : Invalid component", event.entity);
}

void core::Scene::removeEmptyEntities()
{
	// 모든 엔티티를 저장할 벡터 선언
	std::vector<entt::entity> entitiesToRemove;

	// 모든 엔티티를 순회하면서 컴포넌트가 없는 엔티티를 찾아 저장
	for (auto entity : _registry.view<entt::entity>())
	{
		if (_registry.orphan(entity))
			entitiesToRemove.push_back(entity);
	}

	// 컴포넌트가 없는 엔티티 삭제
	for (auto entity : entitiesToRemove)
		_registry.destroy(entity);
}

void core::Scene::removeEmptyEntities(const std::vector<entt::entity>& entities)
{
	// 삭제할 엔티티를 저장할 벡터 선언
	std::vector<entt::entity> entitiesToRemove;

	// 주어진 엔티티 목록을 순회하면서 컴포넌트가 없는 엔티티를 찾아 저장
	for (auto entity : entities)
	{
		if (_registry.orphan(entity))
			entitiesToRemove.push_back(entity);
	}

	// 컴포넌트가 없는 엔티티 삭제
	for (auto entity : entitiesToRemove)
		_registry.destroy(entity);
}

void core::Scene::destroyEntities()
{
	_destroyedEntitiesEvent.clear();

	// 삭제 예정 엔티티 일괄 삭제
	while (!_destroyedEntities.empty())
	{
		destroyEntity(_destroyedEntities.front());
		_destroyedEntities.pop();
	}

	if (!_destroyedEntitiesEvent.empty())
		_dispatcher.trigger<OnDestroyEntity>({ _destroyedEntitiesEvent, *this });
}

void core::Scene::setTimeScale(const OnSetTimeScale& event)
{
	_timeScale = event.timeScale;
}
