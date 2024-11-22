#pragma once

#include "Entity.h"
#include <format>

class Renderer;

namespace core
{
	class Scene;
	class ICollisionHandler;

	/*------------------------------
		시스템 시작
	------------------------------*/
	struct OnPreStartSystem
	{
		Scene* scene = nullptr;
		Renderer* renderer = nullptr;

		OnPreStartSystem(Scene& scene, Renderer* renderer) : scene(&scene), renderer(renderer)
		{
		}
	};

	struct OnStartSystem
	{
		Scene* scene = nullptr;
		Renderer* renderer = nullptr;

		OnStartSystem(Scene& scene, Renderer* renderer) : scene(&scene), renderer(renderer)
		{
		}
	};

	struct OnPostStartSystem
	{
		Scene* scene = nullptr;
		Renderer* renderer = nullptr;

		OnPostStartSystem(Scene& scene, Renderer* renderer) : scene(&scene), renderer(renderer)
		{
		}
	};


	/*------------------------------
		시스템 종료
	------------------------------*/
	struct OnFinishSystem
	{
		Scene* scene = nullptr;
		Renderer* renderer = nullptr;

		OnFinishSystem(Scene& scene, Renderer* renderer) : scene(&scene), renderer(renderer)
		{
		}
	};


	/*------------------------------
		런타임 엔티티 생성
	------------------------------*/
	struct OnCreateEntity
	{
		std::vector<entt::entity> entities;
		Scene* scene = nullptr;

		OnCreateEntity(entt::entity handle, Scene& scene)
			: entities{1, handle}, scene(&scene)
		{
		}

		OnCreateEntity(const std::vector<entt::entity>& entities, Scene& scene)
			: entities(entities), scene(&scene)
		{
		}
	};


	/*------------------------------
		런타임 엔티티 파괴
	------------------------------*/
	struct OnDestroyEntity
	{
		std::vector<entt::entity> entities;
		Scene* scene = nullptr;

		OnDestroyEntity(const std::vector<entt::entity>& entities, Scene& scene)
			: entities(entities), scene(&scene)
		{
		}
	};

	/*------------------------------
		런타임 컴포넌트 삭제
	------------------------------*/
	struct OnRemoveComponent
	{
		entt::entity entity = entt::null;
		entt::id_type componentType = entt::null;

		OnRemoveComponent(entt::entity handle, entt::id_type hash) : entity(handle), componentType(hash)
		{
		}
	};

	/*------------------------------
		씬 변경
	------------------------------*/
	// enqueue 로 사용 권장
	struct OnChangeScene
	{
		std::filesystem::path path;

		OnChangeScene(std::filesystem::path path)
			: path(path)
		{
		}
	};


	/*------------------------------
		충돌 처리 시스템 등록
	------------------------------*/
	/*!
	 * 충돌을 처리하는 시스템이 CollisionCallback 클래스에게 발행하는 이벤트
	 * 시스템 생성자나 OnStartSystem 시점에서 사용
	 * @code
	 * entt::dispatcher->trigger<core::OnRegisterCollisionHandler>({ entt::type_hash<ComponentClass>(), this });
	 * @endcode
	 * id(타입) 와 관련된 모든 충돌을 이 구조체로 이벤트를 연결한 클래스가 받음, CollisionCallback 클래스 참조
	 */
	struct OnRegisterCollisionHandler
	{
		entt::id_type id;
		ICollisionHandler* handler;

		OnRegisterCollisionHandler(const entt::id_type& id, ICollisionHandler* handler)
			: id(id), handler(handler)
		{
		}
	};

	/*!
	 * OnRegisterCollisionHandler 와 사용법은 같음
	 */
	struct OnRemoveCollisionHandler
	{
		entt::id_type id;
		ICollisionHandler* handler;

		OnRemoveCollisionHandler(const entt::id_type& id, ICollisionHandler* handler)
			: id(id), handler(handler)
		{
		}
	};

	/*------------------------------
		메쉬 변경
	------------------------------*/
	struct OnChangeMesh
	{
		Entity entity;
		Renderer* renderer = nullptr;

		OnChangeMesh(entt::entity handle, entt::registry& registry, Renderer* renderer)
			: entity(handle, registry), renderer(renderer)
		{
		}
	};

	/*------------------------------
		트랜스폼 변경
	------------------------------*/
	// TransformSystem, PhysicsSystem 에서 내부적으로 사용됨
	struct OnUpdateTransform
	{
		entt::entity entity;
		entt::registry* registry;

		OnUpdateTransform(entt::entity entity, entt::registry* registry)
			: entity(entity), registry(registry)
		{
		}
	};

	/*------------------------------
		리지드바디 업데이트
	------------------------------*/
	// PhysicsSystem 에서 내부적으로 사용됨
	struct EchoRigidbody
	{
		entt::entity entity;
		entt::registry* registry;

		EchoRigidbody(entt::entity entity, entt::registry* registry)
			: entity(entity), registry(registry)
		{
		}
	};

	/*------------------------------
		콘솔 메시지 전송
	------------------------------*/
	struct OnThrow
	{
		enum Type
		{
			Info,
			Warn,
			Error,
			Debug
		};

		Type exceptionType = Info;
		std::string message;

		template <typename... Args>
		OnThrow(Type type, const std::string& fmt, Args&&... args)
			: exceptionType(type), message(std::vformat(fmt, std::make_format_args(args...)))
		{
		}
	};


	/*------------------------------
		화면비 변경
	------------------------------*/
	struct OnChangeResolution
	{
		bool isFullScreen = false;
		bool isWindowedFullScreen = false;
		LONG width;
		LONG height;
	};

	struct OnDestroyRenderResources
	{
		Scene* scene = nullptr;
		Renderer* renderer = nullptr;

		OnDestroyRenderResources(Scene& scene, Renderer* renderer)
			: scene(&scene), renderer(renderer)
		{
		}
	};

	struct OnCreateRenderResources
	{
		Scene* scene = nullptr;
		Renderer* renderer = nullptr;

		OnCreateRenderResources(Scene& scene, Renderer* renderer)
			: scene(&scene), renderer(renderer)
		{
		}
	};

	struct OnLateCreateRenderResources
	{
		Scene* scene = nullptr;
		Renderer* renderer = nullptr;

		OnLateCreateRenderResources(Scene& scene, Renderer* renderer)
			: scene(&scene), renderer(renderer)
		{
		}
	};


	/*------------------------------
		파티클 갱신
	------------------------------*/
	struct OnParticleTransformUpdate
	{
		entt::entity entity;

		OnParticleTransformUpdate(entt::entity entity) : entity(entity) {}
	};

	/*------------------------------
		타임 스케일 조정
	------------------------------*/
	struct OnSetTimeScale
	{
		float timeScale;

		OnSetTimeScale(float timeScale) : timeScale(timeScale) {}
	};

	struct OnSetVolume
	{
		float volume;

		OnSetVolume(float v) : volume(v) {}
	};

	struct OnUpdateBGMState
	{
		Scene* scene;

		OnUpdateBGMState(Scene& scene) : scene(&scene) {}
	};
}

// 예외 처리 특수화
// core::OnThrow 에서 core::Entity 또는 entt::entity 를 string 화 하는데 사용됨
template <>
struct std::formatter<entt::entity> : std::formatter<std::string>
{
	auto format(entt::entity entity, format_context& ctx) const
	{
		return format_to(ctx.out(), "{}", static_cast<uint32_t>(entity));
	}
};

template <>
struct std::formatter<core::Entity> : std::formatter<std::string>
{
	auto format(const core::Entity& entity, format_context& ctx) const
	{
		return format_to(ctx.out(), "{}", static_cast<uint32_t>(entity));
	}
};
