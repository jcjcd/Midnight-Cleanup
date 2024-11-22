#pragma once
#include "SystemInterface.h"
#include "SystemTraits.h"
#include "CoreSystemEvents.h"

#include "ParticleSystemPass.h"
#include "OITPass.h"
#include "DeferredShadePass.h"
#include "BloomPass.h"

class Material;
class Mesh;
class Texture;


namespace core
{
	struct RenderResources;
	class PostRenderSystem : public ISystem, public IPostRenderSystem
	{
	public:

		PostRenderSystem(Scene& scene);
		~PostRenderSystem();

		void PostRender(Scene& scene, Renderer& renderer, float tick) override;

	private:
		void startSystem(const OnStartSystem& event);
		void finishSystem(const OnFinishSystem& event);

		void lateCreateRenderResources(const OnLateCreateRenderResources& event);
		void destroyRenderResources(const OnDestroyRenderResources& event);

		void initOutline(Scene& scene, Renderer& renderer);
		void finishOutline();
		void outlinePass(Scene& scene, Renderer& renderer, float tick, const Matrix& view, const Matrix& proj, const DirectX::BoundingFrustum& frustum);

	private:
		entt::dispatcher* _dispatcher = nullptr;

		inline static constexpr float _refreshTime = 1 / 60.f;
		float _refreshTimer = 0.0f;

		core::RenderResources* _renderResources = nullptr;

		LONG _width;
		LONG _height;

		std::shared_ptr<Material> _uiMaterial;

		// »ß³¢»ß³©
		std::shared_ptr<Material> _pickingMaterial;
		std::shared_ptr<Material> _outlineComputeMaterial;
		std::shared_ptr<Texture> _pickingTexture;

		// post process
		std::shared_ptr<Material> _postProcessingMaterial;
		std::shared_ptr<Texture> _tempRT;

		// 3D text
		std::shared_ptr<Texture> _textTexture;

		// fxaa
		std::shared_ptr<Material> _fxaaMaterial;


		ParticleSystemPass _particleSystemPass;
		OITPass _oitPass;
		DeferredShadePass _deferredShadePass;
		BloomPass _bloomPass;

		static constexpr float _defaultWidth = 1920.0f;
		static constexpr float _defaultHeight = 1080.0f;
	};
}
DEFINE_SYSTEM_TRAITS(core::PostRenderSystem)

