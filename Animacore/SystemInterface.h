#pragma once

class Renderer;

namespace core
{
	class Scene;
	class Entity;

	class ISystem
	{
	public:
		ISystem(Scene& scene) {}
		virtual ~ISystem() = default;
	};


	/*------------------------------
		Update
	------------------------------*/
	class IPreUpdateSystem
	{
	public:
		virtual ~IPreUpdateSystem() = default;
		virtual void PreUpdate(Scene& scene, float tick) = 0;
	};

	class IUpdateSystem
	{
	public:
		virtual ~IUpdateSystem() = default;
		virtual void operator()(Scene& scene, float tick) = 0;
	};

	class IPostUpdateSystem
	{
	public:
		virtual ~IPostUpdateSystem() = default;
		virtual void PostUpdate(Scene& scene, float tick) = 0;
	};


	/*------------------------------
		Fixed
	------------------------------*/
	class IPreFixedSystem
	{
	public:
		virtual ~IPreFixedSystem() = default;
		virtual void PreFixed(Scene& scene) = 0;
	};

	class IFixedSystem
	{
	public:
		virtual ~IFixedSystem() = default;
		virtual void operator()(Scene& scene) = 0;

		static constexpr float FIXED_TIME_STEP = 1.0f / 60.0f;
	};

	class IPostFixedSystem
	{
	public:
		virtual ~IPostFixedSystem() = default;
		virtual void PostFixed(Scene& scene) = 0;
	};


	/*------------------------------
		Render
	------------------------------*/
	class IPreRenderSystem
	{
	public:
		virtual ~IPreRenderSystem() = default;
		virtual void PreRender(Scene& scene, Renderer& renderer, float tick) = 0;
	};

	class IRenderSystem
	{
	public:
		virtual ~IRenderSystem() = default;
		virtual void operator()(Scene& scene, Renderer& renderer, float tick) = 0;
	};

	class IPostRenderSystem
	{
	public:
		virtual ~IPostRenderSystem() = default;
		virtual void PostRender(Scene& scene, Renderer& renderer, float tick) = 0;
	};


	/*------------------------------
		Collision
	------------------------------*/
	class ICollisionHandler
	{
	public:
		virtual ~ICollisionHandler() = default;
		virtual void OnCollisionEnter(entt::entity self, entt::entity other, entt::registry& registry) = 0;
		virtual void OnCollisionStay(entt::entity self, entt::entity other, entt::registry& registry) = 0;
		virtual void OnCollisionExit(entt::entity self, entt::entity other, entt::registry& registry) = 0;
	};
}
