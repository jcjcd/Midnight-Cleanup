#pragma once

class Material;
class Mesh;
class Texture;

namespace core
{
	class Scene;
	struct RenderResources;

	class BloomPass
	{
	public:
		void Init(Scene& scene, Renderer& renderer, uint32_t width, uint32_t height);
		void Run(Scene& scene, Renderer& renderer, float tick, RenderResources& renderResource);
		void Finish();

		std::shared_ptr<Texture> GetBloomTexture() const { return _extractBrightTextures[_downSampleUAVIndex]; }

	private:
		inline static constexpr uint32_t _downScaleCount = 4;

		uint32_t _width;
		uint32_t _height;

		std::shared_ptr<Texture> _downSampleTextures[_downScaleCount][2];
		std::shared_ptr<Texture> _extractBrightTextures[2];

		std::shared_ptr<Material> _extractBrightAreasMaterial;
		std::shared_ptr<Material> _downSampleMaterial;
		std::shared_ptr<Material> _accumulateMaterial;
		std::shared_ptr<Material> _horizontalBlurMaterial;
		std::shared_ptr<Material> _verticalBlurMaterial;

		uint32_t _downSampleBufferDenominator[_downScaleCount] = { 4, 8, 16, 32 };
		uint32_t _downSampleSRVIndex = 0;
		uint32_t _downSampleUAVIndex = 1;
	};
}

