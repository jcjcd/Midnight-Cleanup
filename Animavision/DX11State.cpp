#include "pch.h"
#include "DX11State.h"
#include "NeoDX11Context.h"

DX11RasterizerState::DX11RasterizerState(NeoDX11Context& context, RasterizerState state)
{
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	switch (state)
	{
		case RasterizerState::CULL_NONE:
			rasterizerDesc.FillMode = D3D11_FILL_SOLID;
			rasterizerDesc.CullMode = D3D11_CULL_NONE;
			break;
		case RasterizerState::CULL_BACK:
			rasterizerDesc.FillMode = D3D11_FILL_SOLID;
			rasterizerDesc.CullMode = D3D11_CULL_BACK;
			rasterizerDesc.FrontCounterClockwise = false;
			rasterizerDesc.DepthClipEnable = true;
			break;
		case RasterizerState::CULL_FRONT:
			rasterizerDesc.FillMode = D3D11_FILL_SOLID;
			rasterizerDesc.CullMode = D3D11_CULL_FRONT;
			break;
		case RasterizerState::WIREFRAME:
			rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
			rasterizerDesc.CullMode = D3D11_CULL_NONE;
			break;
		case RasterizerState::SHADOW:
			rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT{});
			rasterizerDesc.DepthBias = 50000;
			rasterizerDesc.DepthBiasClamp = 0.0f;
			rasterizerDesc.SlopeScaledDepthBias = 1.0f;
			break;	
		case RasterizerState::SHADOW_CULL_NONE:
			rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT{});
			rasterizerDesc.DepthBias = 50000;
			rasterizerDesc.DepthBiasClamp = 0.0f;
			rasterizerDesc.SlopeScaledDepthBias = 1.0f;
			rasterizerDesc.CullMode = D3D11_CULL_NONE;
			break;
		case RasterizerState::PSHADOW:
			rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT{});
			rasterizerDesc.DepthBias = 1500;
			rasterizerDesc.DepthBiasClamp = 0.0f;
			rasterizerDesc.SlopeScaledDepthBias = 1.0f;
			break;		
		case RasterizerState::PSHADOW_CULL_NONE:
			rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT{});
			rasterizerDesc.DepthBias = 1500;
			rasterizerDesc.DepthBiasClamp = 0.0f;
			rasterizerDesc.SlopeScaledDepthBias = 1.0f;
			rasterizerDesc.CullMode = D3D11_CULL_NONE;
			break;
		default:
			break;
	}

	ThrowIfFailed(GetDevice(context)->CreateRasterizerState(&rasterizerDesc, m_State.GetAddressOf()));
}

void DX11RasterizerState::Bind(NeoDX11Context& context)
{
	GetDeviceContext(context)->RSSetState(m_State.Get());
}

void DX11RasterizerState::UnBind(NeoDX11Context& context)
{
	GetDeviceContext(context)->RSSetState(nullptr);
}

DX11DepthStencilState::DX11DepthStencilState(NeoDX11Context& context, DepthStencilState state)
{
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	switch (state)
	{
		case DepthStencilState::DEPTH_ENABLED:
			depthStencilDesc.DepthEnable = true;
			depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
			depthStencilDesc.StencilEnable = false;
			depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			depthStencilDesc.FrontFace.StencilFailOp = depthStencilDesc.FrontFace.StencilDepthFailOp = depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.BackFace = depthStencilDesc.FrontFace;
			break;
		case DepthStencilState::DEPTH_ENABLED_LESS_EQUAL:		// ÀÌ°Å disabled ¾Æ´Ô
			depthStencilDesc.DepthEnable = true;
			depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
			depthStencilDesc.StencilEnable = false;
			depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			depthStencilDesc.FrontFace.StencilFailOp = depthStencilDesc.FrontFace.StencilDepthFailOp = depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.BackFace = depthStencilDesc.FrontFace;
			break;
		case DepthStencilState::NO_DEPTH_WRITE:
			depthStencilDesc.DepthEnable = true;
			depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			break;
		case DepthStencilState::DEPTH_DISABLED:
			depthStencilDesc.DepthEnable = false;
			depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
			depthStencilDesc.StencilEnable = false;
			depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			depthStencilDesc.FrontFace.StencilFailOp = depthStencilDesc.FrontFace.StencilDepthFailOp = depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthStencilDesc.BackFace = depthStencilDesc.FrontFace;
			break;
		default:
			break;
	}

	ThrowIfFailed(GetDevice(context)->CreateDepthStencilState(&depthStencilDesc, m_State.GetAddressOf()));
	m_State->GetDesc(&depthStencilDesc);
}

void DX11DepthStencilState::Bind(NeoDX11Context& context)
{
	GetDeviceContext(context)->OMSetDepthStencilState(m_State.Get(), 0);
}

void DX11DepthStencilState::UnBind(NeoDX11Context& context)
{
	GetDeviceContext(context)->OMSetDepthStencilState(nullptr, 0);
}

DX11BlendState::DX11BlendState(NeoDX11Context& context, BlendState state)
{
	D3D11_BLEND_DESC blendDesc;
	blendDesc = CD3D11_BLEND_DESC{ CD3D11_DEFAULT{} };

	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;

	switch (state)
	{
		case BlendState::NO_BLEND:
			blendDesc.RenderTarget[0].BlendEnable = false;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			break;
		case BlendState::ADDITIVE_BLEND:
			blendDesc.RenderTarget[0].BlendEnable = true;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			break;
		case BlendState::ALPHA_BLEND:
			blendDesc.RenderTarget[0].BlendEnable = true;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			break;
		case BlendState::SUBTRACTIVE_BLEND:
			blendDesc.RenderTarget[0].BlendEnable = true;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			break;
		case BlendState::MODULATE_BLEND:
			blendDesc.IndependentBlendEnable = true;
			blendDesc.RenderTarget[0].BlendEnable = true;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			break;
		case BlendState::TRANSPARENT_BLEND:
			blendDesc.AlphaToCoverageEnable = FALSE; // Alpha to coverage ºñÈ°¼ºÈ­
			blendDesc.IndependentBlendEnable = TRUE; // °³º° ·»´õ Å¸°Ù¿¡ ´ëÇÑ µ¶¸³ÀûÀÎ ºí·»µù ¼³Á¤ È°¼ºÈ­
			// Ã¹ ¹øÂ° ·»´õ Å¸°Ù (Revealing Blend Target)
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;                  // GL_ZERO
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;        // GL_ONE_MINUS_SRC_COLOR
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;                 // GL_FUNC_ADD
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			// µÎ ¹øÂ° ·»´õ Å¸°Ù (Accumulation Blend Target)
			blendDesc.RenderTarget[1].BlendEnable = TRUE;
			blendDesc.RenderTarget[1].SrcBlend = D3D11_BLEND_ONE;   // GL_ONE
			blendDesc.RenderTarget[1].DestBlend = D3D11_BLEND_ONE;  // GL_ONE
			blendDesc.RenderTarget[1].BlendOp = D3D11_BLEND_OP_ADD; // GL_FUNC_ADD
			blendDesc.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			break;
		default:
			break;
	}

	ThrowIfFailed(GetDevice(context)->CreateBlendState(&blendDesc, m_State.GetAddressOf()));
}

void DX11BlendState::Bind(NeoDX11Context& context)
{
	GetDeviceContext(context)->OMSetBlendState(m_State.Get(), nullptr, 0xffffff);
}

void DX11PipelineState::Initialize(std::shared_ptr<DX11RasterizerState> rasterizerOrNull, std::shared_ptr<DX11DepthStencilState> depthStencilOrNull, std::shared_ptr<DX11BlendState> blendOrNull)
{
	if (m_RasterizerState != rasterizerOrNull)
		m_RasterizerState = rasterizerOrNull;

	if (m_DepthStencilState != depthStencilOrNull)
		m_DepthStencilState = depthStencilOrNull;

	if (m_BlendState != blendOrNull)
		m_BlendState = blendOrNull;
}

void DX11PipelineState::Bind(NeoDX11Context& context)
{
	if (m_RasterizerState)
		m_RasterizerState->Bind(context);
	else
		context.GetDeviceContext()->RSSetState(nullptr);

	if (m_DepthStencilState)
		m_DepthStencilState->Bind(context);
	else
		context.GetDeviceContext()->OMSetDepthStencilState(nullptr, 0);

	if (m_BlendState)
		m_BlendState->Bind(context);
	else
		context.GetDeviceContext()->OMSetBlendState(nullptr, nullptr, 0xffffff);
}
