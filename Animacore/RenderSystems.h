#pragma once
#include "SystemTraits.h"
#include "CoreSystemEvents.h"
#include "SystemInterface.h"

class Material;
class Mesh;
class Texture;

namespace core
{
	struct RenderResources;

	class RenderSystem : public ISystem, public IRenderSystem

	{
	public:
		RenderSystem(Scene& scene);
		~RenderSystem() override { _dispatcher->disconnect(this); }

		void operator()(Scene& scene, Renderer& renderer, float tick) override;

	private:
		void preStartSystem(const OnPreStartSystem& event);
		void startSystem(const OnStartSystem& event);
		void finishSystem(const OnFinishSystem& event);
		void createEntity(const OnCreateEntity& event);
		void changeMesh(const OnChangeMesh& event);

		void destroyRenderResources(const OnDestroyRenderResources& event);
		void createRenderResources(const OnCreateRenderResources& event);

	private:
		entt::dispatcher* _dispatcher = nullptr;

		// ½Ì±ÛÅæ ÄÄÆ÷³ÍÆ®¿¡¼­ ¹Þ¾Æ¿Â°Í
		core::RenderResources* _renderResources = nullptr;

		const static inline std::string CB_PER_OBJECT = "cbPerObject";
		const static inline std::string G_EYE_POS_W = "gEyePosW";
		const static inline std::string G_RECIEVE_DECAL = "gRecieveDecal";
		const static inline std::string G_EMISSIVE_FACTOR = "gEmissiveFactor";
		const static inline std::string CB_DISSOLVE_FACTOR = "cbDissolveFactor";

	};
}
DEFINE_SYSTEM_TRAITS(core::RenderSystem)
