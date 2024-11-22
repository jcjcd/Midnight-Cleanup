#include "pch.h"
#include "DeferredShadePass.h"

#include "Scene.h"
#include "RenderComponents.h"
#include "CoreComponents.h"
#include "LightStructure.h"

#include "../Animavision/Renderer.h"
#include "../Animavision/ShaderResource.h"
#include "../Animavision/Material.h"
#include "../Animavision/Shader.h"
#include "../Animavision/Mesh.h"

namespace DeferredSlot
{
	enum
	{
		Albedo,
		ORM,
		Normal,
		PositionW,
		Emissive,
		PreCalculatedLight,
		Mask,

		Count,
	};
}


namespace
{
	struct PerObject
	{
		Matrix gWorld;
		Matrix gWorldInvTranspose;
		Matrix gTexTransform;
		Matrix gView;
		Matrix gProj;
		Matrix gViewProj;
	};
}

void core::DeferredShadePass::Init(Scene& scene, Renderer& renderer, uint32_t width, uint32_t height)
{
	auto&& renderResources = scene.GetRegistry()->ctx().get<core::RenderResources>();
	auto& deferredTextures = renderResources.deferredTextures;

	_dDepthMaterial = Material::Create(renderer.LoadShader("./Shaders/DirectionalLightDepth.hlsl"));
	_sDepthMaterial = Material::Create(renderer.LoadShader("./Shaders/SpotLightDepth.hlsl"));
	_pDepthMaterial = Material::Create(renderer.LoadShader("./Shaders/PointLightDepth.hlsl"));

	TextureDesc desc("dlightDepth", Texture::Type::Texture2DArray, 2048, 2048, 1, Texture::Format::R24G8_TYPELESS, Texture::Usage::DSV, nullptr, Texture::UAVType::NONE, 4);
	desc.dsvFormat = Texture::Format::D24_UNORM_S8_UINT;
	desc.srvFormat = Texture::Format::R24_UNORM_X8_TYPELESS;
	if (!_dLightDepthTexture)
		_dLightDepthTexture = renderer.CreateEmptyTexture(desc);

	desc.name = "slightDepth";
	desc.arraySize = MAX_LIGHT_COUNT;
	desc.width = 1024;
	desc.height = 1024;
	if (!_sLightDepthTexture)
		_sLightDepthTexture = renderer.CreateEmptyTexture(desc);

	desc.name = "plightDepth";
	desc.arraySize = MAX_LIGHT_COUNT * 6;
	if (!_pLightDepthTexture)
		_pLightDepthTexture = renderer.CreateEmptyTexture(desc);

	desc.name = "plightDepthRTs";
	desc.format = Texture::Format::R32_FLOAT;
	desc.usage = Texture::Usage::RTV;
	desc.srvFormat = Texture::Format::R32_FLOAT;
	desc.rtvFormat = Texture::Format::R32_FLOAT;

	if (!_pLightDepthTextureRTs)
		_pLightDepthTextureRTs = renderer.CreateEmptyTexture(desc);

	float decalClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	_decalOutputAlbedo = renderer.CreateEmptyTexture("DecalOutputAlbedo", Texture::Type::Texture2D, width, height, 1,
	                                                 Texture::Format::R32G32B32A32_FLOAT,
	                                                 Texture::Usage::RTV | Texture::Usage::UAV, decalClearColor);

	float decalClearColorORM[] = { -1.0f, -1.0f, -1.0f, -1.0f };

	_decalOutputORM = renderer.CreateEmptyTexture("DecalOutputORM", Texture::Type::Texture2D, width, height, 1,
	                                              Texture::Format::R32G32B32A32_FLOAT,
	                                              Texture::Usage::RTV | Texture::Usage::UAV, decalClearColorORM);

	_deferredMaterial = Material::Create(renderer.LoadShader("./Shaders/DeferredShading.hlsl"));
	_deferredMaterial->SetTexture("gAlbedo", deferredTextures[DeferredSlot::Albedo]);
	_deferredMaterial->SetTexture("gORM", deferredTextures[DeferredSlot::ORM]);
	_deferredMaterial->SetTexture("gNormal", deferredTextures[DeferredSlot::Normal]);
	_deferredMaterial->SetTexture("gPositionW", deferredTextures[DeferredSlot::PositionW]);
	_deferredMaterial->SetTexture("gEmissive", deferredTextures[DeferredSlot::Emissive]);
	_deferredMaterial->SetTexture("gPreCalculatedLighting", deferredTextures[DeferredSlot::PreCalculatedLight]);
	_deferredMaterial->SetTexture("gMask", deferredTextures[DeferredSlot::Mask]);

	_deferredMaterial->SetTexture("gDirectionLightShadowMap", _dLightDepthTexture);
	_deferredMaterial->SetTexture("gSpotLightShadowMap", _sLightDepthTexture);
	_deferredMaterial->SetTexture("gPointLightShadowMap", _pLightDepthTextureRTs);
	_deferredMaterial->SetTexture("gDecalOutputAlbedo", _decalOutputAlbedo);
	_deferredMaterial->SetTexture("gDecalOutputORM", _decalOutputORM);

	_brdfTexture = renderer.CreateTexture("./Resources/Textures/Default/BRDF.dds");
	_prefilteredTexture = renderer.CreateTextureCube("./Resources/Textures/Default/DarkSkyRadiance.dds");
	_irradianceTexture = renderer.CreateTextureCube("./Resources/Textures/Default/DarkSkyIrradiance.dds");

	_deferredMaterial->SetTexture("gIrradiance", _irradianceTexture);
	_deferredMaterial->SetTexture("gPrefilteredEnvMap", _prefilteredTexture);
	_deferredMaterial->SetTexture("gBRDFLUT", _brdfTexture);


}

void core::DeferredShadePass::Run(Scene& scene, Renderer& renderer, float tick, RenderResources& renderResources)
{
	auto& registry = *scene.GetRegistry();

	auto& deferredTextures = renderResources.deferredTextures;

	Matrix view = renderResources.viewMatrix;
	Matrix proj = renderResources.projectionMatrix;
	Matrix viewProj = renderResources.viewProjectionMatrix;

	if (!renderResources.mainCamera)
		return;

	auto& mainCamera = *renderResources.mainCamera;
	auto& cameraTransform = *renderResources.cameraTransform;

	/// 여기에 데칼 코드 추가
	{
		auto viewInveres = view.Invert();
		renderer.ClearTexture(_decalOutputAlbedo.get(), _decalOutputAlbedo->GetClearValue());
		renderer.ClearTexture(_decalOutputORM.get(), _decalOutputORM->GetClearValue());

		for (auto&& [entity, transform, decal] : registry.view<core::WorldTransform, core::Decal>().each())
		{
			if (decal.material == nullptr)
			{
				if (!decal.materialString.empty())
				{
					decal.material = renderer.GetMaterial(decal.materialString);
				}
				continue;
			}

			decal.material->SetTexture("gNormalTexture", deferredTextures[DeferredSlot::Normal]);
			decal.material->SetTexture("gDepthTexture", deferredTextures[DeferredSlot::PositionW]);
			decal.material->SetUAVTexture("OutputTexture", _decalOutputAlbedo);
			decal.material->SetUAVTexture("OrmOutputTexture", _decalOutputORM);

			auto& world = transform.matrix;
			auto worldInverse = world.Invert();
			auto WVP = world * view * proj;

			decal.material->m_Shader->MapConstantBuffer(renderer.GetContext());
			decal.material->m_Shader->SetMatrix("gWVP", WVP);
			decal.material->m_Shader->SetMatrix("gWorldInv", worldInverse);
			decal.material->m_Shader->SetMatrix("gViewInv", viewInveres);
			decal.material->m_Shader->SetFloat("fTargetWidth", static_cast<float>(renderResources.width));
			decal.material->m_Shader->SetFloat("fTargetHeight", static_cast<float>(renderResources.height));
			decal.material->m_Shader->SetFloat("gFadeFactor", decal.fadeFactor);
			decal.material->m_Shader->SetFloat3("gDecalDirection", transform.matrix.Up());
			decal.material->m_Shader->UnmapConstantBuffer(renderer.GetContext());

			//renderer.Submit(*_cubeMesh, *decal.material, 0, PrimitiveTopology::TRIANGLELIST, 1);
			renderer.DispatchCompute(*decal.material, renderResources.width / 16, renderResources.height / 16, 1);
		}
	}


	std::vector<core::DirectionalLightStructure> directionalLights;
	std::vector<core::PointLightStructure> pointLights;
	std::vector<core::PointLightStructure> nonShadowPointLights;
	std::vector<core::SpotLightStructure> spotLights;

	directionalLights.reserve(1);
	pointLights.reserve(MAX_LIGHT_COUNT);
	nonShadowPointLights.reserve(3);
	spotLights.reserve(MAX_LIGHT_COUNT);

	for (auto&& [entity, world, lightCommon, directionalLight] : registry.view<core::WorldTransform, core::LightCommon, core::DirectionalLight>().each())
	{
		core::DirectionalLightStructure light;
		light.color = lightCommon.color.ToVector3();
		light.intensity = lightCommon.intensity;
		light.isOn = lightCommon.isOn;
		light.useShadow = lightCommon.useShadow;
		light.direction = Vector3::Transform(Vector3::Backward, world.rotation);
		light.lightMode = static_cast<int>(lightCommon.lightMode);
		light.direction.Normalize();

		float cascadeEnds[4] = { 0.0f, };
		core::CalculateCascadeMatrices(view, proj, mainCamera.fieldOfView, mainCamera.aspectRatio, light.direction, mainCamera.nearClip, mainCamera.farClip, 4, light.lightViewProjection, cascadeEnds, directionalLight.cascadeEnds);

		light.cascadedEndClip = Vector4{ cascadeEnds[0], cascadeEnds[1], cascadeEnds[2], cascadeEnds[3] };

		directionalLights.emplace_back(light);

		break;
	}

	int tempIndex = 0;

	for (auto pointView = registry.view<core::PointLight, core::WorldTransform, core::LightCommon>(); auto && [entity, pointLight, world, lightCommon] : pointView.each())
	{
		if(lightCommon.useShadow)
		{
			if (!lightCommon.isOn)
			{
				if (_pointLightEntities.contains(entity))
				{
					_pointLightEntities.erase(entity);
					_pointLightMap.erase(entity);

					std::queue<entt::entity> newQueue;
					while (!_pointLightQueue.empty())
					{
						if (auto front = _pointLightQueue.front(); front != entity)
							newQueue.push(front);
						_pointLightQueue.pop();
					}

					newQueue.swap(_pointLightQueue);
				}

				continue;
			}

			if (_pointLightEntities.size() > MAX_LIGHT_COUNT)
			{
				entt::entity eraseEntity = _pointLightQueue.front();
				core::LightCommon& erasePointLight = registry.get<core::LightCommon>(eraseEntity);
				erasePointLight.isOn = false;

				_pointLightMap.erase(eraseEntity);
				_pointLightEntities.erase(eraseEntity);
				_pointLightQueue.pop();

				core::PointLightStructure light;
				light.color = lightCommon.color.ToVector3();
				light.intensity = lightCommon.intensity;
				light.isOn = lightCommon.isOn;
				light.useShadow = lightCommon.useShadow;
				light.position = world.position;
				light.range = pointLight.range;
				light.attenuation = pointLight.attenuation;
				light.lightMode = static_cast<int>(lightCommon.lightMode);

				light.nearZ = 1.0f;

				core::CreatePointLightViewProjMatrices(
					light.position,
					light.lightViewProjection,
					light.nearZ,
					light.range);

				_pointLightMap[entity] = light;
				_pointLightEntities.insert(entity);

				break;
			}

			core::PointLightStructure light;
			light.color = lightCommon.color.ToVector3();
			light.intensity = lightCommon.intensity;
			light.isOn = lightCommon.isOn;
			light.useShadow = lightCommon.useShadow;
			light.position = world.position;
			light.range = pointLight.range;
			light.attenuation = pointLight.attenuation;
			light.lightMode = static_cast<int>(lightCommon.lightMode);

			light.nearZ = 1.0f;

			core::CreatePointLightViewProjMatrices(light.position, light.lightViewProjection, light.nearZ, light.range);

			_pointLightMap[entity] = light;

			if (!_pointLightEntities.contains(entity))
				_pointLightQueue.push(entity);

			_pointLightEntities.insert(entity);
		}
		else
		{
			core::PointLightStructure light;
			light.color = lightCommon.color.ToVector3();
			light.intensity = lightCommon.intensity;
			light.isOn = lightCommon.isOn;
			light.useShadow = false;
			light.position = world.position;
			light.range = pointLight.range;
			light.attenuation = pointLight.attenuation;
			light.lightMode = static_cast<int>(lightCommon.lightMode);

			nonShadowPointLights.push_back(light);

			tempIndex++;
		}
	}

	for (auto& light : _pointLightMap | std::views::values)
		pointLights.emplace_back(light);

	if(nonShadowPointLights.size() > 3)
		nonShadowPointLights.resize(3);

	auto spotView = registry.view<core::SpotLight, core::WorldTransform, core::LightCommon>();
	for (auto&& [entity, spotLight, world, lightCommon] : spotView.each())
	{
		core::SpotLightStructure light;
		light.color = lightCommon.color.ToVector3();
		light.intensity = lightCommon.intensity;
		light.isOn = lightCommon.isOn;
		light.useShadow = lightCommon.useShadow;
		light.position = world.position;
		light.range = spotLight.range;
		light.direction = Vector3::Transform(Vector3::Backward, world.rotation);
		light.direction.Normalize();
		light.attenuation = spotLight.attenuation;
		light.spot = spotLight.angularAttenuationIndex;
		light.lightMode = static_cast<int>(lightCommon.lightMode);

		light.spotAngle = spotLight.spotAngle;
		light.innerAngle = spotLight.innerAngle;

		auto focusPosition = light.position + light.direction * light.range;

		Matrix lightView = DirectX::XMMatrixLookAtLH(light.position, focusPosition, Vector3::Up);

		auto spotAngleRadians = light.spotAngle * (DirectX::XM_PI / 180.0f);
		float nearPlane = 0.1f;
		Matrix lightProj = DirectX::XMMatrixPerspectiveFovLH(spotAngleRadians, 1.0f, nearPlane, light.range);
		light.lightViewProjection = lightView * lightProj;

		spotLights.emplace_back(light);
	}


	PerObject perObject;
	{
		renderer.SetViewport(2048, 2048);
		renderer.SetRenderTargets(0, nullptr, _dLightDepthTexture.get(), false);
		renderer.Clear(_dLightDepthTexture->GetClearValue());
		for (int dLightIndex = 0; dLightIndex < directionalLights.size(); dLightIndex++)
		{
			if (!directionalLights[dLightIndex].isOn || !directionalLights[dLightIndex].useShadow)
				continue;

			for (auto&& [entity, transform, meshRenderer] : registry.view<core::WorldTransform, core::MeshRenderer>().each())
			{
				float distance = (transform.position - cameraTransform.position).Length();

				if (!meshRenderer.isOn || !meshRenderer.receiveShadow || distance >= 100.0f)
					continue;

				if (meshRenderer.mesh == nullptr)
				{
					meshRenderer.mesh = renderer.GetMesh(meshRenderer.meshString);
					continue;
				}

				if (!meshRenderer.isCulling)
					renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::SHADOW_CULL_NONE, DepthStencilState::DEPTH_ENABLED);
				else
					renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::SHADOW, DepthStencilState::DEPTH_ENABLED);

				auto world = transform.matrix;
				auto worldInvTranspose = world.Invert().Transpose();

				perObject.gProj = proj;
				perObject.gView = view;
				perObject.gViewProj = viewProj;
				perObject.gWorld = world;
				perObject.gWorldInvTranspose = worldInvTranspose;
				perObject.gTexTransform = Matrix::Identity;

				for (uint32_t i = 0; i < meshRenderer.mesh->subMeshCount; i++)
				{
					_dDepthMaterial->m_Shader->MapConstantBuffer(renderer.GetContext());
					_dDepthMaterial->m_Shader->SetConstant(CB_PER_OBJECT, &perObject, sizeof(PerObject));
					_dDepthMaterial->m_Shader->SetStruct(DIRECTIONAL_LIGHTS, directionalLights.data());
					_dDepthMaterial->m_Shader->SetInt(USE_ALPHA_MAP, false);

					if (meshRenderer.materials.size() > i)
					{
						if (meshRenderer.materials[i] != nullptr)
						{
							auto iter = meshRenderer.materials[i]->GetTextures().find(G_ALPHA);

							if (iter != meshRenderer.materials[i]->GetTextures().end())
							{
								_dDepthMaterial->m_Shader->SetInt(USE_ALPHA_MAP, true);
								_dDepthMaterial->SetTexture(G_ALPHA, iter->second.Texture);
							}
						}
					}

					_dDepthMaterial->m_Shader->UnmapConstantBuffer(renderer.GetContext());

					renderer.Submit(*meshRenderer.mesh, *_dDepthMaterial, i, PrimitiveTopology::TRIANGLELIST, 1);
				}
			}
		}


		Texture* plTextures[1] =
		{
			_pLightDepthTextureRTs.get()
		};
		renderer.SetViewport(1024, 1024);
		renderer.SetRenderTargets(1, plTextures, _pLightDepthTexture.get(), false);
		renderer.Clear(_pLightDepthTexture->GetClearValue());

		for (int pLightIndex = 0; pLightIndex < pointLights.size(); pLightIndex++)
		{
			if (!pointLights[pLightIndex].isOn || !pointLights[pLightIndex].useShadow)
				continue;

			for (auto&& [entity, transform, meshRenderer] : registry.view<core::WorldTransform, core::MeshRenderer>().each())
			{
				if (!meshRenderer.isOn || !meshRenderer.receiveShadow)
					continue;

				if (meshRenderer.mesh == nullptr)
				{
					meshRenderer.mesh = renderer.GetMesh(meshRenderer.meshString);
					continue;
				}

				auto meshPosition = transform.position;
				auto lightPosition = pointLights[pLightIndex].position;

				auto distance = Vector3::Distance(meshPosition, lightPosition);
				if (distance > pointLights[pLightIndex].range)
					continue;

				if (!meshRenderer.isCulling)
					renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::PSHADOW_CULL_NONE, DepthStencilState::DEPTH_ENABLED);
				else
					renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::PSHADOW, DepthStencilState::DEPTH_ENABLED);

				auto world = transform.matrix;
				auto worldInvTranspose = world.Invert().Transpose();

				perObject.gProj = proj;
				perObject.gView = view;
				perObject.gViewProj = viewProj;
				perObject.gWorld = world;
				perObject.gWorldInvTranspose = worldInvTranspose;
				perObject.gTexTransform = Matrix::Identity;

				_pDepthMaterial->m_Shader->MapConstantBuffer(renderer.GetContext());
				_pDepthMaterial->m_Shader->SetConstant(CB_PER_OBJECT, &perObject, sizeof(PerObject));
				_pDepthMaterial->m_Shader->SetStruct(POINT_LIGHTS, pointLights.data());
				_pDepthMaterial->m_Shader->SetInt(NUM_POINT_LIGHTS, static_cast<int>(pointLights.size()));
				_pDepthMaterial->m_Shader->UnmapConstantBuffer(renderer.GetContext());

				for (uint32_t i = 0; i < meshRenderer.mesh->subMeshCount; i++)
					renderer.Submit(*meshRenderer.mesh, *_pDepthMaterial, i, PrimitiveTopology::TRIANGLELIST, 1);
			}
		}

		renderer.SetViewport(1024, 1024);
		renderer.SetRenderTargets(0, nullptr, _sLightDepthTexture.get(), false);
		renderer.Clear(_sLightDepthTexture->GetClearValue());

		for (int sLightIndex = 0; sLightIndex < spotLights.size(); sLightIndex++)
		{
			if (!spotLights[sLightIndex].isOn || !spotLights[sLightIndex].useShadow)
				continue;

			for (auto&& [entity, transform, meshRenderer] : registry.view<core::WorldTransform, core::MeshRenderer>().each())
			{
				if (!meshRenderer.isOn || !meshRenderer.receiveShadow)
					continue;

				if (meshRenderer.mesh == nullptr)
				{
					meshRenderer.mesh = renderer.GetMesh(meshRenderer.meshString);
					continue;
				}

				auto meshPosition = transform.position;
				auto lightPosition = spotLights[sLightIndex].position;

				auto distance = Vector3::Distance(meshPosition, lightPosition);
				if (distance > spotLights[sLightIndex].range)
					continue;

				if (!meshRenderer.isCulling)
					renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::SHADOW_CULL_NONE, DepthStencilState::DEPTH_ENABLED);
				else
					renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::SHADOW, DepthStencilState::DEPTH_ENABLED);

				auto world = transform.matrix;
				auto worldInvTranspose = world.Invert().Transpose();

				perObject.gProj = proj;
				perObject.gView = view;
				perObject.gViewProj = viewProj;
				perObject.gWorld = world;
				perObject.gWorldInvTranspose = worldInvTranspose;
				perObject.gTexTransform = Matrix::Identity;

				_sDepthMaterial->m_Shader->MapConstantBuffer(renderer.GetContext());
				_sDepthMaterial->m_Shader->SetConstant("cbPerObject", &perObject, sizeof(PerObject));
				_sDepthMaterial->m_Shader->SetStruct("spotLights", spotLights.data());
				_sDepthMaterial->m_Shader->SetInt("numSpotLights", static_cast<int>(spotLights.size()));
				_sDepthMaterial->m_Shader->UnmapConstantBuffer(renderer.GetContext());

				for (uint32_t i = 0; i < meshRenderer.mesh->subMeshCount; i++)
					renderer.Submit(*meshRenderer.mesh, *_sDepthMaterial, i, PrimitiveTopology::TRIANGLELIST, 1);
			}
		}
	}


	// deferred Shading Pass
	{
		renderer.SetViewport(renderResources.width, renderResources.height);
		Texture* textures[] = { renderResources.renderTarget.get() };
		renderer.SetRenderTargets(1, textures, nullptr, false);
		renderer.Clear(renderResources.renderTarget->GetClearValue());

		renderer.ApplyRenderState(BlendState::COUNT, RasterizerState::COUNT, DepthStencilState::COUNT);

		auto world = Matrix::Identity;

		auto worldInvTranspose = world.Invert().Transpose();

		_deferredMaterial->m_Shader->MapConstantBuffer(renderer.GetContext());
		_deferredMaterial->m_Shader->SetMatrix("gWorld", world);
		_deferredMaterial->m_Shader->SetMatrix("gWorldInvTranspose", worldInvTranspose);
		_deferredMaterial->m_Shader->SetMatrix("gView", view);
		_deferredMaterial->m_Shader->SetMatrix("gViewProj", viewProj);
		_deferredMaterial->m_Shader->SetFloat3("gEyePosW", cameraTransform.position);
		_deferredMaterial->m_Shader->SetMatrix("gTexTransform", Matrix::Identity);

		//_deferredMaterial->SetTexture("이름", 텍스쳐);

		_deferredMaterial->m_Shader->SetStruct("directionalLights", directionalLights.data());
		_deferredMaterial->m_Shader->SetStruct("pointLights", pointLights.data());
		_deferredMaterial->m_Shader->SetStruct("nonShadowPointLights", nonShadowPointLights.data());
		_deferredMaterial->m_Shader->SetStruct("spotLights", spotLights.data());
		_deferredMaterial->m_Shader->SetInt("numDirectionalLights", static_cast<int>(directionalLights.size()));
		_deferredMaterial->m_Shader->SetInt("numPointLights", static_cast<int>(pointLights.size()));
		_deferredMaterial->m_Shader->SetInt("numSpotLights", static_cast<int>(spotLights.size()));
		_deferredMaterial->m_Shader->SetFloat2("gScreenSize", { static_cast<float>(renderResources.width), static_cast<float>(renderResources.height) });

		_deferredMaterial->m_Shader->UnmapConstantBuffer(renderer.GetContext());

		renderer.Submit(*renderResources.quadMesh, *_deferredMaterial, 0, PrimitiveTopology::TRIANGLELIST, 1);
	}
}

void core::DeferredShadePass::Finish()
{
	_deferredMaterial.reset();

	_decalOutputAlbedo.reset();

	_irradianceTexture.reset();
	_prefilteredTexture.reset();
	_brdfTexture.reset();


	_dDepthMaterial.reset();
	_sDepthMaterial.reset();
	_pDepthMaterial.reset();

	_dLightDepthTexture.reset();
	_sLightDepthTexture.reset();
	_pLightDepthTexture.reset();

}
