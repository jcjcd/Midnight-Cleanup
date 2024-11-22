#include "pch.h"
#include "PostRenderSystem.h"

#include "Scene.h"
#include "RenderComponents.h"
#include "CoreComponents.h"

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

	struct BloomParams
	{
		Vector4 bloomTint;
		float threshold;
		float intensity;
		int useBloom;
		int bloomCurveType;
	};

	struct FogParams
	{
		Vector4 fogColor;
		float fogDensity;
		float fogStart;
		float fogRange;
		int useFog;
	};

	struct ExposureParams
	{
		bool useExposure;
		float exposure;
		float constrast;
		float saturation;
	};

	struct cbWaterMatrix
	{
		Matrix gReflectionView;
		Matrix gWindDirection;

		float gWaveLength;
		float gWaveHeight;

		float gTime;
		float gWindForce;
		float gDrawMode;

		int gFresnelMode;

		float gSpecPerturb;
		float gSpecPower;

		float gDullBlendFactor;

		int gEnableTextureBlending;
	};

}

core::PostRenderSystem::PostRenderSystem(Scene& scene)
	: ISystem(scene)
{
	_dispatcher = scene.GetDispatcher();
	_dispatcher->sink<OnStartSystem>().connect<&PostRenderSystem::startSystem>(this);
	_dispatcher->sink<OnFinishSystem>().connect<&PostRenderSystem::finishSystem>(this);

	_dispatcher->sink<OnDestroyRenderResources>().connect<&PostRenderSystem::destroyRenderResources>(this);
	_dispatcher->sink<OnLateCreateRenderResources>().connect<&PostRenderSystem::lateCreateRenderResources>(this);

}

core::PostRenderSystem::~PostRenderSystem()
{
	_dispatcher->disconnect(this);
}

void core::PostRenderSystem::PostRender(Scene& scene, Renderer& renderer, float tick)
{
	float widthRatio = _width / _defaultWidth;
	float heightRatio = _height / _defaultHeight;

	auto& registry = *scene.GetRegistry();
	auto& renderRes = scene.GetRegistry()->ctx().get<core::RenderResources>();
	const Matrix& view = renderRes.viewMatrix;
	const Matrix& proj = renderRes.projectionMatrix;
	const Matrix& viewProj = renderRes.viewProjectionMatrix;
	auto cameraTransform = renderRes.cameraTransform;
	auto mainCamera = renderRes.mainCamera;

	Vector3 camPos = Vector3::Zero;
	Matrix camMatrix = Matrix::Identity;

	if (cameraTransform)
	{
		camPos = cameraTransform->position;
		camMatrix = cameraTransform->matrix;
	}




	auto& deferredTextures = renderRes.deferredTextures;

	auto _renderTarget = renderRes.renderTarget;

	DirectX::BoundingFrustum frustum;
	DirectX::BoundingFrustum::CreateFromMatrix(frustum, proj);
	frustum.Transform(frustum, view.Invert());

	_oitPass.oitPass(scene, renderer, view, proj, viewProj, camPos);

	_deferredShadePass.Run(scene, renderer, tick, renderRes);

	if (renderRes.mainCamera)
	{
		Texture* textures[] = { _renderTarget.get() };
		renderer.SetRenderTargets(1, textures, _renderResources->depthTexture.get(), false);
		std::filesystem::path path = "./Resources/Materials/DefaultSky.material";
		if (auto skyMaterial = renderer.GetMaterial(path.string()))
		{
			if (auto skyShader = skyMaterial->GetShader())
			{
				if (auto skyMesh = renderer.GetMesh("Sphere"))
				{
					renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_FRONT, DepthStencilState::DEPTH_ENABLED_LESS_EQUAL);
					skyShader->MapConstantBuffer(renderer.GetContext());
					skyShader->SetMatrix("gWorld", Matrix::CreateScale(2000));
					skyShader->SetMatrix("gTexTransform", Matrix::Identity);
					skyShader->SetFloat3("gEyePosW", camPos);
					skyShader->SetMatrix("gViewProj", viewProj);
					skyShader->UnmapConstantBuffer(renderer.GetContext());

					renderer.Submit(*skyMesh, *skyMaterial, 0, PrimitiveTopology::TRIANGLELIST, 1);
				}
			}
		}
	}

	_oitPass.oitCompositePass(scene, renderer, view, proj, viewProj, _renderResources->quadMesh, _renderTarget);

	/// 여기에 particle pass 추가
	_particleSystemPass.initParticles(scene, renderer, tick);
	_particleSystemPass.renderParticles(scene, renderer, tick, view, proj, _renderResources->renderTarget, _renderResources->depthTexture);

	{
		for (auto&& [entity, world, text] : registry.view<core::WorldTransform, core::Text>().each())
		{
			if (!text.isOn)
				continue;

			float rtClearColor[4] = { 0.0f,0.0f,0.0f,1.0f };

			if (text.font == nullptr || _textTexture == nullptr)
			{
				text.font = renderer.GetFont(text.fontString);
				std::string entityStr = std::to_string(static_cast<uint32_t>(entity));
				_textTexture = renderer.CreateEmptyTexture("TxtTexture", Texture::Type::Texture2D, _width, _height, 1, Texture::Format::R8G8B8A8_UNORM, Texture::Usage::RTV, rtClearColor);
				continue;
			}

			renderer.ClearTexture(_textTexture.get(), rtClearColor);
			renderer.SubmitText(_textTexture.get(), text.font.get(), text.text, text.position, text.size, text.scale, text.color, text.fontSize, static_cast<TextAlign>(text.textAlign), static_cast<TextBoxAlign>(text.textBoxAlign), text.useUnderline, text.useStrikeThrough);

			Texture* textures[] = { _renderTarget.get() };
			renderer.SetRenderTargets(1, textures, renderRes.depthTexture.get(), false);
			renderer.ApplyRenderState(BlendState::ALPHA_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

			auto&& uiShader = _uiMaterial->GetShader();

			Matrix worldMatrix = world.matrix;

			worldMatrix = Matrix::CreateScale(world.scale) * worldMatrix;
			_uiMaterial->SetTexture("gDiffuseMap", _textTexture);
			uiShader->MapConstantBuffer(renderer.GetContext());
			uiShader->SetMatrix("gWorld", worldMatrix);
			uiShader->SetMatrix("gViewProj", view * proj);
			uiShader->SetFloat("layerDepth", 0.0f);
			uiShader->SetFloat4("gColor", { 1.0f,1.0f,1.0f,1.0f });
			uiShader->SetInt("isOn", text.isOn);
			uiShader->SetInt("is3D", true);
			uiShader->SetFloat("maskProgress", 1);
			uiShader->SetInt("maskMode", 0);
			uiShader->UnmapConstantBuffer(renderer.GetContext());

			renderer.Submit(*renderRes.quadMesh, *_uiMaterial);
		}
	}

	{
		Texture* textures[] = { _renderTarget.get() };
		renderer.SetRenderTargets(1, textures, _renderResources->depthTexture.get(), false);
		renderer.ApplyRenderState(BlendState::ALPHA_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

		auto&& uiShader = _uiMaterial->GetShader();

		for (auto&& [entity, world, common, ui] : registry.view<core::WorldTransform, core::UICommon, core::UI3D>().each())
		{
			if (common.texture == nullptr)
			{
				common.texture = renderer.GetUITexture(common.textureString);
				continue;
			}

			if (common.isOn == false)
				continue;

			_uiMaterial->SetTexture("gDiffuseMap", common.texture);
			uiShader->MapConstantBuffer(renderer.GetContext());
			uiShader->SetMatrix("gWorld", world.matrix);
			uiShader->SetMatrix("gViewProj", view * proj);
			uiShader->SetFloat("layerDepth", 0.0f);
			uiShader->SetFloat4("gColor", common.color);
			uiShader->SetInt("isOn", common.isOn);
			uiShader->SetInt("is3D", true);
			uiShader->SetFloat("maskProgress", common.percentage);
			uiShader->SetInt("maskMode", static_cast<int>(common.maskingOption));
			uiShader->UnmapConstantBuffer(renderer.GetContext());

			renderer.Submit(*_renderResources->quadMesh, *_uiMaterial);
		}
	}


	{
		{
			Texture* textures[] = { _tempRT.get() };
			renderer.SetRenderTargets(1, textures, nullptr, false);
			renderer.Clear(_tempRT->GetClearValue());
			renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_DISABLED);
			auto&& quadshader = _renderResources->quadMaterial->GetShader();

			_renderResources->quadMaterial->SetTexture("gDiffuseMap", _renderTarget);
			quadshader->MapConstantBuffer(renderer.GetContext());
			quadshader->SetMatrix("gWorld", Matrix::Identity);
			quadshader->SetMatrix("gView", Matrix::Identity);
			quadshader->SetMatrix("gViewProj", Matrix::Identity);
			quadshader->SetMatrix("gTexTransform", Matrix::Identity);
			quadshader->UnmapConstantBuffer(renderer.GetContext());

			renderer.Submit(*_renderResources->quadMesh, *_renderResources->quadMaterial, PrimitiveTopology::TRIANGLELIST, 1);
		}

		static float elapsedTime = 0.0f;
		elapsedTime += tick;
		cbWaterMatrix cbWater;
		cbWater.gDrawMode = 1.0f;
		cbWater.gFresnelMode = 0;
		cbWater.gWaveLength = 0.5f;
		cbWater.gWaveHeight = 0.2f;
		cbWater.gTime = elapsedTime;
		cbWater.gWindForce = 0.05f;
		cbWater.gSpecPerturb = 4.f;
		cbWater.gSpecPower = 364.f;
		cbWater.gDullBlendFactor = 0.2f;
		cbWater.gWindDirection = Matrix::Identity;
		cbWater.gEnableTextureBlending = 1;

		Matrix reflectionViewMatrix;
		Vector3 cameraPosition = camPos;
		Vector3 targetPos = cameraPosition + camMatrix.Backward() * 100.0f;

		PerObject perObject;

		Texture* textures[] = { _renderTarget.get() };
		renderer.SetRenderTargets(1, textures, renderRes.depthTexture.get(), false);
		renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_NONE, DepthStencilState::DEPTH_ENABLED);

		for (auto&& [entity, transform, meshRenderer, renderAttributes] : registry.view<core::WorldTransform, core::MeshRenderer, core::RenderAttributes>().each())
		{
			if (!meshRenderer.isOn)
				continue;

			if (!(renderAttributes.flags & core::RenderAttributes::Flag::IsWater))
				continue;

			if (meshRenderer.mesh == nullptr)
			{
				meshRenderer.mesh = renderer.GetMesh(meshRenderer.meshString);
				continue;
			}

			if (meshRenderer.materials.size() <= 0)
			{
				for (auto&& materialString : meshRenderer.materialStrings)
				{
					meshRenderer.materials.push_back(renderer.GetMaterial(materialString));
				}
				continue;
			}

			auto& world = transform.matrix;
			auto worldInvTranspose = world.Invert().Transpose();

			for (uint32_t i = 0; i < (uint32_t)meshRenderer.materials.size(); ++i)
			{
				auto material = meshRenderer.materials[i];
				if (material == nullptr)
				{
					continue;
				}
				if (i >= meshRenderer.mesh->subMeshCount)
				{
					OutputDebugStringA(std::to_string(uint32_t(entity)).c_str());
					OutputDebugStringA("「Submesh index out of range」\n");
					continue;
				}

				perObject.gProj = proj;
				perObject.gView = view;
				perObject.gViewProj = viewProj;
				perObject.gWorld = world;
				perObject.gWorldInvTranspose = worldInvTranspose;
				perObject.gTexTransform = Matrix::Identity;

				float waterHeight = transform.position.y;
				float reflectionCamYCoord = -cameraPosition.y + 2 * waterHeight;
				Vector3 reflectionCamPosition = Vector3(cameraPosition.x, reflectionCamYCoord, cameraPosition.z);

				float reflectionTargetYCoord = -targetPos.y + 2 * waterHeight;
				Vector3 reflectionCamTarget = Vector3(targetPos.x, reflectionTargetYCoord, targetPos.z);

				Vector3 forwardVector = reflectionCamTarget - reflectionCamPosition;
				Vector3 sideVector = camMatrix.Right();
				Vector3 reflectionCamUp = DirectX::XMVector3Cross(forwardVector, sideVector);
				//DirectX::XMVector3Cross(sideVector, forwardVector)

				reflectionViewMatrix = DirectX::XMMatrixLookAtLH(reflectionCamPosition, reflectionCamTarget, reflectionCamUp);
				cbWater.gReflectionView = reflectionViewMatrix;

				material->SetTexture("RefractionMap", _tempRT);
				material->m_Shader->MapConstantBuffer(renderer.GetContext());
				material->m_Shader->SetConstant("cbPerObject", &perObject, sizeof(PerObject));
				material->m_Shader->SetConstant("cbWaterMatrix", &cbWater, sizeof(cbWaterMatrix));
				material->m_Shader->SetFloat3("gEyePosW", cameraPosition);
				material->m_Shader->SetInt("gRecieveDecal", meshRenderer.canReceivingDecal);
				material->m_Shader->SetInt("gReflect", false);
				material->m_Shader->SetInt("gRefract", false);

				material->m_Shader->UnmapConstantBuffer(renderer.GetContext());

				renderer.Submit(*meshRenderer.mesh, *material, i, PrimitiveTopology::TRIANGLELIST, 1);




			}
		}
	}

	/// 여기에 아웃라인 패스
	outlinePass(scene, renderer, tick, view, proj, frustum);

	/// 여기에 bloom 추가
	_bloomPass.Run(scene, renderer, tick, renderRes);

	/// post proccesing
	{
		Texture* ppTextures[] = { _tempRT.get() };
		renderer.SetRenderTargets(1, ppTextures, nullptr, false);
		renderer.Clear(_tempRT->GetClearValue());
		renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_DISABLED);

		auto&& postProcessingShader = _postProcessingMaterial->GetShader();

		for (auto&& [entity, volume] : registry.view<core::PostProcessingVolume>().each())
		{
			BloomParams bloomParams = {};
			FogParams fogParams = {};
			ExposureParams exposureParams = {};

			bloomParams.bloomTint = volume.bloomTint;
			bloomParams.threshold = volume.bloomThreshold;
			bloomParams.intensity = volume.bloomIntensity;
			bloomParams.useBloom = volume.useBloom;
			bloomParams.bloomCurveType = static_cast<int>(volume.bloomCurve);

			fogParams.fogColor = volume.fogColor;
			fogParams.fogDensity = volume.fogDensity;
			fogParams.fogStart = volume.fogStart;
			fogParams.fogRange = volume.fogRange;
			fogParams.useFog = volume.useFog;

			exposureParams.useExposure = volume.useExposure;
			exposureParams.exposure = volume.exposure;
			exposureParams.constrast = volume.contrast;
			exposureParams.saturation = volume.saturation;

			_postProcessingMaterial->SetTexture("gSrcTexture", _renderTarget);
			_postProcessingMaterial->SetTexture("gBloomTexture", _bloomPass.GetBloomTexture());
			_postProcessingMaterial->SetTexture("gPositionW", _renderResources->deferredTextures[DeferredSlot::PositionW]);

			postProcessingShader->MapConstantBuffer(renderer.GetContext());
			postProcessingShader->SetMatrix("gWorld", Matrix::Identity);
			postProcessingShader->SetMatrix("gView", Matrix::Identity);
			postProcessingShader->SetMatrix("gViewProj", Matrix::Identity);
			postProcessingShader->SetMatrix("gTexTransform", Matrix::Identity);

			postProcessingShader->SetConstant("cbBloom", &bloomParams, sizeof(BloomParams));
			postProcessingShader->SetConstant("cbFog", &fogParams, sizeof(FogParams));
			postProcessingShader->SetConstant("cbColorAdjust", &exposureParams, sizeof(ExposureParams));
			postProcessingShader->UnmapConstantBuffer(renderer.GetContext());

			renderer.Submit(*_renderResources->quadMesh, *_postProcessingMaterial);

			break;
		}

		Texture* textures[] = { _renderTarget.get() };
		renderer.SetRenderTargets(1, textures, nullptr, false);
		renderer.Clear(_renderTarget->GetClearValue());
		renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_DISABLED);

		if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		{
			auto&& quadshader = _renderResources->quadMaterial->GetShader();
			_renderResources->quadMaterial->SetTexture("gDiffuseMap", _tempRT);
			quadshader->MapConstantBuffer(renderer.GetContext());
			quadshader->SetMatrix("gWorld", Matrix::Identity);
			quadshader->SetMatrix("gView", Matrix::Identity);
			quadshader->SetMatrix("gViewProj", Matrix::Identity);
			quadshader->SetMatrix("gTexTransform", Matrix::Identity);

			quadshader->UnmapConstantBuffer(renderer.GetContext());

			renderer.Submit(*_renderResources->quadMesh, *_renderResources->quadMaterial, PrimitiveTopology::TRIANGLELIST, 1);
		}
		else
		{
			auto&& fxaaShader = _fxaaMaterial->GetShader();
			_fxaaMaterial->SetTexture("screenTexture", _tempRT);
			fxaaShader->MapConstantBuffer(renderer.GetContext());
			fxaaShader->SetMatrix("gWorld", Matrix::Identity);
			fxaaShader->SetMatrix("gView", Matrix::Identity);
			fxaaShader->SetMatrix("gViewProj", Matrix::Identity);
			fxaaShader->SetMatrix("gTexTransform", Matrix::Identity);

			fxaaShader->SetFloat2("screenTextureSize", { static_cast<float>(_width), static_cast<float>(_height) });
			fxaaShader->SetFloat2("inverseScreenSize", { 1.0f / static_cast<float>(_width), 1.0f / static_cast<float>(_height) });
			fxaaShader->SetInt("enableFXAA", 1);
			fxaaShader->UnmapConstantBuffer(renderer.GetContext());

			renderer.Submit(*_renderResources->quadMesh, *_fxaaMaterial, PrimitiveTopology::TRIANGLELIST, 1);
		}
	}

	{
		auto comp = [](const uint32_t& lhs, const uint32_t& rhs) {
			return lhs > rhs;
			};

		std::vector<entt::entity> bgs;
		std::map<uint32_t, std::vector<entt::entity>, std::function<bool(uint32_t, uint32_t)>> uis(comp);

		Texture* textures[] = { _renderTarget.get() };
		renderer.SetRenderTargets(1, textures, _renderResources->depthTexture.get(), false);
		renderer.ClearTexture(_renderResources->depthTexture.get(), _renderResources->depthTexture->GetClearValue());
		auto&& uiShader = _uiMaterial->GetShader();

		for (auto&& [entity, common, ui] : registry.view<core::UICommon, core::UI2D>().each())
		{
			if (common.isOn == false)
				continue;

			auto& texture = common.texture;

			if (common.texture == nullptr || texture != renderer.GetUITexture(common.textureString))
			{
				common.texture = renderer.GetUITexture(common.textureString);
				continue;
			}

			if (core::Button* button = registry.try_get<core::Button>(entity))
			{
				if (core::CheckBox* checkBox = registry.try_get<core::CheckBox>(entity))
				{
					if (checkBox->isChecked)
					{
						common.textureString = checkBox->checkedTextureString;
						common.texture = renderer.GetUITexture(checkBox->checkedTextureString);
					}
					else
					{
						common.textureString = checkBox->uncheckedTextureString;
						common.texture = renderer.GetUITexture(checkBox->uncheckedTextureString);
					}
				}
				else
				{
					if (button->defaultTextureString.empty())
						button->defaultTextureString = common.textureString;

					if (button->isHovered && !button->isPressed)
					{
						common.color.w = 1.0f;
						if (common.texture != button->highlightTexture)
						{
							if (!button->highlightTextureString.empty())
							{
								common.textureString = button->highlightTextureString;

								if (button->highlightTexture == nullptr)
									common.texture = renderer.GetUITexture(button->highlightTextureString);
							}

							if (button->highlightTexture)
								common.texture = button->highlightTexture;
						}
					}
					if (button->isPressed)
					{
						common.color.w = 1.0f;
						if (common.texture != button->selectedTexture)
						{
							if (!button->selectedTextureString.empty())
							{
								common.textureString = button->selectedTextureString;

								if (button->selectedTexture == nullptr)
									common.texture = renderer.GetUITexture(button->selectedTextureString);
							}

							if (button->selectedTexture)
								common.texture = button->selectedTexture;
						}
					}
					if (!button->isHovered && !button->isPressed)
					{
						common.textureString = button->defaultTextureString;

						if (core::Text* text = registry.try_get<core::Text>(entity))
						{
							core::ComboBox* comboBox = registry.try_get<core::ComboBox>(entity);

							if (!comboBox)
								common.color.w = 0.0f;
						}
					}
				}
			}

			if (ui.isCanvas)
				bgs.emplace_back(entity);
			else
			{
				auto findIt = uis.find(ui.layerDepth);
				if (findIt != uis.end())
					findIt->second.emplace_back(entity);
				else
				{
					std::vector<entt::entity> curUIs;
					curUIs.emplace_back(entity);
					uis[ui.layerDepth] = curUIs;
				}
			}
		}

		renderer.ApplyRenderState(BlendState::ALPHA_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);
		for (auto& entity : bgs)
		{
			core::UICommon& common = registry.get<core::UICommon>(entity);
			core::UI2D& ui = registry.get<core::UI2D>(entity);

			_uiMaterial->SetTexture("gDiffuseMap", common.texture);
			uiShader->MapConstantBuffer(renderer.GetContext());

			Matrix uiNDC = Matrix::Identity;

			uiNDC *= Matrix::CreateRotationZ(ui.rotation * DirectX::XM_PI / 180.0f);
			uiNDC *= Matrix::CreateScale(ui.size.x / _width * widthRatio, ui.size.y / _height * heightRatio, 1.0f);
			uiNDC *= Matrix::CreateTranslation(ui.position.x * 2 / _width * heightRatio, ui.position.y * 2 / _height * heightRatio, 0);

			uiShader->SetMatrix("gWorld", uiNDC);
			uiShader->SetMatrix("gViewProj", Matrix::Identity);

			uiShader->SetFloat("layerDepth", ui.layerDepth / 100.0f);
			uiShader->SetFloat4("gColor", common.color);
			uiShader->SetInt("isOn", common.isOn);
			uiShader->SetInt("is3D", false);
			uiShader->SetFloat("maskProgress", common.percentage);
			uiShader->SetInt("maskMode", static_cast<int>(common.maskingOption));
			uiShader->UnmapConstantBuffer(renderer.GetContext());

			renderer.Submit(*_renderResources->quadMesh, *_uiMaterial);
		}

		renderer.ApplyRenderState(BlendState::ALPHA_BLEND, RasterizerState::CULL_BACK, DepthStencilState::NO_DEPTH_WRITE);
		for (auto& vec : uis | std::views::values)
		{
			for (auto& entity : vec)
			{
				core::UICommon& common = registry.get<core::UICommon>(entity);
				core::UI2D& ui = registry.get<core::UI2D>(entity);

				_uiMaterial->SetTexture("gDiffuseMap", common.texture);
				uiShader->MapConstantBuffer(renderer.GetContext());

				Matrix uiNDC = Matrix::Identity;

				uiNDC *= Matrix::CreateRotationZ(ui.rotation * DirectX::XM_PI / 180.0f);
				uiNDC *= Matrix::CreateScale(ui.size.x / _width * widthRatio, ui.size.y / _height * heightRatio, 1.0f);
				uiNDC *= Matrix::CreateTranslation(ui.position.x * 2 / _width * widthRatio, ui.position.y * 2 / _height * heightRatio, 0);

				uiShader->SetMatrix("gWorld", uiNDC);
				uiShader->SetMatrix("gViewProj", Matrix::Identity);

				uiShader->SetFloat("layerDepth", ui.layerDepth / 100.0f);
				uiShader->SetFloat4("gColor", common.color);
				uiShader->SetInt("isOn", common.isOn);
				uiShader->SetInt("is3D", false);
				uiShader->SetFloat("maskProgress", common.percentage);
				uiShader->SetInt("maskMode", static_cast<int>(common.maskingOption));
				uiShader->UnmapConstantBuffer(renderer.GetContext());

				renderer.Submit(*_renderResources->quadMesh, *_uiMaterial);
			}
		}
	}

	auto& config = registry.ctx().get<core::Configuration>();

	for (auto&& [entity, text] : registry.view<core::Text>(entt::exclude_t<core::WorldTransform, core::ComboBox>()).each())
	{
		if (text.isOn == false)
			continue;

		if (text.font == nullptr)
		{
			text.font = renderer.GetFont(text.fontString);
			continue;
		}

		Vector2 textPos = { text.position.x * widthRatio,text.position.y * heightRatio };
		textPos.x += config.width / 2.0f;
		textPos.y += config.height / 2.0f;
		textPos.x += text.leftPadding;

		renderer.SubmitText(_renderTarget.get(), text.font.get(), text.text, textPos, text.size, text.scale, text.color, text.fontSize, static_cast<TextAlign>(text.textAlign), static_cast<TextBoxAlign>(text.textBoxAlign), text.useUnderline, text.useStrikeThrough);
	}

	for (auto&& [entity, text, comboBox] : registry.view<core::Text, core::ComboBox>(entt::exclude_t<core::WorldTransform>()).each())
	{
		if (text.isOn == false)
			continue;

		if (text.font == nullptr)
		{
			text.font = renderer.GetFont(text.fontString);
			continue;
		}

		Vector2 textPos = { text.position.x * widthRatio,text.position.y * heightRatio };
		textPos.x += config.width / 2.0f;
		textPos.y += config.height / 2.0f;
		textPos.x += text.leftPadding;

		Vector2 tempSize = { text.size.x * widthRatio, text.size.y * heightRatio };

		renderer.SubmitComboBox(_renderTarget.get(), text.font.get(), reinterpret_cast<std::vector<std::string>&>(comboBox.comboBoxTexts), comboBox.curIndex, comboBox.isOn, textPos, text.leftPadding, tempSize, text.scale, text.color, text.fontSize, static_cast<TextAlign>(text.textAlign), static_cast<TextBoxAlign>(text.textBoxAlign));
	}

	{
		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderer.SetRenderTargets(1, nullptr);
		renderer.Clear(clearColor);

		renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

		auto&& quadshader = _renderResources->quadMaterial->GetShader();
		_renderResources->quadMaterial->SetTexture("gDiffuseMap", _renderTarget);
		quadshader->MapConstantBuffer(renderer.GetContext());
		quadshader->SetMatrix("gWorld", Matrix::Identity);
		quadshader->SetMatrix("gView", Matrix::Identity);
		quadshader->SetMatrix("gViewProj", Matrix::Identity);
		quadshader->SetMatrix("gTexTransform", Matrix::Identity);

		quadshader->UnmapConstantBuffer(renderer.GetContext());

		renderer.Submit(*_renderResources->quadMesh, *_renderResources->quadMaterial, PrimitiveTopology::TRIANGLELIST, 1);
	}
}

void core::PostRenderSystem::startSystem(const OnStartSystem& event)
{
	auto&& renderResources = event.scene->GetRegistry()->ctx().get<core::RenderResources>();

	//auto& deferredTextures = renderResources.deferredTextures;
	_renderResources = &renderResources;
	/// 일단 실행하려고 테스트
	//_deferredMaterial->SetTexture("reflectionTexture", _dummyTexture);

	_width = renderResources.width;
	_height = renderResources.height;

	_deferredShadePass.Init(*event.scene, *event.renderer, _width, _height);
	_uiMaterial = Material::Create(event.renderer->LoadShader("./Shaders/UI.hlsl"));


	initOutline(*event.scene, *event.renderer);
	_oitPass.initOITResources(*event.scene, *event.renderer, _width, _height);
	_particleSystemPass.startParticlePass(*event.scene, *event.renderer);
	_bloomPass.Init(*event.scene, *event.renderer, _width, _height);

	_postProcessingMaterial = Material::Create(event.renderer->LoadShader("./Shaders/PostProcessing.hlsl"));

	_fxaaMaterial = Material::Create(event.renderer->LoadShader("./Shaders/fxaa.hlsl"));

	_tempRT = event.renderer->CreateEmptyTexture("PostProcessingRT", Texture::Type::Texture2D, _width, _height, 1, Texture::Format::R8G8B8A8_UNORM, Texture::Usage::RTV);
}

void core::PostRenderSystem::finishSystem(const OnFinishSystem& event)
{
	_bloomPass.Finish();

	_uiMaterial.reset();

	_deferredShadePass.Finish();

	finishOutline();

	_oitPass.finishOITResources();

	_particleSystemPass.finishParticlePass(*event.scene);

	event.scene->GetRegistry()->ctx().erase<core::RenderResources>();
}

void core::PostRenderSystem::lateCreateRenderResources(const OnLateCreateRenderResources& event)
{
	startSystem(OnStartSystem{ (*event.scene), event.renderer });
}

void core::PostRenderSystem::destroyRenderResources(const OnDestroyRenderResources& event)
{
	finishSystem(OnFinishSystem{ (*event.scene), event.renderer });
}

void core::PostRenderSystem::initOutline(Scene& scene, Renderer& renderer)
{
	float pickingClearColor[] = { -1, -1, -1, -1 };
	_pickingTexture = renderer.CreateEmptyTexture("PickingRT", Texture::Type::Texture2D, _width, _height, 1, Texture::Format::R32_SINT, Texture::Usage::RTV, pickingClearColor);
	_pickingMaterial = Material::Create(renderer.LoadShader("./Shaders/pickingVSPS.hlsl"));
	_outlineComputeMaterial = Material::Create(renderer.LoadShader("./Shaders/CS_outline.hlsl"));

	_outlineComputeMaterial->SetTexture("pickingRTV", _pickingTexture);
	_outlineComputeMaterial->SetUAVTexture("finalRenderTarget", _renderResources->renderTarget);
}

void core::PostRenderSystem::finishOutline()
{
	_pickingTexture.reset();
	_pickingMaterial.reset();
	_outlineComputeMaterial.reset();
}

void core::PostRenderSystem::outlinePass(Scene& scene, Renderer& renderer, float tick, const Matrix& view, const Matrix& proj, const DirectX::BoundingFrustum& frustum)
{
	if(!scene.IsPlaying())
		return;

	auto& registry = *scene.GetRegistry();

	std::vector<entt::entity> selectedEntities;

	{
		Texture* textures[] = { _pickingTexture.get() };
		renderer.SetRenderTargets(1, textures, _renderResources->depthTexture.get(), false);
		float R32ClearColor[] = { -1, -1, -1, -1 };
		renderer.Clear(R32ClearColor);
		renderer.ApplyRenderState(BlendState::NO_BLEND, RasterizerState::CULL_BACK, DepthStencilState::DEPTH_ENABLED);

		for (auto&& [entity, transform, meshRenderer, renderAttribute] : registry.view<core::WorldTransform, core::MeshRenderer, core::RenderAttributes>().each())
		{
			if (meshRenderer.mesh == nullptr)
			{
				continue;
			}

			DirectX::BoundingBox boundingBox = meshRenderer.mesh->boundingBox;
			boundingBox.Transform(boundingBox, transform.matrix);
			if (frustum.Intersects(boundingBox) == DirectX::DISJOINT)
				continue;

			if (meshRenderer.materials.size() <= 0)
			{
				continue;
			}

			auto& world = transform.matrix;

			auto worldInvTranspose = world.Invert().Transpose();
			Matrix viewProj = view * proj;

			// render Attribute가져온다. try get으로
			if (!(renderAttribute.flags & core::RenderAttributes::Flag::OutLine))
				continue;

			for (uint32_t i = 0; i < (uint32_t)meshRenderer.materials.size(); ++i)
			{
				auto material = _pickingMaterial;
				if (i >= meshRenderer.mesh->subMeshCount)
					continue;

				material->m_Shader->MapConstantBuffer(renderer.GetContext());
				material->m_Shader->SetMatrix("gWorld", world);
				material->m_Shader->SetMatrix("gViewProj", viewProj);

				material->m_Shader->SetInt("pickingColor", static_cast<int>(entity));
				selectedEntities.push_back(entity);

				material->m_Shader->UnmapConstantBuffer(renderer.GetContext());
				renderer.Submit(*meshRenderer.mesh, *material, i, PrimitiveTopology::TRIANGLELIST, 1);
			}
		}
	}

	{
		auto&& computeShader = _outlineComputeMaterial->GetShader();
		computeShader->MapConstantBuffer(renderer.GetContext());
		computeShader->SetFloat3("mainOutlineColor", { 0.9f, 0.9f, 0.9f });
		computeShader->SetInt("outlineThickness", 3);
		computeShader->SetFloat3("secondOutlineColor", { 0.9f, 0.9f, 0.9f });
		computeShader->SetInt("numSelections", static_cast<int>(selectedEntities.size()));
		computeShader->SetInt("msaaOn", 0);

		if (!selectedEntities.empty())
		{
			computeShader->SetConstant("SelectionListBuffer", (void*)selectedEntities.data(), uint32_t(sizeof(entt::entity) * selectedEntities.size()));
		}
		else
		{
			if (renderer.s_Api == Renderer::API::DirectX11)
			{
				computeShader->SetInt("Selection_0", 0);
				computeShader->SetInt("Selection_1", 0);
				computeShader->SetInt("Selection_2", 0);
				computeShader->SetInt("Selection_3", 0);
				computeShader->SetInt("Selection_4", 0);
				computeShader->SetInt("Selection_5", 0);
				computeShader->SetInt("Selection_6", 0);
				computeShader->SetInt("Selection_7", 0);
				computeShader->SetInt("Selection_8", 0);
				computeShader->SetInt("Selection_9", 0);
			}
		}
		computeShader->UnmapConstantBuffer(renderer.GetContext());
		renderer.DispatchCompute(*_outlineComputeMaterial, _width / 16, _height / 16, 1);
	}
}
