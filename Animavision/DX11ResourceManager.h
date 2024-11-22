#pragma once

#include "Renderer.h"
#include "DX11Texture.h"

#include <unordered_map>
#include <string>
#include <memory>
#include <random>

#include "DX11Relatives.h"

class DX11VertexBuffer;
class DX11IndexBuffer;
class DX11ConstantBuffer;
class DX11Shader;
class DX11Sampler;
struct DX11ResourceBinding;
struct ParticleInfo;
class NeoDX11Context;
class DX11DepthStencilState;
class DX11BlendState;
class DX11RasterizerState;

using ParticleBuffers = std::tuple<std::shared_ptr<Texture>, std::shared_ptr<Texture>, std::shared_ptr<Texture>>;

class DX11ResourceManager
{
	friend class ParticleManager;

public:
	DX11ResourceManager(NeoDX11Context* context);
	~DX11ResourceManager();

	std::shared_ptr<DX11Texture> GetTexture(const std::string& name);
	std::shared_ptr<DX11Texture> CreateTexture(const std::string& path);
	std::shared_ptr<DX11Texture> CreateEmptyTexture(std::string name, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Format format, Texture::Usage usage, float* clearColor, Texture::UAVType uavType, uint32_t arraySize);
	std::shared_ptr<DX11Texture> CreateEmptyTexture(const TextureDesc& desc);
	std::shared_ptr<DX11Texture> CreateTexture2D(const std::string& path);
	std::shared_ptr<DX11Texture> CreateLightmapTexture(const std::string& path);
	std::shared_ptr<DX11Texture> CreateTexture3D(const std::string& path);
	std::shared_ptr<DX11Texture> CreateTexture2DArray(const std::string& path);
	std::shared_ptr<DX11Texture> CreateTextureCube(const std::string& path);
	std::shared_ptr<DX11Shader> CreateShader(const std::string& path);
	std::shared_ptr<DX11Sampler> CreateSampler(D3D11_TEXTURE_ADDRESS_MODE addressMode, const DX11ResourceBinding& samplerBinding);
	std::shared_ptr<DX11DepthStencilState> CreateDepthStencilState(DepthStencilState state);
	std::shared_ptr<DX11BlendState> CreateBlendState(BlendState state);
	std::shared_ptr<DX11RasterizerState> CreateRasterizerState(RasterizerState state);
	void ReleaseTexture(const std::string& name);

	// Particle
	std::shared_ptr<DX11Texture> CreateRandomTexture();
	std::shared_ptr<DX11Texture> CreateIndirectDrawTexture();
	ParticleBuffers CreateParticleBuffers(uint32_t maxCount, uint32_t entity);
	float GetRandomSeed(float min, float max);

private:
	NeoDX11Context* m_Context = nullptr;

	robin_hood::unordered_map<std::string, std::shared_ptr<DX11Texture>> m_TextureMap;
	robin_hood::unordered_map<std::string, std::shared_ptr<DX11Shader>> m_ShaderMap;
	std::vector<std::shared_ptr<DX11Sampler>> m_Samplers;
	//robin_hood::unordered_map<std::string, std::shared_ptr<DX11Sampler>> m_SamplerMap;
	robin_hood::unordered_map<DepthStencilState, std::shared_ptr<DX11DepthStencilState>> m_DepthStencilStateMap;
	robin_hood::unordered_map<BlendState, std::shared_ptr<DX11BlendState>> m_BlendStateMap;
	robin_hood::unordered_map<RasterizerState, std::shared_ptr<DX11RasterizerState>> m_RasterizerStateMap;

	// random texture
	std::random_device m_RandomDevice;
	std::mt19937 m_RandomGen;
};

