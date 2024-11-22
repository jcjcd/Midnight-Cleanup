#include "pch.h"
#include "ParticleSystemPass.h"

#include "Scene.h"
#include "RenderComponents.h"
#include "CoreComponents.h"

#include "../Animavision/Renderer.h"
#include "../Animavision/ShaderResource.h"
#include "../Animavision/Material.h"
#include "../Animavision/Shader.h"


void core::ParticleSystemPass::removeParticle(entt::registry& registry, entt::entity entity)
{
	if (_particleObjectMap.contains(entity))
		_particleObjectMap.erase(entity);
}

void core::ParticleSystemPass::startParticlePass(Scene& scene, Renderer& renderer)
{
	_initDeadlistMaterial = Material::Create(renderer.LoadShader("./Shaders/particleDeadListInit.hlsl"));
	_initParticleMaterial = Material::Create(renderer.LoadShader("./Shaders/particleInit.hlsl"));
	_emitParticleMaterial = Material::Create(renderer.LoadShader("./Shaders/particleEmit.hlsl"));
	_simulateParticleMaterial = Material::Create(renderer.LoadShader("./Shaders/particleSimulate.hlsl"));
	_renderParticleMaterial = Material::Create(renderer.LoadShader("./Shaders/particleRender.hlsl"));

	_randomTexture = renderer.CreateRandomTexture();
	_indirectDrawTexture = renderer.CreateIndirectDrawTexture();

	scene.GetRegistry()->on_destroy<core::ParticleSystem>().connect<&ParticleSystemPass::removeParticle>(this);
	scene.GetDispatcher()->sink<OnParticleTransformUpdate>().connect<&ParticleSystemPass::forParticleTransformUpdate>(this);
}

void core::ParticleSystemPass::initParticles(Scene& scene, Renderer& renderer, float tick)
{
	auto&& registry = scene.GetRegistry();

	for (auto&& [entity, transform, particleSystem] : registry->view<core::WorldTransform, core::ParticleSystem>().each())
	{
		if (_particleObjectMap.contains(entity))
			continue;

		uint32_t entityNum = static_cast<uint32_t>(entity);
		ParticleObject newParticle = ParticleObject(renderer, particleSystem.mainData.maxParticleCounts, transform.matrix, entityNum);

		newParticle.mainData.startColor[0] = particleSystem.mainData.startColor0;
		newParticle.mainData.startColor[1] = particleSystem.mainData.startColorOption == ParticleSystem::Option::Constant ? particleSystem.mainData.startColor0 : particleSystem.mainData.startColor1;
		newParticle.mainData.startLifeTime[0] = particleSystem.mainData.startLifeTime.x;
		newParticle.mainData.startLifeTime[1] = particleSystem.mainData.startLifeTimeOption == ParticleSystem::Option::Constant ? particleSystem.mainData.startLifeTime.x : particleSystem.mainData.startLifeTime.y;
		newParticle.mainData.startSpeed[0] = particleSystem.mainData.startSpeed.x;
		newParticle.mainData.startSpeed[1] = particleSystem.mainData.startSpeedOption == ParticleSystem::Option::Constant ? particleSystem.mainData.startSpeed.x : particleSystem.mainData.startSpeed.y;
		newParticle.mainData.startRotation[0] = particleSystem.mainData.startRotation.x;
		newParticle.mainData.startRotation[1] = particleSystem.mainData.startRotationOption == ParticleSystem::Option::Constant ? particleSystem.mainData.startRotation.x : particleSystem.mainData.startRotation.y;
		newParticle.mainData.startSize[0] = particleSystem.mainData.startSize.x;
		newParticle.mainData.startSize[1] = particleSystem.mainData.startSizeOption == ParticleSystem::Option::Constant ? particleSystem.mainData.startSize.x : particleSystem.mainData.startSize.y;
		newParticle.mainData.gravityModifier[0] = particleSystem.mainData.gravityModifier.x;
		newParticle.mainData.gravityModifier[1] = particleSystem.mainData.gravityModifierOption == ParticleSystem::Option::Constant ? particleSystem.mainData.gravityModifier.x : particleSystem.mainData.gravityModifier.y;
		newParticle.mainData.simulationSpeed = particleSystem.mainData.simulationSpeed;
		newParticle.mainData.maxParticleCount = particleSystem.mainData.maxParticleCounts;

		newParticle.shapeData.transform = Matrix::CreateScale(particleSystem.shapeData.scale) * Matrix::CreateFromYawPitchRoll(particleSystem.shapeData.rotation) * Matrix::CreateTranslation(particleSystem.shapeData.position);
		newParticle.shapeData.scale = particleSystem.shapeData.scale;
		newParticle.shapeData.rotation = particleSystem.shapeData.rotation;
		newParticle.shapeData.position = particleSystem.shapeData.position;
		newParticle.shapeData.shapeType = static_cast<int>(particleSystem.shapeData.shapeType);
		newParticle.shapeData.modeType =/* static_cast<int>(particleSystem.shapeData.modeType)*/0;
		newParticle.shapeData.angleInRadian = particleSystem.shapeData.angleInDegree * (DirectX::XM_PI / 180.0f);
		newParticle.shapeData.radius = particleSystem.shapeData.radius;
		newParticle.shapeData.donutRadius = particleSystem.shapeData.donutRadius;
		newParticle.shapeData.arcInRadian = particleSystem.shapeData.arcInDegree * (DirectX::XM_PI / 180.0f);
		newParticle.shapeData.spread = particleSystem.shapeData.spread;
		newParticle.shapeData.radiusThickness = particleSystem.shapeData.radiusThickness;

		newParticle.velocityOverLifeTimeData.velocity = particleSystem.velocityOverLifeTimeData.velocity;
		newParticle.velocityOverLifeTimeData.orbital = particleSystem.velocityOverLifeTimeData.orbital;
		newParticle.velocityOverLifeTimeData.offset = particleSystem.velocityOverLifeTimeData.offset;
		newParticle.velocityOverLifeTimeData.isUsed = particleSystem.velocityOverLifeTimeData.isUsed;

		newParticle.limitVelocityOverLifeTimeData.speed = particleSystem.limitVelocityOverLifeTimeData.speed;
		newParticle.limitVelocityOverLifeTimeData.dampen = particleSystem.limitVelocityOverLifeTimeData.dampen;
		newParticle.limitVelocityOverLifeTimeData.isUsed = particleSystem.limitVelocityOverLifeTimeData.isUsed;

		newParticle.forceOverLifeTimeData.force = particleSystem.forceOverLifeTimeData.force;
		newParticle.forceOverLifeTimeData.isUsed = particleSystem.forceOverLifeTimeData.isUsed;

		newParticle.colorOverLifeTimeData.alphaRatioCount = std::min<int>(static_cast<int>(particleSystem.colorOverLifeTimeData.alphaRatios.size()), 8);
		newParticle.colorOverLifeTimeData.colorRatioCount = std::min<int>(static_cast<int>(particleSystem.colorOverLifeTimeData.colorRatios.size()), 8);
		for (int i = 0; i < newParticle.colorOverLifeTimeData.alphaRatioCount; i++)
			newParticle.colorOverLifeTimeData.alphaRatios[i] = Vector4{ particleSystem.colorOverLifeTimeData.alphaRatios[i].x, particleSystem.colorOverLifeTimeData.alphaRatios[i].y,0,0 };
		for (int i = 0; i < newParticle.colorOverLifeTimeData.colorRatioCount; i++)
			newParticle.colorOverLifeTimeData.colorRatios[i] = particleSystem.colorOverLifeTimeData.colorRatios[i];
		newParticle.colorOverLifeTimeData.isUsed = particleSystem.colorOverLifeTimeData.isUsed;

		newParticle.sizeOverLifeTimeData.pointA = particleSystem.sizeOverLifeTimeData.pointA;
		newParticle.sizeOverLifeTimeData.pointB = particleSystem.sizeOverLifeTimeData.pointB;
		newParticle.sizeOverLifeTimeData.pointC = particleSystem.sizeOverLifeTimeData.pointC;
		newParticle.sizeOverLifeTimeData.pointD = particleSystem.sizeOverLifeTimeData.pointD;
		newParticle.sizeOverLifeTimeData.isUsed = particleSystem.sizeOverLifeTimeData.isUsed;

		newParticle.rotationOverLifeTimeData.angularVelocityInRadian = particleSystem.rotationOverLifeTimeData.angularVelocityInDegree * (DirectX::XM_PI / 180.0f);
		newParticle.rotationOverLifeTimeData.isUsed = particleSystem.rotationOverLifeTimeData.isUsed;

		newParticle.renderData.baseColor = particleSystem.renderData.baseColor;
		newParticle.renderData.emissiveColor = particleSystem.renderData.emissiveColor;
		newParticle.renderData.texTransform = Matrix::CreateScale(particleSystem.renderData.tiling.x, particleSystem.renderData.tiling.y, 0) * Matrix::CreateTranslation(particleSystem.renderData.offset.x, particleSystem.renderData.offset.y, 0);
		newParticle.renderData.renderMode = static_cast<int>(particleSystem.renderData.renderModeType);
		newParticle.renderData.colorMode = static_cast<int>(particleSystem.renderData.colorModeType);
		newParticle.renderData.useAlbedoMap = particleSystem.renderData.useBaseColorTexture;
		newParticle.renderData.useEmissiveMap = particleSystem.renderData.useEmissiveTexture;
		newParticle.renderData.alphaCutOff = particleSystem.renderData.alphaCutOff;

		_particleObjectMap[entity] = newParticle;
	}

	for (auto& [entity, particle] : _particleObjectMap)
		particle.SetFrameTime(tick);
}

void core::ParticleSystemPass::renderParticles(Scene& scene, Renderer& renderer, float tick, Matrix view, Matrix proj, std::shared_ptr<Texture> renderTargetTexture, std::shared_ptr<Texture> depthStencilTexture)
{
	auto align = [](int value, int alignment) { return (value + alignment - 1) / alignment * alignment; };

	for (auto& [entity, particle] : _particleObjectMap)
	{
		core::ParticleSystem& particleSystem = scene.GetRegistry()->get<core::ParticleSystem>(entity);

		if (!particleSystem.isOn)
			continue;

		if (!particleSystem.instanceData.isEmit && !particleSystem.mainData.isLooping && particleSystem.mainData.duration < particle.instanceData.timePos)
		{
			particleSystem.isOn = false;
			continue;
		}

		if (particleSystem.instanceData.isReset || particle.isFirstTime)
		{
			particle.deadListBufferTexture->SetInitialCount(0);
			_initDeadlistMaterial->SetUAVTexture("gDeadListToAddTo", particle.deadListBufferTexture);
			renderer.DispatchCompute(*_initDeadlistMaterial, align(particle.mainData.maxParticleCount, 256) / 256, 1, 1);

			particle.particleBufferTexture->SetInitialCount(-1);
			_initParticleMaterial->SetUAVTexture("gParticleBuffer", particle.particleBufferTexture);
			renderer.DispatchCompute(*_initParticleMaterial, align(particle.mainData.maxParticleCount, 256) / 256, 1, 1);

			particle.Reset();
			particleSystem.instanceData.isReset = false;
			particle.isFirstTime = false;
		}

		particle.CalculateNumToEmit(particleSystem);

		if (particleSystem.textureSheetAnimationData.isUsed)
			particle.UpdateTextureSheetAnimation(particleSystem);

		particle.particleBufferTexture->SetInitialCount(-1);
		_emitParticleMaterial->SetUAVTexture("gParticleBuffer", particle.particleBufferTexture);
		particle.deadListBufferTexture->SetInitialCount(-1);
		_emitParticleMaterial->SetUAVTexture("gDeadListToAllocFrom", particle.deadListBufferTexture);
		_emitParticleMaterial->SetTexture("gRandomTexture", _randomTexture);

		auto emitShader = _emitParticleMaterial->GetShader();

		emitShader->MapConstantBuffer(renderer.GetContext(), "cbParticleObject");
		emitShader->SetStruct("gParticleInstance", &particle.instanceData);
		emitShader->SetStruct("gParticleMain", &particle.mainData);
		emitShader->SetStruct("gParticleShape", &particle.shapeData);
		emitShader->UnmapConstantBuffer(renderer.GetContext(), "cbParticleObject");
		if (Buffer* deadListCB = emitShader->GetConstantBuffer("cbDeadList", 0))
			renderer.CopyStructureCount(deadListCB, 0, particle.deadListBufferTexture.get());
		else
			OutputDebugStringA("constant buffer is not found");

		renderer.DispatchCompute(*_emitParticleMaterial, align(particle.instanceData.numToEmit, 1024) / 1024, 1, 1);

		particle.particleBufferTexture->SetInitialCount(-1);
		particle.deadListBufferTexture->SetInitialCount(-1);
		particle.aliveListBufferTexture->SetInitialCount(0);
		_indirectDrawTexture->SetInitialCount(-1);

		_simulateParticleMaterial->SetUAVTexture("gParticleBuffer", particle.particleBufferTexture);
		_simulateParticleMaterial->SetUAVTexture("gDeadList", particle.deadListBufferTexture);
		_simulateParticleMaterial->SetUAVTexture("gIndexBuffer", particle.aliveListBufferTexture);
		_simulateParticleMaterial->SetUAVTexture("gDrawArgs", _indirectDrawTexture);

		auto simulateShader = _simulateParticleMaterial->GetShader();

		simulateShader->MapConstantBuffer(renderer.GetContext());
		simulateShader->SetStruct("gParticleInstance", &particle.instanceData);
		simulateShader->SetStruct("gParticleMain", &particle.mainData);
		simulateShader->SetStruct("gParticleForceOverLifeTime", &particle.forceOverLifeTimeData);
		simulateShader->SetStruct("gParticleLimitVelocityOverLifeTime", &particle.limitVelocityOverLifeTimeData);
		simulateShader->SetStruct("gParticleVelocityOverLifeTime", &particle.velocityOverLifeTimeData);
		simulateShader->SetStruct("gParticleRotationOverLifeTime", &particle.rotationOverLifeTimeData);
		simulateShader->SetStruct("gParticleColorOverLifeTime", &particle.colorOverLifeTimeData);
		simulateShader->SetStruct("gParticleSizeOverLifeTime", &particle.sizeOverLifeTimeData);
		simulateShader->UnmapConstantBuffer(renderer.GetContext());

		renderer.DispatchCompute(*_simulateParticleMaterial, align(particle.mainData.maxParticleCount, 256) / 256, 1, 1);

		Texture* textures[] = { renderTargetTexture.get() };
		renderer.SetRenderTargets(1, textures, nullptr, false);

		if (particleSystem.renderData.isTwoSide)
		{
			switch (particleSystem.renderData.renderModeType)
			{
			case ParticleSystem::RenderModule::RenderMode::Additive:
				renderer.ApplyRenderState(BlendState::ADDITIVE_BLEND, RasterizerState::CULL_NONE, DepthStencilState::DEPTH_ENABLED);
				break;
			case ParticleSystem::RenderModule::RenderMode::AlphaBlend:
				renderer.ApplyRenderState(BlendState::ALPHA_BLEND, RasterizerState::CULL_NONE, DepthStencilState::DEPTH_ENABLED);
				break;
			case ParticleSystem::RenderModule::RenderMode::Modulate:
				renderer.ApplyRenderState(BlendState::MODULATE_BLEND, RasterizerState::CULL_NONE, DepthStencilState::DEPTH_ENABLED);
				break;
			case ParticleSystem::RenderModule::RenderMode::Subtractive:
				renderer.ApplyRenderState(BlendState::SUBTRACTIVE_BLEND, RasterizerState::CULL_NONE, DepthStencilState::DEPTH_ENABLED);
				break;
			default:
				assert(false && "wrong render mode");
				break;
			}
		}
		else
		{
			switch (particleSystem.renderData.renderModeType)
			{
			case ParticleSystem::RenderModule::RenderMode::Additive:
				renderer.ApplyRenderState(BlendState::ADDITIVE_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);
				break;
			case ParticleSystem::RenderModule::RenderMode::AlphaBlend:
				renderer.ApplyRenderState(BlendState::ALPHA_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);
				break;
			case ParticleSystem::RenderModule::RenderMode::Modulate:
				renderer.ApplyRenderState(BlendState::MODULATE_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);
				break;
			case ParticleSystem::RenderModule::RenderMode::Subtractive:
				renderer.ApplyRenderState(BlendState::SUBTRACTIVE_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);
				break;
			default:
				assert(false && "wrong render mode");
				break;
			}
		}

		_renderParticleMaterial->SetTexture("gParticleBuffer", particle.particleBufferTexture);
		_renderParticleMaterial->SetTexture("gIndexBuffer", particle.aliveListBufferTexture);

		if (!particleSystem.baseColorTexture)
			particleSystem.baseColorTexture = renderer.GetParticleTexture(particleSystem.renderData.baseColorTextureString.c_str());

		if (!particleSystem.emissiveTexture)
			particleSystem.emissiveTexture = renderer.GetParticleTexture(particleSystem.renderData.emissiveTextureString.c_str());

		_renderParticleMaterial->SetTexture("gAlbedoTexture", particleSystem.baseColorTexture);
		_renderParticleMaterial->SetTexture("gEmissiveTexture", particleSystem.emissiveTexture);
		_renderParticleMaterial->SetTexture("gDepthTexture", depthStencilTexture);

		auto renderShader = _renderParticleMaterial->GetShader();

		renderShader->MapConstantBuffer(renderer.GetContext());
		renderShader->SetStruct("gParticleRender", &particle.renderData);
		renderShader->SetMatrix("gView", view);
		renderShader->SetMatrix("gProj", proj);
		renderShader->UnmapConstantBuffer(renderer.GetContext());

		renderer.SubmitInstancedIndirect(*_renderParticleMaterial, _indirectDrawTexture.get(), 0, PrimitiveTopology::POINTLIST);
	}
}

void core::ParticleSystemPass::finishParticlePass(Scene& scene)
{
	_randomTexture.reset();
	_indirectDrawTexture.reset();

	_initDeadlistMaterial.reset();
	_initParticleMaterial.reset();
	_emitParticleMaterial.reset();
	_simulateParticleMaterial.reset();
	_renderParticleMaterial.reset();

	_particleObjectMap.clear();

	scene.GetRegistry()->on_destroy<core::ParticleSystem>().disconnect(this);
	scene.GetDispatcher()->disconnect(this);
}

/// <summary>
/// 트랜스폼 업데이트를 위해서 일단 지워주는 함수
/// </summary>
/// <param name="event"></param>
void core::ParticleSystemPass::forParticleTransformUpdate(const OnParticleTransformUpdate& event)
{
	if (!_particleObjectMap.contains(event.entity))
		return;

	_particleObjectMap.erase(event.entity);
}
