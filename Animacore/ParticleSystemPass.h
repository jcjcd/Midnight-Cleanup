#pragma once
#include "ParticleStructures.h"

class Material;
class Mesh;
class Texture;


namespace core
{
	struct OnParticleTransformUpdate;

	class ParticleSystemPass
	{
	public:
		void startParticlePass(Scene& scene, Renderer& renderer);
		void initParticles(Scene& scene, Renderer& renderer, float tick);
		void removeParticle(entt::registry& registry, entt::entity entity);
		void renderParticles(Scene& scene, Renderer& renderer, float tick, Matrix view, Matrix proj, 
			std::shared_ptr<Texture> renderTargetTexture, std::shared_ptr<Texture> depthStencilTexture);
		void finishParticlePass(Scene& scene);

	private:
		void forParticleTransformUpdate(const OnParticleTransformUpdate& event);

	private:
		// particle system
		std::shared_ptr<Material> _initDeadlistMaterial;
		std::shared_ptr<Material> _initParticleMaterial;
		std::shared_ptr<Material> _emitParticleMaterial;
		std::shared_ptr<Material> _simulateParticleMaterial;
		std::shared_ptr<Material> _renderParticleMaterial;

		std::shared_ptr<Texture> _randomTexture;
		std::shared_ptr<Texture> _indirectDrawTexture;

		std::unordered_map<entt::entity, ParticleObject> _particleObjectMap;
	};
}

