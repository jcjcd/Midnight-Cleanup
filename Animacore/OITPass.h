#pragma once

class Material;
class Mesh;
class Texture;

namespace core
{
	class OITPass
	{
	public:
		void initOITResources(Scene& scene, Renderer& renderer, uint32_t width, uint32_t height);
		void oitPass(core::Scene& scene, Renderer& renderer, const Matrix& view, const Matrix& proj, const Matrix& viewProj, const Vector3& camPos);
		void oitCompositePass(core::Scene& scene, Renderer& renderer, const Matrix& view, const Matrix& proj, const Matrix& viewProj, std::shared_ptr<Mesh> quadMesh, std::shared_ptr<Texture> renderTargetTexture);
		void finishOITResources();

	private:
		/// OIT
		std::shared_ptr<Texture> _revealageTexture;
		std::shared_ptr<Texture> _accumulationTexture;

		/// OIT - ÇÕÄ¡±â
		std::shared_ptr<Material> _oitCombineMaterial;

	};
}
