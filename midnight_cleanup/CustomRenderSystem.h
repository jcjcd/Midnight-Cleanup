#pragma once
#include "Animacore/SystemInterface.h"
#include "Animacore/SystemTraits.h"

#include "Animavision/VideoTexture.h"

namespace mc
{
	class CustomRenderSystem : public core::ISystem, public core::IRenderSystem, public core::IUpdateSystem, public core::IFixedSystem
	{
	public:
		CustomRenderSystem(core::Scene& scene);
		~CustomRenderSystem() override;

		void operator()(core::Scene& scene, float tick) override;
		void operator()(core::Scene& scene) override;
		void operator()(core::Scene& scene, Renderer& renderer, float tick) override;

	private:
		void preStartSystem(const core::OnPreStartSystem& event);
		void finishSystem(const core::OnFinishSystem& event);

	private:
		const static inline std::string CB_PER_OBJECT = "cbPerObject";
		const static inline std::string G_EYE_POS_W = "gEyePosW";
		const static inline std::string G_RECIEVE_DECAL = "gRecieveDecal";
		const static inline std::string G_EMISSIVE_FACTOR = "gEmissiveFactor";

		entt::dispatcher* _dispatcher = nullptr;

		bool _skip1Frame = false;

		robin_hood::unordered_map<entt::entity, VideoTexture> _videoTextures;
	};
}
DEFINE_SYSTEM_TRAITS(mc::CustomRenderSystem)
