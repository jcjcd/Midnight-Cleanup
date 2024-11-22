#include "pch.h"
#include "NeoDX11Context.h"
#include "DX11Texture.h"
#include "ResourceFontLoader.h"
#include "FontRenderer.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

NeoDX11Context::NeoDX11Context(const HWND hWnd, uint32_t width, uint32_t height)
	: m_Width(width), m_Height(height)
{
	uint32_t createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = hWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	D3D_FEATURE_LEVEL featureLevel = {};

	ThrowIfFailed(D3D11CreateDeviceAndSwapChain(
		0,
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		createDeviceFlags,
		0,
		0,
		D3D11_SDK_VERSION,
		&sd,
		m_SwapChain.GetAddressOf(),
		m_Device.GetAddressOf(),
		&featureLevel,
		m_DeviceContext.GetAddressOf()));

	if (featureLevel != D3D_FEATURE_LEVEL_11_0)
	{
		throw std::runtime_error("Direct3D Feature Level 11 unsupported.");
	}

	createSwapChainTextures();

	initDWrite();
}

NeoDX11Context::~NeoDX11Context()
{
	m_DWriteFactory->UnregisterFontCollectionLoader(ResourceFontCollectionLoader::GetLoader());
	m_DWriteFactory->UnregisterFontFileLoader(ResourceFontFileLoader::GetLoader());

	m_D2DSolidColorBrush.Reset();
	m_D2DRenderTarget.Reset();
	m_DWriteFactory.Reset();
	m_D2DFactory.Reset();

	m_RenderTargets.clear();
	m_CurrentDSV.Reset();

	m_BackBufferCopySRV.Reset();
	m_SwapChainDSV.Reset();
	m_SwapChainRTV.Reset();
	m_SwapChain.Reset();
	m_DeviceContext.Reset();

#ifdef _DEBUG
	// WOO : Report Live Objects
	IDXGIDevice* dxgiDevice = nullptr;
	ThrowIfFailed(m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice)));
	IDXGIAdapter* dxgiAdapter = nullptr;
	ThrowIfFailed(dxgiDevice->GetAdapter(&dxgiAdapter));
	IDXGIAdapter1* dxgiAdapter1 = nullptr;
	ThrowIfFailed(dxgiAdapter->QueryInterface(__uuidof(IDXGIAdapter1), reinterpret_cast<void**>(&dxgiAdapter1)));
	ID3D11Debug* d3dDebug = nullptr;
	ThrowIfFailed(m_Device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3dDebug)));
	d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_IGNORE_INTERNAL);
	d3dDebug->Release(); d3dDebug = nullptr;
	dxgiAdapter1->Release(); dxgiAdapter1 = nullptr;
	dxgiAdapter->Release(); dxgiAdapter = nullptr;
	dxgiDevice->Release(); dxgiDevice = nullptr;
#endif

	m_Device.Reset();
}

void NeoDX11Context::Resize(uint32_t width, uint32_t height)
{
	if (m_Width == width && m_Height == height)
		return;

	if (!(m_Width > 0 && m_Height > 0))
	{
		return;
	}

	m_Width = width;
	m_Height = height;

	m_SwapChainRTV.Reset();

	ThrowIfFailed(m_SwapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

	if (m_RenderTargets.size() > 0)
	{
		for (auto& rt : m_RenderTargets)
		{
			DX11Texture* dx11Texture = static_cast<DX11Texture*>(rt);
			dx11Texture->Resize(this, width, height);
		}
	}

	if (m_DepthStencil != nullptr)
	{
		m_CurrentDSV.Reset();

		DX11Texture* dx11Texture = static_cast<DX11Texture*>(m_DepthStencil);
		dx11Texture->Resize(this, width, height);

		m_CurrentDSV = dx11Texture->GetDSV();
	}

	createSwapChainTextures();
}

void NeoDX11Context::SetFullScreen(bool fullscreen)
{
	m_IsFullScreen = fullscreen;
	m_SwapChain->SetFullscreenState(fullscreen, nullptr);

}

void NeoDX11Context::SetRenderTargets(uint32_t numRenderTargets, Texture* renderTargets[], Texture* depthStencil /*= nullptr*/, bool useDefaultDSV /*= true*/)
{
	m_RenderTargets.clear();
	if (renderTargets == nullptr)
	{
		if (numRenderTargets == 0)
		{
			if (depthStencil)
			{
				DX11Texture* dx11Texture = static_cast<DX11Texture*>(depthStencil);
				/*if (dx11Texture->m_Width != m_Width || dx11Texture->m_Height != m_Height)
					dx11Texture->Resize(this, m_Width, m_Height);*/

				m_DepthStencil = depthStencil;
				auto dsv = dx11Texture->GetDSV();
				m_CurrentDSV = dsv;
				m_DeviceContext->OMSetRenderTargets(0, nullptr, m_CurrentDSV.Get());
			}
			else
				OutputDebugStringA("nothing binding?\n");
		}
		else
		{
			if (depthStencil == nullptr)
			{
				m_CurrentDSV = m_SwapChainDSV;
				m_DepthStencil = nullptr;
				m_DeviceContext->OMSetRenderTargets(1, m_SwapChainRTV.GetAddressOf(), m_SwapChainDSV.Get());
			}
			else
			{
				DX11Texture* dx11Texture = static_cast<DX11Texture*>(depthStencil);
				/*if (dx11Texture->m_Width != m_Width || dx11Texture->m_Height != m_Height)
					dx11Texture->Resize(this, m_Width, m_Height);*/

				m_DepthStencil = depthStencil;
				auto dsv = dx11Texture->GetDSV();
				m_CurrentDSV = dsv;
				m_DeviceContext->OMSetRenderTargets(1, m_SwapChainRTV.GetAddressOf(), m_CurrentDSV.Get());
			}
		}

	}
	else
	{
		for (uint32_t i = 0; i < numRenderTargets; ++i)
		{
			DX11Texture* dx11Texture = static_cast<DX11Texture*>(renderTargets[i]);

			if (dx11Texture->m_Width != m_Width || dx11Texture->m_Height != m_Height)
				dx11Texture->Resize(this, m_Width, m_Height);

			m_RenderTargets.push_back(dx11Texture);
		}

		std::vector<ID3D11RenderTargetView*> rtvs;
		for (auto& rt : m_RenderTargets)
		{
			DX11Texture* dx11Texture = static_cast<DX11Texture*>(rt);
			rtvs.push_back(dx11Texture->GetRTV());
		}

		if (depthStencil)
		{
			DX11Texture* dx11Texture = static_cast<DX11Texture*>(depthStencil);
			/*if (dx11Texture->m_Width != m_Width || dx11Texture->m_Height != m_Height)
				dx11Texture->Resize(this, m_Width, m_Height);*/

			m_DepthStencil = depthStencil;
			auto dsv = dx11Texture->GetDSV();
			m_CurrentDSV = dsv;
			m_DeviceContext->OMSetRenderTargets(static_cast<uint32_t>(rtvs.size()), rtvs.data(), m_CurrentDSV.Get());
		}
		else
		{
			if (useDefaultDSV)
			{
				m_CurrentDSV = m_SwapChainDSV;
				m_DepthStencil = nullptr;
				m_DeviceContext->OMSetRenderTargets(static_cast<uint32_t>(rtvs.size()), rtvs.data(), m_SwapChainDSV.Get());
			}
			else
			{
				m_CurrentDSV = nullptr;
				m_DepthStencil = nullptr;
				m_DeviceContext->OMSetRenderTargets(static_cast<uint32_t>(rtvs.size()), rtvs.data(), nullptr);
			}
		}
	}
}

void NeoDX11Context::SetViewport(uint32_t width, uint32_t height)
{
	m_Width = width;
	m_Height = height;

	m_Viewport = CD3D11_VIEWPORT();
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.Width = static_cast<float>(m_Width);
	m_Viewport.Height = static_cast<float>(m_Height);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_DeviceContext->RSSetViewports(1, &m_Viewport);
}

void NeoDX11Context::Clear(const float* RGBA)
{
	if (m_RenderTargets.size() > 0)
	{
		for (auto& rt : m_RenderTargets)
		{
			DX11Texture* dx11Texture = static_cast<DX11Texture*>(rt);
			m_DeviceContext->ClearRenderTargetView(dx11Texture->GetRTV(), RGBA);
		}
	}
	else
		m_DeviceContext->ClearRenderTargetView(m_SwapChainRTV.Get(), RGBA);

	if (m_CurrentDSV)
		m_DeviceContext->ClearDepthStencilView(m_CurrentDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void NeoDX11Context::BeginRender()
{
	m_DeviceContext->ClearRenderTargetView(m_SwapChainRTV.Get(), DirectX::Colors::CornflowerBlue);
	m_DeviceContext->ClearDepthStencilView(m_SwapChainDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	m_DeviceContext->RSSetViewports(1, &m_Viewport);
}

void NeoDX11Context::EndRender()
{
	m_SwapChain->Present(m_IsVSync, 0);
}

void NeoDX11Context::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	m_DeviceContext->IASetPrimitiveTopology(primitiveTopology);
}

void NeoDX11Context::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation)
{
	m_DeviceContext->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void NeoDX11Context::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
	m_DeviceContext->Dispatch(x, y, z);
}

void NeoDX11Context::DrawInstancedIndirect(ID3D11Buffer* argsBuffer, uint32_t argsOffset)
{
	m_DeviceContext->DrawInstancedIndirect(argsBuffer, argsOffset);
}

void NeoDX11Context::DrawText(Font* font, std::string text, float left, float top, float right, float bottom, DirectX::SimpleMath::Color color, bool useUnderline, bool useStrikeThrough)
{
	std::wstring wstr = stringToWString(text);
	D2D1_RECT_F rect = D2D1::RectF(left, top, right, bottom);

	float offsetX = left;
	float offsetY = top;
	float sizeX = right - left;
	float sizeY = bottom - top;

	font->textLayout.Reset();
	ThrowIfFailed(m_DWriteFactory->CreateTextLayout(
		wstr.c_str(),
		static_cast<uint32_t>(wstr.length()),
		font->textFormat.Get(),   // 기존의 IDWriteTextFormat 사용
		sizeX,
		sizeY,
		reinterpret_cast<IDWriteTextLayout**>(font->textLayout.GetAddressOf())
	));

	if (useUnderline)
	{
		DWRITE_TEXT_RANGE textRange = { 0, static_cast<uint32_t>(wstr.length()) };
		ThrowIfFailed(font->textLayout->SetUnderline(true, textRange));
	}

	if (useStrikeThrough)
	{
		DWRITE_TEXT_RANGE textRange = { 0, static_cast<uint32_t>(wstr.length()) };
		ThrowIfFailed(font->textLayout->SetStrikethrough(true, textRange));
	}

	m_D2DRenderTarget->BeginDraw();

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> outlineBrush;
	m_D2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &outlineBrush);

	D2D1::ColorF curColor = D2D1::ColorF(color.BGRA());
	if (m_D2DSolidColorBrush)
	{
		auto prevColor = m_D2DSolidColorBrush->GetColor();
		if (prevColor.a != curColor.a || prevColor.r != curColor.r || prevColor.g != curColor.g || prevColor.b != curColor.b)
			m_D2DSolidColorBrush->SetColor(curColor);
	}
	else
		ThrowIfFailed(m_D2DRenderTarget->CreateSolidColorBrush(curColor, m_D2DSolidColorBrush.GetAddressOf()));

	FontRenderer* fontRenderer = new FontRenderer(m_D2DFactory.Get(), m_D2DRenderTarget.Get(), outlineBrush.Get(), m_D2DSolidColorBrush.Get());

	ThrowIfFailed(font->textLayout->Draw(
		nullptr,
		fontRenderer,
		offsetX,
		offsetY
	));

	m_D2DRenderTarget->EndDraw();

	m_D2DRenderTarget = nullptr;
	delete fontRenderer;
}

void NeoDX11Context::CreateD2DRenderTarget(Texture* rt)
{
	DX11Texture* dx11Texture = static_cast<DX11Texture*>(rt);

	Microsoft::WRL::ComPtr<IDXGISurface> dxgiSurface;
	ThrowIfFailed(dx11Texture->GetTexture2D()->QueryInterface(__uuidof(IDXGISurface), reinterpret_cast<void**>(dxgiSurface.GetAddressOf())));

	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
		96.0f, 96.0f);

	ThrowIfFailed(m_D2DFactory->CreateDxgiSurfaceRenderTarget(dxgiSurface.Get(), &props, m_D2DRenderTarget.GetAddressOf()));
}

void NeoDX11Context::SetRTTransform(Vector2 scale, Vector2 translation)
{
	m_D2DRenderTarget->SetTransform(
		D2D1::Matrix3x2F::Scale(scale.x, scale.y, D2D1::Point2F(translation.x, translation.y))
	);
}

void NeoDX11Context::CreateTextFormat(Font* font)
{
	if (font->textFormat)
		font->textFormat.Reset();

	ThrowIfFailed(m_DWriteFactory->CreateTextFormat(
		font->fontName.c_str(),
		font->fontCollection.Get(),
		font->font->GetWeight(),
		font->font->GetStyle(),
		font->font->GetStretch(),
		convertPointSizeToDIP(font->fontSize),
		L"ko-KR",
		font->textFormat.GetAddressOf()
	));
}

void NeoDX11Context::CreateTextFormat(Font* font, std::filesystem::path path)
{
	std::wstring wpath = path.wstring();
	const void* key = wpath.c_str();
	uint32_t keySize = static_cast<uint32_t>((wpath.length() + 1) * sizeof(wchar_t));

	ThrowIfFailed(m_DWriteFactory->CreateCustomFontFileReference(
		key,
		keySize,
		ResourceFontFileLoader::GetLoader(),
		font->fontFile.GetAddressOf()));

	ThrowIfFailed(m_DWriteFactory->CreateCustomFontCollection(
		ResourceFontCollectionLoader::GetLoader(),
		key,
		keySize,
		font->fontCollection.GetAddressOf()));

	uint32_t familyCount = font->fontCollection->GetFontFamilyCount();
	if (familyCount == 0)
		return;

	ThrowIfFailed(font->fontCollection->GetFontFamily(0, font->fontFamily.GetAddressOf()));

	uint32_t nameLength = 0;
	ThrowIfFailed(font->fontFamily->GetFamilyNames(font->localizedString.GetAddressOf()));
	ThrowIfFailed(font->fontFamily->GetFont(0, font->font.GetAddressOf()));

	ThrowIfFailed(font->font->CreateFontFace(font->fontFace.GetAddressOf()));

	ThrowIfFailed(font->localizedString->GetStringLength(0, &nameLength));
	font->fontName.resize(nameLength + 1);
	ThrowIfFailed(font->localizedString->GetString(0, font->fontName.data(), nameLength + 1));

	ThrowIfFailed(m_DWriteFactory->CreateTextFormat(
		font->fontName.c_str(),
		font->fontCollection.Get(),
		font->font->GetWeight(),
		font->font->GetStyle(),
		font->font->GetStretch(),
		convertPointSizeToDIP(font->fontSize),
		L"ko-KR",
		font->textFormat.GetAddressOf()
	));
}

void NeoDX11Context::DrawComboBox(Font* font, std::vector<std::string> text, uint32_t curIndex, float left, float top, float right, float bottom, DirectX::SimpleMath::Color color, bool isComboBoxOpen, float leftPadding)
{
	m_D2DRenderTarget->BeginDraw();

	D2D1_RECT_F rect = D2D1::RectF(left, top, right, bottom);
	std::wstring curText = stringToWString(text[curIndex]);

	float offsetX = left;
	float offsetY = top;
	float sizeX = right - left;
	float sizeY = bottom - top;

	font->textLayout.Reset();
	ThrowIfFailed(m_DWriteFactory->CreateTextLayout(
		curText.c_str(),
		static_cast<uint32_t>(curText.length()),
		font->textFormat.Get(),   // 기존의 IDWriteTextFormat 사용
		sizeX,
		sizeY,
		reinterpret_cast<IDWriteTextLayout**>(font->textLayout.GetAddressOf())
	));

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> outlineBrush;
	m_D2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &outlineBrush);

	D2D1::ColorF curColor = D2D1::ColorF(color.BGRA());
	if (m_D2DSolidColorBrush)
	{
		auto prevColor = m_D2DSolidColorBrush->GetColor();
		if (prevColor.a != curColor.a || prevColor.r != curColor.r || prevColor.g != curColor.g || prevColor.b != curColor.b)
			m_D2DSolidColorBrush->SetColor(curColor);
	}
	else
		ThrowIfFailed(m_D2DRenderTarget->CreateSolidColorBrush(curColor, m_D2DSolidColorBrush.GetAddressOf()));

	FontRenderer* fontRenderer = new FontRenderer(m_D2DFactory.Get(), m_D2DRenderTarget.Get(), outlineBrush.Get(), m_D2DSolidColorBrush.Get());

	ThrowIfFailed(font->textLayout->Draw(
		nullptr,
		fontRenderer,
		offsetX,
		offsetY
	));

	Microsoft::WRL::ComPtr<ID2D1PathGeometry1> triangleGeometry;
	ThrowIfFailed(m_D2DFactory->CreatePathGeometry(triangleGeometry.GetAddressOf()));

	Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
	ThrowIfFailed(triangleGeometry->Open(sink.GetAddressOf()));
	sink->BeginFigure(D2D1::Point2F(offsetX + sizeX * 0.82f, offsetY + sizeY * 0.4f), D2D1_FIGURE_BEGIN_FILLED);
	sink->AddLine(D2D1::Point2F(offsetX + sizeX * 0.88f, offsetY + sizeY * 0.4f));
	sink->AddLine(D2D1::Point2F(offsetX + sizeX * 0.85f, offsetY + sizeY * 0.7f));
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();

	m_D2DRenderTarget->FillGeometry(triangleGeometry.Get(), m_D2DSolidColorBrush.Get());

	if (isComboBoxOpen)
	{
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> blackBrush;
		m_D2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &blackBrush);

		for (size_t i = 0; i < text.size(); i++)
		{
			D2D1_RECT_F itemRect = D2D1::RectF(left - leftPadding, top + (i + 1) * sizeY, right - leftPadding, bottom + (i + 1) * sizeY);
			m_D2DRenderTarget->FillRectangle(&itemRect, blackBrush.Get());
			m_D2DRenderTarget->DrawRectangle(&itemRect, m_D2DSolidColorBrush.Get());

			std::wstring wstr = stringToWString(text[i]);

			font->textLayout.Reset();
			ThrowIfFailed(m_DWriteFactory->CreateTextLayout(
				wstr.c_str(),
				static_cast<uint32_t>(wstr.length()),
				font->textFormat.Get(),   // 기존의 IDWriteTextFormat 사용
				sizeX,
				sizeY,
				reinterpret_cast<IDWriteTextLayout**>(font->textLayout.GetAddressOf())
			));

			ThrowIfFailed(font->textLayout->Draw(
				nullptr,
				fontRenderer,
				offsetX,
				offsetY + (i + 1) * sizeY
			));
		}
	}

	m_D2DRenderTarget->EndDraw();

	m_D2DRenderTarget = nullptr;
	delete fontRenderer;
}

ID3D11ShaderResourceView* NeoDX11Context::GetRenderTargetSRV()
{
	ID3D11Texture2D* backBuffer = nullptr;
	ThrowIfFailed(m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));

	D3D11_TEXTURE2D_DESC desc = {};
	backBuffer->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	ID3D11Texture2D* copyTexture = nullptr;
	ThrowIfFailed(m_Device->CreateTexture2D(&desc, nullptr, &copyTexture));

	m_DeviceContext->CopyResource(copyTexture, backBuffer);

	m_BackBufferCopySRV.Reset();

	ThrowIfFailed(m_Device->CreateShaderResourceView(copyTexture, nullptr, m_BackBufferCopySRV.GetAddressOf()));

	ReleaseCOM(backBuffer);
	ReleaseCOM(copyTexture);

	return m_BackBufferCopySRV.Get();
}

void NeoDX11Context::createSwapChainTextures()
{
	m_SwapChainDSV.Reset();

	ID3D11Texture2D* backBuffer = nullptr;
	ThrowIfFailed(m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));
	if (backBuffer == nullptr)
		MessageBox(nullptr, L"backBuffer is nullptr", L"Error", MB_ICONERROR);
	else
	{
		D3D11_TEXTURE2D_DESC desc = {};
		backBuffer->GetDesc(&desc);
		ThrowIfFailed(m_Device->CreateRenderTargetView(backBuffer, nullptr, m_SwapChainRTV.GetAddressOf()));
	}

	ReleaseCOM(backBuffer);

	ID3D11Texture2D* depthStencilBuffer = nullptr;
	D3D11_TEXTURE2D_DESC depthStencilDesc = {};
	depthStencilDesc.Width = m_Width;
	depthStencilDesc.Height = m_Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;
	ThrowIfFailed(m_Device->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilBuffer));

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	depthStencilViewDesc.Flags = 0;
	ThrowIfFailed(m_Device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, m_SwapChainDSV.GetAddressOf()));

	ReleaseCOM(depthStencilBuffer);

	m_Viewport = CD3D11_VIEWPORT();
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.Width = static_cast<float>(m_Width);
	m_Viewport.Height = static_cast<float>(m_Height);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_BackBufferCopySRV = nullptr;
}

void NeoDX11Context::initDWrite()
{
	// Direct2D 팩토리 생성
	ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_D2DFactory.GetAddressOf()));

	// DirectWrite 팩토리 생성
	ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_DWriteFactory.GetAddressOf())));

	if (!ResourceFontFileLoader::IsLoaderInitialized() || !ResourceFontCollectionLoader::IsLoaderInitialized())
		return;

	ThrowIfFailed(m_DWriteFactory->RegisterFontFileLoader(ResourceFontFileLoader::GetLoader()));

	ThrowIfFailed(m_DWriteFactory->RegisterFontCollectionLoader(ResourceFontCollectionLoader::GetLoader()));
}

std::wstring NeoDX11Context::stringToWString(const std::string& str)
{
	if (str.empty())
		return std::wstring();

	// CP_ACP -> UTF-16 시도
	int wideSizeAcp = MultiByteToWideChar(CP_ACP, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);

	// 변환 실패 처리
	if (wideSizeAcp <= 0)
		throw std::runtime_error("Failed to convert string to wstring.");

	// 변환할 문자열 버퍼 생성
	std::wstring wstrAcp(wideSizeAcp, 0);

	MultiByteToWideChar(CP_ACP, 0, str.data(), static_cast<int>(str.size()), &wstrAcp[0], wideSizeAcp);

	// UTF-8 -> UTF-16 시도
	int wideSizeUtf8 = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);

	// 변환 실패 처리
	if (wideSizeUtf8 <= 0)
		throw std::runtime_error("Failed to convert string to wstring.");

	// 변환할 문자열 버퍼 생성
	std::wstring wstrUtf8(wideSizeUtf8, 0);

	MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &wstrUtf8[0], wideSizeUtf8);

	if (wstrAcp.size() < wstrUtf8.size())
		return wstrAcp;
	else
		return wstrUtf8;

	// 유효성 검사
	/*if (!wstrAcp.empty() && isValidString(wstrAcp))
		return wstrAcp;
	else if (!wstrUtf8.empty() && isValidString(wstrUtf8))
		return wstrUtf8;

	throw std::runtime_error("Failed to convert string to wstring using both CP_UTF8 and CP_ACP.");*/
}

bool NeoDX11Context::isValidString(const std::wstring& str)
{
	// 빈 문자열은 유효하지 않다고 판단
	if (str.empty())
		return false;

	// 문자열 내 각 문자를 순회하면서 유효한 유니코드 범위인지 검사
	for (wchar_t c : str)
	{
		// 한글 범위인지 검사 (현대 한글 음절 범위 및 한글 자모 포함)
		if ((c >= 0xAC00 && c <= 0xD7A3) || // 현대 한글 음절
			(c >= 0x1100 && c <= 0x11FF) || // 한글 자모 (초성, 중성, 종성)
			(c >= 0x3130 && c <= 0x318F))   // 한글 호환 자모
		{
			// 한글 문자가 포함되어 있음. 즉, 유효하다고 판단
			return true;
		}
	}

	// 한글이 전혀 포함되지 않은 경우 유효하지 않다고 판단
	return false;
}

float NeoDX11Context::convertPointSizeToDIP(float pointSize)
{
	return pointSize * 96.0f / 72.0f;
}
