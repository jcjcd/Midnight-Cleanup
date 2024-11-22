#pragma once
#include "LightStructure.h"

class Material;
class Mesh;
class Texture;

namespace core
{
	class Scene;
	struct RenderResources;

	class DeferredShadePass
	{
	public:
		void Init(Scene& scene, Renderer& renderer, uint32_t width, uint32_t height);
		void Run(Scene& scene, Renderer& renderer, float tick, RenderResources& renderResource);
		void Finish();

	private:
		constexpr static uint32_t MAX_LIGHT_COUNT = 3;


		const static inline std::string CB_PER_OBJECT = "cbPerObject";
		const static inline std::string DIRECTIONAL_LIGHTS = "directionalLights";
		const static inline std::string USE_ALPHA_MAP = "useAlphaMap";
		const static inline std::string G_ALPHA = "gAlpha";
		const static inline std::string POINT_LIGHTS = "pointLights";
		const static inline std::string NUM_POINT_LIGHTS = "numPointLights";

		std::shared_ptr<Texture> _decalOutputAlbedo;
		std::shared_ptr<Texture> _decalOutputORM;

		std::shared_ptr<Material> _deferredMaterial;
		// IBL
		std::shared_ptr<Texture> _irradianceTexture;
		std::shared_ptr<Texture> _prefilteredTexture;
		std::shared_ptr<Texture> _brdfTexture;

		// shadow
		std::shared_ptr<Material> _dDepthMaterial;
		std::shared_ptr<Material> _sDepthMaterial;
		std::shared_ptr<Material> _pDepthMaterial;

		std::shared_ptr<Texture> _dLightDepthTexture;
		std::shared_ptr<Texture> _sLightDepthTexture;
		std::shared_ptr<Texture> _pLightDepthTexture;
		std::shared_ptr<Texture> _pLightDepthTextureRTs;

		std::unordered_map<entt::entity, core::PointLightStructure> _pointLightMap;
		std::set<entt::entity> _pointLightEntities;
		std::queue<entt::entity> _pointLightQueue;
	};
}

