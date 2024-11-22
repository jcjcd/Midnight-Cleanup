#pragma once
#include "SystemInterface.h"
#include "SystemTraits.h"

class Material;
class Mesh;
class Texture;

namespace core
{
	class RaytraceRenderSystem : public ISystem, public IRenderSystem
	{
	public:
		RaytraceRenderSystem(Scene& scene);
		~RaytraceRenderSystem() override { _dispatcher->disconnect(this); }

		void operator()(Scene& scene, Renderer& renderer, float tick) override;

	private:
		void startSystem(const OnStartSystem& event);
		void finishSystem(const OnFinishSystem& event);
		void createEntity(const OnCreateEntity& event);
		void changeMesh(const OnChangeMesh& event);

	private:
		entt::dispatcher* _dispatcher = nullptr;


		// common
		std::shared_ptr<Material> _quadMaterial;
		std::shared_ptr<Mesh> _quadMesh;


		// resize
		LONG _width = 1920;
		LONG _height = 1080;

	};
}
DEFINE_SYSTEM_TRAITS(core::RaytraceRenderSystem)

