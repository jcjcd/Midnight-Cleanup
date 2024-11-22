#pragma once
#include "SystemTraits.h"
#include "SystemInterface.h"

class Material;
class Mesh;
class Texture;

namespace core
{
	struct Camera;
	struct WorldTransform;
	struct RenderResources;

	class PreRenderSystem : public ISystem, public IPreRenderSystem
	{
	public:
		PreRenderSystem(Scene& scene);
		~PreRenderSystem();

		void PreRender(Scene& scene, Renderer& renderer, float tick) override;

	private:
		void preStartSystem(const OnPreStartSystem& event);
		void startSystem(const OnStartSystem& event);
		void finishSystem(const OnFinishSystem& event);

		void destroyRenderResources(const OnDestroyRenderResources& event);
		void createRenderResources(const OnCreateRenderResources& event);


	private:	
		entt::dispatcher* _dispatcher = nullptr;

		Camera* _mainCamera = nullptr;
		WorldTransform* _mainCameraTransform = nullptr;

		core::RenderResources* _renderResources;
	};
}
DEFINE_SYSTEM_TRAITS(core::PreRenderSystem)
