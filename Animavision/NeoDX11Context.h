#pragma once

#include "RendererContext.h"

#include "DX11Relatives.h"

class Texture;
struct Font;

class NeoDX11Context : public RendererContext
{
	friend class NeoWooDXI;

public:
	NeoDX11Context(const HWND hWnd, uint32_t width, uint32_t height);
	virtual ~NeoDX11Context();

	// RendererContext을(를) 통해 상속됨
	virtual void Resize(uint32_t width, uint32_t height) override;

	void SetFullScreen(bool fullscreen);
	void SetVSync(bool vsync) { m_IsVSync = vsync; }
	void SetRenderTargets(uint32_t numRenderTargets, Texture* renderTargets[], Texture* depthStencil = nullptr, bool useDefaultDSV = true);
	void SetViewport(uint32_t width, uint32_t height);
	void Clear(const float* RGBA);
	void BeginRender();
	void EndRender();
	void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY primitiveTopology);
	void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation);
	void Dispatch(uint32_t x, uint32_t y, uint32_t z);
	void DrawInstancedIndirect(ID3D11Buffer* argsBuffer, uint32_t argsOffset);
	void DrawText(Font* font, std::string text, float left, float top, float right, float bottom, DirectX::SimpleMath::Color color, bool useUnderline, bool useStrikeThrough);
	void CreateD2DRenderTarget(Texture* rt);
	void SetRTTransform(Vector2 scale, Vector2 translation);
	void CreateTextFormat(Font* font);
	void CreateTextFormat(Font* font, std::filesystem::path path);
	void DrawComboBox(Font* font, std::vector<std::string> text, uint32_t curIndex, float left, float top, float right, float bottom, DirectX::SimpleMath::Color color, bool isComboBoxOpen, float leftPadding);

	ID3D11Device* GetDevice() const { return m_Device.Get(); }
	ID3D11DeviceContext* GetDeviceContext() const { return m_DeviceContext.Get(); }
	IDXGISwapChain* GetSwapChain() const { return m_SwapChain.Get(); }
	ID3D11ShaderResourceView* GetRenderTargetSRV();

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }
	bool IsFullScreen() const { return m_IsFullScreen; }
	bool IsVSync() const { return m_IsVSync; }

private:
	void createSwapChainTextures();

	// DWrite
	void initDWrite();
	std::wstring stringToWString(const std::string& str);
	bool isValidString(const std::wstring& str);
	float convertPointSizeToDIP(float pointSize);

private:
	Microsoft::WRL::ComPtr<ID3D11Device> m_Device = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_DeviceContext = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_SwapChain = nullptr;

	// SwapChain Textures
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_SwapChainRTV = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_SwapChainDSV = nullptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_BackBufferCopySRV = nullptr;
	D3D11_VIEWPORT m_Viewport;

	// Render Targets & Depth Stencil
	std::vector<Texture*> m_RenderTargets;
	Texture* m_DepthStencil = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_CurrentDSV = nullptr;

	uint32_t m_Width = 0;
	uint32_t m_Height = 0;

	bool m_IsFullScreen = false;
	bool m_IsVSync = false;

	// DWrite
	Microsoft::WRL::ComPtr<IDWriteFactory> m_DWriteFactory = nullptr;
	Microsoft::WRL::ComPtr<ID2D1Factory3> m_D2DFactory = nullptr;
	Microsoft::WRL::ComPtr<ID2D1RenderTarget> m_D2DRenderTarget = nullptr;
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_D2DSolidColorBrush = nullptr;
	float m_FontSize = 32.0f;
};

