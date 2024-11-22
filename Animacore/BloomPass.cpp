#include "pch.h"
#include "BloomPass.h"

#include "Scene.h"
#include "RenderComponents.h"
#include "CoreComponents.h"

#include "../Animavision/Renderer.h"
#include "../Animavision/ShaderResource.h"
#include "../Animavision/Material.h"
#include "../Animavision/Shader.h"
#include "../Animavision/Mesh.h"

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
		float threshold;
		float scatter;
		int useScatter;
		float pad;
	};	
}

void core::BloomPass::Init(Scene& scene, Renderer& renderer, uint32_t width, uint32_t height)
{
	_width = width;
	_height = height;

	_extractBrightAreasMaterial = Material::Create(renderer.LoadShader("./Shaders/BloomExtractBrightAreas.hlsl"));
	_downSampleMaterial = Material::Create(renderer.LoadShader("./Shaders/BloomDownSample.hlsl"));
	_accumulateMaterial = Material::Create(renderer.LoadShader("./Shaders/BloomAccumulate.hlsl"));
	_horizontalBlurMaterial = Material::Create(renderer.LoadShader("./Shaders/BlurHorizontal.hlsl"));
	_verticalBlurMaterial = Material::Create(renderer.LoadShader("./Shaders/BlurVertical.hlsl"));

	unsigned short extractBrightWidth = std::max<unsigned short>(1, width / 2);
	unsigned short extractBrightHeight = std::max<unsigned short>(1, height / 2);

	TextureDesc desc("ExtractBrightTextures0", Texture::Type::Texture2D, extractBrightWidth, extractBrightHeight, 1, Texture::Format::R8G8B8A8_UNORM, Texture::Usage::UAV);
	_extractBrightTextures[0] = renderer.CreateEmptyTexture(desc);
	desc.name = "ExtractBrightTextures1";
	_extractBrightTextures[1] = renderer.CreateEmptyTexture(desc);

	for (uint32_t i = 0; i < _downScaleCount; i++)
	{
		unsigned short width = std::max<unsigned short>(1, static_cast<unsigned short>(_width) / _downSampleBufferDenominator[i]);
		unsigned short height = std::max<unsigned short>(1, static_cast<unsigned short>(_height) / _downSampleBufferDenominator[i]);

		desc.width = width;
		desc.height = height;

		desc.name = "DownSampleTextures" + std::to_string(i) + '0';
		_downSampleTextures[i][0] = renderer.CreateEmptyTexture(desc);
		desc.name = "DownSampleTextures" + std::to_string(i) + '1';
		_downSampleTextures[i][1] = renderer.CreateEmptyTexture(desc);
	}
}

void core::BloomPass::Run(Scene& scene, Renderer& renderer, float tick, RenderResources& renderResource)
{
	entt::registry& registry = *scene.GetRegistry();

	auto align = [](int value, int aligment)
		{
			return (value + aligment - 1) / aligment * aligment;
		};

	unsigned short width = std::max<unsigned short>(1, static_cast<unsigned short>(_width) / 2);
	unsigned short height = std::max<unsigned short>(1, static_cast<unsigned short>(_height) / 2);

	BloomParams params = {};
	bool useBloom = false;

	for (auto&& [entity, volume] : registry.view<core::PostProcessingVolume>().each())
	{
		params.threshold = volume.bloomThreshold;
		params.scatter = volume.bloomScatter;
		params.useScatter = volume.useBloomScatter;
		useBloom = true;
	}

	if (!useBloom)
		return;

	// ¹àÀº ºÎºÐ ÃßÃâ
	{
		_extractBrightAreasMaterial->SetTexture("gInputTexture", renderResource.renderTarget);
		_extractBrightAreasMaterial->SetUAVTexture("gOutputTexture", _extractBrightTextures[_downSampleSRVIndex]);
		std::shared_ptr<Shader>&& extractShader = _extractBrightAreasMaterial->GetShader();
		extractShader->MapConstantBuffer(renderer.GetContext());
		extractShader->SetConstant("BloomParams", &params, sizeof(BloomParams));
		extractShader->UnmapConstantBuffer(renderer.GetContext());
		int dispatchX = align(width, 16) / 16;
		int dispatchY = align(height, 16) / 16;
		renderer.DispatchCompute(*_extractBrightAreasMaterial, dispatchX, dispatchY, 1);
	}

	{
		width = std::max<unsigned short>(1, static_cast<unsigned short>(_width) / _downSampleBufferDenominator[0]);
		height = std::max<unsigned short>(1, static_cast<unsigned short>(_height) / _downSampleBufferDenominator[0]);

		// ´Ù¿î »ùÇÃ¸µ
		_downSampleMaterial->SetTexture("gInputTexture", _extractBrightTextures[_downSampleSRVIndex]);
		_downSampleMaterial->SetUAVTexture("gOutputTexture", _downSampleTextures[0][_downSampleSRVIndex]);
		int dispatchX = align(width, 16) / 16;
		int dispatchY = align(height, 16) / 16;
		renderer.DispatchCompute(*_downSampleMaterial, dispatchX, dispatchY, 1);

		// ¼öÆò Èå¸®±â
		_horizontalBlurMaterial->SetTexture("gInputTexture", _downSampleTextures[0][_downSampleSRVIndex]);
		_horizontalBlurMaterial->SetUAVTexture("gOutputTexture", _downSampleTextures[0][_downSampleUAVIndex]);
		dispatchX = align(width, 256) / 256;
		dispatchY = height;
		renderer.DispatchCompute(*_horizontalBlurMaterial, dispatchX, dispatchY, 1);
		std::swap(_downSampleSRVIndex, _downSampleUAVIndex);

		// ¼öÁ÷ Èå¸®±â
		_verticalBlurMaterial->SetTexture("gInputTexture", _downSampleTextures[0][_downSampleSRVIndex]);
		_verticalBlurMaterial->SetUAVTexture("gOutputTexture", _downSampleTextures[0][_downSampleUAVIndex]);
		dispatchX = width;
		dispatchY = align(height, 256) / 256;
		renderer.DispatchCompute(*_verticalBlurMaterial, dispatchX, dispatchY, 1);
		std::swap(_downSampleSRVIndex, _downSampleUAVIndex);
	}

	for (uint32_t i = 0; i < _downScaleCount - 1; i++)
	{
		if (width <= 0 || height <= 0)
			break;

		width = std::max<unsigned short>(1, static_cast<unsigned short>(_width) / _downSampleBufferDenominator[i + 1]);
		height = std::max<unsigned short>(1, static_cast<unsigned short>(_height) / _downSampleBufferDenominator[i + 1]);

		// ´Ù¿î »ùÇÃ¸µ
		_downSampleMaterial->SetTexture("gInputTexture", _downSampleTextures[i][_downSampleSRVIndex]);
		_downSampleMaterial->SetUAVTexture("gOutputTexture", _downSampleTextures[i + 1][_downSampleSRVIndex]);
		int dispatchX = align(width, 16) / 16;
		int dispatchY = align(height, 16) / 16;
		renderer.DispatchCompute(*_downSampleMaterial, dispatchX, dispatchY, 1);

		// ¼öÆò Èå¸®±â
		_horizontalBlurMaterial->SetTexture("gInputTexture", _downSampleTextures[i + 1][_downSampleSRVIndex]);
		_horizontalBlurMaterial->SetUAVTexture("gOutputTexture", _downSampleTextures[i + 1][_downSampleUAVIndex]);
		dispatchX = align(width, 256) / 256;
		dispatchY = height;
		renderer.DispatchCompute(*_horizontalBlurMaterial, dispatchX, dispatchY, 1);
		std::swap(_downSampleSRVIndex, _downSampleUAVIndex);

		// ¼öÁ÷ Èå¸®±â
		_verticalBlurMaterial->SetTexture("gInputTexture", _downSampleTextures[i + 1][_downSampleSRVIndex]);
		_verticalBlurMaterial->SetUAVTexture("gOutputTexture", _downSampleTextures[i + 1][_downSampleUAVIndex]);
		dispatchX = width;
		dispatchY = align(height, 256) / 256;
		renderer.DispatchCompute(*_verticalBlurMaterial, dispatchX, dispatchY, 1);
		std::swap(_downSampleSRVIndex, _downSampleUAVIndex);
	}

	{
		width = std::max<unsigned short>(1, static_cast<unsigned short>(_width) / _downSampleBufferDenominator[_downScaleCount - 2]);
		height = std::max<unsigned short>(1, static_cast<unsigned short>(_height) / _downSampleBufferDenominator[_downScaleCount - 2]);

		// ¾÷ »ùÇÃ¸µ
		_accumulateMaterial->SetTexture("gInputTexture", _downSampleTextures[_downScaleCount - 1][_downSampleSRVIndex]);
		_accumulateMaterial->SetTexture("gDownSampledTexture", _downSampleTextures[_downScaleCount - 2][_downSampleSRVIndex]);
		_accumulateMaterial->SetUAVTexture("gOutputTexture", _downSampleTextures[_downScaleCount - 2][_downSampleUAVIndex]);

		auto&& accumulateShader = _accumulateMaterial->GetShader();
		accumulateShader->MapConstantBuffer(renderer.GetContext());
		accumulateShader->SetConstant("BloomParams", &params, sizeof(BloomParams));
		accumulateShader->UnmapConstantBuffer(renderer.GetContext());
		int dispatchX = align(width, 16) / 16;
		int dispatchY = align(height, 16) / 16;
		renderer.DispatchCompute(*_accumulateMaterial, dispatchX, dispatchY, 1);

		// ¼öÆò Èå¸®±â
		_horizontalBlurMaterial->SetTexture("gInputTexture", _downSampleTextures[_downScaleCount - 2][_downSampleUAVIndex]);
		_horizontalBlurMaterial->SetUAVTexture("gOutputTexture", _downSampleTextures[_downScaleCount - 2][_downSampleSRVIndex]);
		dispatchX = align(width, 256) / 256;
		dispatchY = height;
		renderer.DispatchCompute(*_horizontalBlurMaterial, dispatchX, dispatchY, 1);
		std::swap(_downSampleSRVIndex, _downSampleUAVIndex);

		// ¼öÁ÷ Èå¸®±â
		_verticalBlurMaterial->SetTexture("gInputTexture", _downSampleTextures[_downScaleCount - 2][_downSampleUAVIndex]);
		_verticalBlurMaterial->SetUAVTexture("gOutputTexture", _downSampleTextures[_downScaleCount - 2][_downSampleSRVIndex]);
		dispatchX = width;
		dispatchY = align(height, 256) / 256;
		renderer.DispatchCompute(*_verticalBlurMaterial, dispatchX, dispatchY, 1);
		std::swap(_downSampleSRVIndex, _downSampleUAVIndex);
	}

	for (uint32_t i = _downScaleCount - 2; i >= 1; i--)
	{
		width = std::max<unsigned short>(1, static_cast<unsigned short>(_width) / _downSampleBufferDenominator[i - 1]);
		height = std::max<unsigned short>(1, static_cast<unsigned short>(_height) / _downSampleBufferDenominator[i - 1]);

		if (width <= 0 || height <= 0)
			break;

		// ¾÷ »ùÇÃ¸µ
		_accumulateMaterial->SetTexture("gInputTexture", _downSampleTextures[i][_downSampleUAVIndex]);
		_accumulateMaterial->SetTexture("gDownSampledTexture", _downSampleTextures[i - 1][_downSampleSRVIndex]);
		_accumulateMaterial->SetUAVTexture("gOutputTexture", _downSampleTextures[i - 1][_downSampleUAVIndex]);
		
		auto&& accumulateShader = _accumulateMaterial->GetShader();
		accumulateShader->MapConstantBuffer(renderer.GetContext());
		accumulateShader->SetConstant("BloomParams", &params, sizeof(BloomParams));
		accumulateShader->UnmapConstantBuffer(renderer.GetContext());
		int dispatchX = align(width, 16) / 16;
		int dispatchY = align(height, 16) / 16;
		renderer.DispatchCompute(*_accumulateMaterial, dispatchX, dispatchY, 1);

		// ¼öÆò Èå¸®±â
		_horizontalBlurMaterial->SetTexture("gInputTexture", _downSampleTextures[i - 1][_downSampleUAVIndex]);
		_horizontalBlurMaterial->SetUAVTexture("gOutputTexture", _downSampleTextures[i - 1][_downSampleSRVIndex]);
		dispatchX = align(width, 256) / 256;
		dispatchY = height;
		renderer.DispatchCompute(*_horizontalBlurMaterial, dispatchX, dispatchY, 1);
		std::swap(_downSampleSRVIndex, _downSampleUAVIndex);

		// ¼öÁ÷ Èå¸®±â
		_verticalBlurMaterial->SetTexture("gInputTexture", _downSampleTextures[i - 1][_downSampleUAVIndex]);
		_verticalBlurMaterial->SetUAVTexture("gOutputTexture", _downSampleTextures[i - 1][_downSampleSRVIndex]);
		dispatchX = width;
		dispatchY = align(height, 256) / 256;
		renderer.DispatchCompute(*_verticalBlurMaterial, dispatchX, dispatchY, 1);
		std::swap(_downSampleSRVIndex, _downSampleUAVIndex);
	}

	width = std::max<unsigned short>(1, static_cast<unsigned short>(_width) / 2);
	height = std::max<unsigned short>(1, static_cast<unsigned short>(_height) / 2);

	// ¾÷ »ùÇÃ¸µ
	_accumulateMaterial->SetTexture("gInputTexture", _downSampleTextures[0][_downSampleUAVIndex]);
	_accumulateMaterial->SetTexture("gDownSampledTexture", _extractBrightTextures[_downSampleSRVIndex]);
	_accumulateMaterial->SetUAVTexture("gOutputTexture", _extractBrightTextures[_downSampleUAVIndex]);
	
	auto&& accumulateShader = _accumulateMaterial->GetShader();
	accumulateShader->MapConstantBuffer(renderer.GetContext());
	accumulateShader->SetConstant("BloomParams", &params, sizeof(BloomParams));
	accumulateShader->UnmapConstantBuffer(renderer.GetContext());
	int dispatchX = align(width, 16) / 16;
	int dispatchY = align(height, 16) / 16;
	renderer.DispatchCompute(*_accumulateMaterial, dispatchX, dispatchY, 1);

	// ¼öÆò Èå¸®±â
	_horizontalBlurMaterial->SetTexture("gInputTexture", _extractBrightTextures[_downSampleUAVIndex]);
	_horizontalBlurMaterial->SetUAVTexture("gOutputTexture", _extractBrightTextures[_downSampleSRVIndex]);
	dispatchX = align(width, 256) / 256;
	dispatchY = height;
	renderer.DispatchCompute(*_horizontalBlurMaterial, dispatchX, dispatchY, 1);
	std::swap(_downSampleSRVIndex, _downSampleUAVIndex);

	// ¼öÁ÷ Èå¸®±â
	_verticalBlurMaterial->SetTexture("gInputTexture", _extractBrightTextures[_downSampleUAVIndex]);
	_verticalBlurMaterial->SetUAVTexture("gOutputTexture", _extractBrightTextures[_downSampleSRVIndex]);
	dispatchX = width;
	dispatchY = align(height, 256) / 256;
	renderer.DispatchCompute(*_verticalBlurMaterial, dispatchX, dispatchY, 1);
	std::swap(_downSampleSRVIndex, _downSampleUAVIndex);
}

void core::BloomPass::Finish()
{
	for (uint32_t i = 0; i < _downScaleCount; i++)
	{
		for (auto& texture : _downSampleTextures[i])
			texture.reset();
	}

	for (auto& texture : _extractBrightTextures)
		texture.reset();

	_extractBrightAreasMaterial.reset();
	_downSampleMaterial.reset();
	_accumulateMaterial.reset();
	_horizontalBlurMaterial.reset();
}
