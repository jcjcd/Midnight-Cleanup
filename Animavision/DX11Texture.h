#pragma once

#include "DX11Shader.h"
#include "ShaderResource.h"
#include "DX11Relatives.h"

class RendererContext;
struct TextureDesc;

class DX11Texture : public Texture
{
	friend class ParticleManager;
	friend class DX11ResourceManager;
	friend class DX11RenderTarget;
	friend class DX11DepthStencil;
	friend class NeoDX11Context;

public:
	DX11Texture(RendererContext* context, std::string_view path, Type textureType);
	DX11Texture(RendererContext* context, std::string_view path, DirectX::TexMetadata metaData, DirectX::ScratchImage& image);
	DX11Texture(RendererContext* context, std::string name, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Format format, Texture::Usage usage, float* clearColor, Texture::UAVType uavType, uint32_t arraySize);
	DX11Texture(RendererContext* context, std::string name, uint32_t width, uint32_t height, Texture::Format format, bool isDynamic);
	DX11Texture(RendererContext* context, const TextureDesc& desc);
	DX11Texture() = default;
	virtual ~DX11Texture();

	void Bind(RendererContext* context, const DX11ResourceBinding& textureBinding);
	virtual void Unbind(RendererContext* context);
	void BindUAV(RendererContext* context, const DX11ResourceBinding& uavBinding, uint32_t numRenderTarget = 1, ID3D11RenderTargetView* rtvPtr[] = nullptr, ID3D11DepthStencilView* dsv = nullptr);
	void UnbindUAV(RendererContext* context);
	void Resize(RendererContext* context, uint32_t width, uint32_t height);

	bool UpdateFromIYUV(RendererContext* context, const uint8_t* data, size_t dataSize);

	virtual uint32_t GetWidth() const { return m_Width; }
	virtual uint32_t GetHeight() const { return m_Height; }
	virtual uint32_t GetDepth() const { return m_Depth; }
	virtual float* GetClearValue() override { return m_ClearValue; }

	virtual void* GetShaderResourceView();
	ID3D11ShaderResourceView* GetSRV() { return m_SRV.Get(); }
	ID3D11ShaderResourceView** GetSRVAddress() { return m_SRV.GetAddressOf(); }
	ID3D11RenderTargetView* GetRTV() { return m_RTV.Get(); }
	ID3D11RenderTargetView** GetRTVAddress() { return m_RTV.GetAddressOf(); }
	ID3D11UnorderedAccessView* GetUAV() { return m_UAV.Get(); }
	ID3D11UnorderedAccessView** GetUAVAddress() { return m_UAV.GetAddressOf(); }
	ID3D11DepthStencilView* GetDSV() { return m_DSV.Get(); }
	void SetDSV(ID3D11DepthStencilView* dsv) { m_DSV = dsv; }
	ID3D11DepthStencilView** GetDSVAddress() { return m_DSV.GetAddressOf(); }
	ID3D11Buffer* GetBuffer() { return m_Buffer.Get(); }
	ID3D11Buffer** GetBufferAddress() { return m_Buffer.GetAddressOf(); }
	std::string GetName() const { return m_Name; }
	void SetName(std::string val) { m_Name = val; }
	Texture::Usage GetUsage() const { return m_Usage; }

	void SetTexture2D(ID3D11Texture2D* texture) { m_Texture2D = texture; }
	ID3D11Texture2D* GetTexture2D() { return m_Texture2D.Get(); }
	ID3D11Texture2D** GetTexture2DAddress() { return m_Texture2D.GetAddressOf(); }

	static std::shared_ptr<DX11Texture> Create(Renderer* renderer, const std::string& path, Type type);
	static std::shared_ptr<DX11Texture> Create(Renderer* renderer, const std::string& path, Type type, bool isUI);

private:
	void createTexture2D(RendererContext* context, const DirectX::ScratchImage& image, const DirectX::TexMetadata& metaData);
	void createTexture2DCube(RendererContext* context, const DirectX::ScratchImage& image, const DirectX::TexMetadata& metadata);
	void createTexture3D(RendererContext* context, const DirectX::ScratchImage& image, const DirectX::TexMetadata& metadata);
	void createTexture2DArray(RendererContext* context, const DirectX::ScratchImage& image, const DirectX::TexMetadata& metadata);

private:
	uint32_t m_Slot = 0;
	uint32_t m_Depth = 1;
	uint32_t m_ArraySize = 1;

	ShaderType m_ShaderType = ShaderType::Count;
	Usage m_Usage = Usage::NONE;
	uint32_t m_MipLevels = 1;
	Texture::UAVType m_UAVType = Texture::UAVType::NONE;
	Format m_Format = Format::UNKNOWN;


	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_Texture2D = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture3D> m_Texture3D = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_Texture2DArray = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_TextureCube = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_TextureCubeArray = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_Texture2DMS = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_Texture2DMSArray = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture1D> m_Texture1D = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Texture1D> m_Texture1DArray = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_TextureBuffer = nullptr;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_SRV = nullptr;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_UAV = nullptr;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_RTV = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_DSV = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer = nullptr;
	std::string m_Name = "";

	float m_ClearValue[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
